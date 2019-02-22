#include <SPI.h>        
#include <WiFiNINA.h>    
#include <RTCZero.h>  // allow to control and use internal RTC


char ssid[] = "Vodafone-34268501";
char pass[] = "aabepsab4jw49vl";

RTCZero rtc;

// **********************    WIFI     ********************
int status = WL_IDLE_STATUS;
WiFiSSLClient client;

// **********************   HTTPS REQUEST TO SEND DATA TO GOOGLE    ********************
const String request = "https://script.google.com/macros/s/";
const String webApp = "AKfycbyRvPCM-aSwTAXEwuatMwKXuws6C4RkwEcqd6pn6DWiq4XXGbrm";
int humidity = 33;
int temperature = 55;
int ligthint = 32;
bool sendRequest = true;
char server [] = "script.google.com";

// ********************** MAIN PROGRAM ********************
void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println("Start");
  connectToAP(); // connecition to the WIFI
  printWifiStatus(); // print WIFI status & infos
  Serial.println("Sending the request");
  httpRequest();
  Serial.println("Listening to the client");
  listenToClient();
  
}

void loop() {
  
  if (sendRequest) {

    httpRequest();
    listenToClient();
    }
    
  delay(1000);
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

// ********************** HTTP REQUEST **********************
void httpRequest() {
  sendRequest = false;

  // Print request time
  Serial.println();
  Serial.print("Request sent @ ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
  Serial.println();

  Serial.println("connecting...");

  if (client.connect(server, 443)) {
    // Make a HTTP request:
    String URL = request + webApp;
    URL = URL + "/exec?humidity=" + String(humidity, DEC);
    URL = URL + "&temperature=" + String(temperature, DEC);
    URL = URL + "&lightint=" + String(ligthint, DEC);
    client.println("GET " + URL + " HTTP/1.1");
    client.println("Host: script.google.com");
    client.println("Connection: close");
    client.println();
    Serial.println("request sent");
  }
  else {
    Serial.println("connection failed");
  }
}

/* Using millis() this function waits for 5 seconds the response from the host website 
 and it prints out on Serial console any character received, then the client is closed.*/
void listenToClient()
{
  unsigned long startTime = millis();
  bool received = false;

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

/* An handy function that puts a leading zero to any single digit 
number to be printed on Serial console, offering a better formatting on screen.*/

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}
