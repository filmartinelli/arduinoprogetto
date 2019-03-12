//
//  Sensors.h
//  Arduino
//
//  Created by Filippo Martinelli on 04/03/2019.
//

#ifndef Sensors_h
#define Sensors_h
    #include "Arduino.h"
    #include <pins_arduino.h>
    #include <SPI.h>
    #include <WiFiNINA.h>
    #include <Wire.h>
    #include <SPI.h>
    #include <SD.h>
    #include "cactus_io_AM2302.h"
    #include "MyTime.h"

const String REQUEST = "https://script.google.com/macros/s/";
const String WEB_APPLICATION = "AKfycbyRvPCM-aSwTAXEwuatMwKXuws6C4RkwEcqd6pn6DWiq4XXGbrm";

#define LMAX        800
#define LMIN        100

class Measurement {
private:
    int _pinLight;
    int _pinDHT;
    AM2302 dht = AM2302(_pinDHT);
    int Lmin;
    int Lmax;
    float light;
    
public:
    Measurement();

    String time;
    float temperature;
    float humidity;
    float lightint;
    
    void begin(int pinLight, int pinDHT);
    bool httpsDataSend(WiFiSSLClient client);
    bool saveDataSD(String dir, String file_name);
    void measure(RTCZero *rtc);
};

#endif /* Sensors_h */

