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
#include "cactus_io_AM2302.h"

Measurement::Measurement() {
    _pinLight = A1;
    _pinDHT = 0;
    Lmin = LMIN;
    Lmax = LMAX;
    AM2302 temp(_pinDHT, 6);
    dht = temp;
    light = 0;
    time = "";
    temperature = 0;
    humidity = 0;
    lightint = 0;
}

void Measurement::begin(int pinLight, int pinDHT) {
    _pinLight = pinLight;
    _pinDHT = pinDHT;
    pinMode(pinLight, INPUT);
    Lmin = LMIN;
    Lmax = LMAX;
    pinMode(pinDHT,INPUT);
    AM2302 temp(pinDHT);
    dht = temp;
    dht.begin();
    
}

void Measurement::measure(RTCZero *rtc) {
   
    light = analogRead(_pinLight);
    delay(10);
    
    light = (light + analogRead(_pinLight))/2;
    
    if (light <= Lmin){
        Lmin = light;
    }
    
    if (Lmax <= light){
        Lmax = light;
    }
    
    time = hours_withpoints(rtc);
    lightint = (((float)light)/Lmax)*100;
    
    dht.readHumidity();
    dht.readTemperature();
    
    if (isnan(dht.humidity) || isnan(dht.temperature_C)) {
        Serial.println("Il sensore ha sbagliato la lettura");
        return;
    }
    
    humidity = dht.humidity;
    temperature = dht.temperature_C;
    Serial.println(String(humidity));
    Serial.println(String(temperature));
}

bool Measurement::httpsDataSend(WiFiSSLClient client) {
    Serial.println("Connecting...");
    bool received = false;
    if (client.connect("script.google.com", 443)) {
        // Make a HTTP request:
        String URL = REQUEST + WEB_APPLICATION;
        URL = URL + "/exec?humidity=" + String(humidity, DEC);
        URL = URL + "&temperature=" + String(temperature, DEC);
        URL = URL + "&lightint=" + String(lightint, DEC);
        
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
        Serial.write(c);
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
    File txt;
    if (SD.exists(dir + "/" + file_name)) {
        txt = SD.open(dir + "/" + file_name, FILE_WRITE);
    } else {
        if (!SD.exists(dir)) {
            SD.mkdir(dir);
        }
        txt = SD.open(dir + "/" + file_name, FILE_WRITE);
        txt.println("Time,Temperature,Humidity,Light Intensity");
    }
    
    if(txt) {
        Serial.println(F("File txt opening successful"));
    } else {
        Serial.println(F("File txt opening failed"));
        return false;
    }
    txt.print(time);
    txt.print(",");
    txt.print(temperature);
    txt.print(",");
    txt.print(humidity);
    txt.print(",");
    txt.println(lightint);

    txt.close();
    return true;
}
