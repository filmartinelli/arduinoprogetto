//
//  GDriveUpload.h
//  Arduino
//
//  Created by Filippo Martinelli on 02/03/2019.
//

#ifndef GDriveUpload_h
#define GDriveUpload_h
    #include "Arduino.h"
    #include <pins_arduino.h>
    #include <SPI.h>
    #include <WiFiNINA.h>
    #include <Wire.h>
    #include <SPI.h>
    #include <SD.h>



const String CLIENT_ID = "288233591191-id7ch00pnut9maaonhi9re8lrg5soc5q.apps.googleusercontent.com";
const String CLIENT_SECRET = "EuYWon-YFgewHz36Zo6KHJxG";
const String SCOPE = "https://www.googleapis.com/auth/drive.file";

// STRUCTURES
class TokenRequestData {
public:
    TokenRequestData();
    TokenRequestData(String deviceCode, String userCode, int expiresIn, int Interval, String verificationUrl, WiFiSSLClient Client);
    
    String device_code;
    String user_code;
    int expires_in;
    int interval;
    String verification_url;
    WiFiSSLClient client;
    
    void httpsAskForTokenData();
    
};

class Token {
    TokenRequestData data;
    void httpsTokenPolling(class TokenRequestData requestData);
public:
    Token();
    Token(class TokenRequestData Data, String AccessToken, String RefreshToken, int ExpiresIn);
    
    String access_token;
    String refresh_token;
    int expires_in;
    String token_type;
    
    void httpsTokenRequest(class TokenRequestData requestData);
    void httpsTokenRefresh();
    WiFiSSLClient getClient();
    
    
};

// FUNCTIONS

bool httpsUploadFromSD(class Token token, String filepath);

#endif /* GDriveUpload_h */
