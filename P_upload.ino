#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTCZero.h>  // allow to control and use internal RTC



//char ssid[] = "Vodafone-34268501";
//char pass[] = "aabepsab4jw49vl";
char ssid[] = "Rete-Laura";
char pass[] = "ViaMolini9cCodroipo33033";

RTCZero rtc;

// **********************    WIFI     ********************
int status = WL_IDLE_STATUS;
WiFiSSLClient client;

bool sendRequest = true;

// **********************    G API     ********************
String const client_id = "288233591191-id7ch00pnut9maaonhi9re8lrg5soc5q.apps.googleusercontent.com";
String const client_secret = "EuYWon-YFgewHz36Zo6KHJxG";
String const scope = "https://www.googleapis.com/auth/drive.file";

// These values are already provided. After some times they expires, so it will be necessary to repeat the one-time
// procedure to obtain new values. For further details, see the documentation

bool thereistoken = true; //change this if you want to start the one-time procedure;

struct TokenRequest {
  String device_code = "AH-1Ng2bazUYxFDGEvXlg9emceh4bWq0JRiPQ1AUcVef0a3uKjQNfiPQAKnj8i-qZu8AAsf-8WXi2JLlpXWKmzztFjMrRThYsQ";
  String user_code = "KSW-WHK-BCT";
  int expires_in = 1800;
  int interval = 5;
  String verification_url = "https://www.google.com/device";
} tokenReq ;

struct Token {
  String access_token = "ya29.GlvABjrRzZfOsDkcdokbWqOGvKpKsW4nwSE5bnABn9dwuvcCBtybfV1bVQWZpJtM7jSKZAJlpI6skqLZ5OfgxtGidnfKz0Do9Hbqma7Ck-r8T8LXfziy6tQQ_hGO";
  String refresh_token = "1/YdqgaGvN3-ozCvfUSTH-8PGRH_15KsHp7-7t2oR14wY";
  int expires_in = 3600;
  String token_type = "Bearer";
} token;

// ********************** MAIN PROGRAM ********************

void setup() {
  delay(3000);
  rtc.begin();
  Wire.begin();
  Serial.begin(115200);
  Serial.println("Start");
  connectToAP(); // connecition to the WIFI
  printWifiStatus(); // print WIFI status & infos
  SPI.begin();

  //Initialize SD Card
  while (!SD.begin()) {
    Serial.println(F("SD Card Error!")); delay(1000);
  }
  Serial.println(F("SD Card detected."));


  if (!thereistoken) {
    tokenReq = httpToken();
    token = httpPolling(tokenReq);
  }


}



void loop() {
  httpUpload(token, "PROVA.JPG");
  listenToClient();


  delay(1000000);
}

// ********************** FUNCTIONS ********************
//
//
//
// ********************** WIFI CONNECTION ********************
void connectToAP() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }


  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    //WiFi.config(ip);

    // wait 1 second for connection:
    delay(1000);
  }
  Serial.println("Connected to the access point");
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip); // to set MY IP ADDRESS

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// ********************** HTTP REQUESTs **********************

TokenRequest httpToken() {
  TokenRequest doc;
  bool json_start = false;

  // token request format
  String token_request = "client_id=" + client_id + "&scope=" + scope ;


  // Print request time
  Serial.println();
  Serial.print("Token request sent @ ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
  Serial.println();

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
    Serial.println("request sent");
    received = false;
  } else {
    Serial.println("connection failed");
    received = true;
  }

  //Listen to the client
  unsigned long startTime = millis();
  while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
    String code;
    int i = 0;
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
        Serial.write(c);

      }

      bool start_writing = false;
      String temp = "";
      int k = 0;
      Serial.println(code.length());
      for (i = 1; i < code.length(); i++) {
        if (code[i] == '\\') {
          start_writing = false;
          switch (k) {
            case 0:
              doc.device_code = temp;
              break;
            case 1:
              doc.user_code = temp;
              break;
            case 2:
              doc.expires_in = temp.toInt();
              break;
            case 3:
              doc.interval = temp.toInt();
              break;
            case 4:
              doc.verification_url = temp;
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
  client.stop();
  return doc;
}

Token httpPolling(TokenRequest request) {
  // Initializing the token
  Token doc;
  doc.access_token = "";
  doc.refresh_token = "";
  doc.expires_in = 0;
  doc.token_type = "Bearer";
  bool json_start = false;
  unsigned long start = millis();

  // polling request format
  String polling = "client_id=" + client_id + "&client_secret=" + client_secret + "&code=" + request.device_code + "&grant_type=http://oauth.net/grant_type/device/1.0";

  // Connecting
  while (millis() - start < (request.expires_in) * 1000) {
    Serial.println("Polling...");
    bool received = false;

    // Sending the polling request

    if (client.connect("www.googleapis.com", 443)) {
      client.println("POST /oauth2/v4/token HTTP/1.1");
      client.println("Host: www.googleapis.com");
      client.println("User-Agent: Arduino Camera");
      client.println("Connecion: close");
      client.println("Content-length: " + String(polling.length()));
      client.println("Content-Type: application/x-www-form-urlencoded;");
      client.println();
      client.println(polling);
      Serial.println("polling sent");
      received = false;
    } else {
      Serial.println("connection failed");
      received = true;
    }

    //Listen to the client
    unsigned long startTime = millis();
    while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
      String code;
      int i = 0;
      while (client.available() && i < 12) {
        received = true;
        char c = client.read();
        Serial.write(c);
        code = code + c;
        i++;
      }



      //When I reckognize 200 I enter here and identify the token;

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
                doc.access_token = temp;
                break;
              case 1:
                doc.expires_in = temp.toInt();
                break;
              case 2:
                doc.refresh_token = temp;
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
        return doc;
      } else {
        while (client.available()) {
          received = true;
          char c = client.read();
          Serial.write(c);
        }
      }
    }
    Serial.println("here");
    delay(request.interval * 1000);
  }

  client.stop();
  return doc;
}




void httpTokenRefresh () {
  // refresh token request format
  String refresh = "client_id=" + client_id + "&client_secret=" + client_secret + "&refresh_token=" + token.refresh_token + "&grant_type=refresh_token";
  bool received = false;

  // Sending the refresh token request

  if (client.connect("www.googleapis.com", 443)) {
    client.println("POST /oauth2/v4/token HTTP/1.1");
    client.println("Host: www.googleapis.com");
    client.println("User-Agent: Arduino Camera");
    client.println("Connecion: close");
    client.println("Content-length: " + String(refresh.length()));
    client.println("Content-Type: application/x-www-form-urlencoded;");
    client.println();
    client.println(refresh);
    Serial.println("Refresh request sent");
    received = false;
  } else {
    Serial.println("connection failed");
    received = true;
  }
  /*
    //Listen to the client
    unsigned long startTime = millis();
    while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
      String code;
      int i = 0;
      while (client.available() && i < 12) {
        received = true;
        char c = client.read();
        Serial.write(c);
        code = code + c;
        i++;
      }



      //When I reckognize 200 I enter here and identify the token;

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
                doc.access_token = temp;
                break;
              case 1:
                doc.expires_in = temp.toInt();
                break;
              case 2:
                doc.refresh_token = temp;
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
        return doc;
      } else {
        while (client.available()) {
          received = true;
          char c = client.read();
          Serial.write(c);
        }
      }
    }
    Serial.println("here");
    delay(request.interval * 1000);
    }

    client.stop();*/
  return;
}
















void httpUpload (Token token, String filepath) {
  File image = SD.open(filepath, FILE_READ);
  if (!image) {
    Serial.println("FILE OPEN FAILED");
    return;
  } else {
    Serial.println("IMAGE OPENED of size" + String(image.size()));
  }
  bool received = false;
  // Connecting
  Serial.println("Asking for upload...");

  // Sending the upload request
  if (client.connect("www.googleapis.com", 443)) {

    client.println("POST /upload/drive/v3/files?uploadType=resumable HTTP/1.1");
    client.println("Host: www.googleapis.com");
    client.println("Authorization: " + token.token_type + " " + token.access_token);
    client.println("User-Agent: Arduino Camera");
    client.println("Content-Length: 0");
    client.println("Content-Type: application/json; charset=UTF-8");
    client.println("X-Upload-Content-Type: image/jpeg");
    client.println("X-Upload-Content-Length: " + String(image.size()));
    client.println("Connecion: close");
    client.println();

    Serial.println("Ulpoad request sent");
    received = false;
  } else {
    Serial.println("connection failed");
    received = true;
  }



  //Listen to the client
  unsigned long startTime = millis();
  String code = "";
  String uploadID = "";
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
    if (code == "HTTP/1.1 200") {
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == ':') {
          c = client.read();
          Serial.write(c);
          c = client.read();
          Serial.write(c);
          do {
            uploadID = uploadID + c;
            c = client.read();
            Serial.write(c);
          } while (c != '\n');
          break;
        }

      }
    }
  }

  if (code == "HTTP/1.1 200") {
    // I stop the previous client session, because now I start a new one, to do the PUT request and upload the file
    client.stop();

    // I have obtained the uploadID, now I start uploading
    String location = "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&upload_id=" + uploadID;

    if (client.connect("www.googleapis.com", 443)) {
      client.println("PUT " + location + " HTTP/1.1");
      client.println("User-Agent: Arduino Camera");
      client.println("Content-Length: "  + String(image.size()));
      client.println("Connecion: close");
      client.println();
      while (image.available()) {
        client.write(image.read());
      }
      image.close();

      Serial.println("Upload done");
      received = false;
    } else {
      Serial.println("connection failed");
      received = true;
    }

  }

}





/* Using millis() this function waits for 5 seconds the response from the host website
  and it prints out on Serial console any character received, then the client is closed.*/
void listenToClient()
{
  unsigned long startTime = millis();
  bool received = false;
  if (client.available()) {
    Serial.println("client available");
  }

  while ((millis() - startTime < 5000) && !received) { //try to listen for 5 seconds
    while (client.available()) {
      received = true;
      char c = client.read();
      Serial.write(c);
    }
  }
  client.stop();
  Serial.println();
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}
