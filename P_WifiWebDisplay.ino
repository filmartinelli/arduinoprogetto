#include <SPI.h>        
#include <WiFiNINA.h>    
#include <RTCZero.h>  // allow to control and use internal RTC


char ssid[] = "Vodafone-34268501";
char pass[] = "aabepsab4jw49vl";

// **********************    WIFI     ********************
int status = WL_IDLE_STATUS;
WiFiServer server = WiFiServer(80);
//IPAddress ip(192, 168, 1, 60);

// ********************** MAIN PROGRAM ********************
void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println("Start");
  connectToAP(); // connecition to the WIFI
  printWifiStatus(); // print WIFI status & infos
  server.begin(); // begin the server
}

void loop() {

  WiFiClient client = server.available(); //
  if(client)
  {
    boolean BlankLine = true;
    while(client.connected())
    {
      if(client.available())
      {
        char c = client.read();

        if(c == '\n' && BlankLine)
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html\n");
          client.println("<html><head><META HTTP-EQUIV=""refresh"" CONTENT=""5"">\n");
          client.println("<title>Arduino Web Server</title></head>");
          client.println("<body style=\"background-color:#5EA084\">\n");
          client.println("<h1>Arduino Web Server</h1>");
          client.println("<h3>Ho collegato ARDUINO al Web</h3>");      
          client.println("</body>\n</html>");

          break;
        }
        if(c == '\n')
        {
          BlankLine = true;
        }
        else if(c != '\r')
        {
          BlankLine = false;
        }
      }
    }
    delay(100);
    client.stop();
  }
  
}

// ********************** FUNCTIONS ********************
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
