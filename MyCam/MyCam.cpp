//
//  MyCam.cpp
//  Arduino
//
//  Created by Filippo Martinelli on 04/03/2019.
//

#include "MyCam.h"
#include "Arduino.h"
#include <pins_arduino.h>
#include "GDriveUpload.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ArduCAM.h>
#include "memorysaver.h"

ArduCAM camera_setup(int camera_cs) {
    ArduCAM myCAM(OV2640, camera_cs);
    uint8_t vid, pid;
    uint8_t temp;
    Serial.println(F("ArduCAM Start!"));
    
    digitalWrite(camera_cs, HIGH);
    
    //Reset the CPLD
    myCAM.write_reg(0x07, 0x80);
    delay(100);
    myCAM.write_reg(0x07, 0x00);
    delay(100);
    
    while (1) {
        //Check if the ArduCAM SPI bus is OK
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        temp = myCAM.read_reg(ARDUCHIP_TEST1);
        
        if (temp != 0x55) {
            Serial.println(F("SPI interface Error!"));
            delay(1000); continue;
        } else {
            Serial.println(F("SPI interface OK.")); break;
        }
    }
    
    while (1) {
        //Check if the camera module type is OV2640
        myCAM.wrSensorReg8_8(0xff, 0x01);
        myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
        
        if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
            Serial.println(F("Can't find OV2640 module!"));
            delay(1000);
            continue;
        }
        else {
            Serial.println(F("OV2640 detected."));
            break;
        }
    }
    
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    
    // Camera format is set
    myCAM.OV2640_set_JPEG_size(OV2640_800x600);
    delay(1000);
    return myCAM;
}

void power_mode(ArduCAM *myCAM, bool high) {
    if (high) {
        // high power mode
        (*myCAM).clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
    } else {
        // Standby mode
       (*myCAM).set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
    }
}

void start_capture_picture(ArduCAM *myCAM) {
    //Flush the FIFO
    (*myCAM).flush_fifo();
    //Clear the capture done flag
    (*myCAM).clear_fifo_flag();
    //Start capture
    (*myCAM).start_capture();
    Serial.println(F("Start Capture"));
    return;
}

bool save_to_SD (ArduCAM *myCAM, String dir, String file_name) {
    File image;
    uint8_t temp = 0, temp_last = 0;
    byte buf[256];
    bool is_header = false;
    static int i = 0;
    uint32_t length = (*myCAM).read_fifo_length();
    
    Serial.println("Saving the file in the SD card...");
    
    // Checking if the directory exists
    if (!SD.exists("Arduino/" + dir + "/pictures")) {
        SD.mkdir("Arduino/" + dir + "/pictures");
    }
    
    // Opening the file
    image = SD.open("Arduino/" + dir + "/pictures/" + file_name, O_WRITE | O_CREAT | O_TRUNC);
    if (!image) {
        Serial.println("FILE OPEN FAILED");
        return false;
    }
    
    // Starting the writing
    (*myCAM).CS_LOW();
    (*myCAM).set_fifo_burst();
    // DO NOT USE ANY OTHER SPI COMMANDS IN BURST OPERTATION
    while (length--)
    {
        temp_last = temp;
        
        // I start the communication
        temp =  SPI.transfer(0x00);
        
        //Read JPEG data from FIFO
        
        //If find the end, break while
        if ( (temp == 0xD9) && (temp_last == 0xFF) )
        {
            buf[i++] = temp;  //save the last  0XD9
            //Write the remain bytes in the buffer
            (*myCAM).CS_HIGH();
            image.write(buf, i);
            
            //Close the file
            image.close();
            Serial.println(F("Image save OK."));
            is_header = false;
            i = 0;
        }
        
        if (is_header == true)
        {
            //Write image data to buffer if not full
            if (i < 256)
                buf[i++] = temp;
            
            //Write 256 bytes image data to file
            else
            {
                (*myCAM).CS_HIGH();
                image.write(buf, 256);
                i = 0;
                buf[i++] = temp;
                (*myCAM).CS_LOW();
                (*myCAM).set_fifo_burst();
            }
        }
        else if ((temp == 0xD8) & (temp_last == 0xFF))
        {
            is_header = true;
            buf[i++] = temp_last;
            buf[i++] = temp;
        }
    }
    
    return true;
}

bool httpsUploadFromArducam(ArduCAM *myCAM, String file_name, class Token token) {
    String name_metadata = "{\"name\": \"" + file_name + "\"}";
    WiFiSSLClient client = token.getClient();
    
    // uint8_t temp = 0, temp_last = 0;
    // byte buf[256];
    static int i = 0;
   //  bool is_header = false;
    uint32_t length = (*myCAM).read_fifo_length();
    
    bool received = false;
    bool trytorefresh = false;
    unsigned long startTime = millis();
    String code = "";
    String uploadID = "";
    
    // Connecting and asking for UPLOAD
    Serial.println("Asking for upload...");
    do {
    
        // Sending the upload request
        if (client.connect("www.googleapis.com", 443)) {
            client.println("POST /upload/drive/v3/files?uploadType=resumable HTTP/1.1");
            client.println("Host: www.googleapis.com");
            client.println("Authorization: " + token.token_type + " " + token.access_token);
            client.println("Content-Length: " + String(name_metadata.length()));
            client.println("Content-Type: application/json; charset=UTF-8");
            client.println("X-Upload-Content-Type: image/jpeg");
            client.println("X-Upload-Content-Length: " + String(length));
            client.println("Connection: close");
            client.println();
            client.println(name_metadata);
            client.println();
            
            Serial.println("Upload request sent");
            received = false;
        } else {
            Serial.println("Connection failed");
            received = true;
        }
        
        
        //Listen to the client
        
        while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
            int i = 0;
            while (client.available() && i < 12) {
                received = true;
                char c = client.read();
                Serial.write(c);
                code = code + c;
                i++;
            }
            
            //When I reckognize 200 I enter here and identify the uploadID;
            if(i>0) {
                if (code == "HTTP/1.1 200") {
                    
                    if (trytorefresh) {
                        trytorefresh = !trytorefresh;
                    }
                    
                    while (client.available()) {
                        char c = client.read(); // here I print in the Serial the response
                        Serial.write(c);
                        if (c == ':') {
                            c = client.read();
                            Serial.write(c);
                            c = client.read();
                            Serial.write(c);
                            do {
                                uploadID = uploadID + c;  // Here I identify UploadID from the resp
                                c = client.read();
                                Serial.write(c);
                            } while (c != '\n');
                            break;
                        }
                        
                    }
                    break;
                }
                else if (code == "HTTP/1.1 401") {
                    while(client.available()) {
                        char c = client.read();
                        Serial.write(c);
                    }
                    // If credentials are not valide, I'll try once to refresh the token;
                    if(!trytorefresh){
                        Serial.println("\nProbably you need to refresh the token\nI'm trying to refresh\n");
                        token.httpsTokenRefresh();
                        trytorefresh = !trytorefresh;
                        code = "";
                    } else if (trytorefresh) {
                        trytorefresh = !trytorefresh;
                    }
                }
                else if (code == "HTTP/1.1 400") {
                    if (trytorefresh) {
                        trytorefresh = !trytorefresh;
                    }
                    while(client.available()) {
                        char c = client.read();
                        Serial.write(c);
                    }
                    break;
                } else {
                    break;
                }
            } else if (trytorefresh) {
                trytorefresh = !trytorefresh;
            }
            
        }
    } while (trytorefresh); // I try to refresh once if the resposnse is 401
    
    uint32_t total_length = length;
    
    if (code == "HTTP/1.1 200") {
        Serial.println("Token request has been successful. Starting upload");
        bool successful = false;
        // I have obtained the uploadID, now I start uploading
        String location = "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&upload_id=" + uploadID;
        
        while(!successful){
                // I stop the previous client session, because now I start a new one, to do the PUT request and upload the file
                client.stop();
            if (client.connect("www.googleapis.com", 443)) {
                client.println("PUT " + location + " HTTP/1.1");
                client.println("User-Agent: Arduino Camera");
                client.println("Content-Length: "  + String(length));
                client.println("Connection: close");
                client.println();
                
                uint32_t temp_length = length;
                
                while(temp_length--) {
                    client.write((*myCAM).read_fifo());
                }
                Serial.println("...");
                client.write((*myCAM).read_fifo());
                received = false;
            } else {
                Serial.println("Connection failed");
                received = true;
            }

            // Listenig to the client to check if the upload has been successful
            startTime = millis();
            String code = "";
            
            while ((millis() - startTime < 10000) && !received) { //try to listen for 5 seconds
                int i = 0;
                while (client.available() && i < 12) {
                    received = true;
                    char c = client.read();
                    Serial.write(c);
                    code = code + c;
                    i++;
                }

                if (code == "HTTP/1.1 200" || code == "HTTP/1.1 201")
                {
                    while(client.available()) {
                        char c = client.read();
                        Serial.write(c);
                    }
                    Serial.println("\nUpload successful");
                    successful = true;
                    client.flush();
                    client.stop();
                    return successful;
                }
                
                // HTTP 308 I have to restart my upload
                
                else if (code == "HTTP/1.1 308") {
                    while(client.available()) {
                        char c = client.read();
                        Serial.write(c);
                    }
                    client.flush();
                    client.stop();
                    Serial.println("\n308 response code.\nUpload interrupted. Starting a new session");
                    // I have to reset the FIFO read pointer 
                    (*myCAM).write_reg(0x04, 1010000);
                    delay(1000);
                } else {
                    if (received) {
                        while(client.available()) {
                            char c = client.read();
                            Serial.write(c);
                        }
                    }
                }
        }
            
            if (!received) {
                // I have to reset the FIFO read pointer
                (*myCAM).write_reg(0x04, 1010000);
                client.flush();
                client.stop();
                Serial.println("\nNo response.\nUpload interrupted. Starting a new session");
                delay(1000);
            }
    }
    }
    
    else if (code == "HTTP/1.1 401" && trytorefresh) {
        Serial.println("Upload failed. Probably you need a new token");
        client.flush();
        client.stop();
        return false;
    }
    
    Serial.println("\nUpload failed");
    client.flush();
    client.stop();
    return false;
}
