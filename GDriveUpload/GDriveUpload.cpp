//
//  GDriveUpload.cpp
//  Arduino
//
//  Created by Filippo Martinelli on 02/03/2019.
//
#include "Arduino.h"
#include <pins_arduino.h>
#include "GDriveUpload.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>


// TokenRequestData methods
TokenRequestData::TokenRequestData()
{
    device_code = "";
    user_code = "";
    expires_in = 1800;
    interval = 5;
    verification_url = "https://www.google.com/device";
}

TokenRequestData::TokenRequestData(String deviceCode, String userCode, int expiresIn, int Interval, String verificationUrl, WiFiSSLClient Client)
{
    device_code = deviceCode;
    user_code = userCode;
    expires_in = expiresIn;
    interval = Interval;
    verification_url = verificationUrl;
    client = Client;
}

void TokenRequestData::httpsAskForTokenData() {
    bool json_start = false;
    
    // Token request format
    String token_request = "client_id=" + CLIENT_ID + "&scope=" + SCOPE ;
    
    // Connecting
    Serial.println("connecting...");
    bool received = false;
    
    if (client.connect("accounts.google.com", 443)) {
        client.println("POST /o/oauth2/device/code HTTP/1.1");
        client.println("Host: accounts.google.com");
        client.println("User-Agent: Arduino Camera");
        client.println("Connecion: close");
        client.println("Content-length: " + String(token_request.length()));
        client.println("Content-Type: application/x-www-form-urlencoded;");
        client.println();
        client.println(token_request);
        Serial.println("Request sent");
        received = false;
    } else {
        Serial.println("Connection failed");
        received = true;
    }
    
    //Listen to the client
    unsigned long startTime = millis();
    while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
        String code;
        int i = 0;
        
        // CATCHING THE RESPONSE CODE
        while (client.available() && i < 12) {
            received = true;
            char c = client.read();
            Serial.write(c);
            code = code + c;
            i++;
        }
        
        if (code == "HTTP/1.1 200") {
            while (client.available()) {
                received = true;
                char c = client.read();
                Serial.write(c);
                if (c == '{') {
                    json_start = true;
                    break;
                }
            }
            
            // Writing the JSON data in a string
            if (json_start) {
                char c = client.read();
                code = "\0";
                Serial.write(c);
                do {
                    if (c != '\n' && c != ' ' && c != ',' && c != '"' ) {
                        code = code + c;
                    } if (c == '\n') {
                        code = code + '\\';
                    }
                    c = client.read();
                    Serial.write(c);
                } while (c != '}');
                c = client.read();
                Serial.write(c);
                
            }
            
            // Finding the JSON parameters from the String
            bool start_writing = false;
            String temp = "";
            int k = 0;
            Serial.println(code.length());
            for (i = 1; i < code.length(); i++) {
                if (code[i] == '\\') {
                    start_writing = false;
                    switch (k) {
                        case 0:
                            device_code = temp;
                            break;
                        case 1:
                            user_code = temp;
                            break;
                        case 2:
                            expires_in = temp.toInt();
                            break;
                        case 3:
                            interval = temp.toInt();
                            break;
                        case 4:
                            verification_url = temp;
                            break;
                        default:
                            break;
                    }
                    k++;
                    temp = "";
                }
                
                if (start_writing) {
                    temp = temp + code[i];
                }
                
                if (code[i] == ':') {
                    start_writing = true;
                }
            }
        }
        else {
            while(client.available()) {
                char c = client.read();
                Serial.write(c);
            }
        }
    }
    client.stop();
    return;
}


// Token methods
Token :: Token()
{
    data = TokenRequestData();
    access_token = "";
    refresh_token = "";
    expires_in = 3600;
    token_type = "Bearer";
}

Token :: Token(class TokenRequestData Data, String AccessToken, String RefreshToken, int ExpiresIn)
{
    data = Data;
    access_token = AccessToken;
    refresh_token = RefreshToken;
    expires_in = ExpiresIn;
    token_type = "Bearer";
}

void Token :: httpsTokenPolling(class TokenRequestData requestData)
{
    
    bool json_start = false;
    unsigned long start = millis();
    
    // polling request format
    String polling = "client_id=" + CLIENT_ID + "&client_secret=" + CLIENT_SECRET + "&code=" + requestData.device_code + "&grant_type=http://oauth.net/grant_type/device/1.0";
    
    // I will poll every requestData.interval*1000 seconds before the expiration time
    while (millis() - start < (requestData.expires_in) * 1000) {
        Serial.println("Polling...");
        bool received = false;
        
        // Sending the polling request
        
        if (requestData.client.connect("www.googleapis.com", 443)) {
            requestData.client.println("POST /oauth2/v4/token HTTP/1.1");
            requestData.client.println("Host: www.googleapis.com");
            requestData.client.println("User-Agent: Arduino Camera");
            requestData.client.println("Connecion: close");
            requestData.client.println("Content-length: " + String(polling.length()));
            requestData.client.println("Content-Type: application/x-www-form-urlencoded;");
            requestData.client.println();
            requestData.client.println(polling);
            Serial.println("Polling request sent");
            received = false;
        } else {
            Serial.println("Connection failed");
            received = true;
        }
        
        //Listen to the client
        unsigned long startTime = millis();
        while ((millis() - startTime < 500) && !received) { //try to listen for 0,5 seconds
            String code;
            int i = 0;
            
            // I identify the response code
            while (requestData.client.available() && i < 12) {
                received = true;
                char c = requestData.client.read();
                Serial.write(c);
                code = code + c;
                i++;
            }
            
            //When I reckognize 200 I enter here and identify the token;
            
            if (code == "HTTP/1.1 200") {
                while (requestData.client.available()) {
                    received = true;
                    char c = requestData.client.read();
                    Serial.write(c);
                    if (c == '{') {
                        json_start = true;
                        break;
                    }
                }
                
                if (json_start) {
                    char c = requestData.client.read();
                    code = "\0";
                    Serial.write(c);
                    do {
                        if (c != '\n' && c != ' ' && c != ',' && c != '"' ) {
                            code = code + c;
                        } if (c == '\n') {
                            code = code + '\\';
                        }
                        c = requestData.client.read();
                        Serial.write(c);
                    } while (c != '}');
                    c = requestData.client.read();
                    Serial.write(c);
                    
                }
                
                bool start_writing = false;
                String temp = "";
                int k = 0;
                Serial.println(code.length());
                for (i = 1; i < code.length() && k < 3; i++) {
                    if (code[i] == '\\') {
                        start_writing = false;
                        switch (k) {
                            case 0:
                                access_token = temp;
                                break;
                            case 1:
                                expires_in = temp.toInt();
                                break;
                            case 2:
                                refresh_token = temp;
                                break;
                            default:
                                break;
                        }
                        k++;
                        temp = "";
                    }
                    
                    if (start_writing) {
                        temp = temp + code[i];
                    }
                    
                    if (code[i] == ':') {
                        start_writing = true;
                    }
                }
                Serial.println("Token obtained");
                requestData.client.stop();
                return;
            } else {
                while (requestData.client.available()) {
                    received = true;
                    char c = requestData.client.read();
                    Serial.write(c);
                }
            }
        }
        
        delay(requestData.interval * 1000);
    }
    
    requestData.client.stop();
    return;
}

void Token :: httpsTokenRequest(class TokenRequestData requestData) {
    data = requestData;
    httpsTokenPolling(data);
    return;
}

void Token :: httpsTokenRefresh() {
    // refresh token request format
    String refresh = "client_id=" + CLIENT_ID + "&client_secret=" + CLIENT_SECRET + "&refresh_token=" + refresh_token + "&grant_type=refresh_token";
    bool received = false;
    bool json_start = false;
    
    // Sending the refresh token request
    
    if (data.client.connect("www.googleapis.com", 443)) {
        data.client.println("POST /oauth2/v4/token HTTP/1.1");
        data.client.println("Host: www.googleapis.com");
        data.client.println("User-Agent: Arduino Camera");
        data.client.println("Connection: close");
        data.client.println("Content-length: " + String(refresh.length()));
        data.client.println("Content-Type: application/x-www-form-urlencoded;");
        data.client.println();
        data.client.println(refresh);
        Serial.println("Refresh request sent");
        received = false;
    } else {
        Serial.println("Connection failed");
        received = true;
    }
    
    //Listen to the client
    unsigned long startTime = millis();
    String code = "";
    while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
        int i = 0;
        
        // CATCHING THE RESPONSE CODE
        while (data.client.available() && i < 12) {
            received = true;
            char c = data.client.read();
            Serial.write(c);
            code = code + c;
            i++;
        }
        
        if (code == "HTTP/1.1 200") {
            while (data.client.available()) {
                received = true;
                char c = data.client.read();
                Serial.write(c);
                if (c == '{') {
                    json_start = true;
                    break;
                }
            }
            
            // Writing the JSON data in a string
            if (json_start) {
                char c = data.client.read();
                code = "\0";
                Serial.write(c);
                do {
                    if (c != '\n' && c != ' ' && c != ',' && c != '"' ) {
                        code = code + c;
                    } if (c == '\n') {
                        code = code + '\\';
                    }
                    c = data.client.read();
                    Serial.write(c);
                } while (c != '}');
                c = data.client.read();
                Serial.write(c);
                
            }
            
            // Finding the JSON parameters from the String
            bool start_writing = false;
            String temp = "";
            int k = 0;
            Serial.println(code.length());
            for (i = 1; i < code.length(); i++) {
                if (code[i] == '\\') {
                    start_writing = false;
                    switch (k) {
                        case 0:
                            access_token = temp;
                            break;
                        case 1:
                            expires_in = temp.toInt();
                            data.client.stop();
                            Serial.println("Token refreshed");
                            return;
                            break;
                        default:
                            break;
                    }
                    k++;
                    temp = "";
                }
                
                if (start_writing) {
                    temp = temp + code[i];
                }
                
                if (code[i] == ':') {
                    start_writing = true;
                }
            }
        }
    }
    
    if(code != "HTTP/1.1 200") {
        while(data.client.available()) {
            char c = data.client.read();
            Serial.write(c);
        }
        Serial.println("\n *** Unable to refresh the token. A new token is probably needed *** \n");
    }
    data.client.stop();
    return;
}

WiFiSSLClient Token :: getClient()
{
    return data.client;
}

// Functions
bool httpsUploadFromSD(class Token token, String filepath) {
    WiFiSSLClient client = token.getClient();               // I get the client class
    File image = SD.open(filepath, FILE_READ);              // I open the file I want to send
    
    String name_metadata ="{\"name\": \"" + String(image.name()) + "\"}";
    if (!image) {
        Serial.println("FILE OPEN FAILED");
        return false;
    } else {
        Serial.println("IMAGE OPENED of size" + String(image.size()));
    }
    
    
    bool received = false;
    bool trytorefresh = false;
    unsigned long startTime = 0;
    String code = "";
    String uploadID = "";
    
    // Connecting -- Asking for upload permission
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
            client.println("X-Upload-Content-Length: " + String(image.size()));
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
        
        //Listen to the client responsse
        startTime = millis();
        code = "";
        
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
    
    if (code == "HTTP/1.1 200") {
        Serial.println("Token request has been succesful. Starting upload");
        bool succesful = false;
        // I have obtained the uploadID, now I start uploading
        String location = "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&upload_id=" + uploadID;
        
        received = true;
        
        while(!succesful){                              // I upload until it is successful
            
            // I stop the previous client session, because now I start a new one, to do the PUT request and upload the file
            client.stop();
            
            // Upload request

            if (client.connect("www.googleapis.com", 443)) {
                client.println("PUT " + location + " HTTP/1.1");
                client.println("User-Agent: Arduino Camera");
                client.println("Content-Length: "  + String(image.size()));
                client.println("Connection: close");
                client.println();
                while (image.available()) {
                    client.write(image.read());         // Here I send the bytes of the image
                }
                Serial.println("...");
                image.close();
                received = false;
            } else {
                Serial.println("Connection failed");
                received = true;
            }

            
            // Listening to the response
            startTime = millis();
            String code = "";
            
            while (millis() - startTime < 15000 && !received) { //try to listen for 5 seconds
                int i = 0;
                while (client.available() && i < 12) {
                    received = true;
                    char c = client.read();
                    Serial.write(c);
                    code = code + c;
                    i++;
                }
                // HTTP 200 OK
                if (code == "HTTP/1.1 200" || code == "HTTP/1.1 201")
                {
                    while(client.available()) {
                        char c = client.read();
                        Serial.write(c);
                    }
                    Serial.println("\nUpload successful");
                    succesful = true;
                    client.stop();
                    return succesful;
                    
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
                    
                    // I have to open image again
                    image = SD.open(filepath, FILE_READ);
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
                client.flush();
                client.stop();
                Serial.println("\nNo response.\nUpload interrupted. Starting a new session");
                
                // I have to open image again
                image = SD.open(filepath, FILE_READ);
            }
            
        }

    } else if (code == "HTTP/1.1 401" && trytorefresh) {
        Serial.println("\nUpload failed. Probably you need a new token");
        client.flush();
        client.stop();
        return false;
    }
    
    Serial.println("\nUpload failed");
    client.flush();
    client.stop();
    return false;
}




