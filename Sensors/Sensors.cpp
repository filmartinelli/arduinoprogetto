//
//  Sensors.cpp
//  Arduino
//
//  Created by Filippo Martinelli on 04/03/2019.
//

#include "Sensors.h"
#include "Arduino.h"
#include <pins_arduino.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "MyTime.h"

Measurement::Measurement() {
    time = "";
    temperature = 0;
    humidity = 0;
    lightint = 0;
}

bool Measurement::httpsDataSend(WiFiSSLClient client) {
    Serial.println("Connecting...");
    bool received = false;
    if (client.connect("script.google.com", 443)) {
        // Make a HTTP request:
        String URL = REQUEST + WEB_APPLICATION;
        
        String temp = String(humidity, DEC);
        temp.replace(".", ",");
        URL = URL + "/exec?humidity=" + temp;
        temp = String(temperature, DEC);
        temp.replace(".", ",");
        URL = URL + "&temperature=" + temp;
        temp = String(lightint, DEC);
        temp.replace(".", ",");
        URL = URL + "&lightint=" + temp;
        
        client.println("GET " + URL + " HTTP/1.1");
        client.println("Host: script.google.com");
        client.println("User-Agent: Arduino Camera");
        client.println("Connection: close");
        client.println();
        Serial.println("Request sent");
        received = false;
    }
    else {
        Serial.println("Connection failed");
        return false;
    }
    
    //Listen to the client
    unsigned long startTime = millis();
    String code = "";
    
    while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
        int i = 0;
        while (client.available() && i < 12) {
            received = true;
            char c = client.read();
            Serial.write(c);
            code = code + c;
            i++;
        }
    }
    
    while (client.available()) {
        char c = client.read();
        //Serial.write(c);
    }
    
    //When I reckognize 200 I enter here and identify the uploadID;
    if (code == "HTTP/1.1 200" || code == "HTTP/1.1 302") {
        Serial.println("Data sending successful");
        client.stop();
        return true;
    } else {
        Serial.println("Data sending unsuccessful");
        client.stop();
        return false;
    }
    client.stop();
    return false;
}
            
bool Measurement::saveDataSD(String dir, String file_name){
    Serial.println("Saving data on the SD");
    file_name = file_name + ".CSV";
    File txt;
    Serial.print(SD.exists("ARDUINO/" + dir + "/" + file_name));
    Serial.print(SD.exists("ARDUINO/" + dir + "/" + file_name));
    if (SD.exists("ARDUINO/" + dir + "/" + file_name)) {
        txt = SD.open("ARDUINO/" + dir + "/" + file_name, FILE_WRITE);
        Serial.println("File already existing");
    } else {
        if (!SD.exists("ARDUINO/" + dir)) {
            SD.mkdir("ARDUINO/" + dir);
            Serial.println("Creation new directory");
        }
        txt = SD.open("ARDUINO/" + dir + "/" + file_name, FILE_WRITE);
        txt.println("Time;Temperature;Humidity;Light Intensity");
        Serial.println("Creation new file");
    }
    
    if(txt) {
        Serial.println(F("File txt opening successful"));
    } else {
        Serial.println(F("File txt opening failed"));
        txt.close();
        return false;
    }
    txt.print(time);
    txt.print(";");
    txt.print(temperature);
    txt.print(";");
    txt.print(humidity);
    txt.print(";");
    txt.println(lightint);

    txt.close();
    Serial.println(F("Saving data successful"));
    return true;
}
