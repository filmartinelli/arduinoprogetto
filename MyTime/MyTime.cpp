//
//  MyTime.cpp
//  Arduino
//
//  Created by Filippo Martinelli on 02/03/2019.
//
#include "Arduino.h"
#include <pins_arduino.h>
#include <RTCZero.h>
#include "MyTime.h"

void print2digits(int number) {
    if (number < 10) {
        Serial.print("0");
    }
    Serial.print(number);
}

String hours_withpoints(RTCZero *rtc){
    String time_str;
    if ((*rtc).getHours() < 10) {
        time_str = "0" + (*rtc).getHours();
    }
    else {
        time_str = (*rtc).getHours();
    }
    if ((*rtc).getMinutes() < 10) {
        time_str = time_str + ":0" + (*rtc).getMinutes();
    }
    else {
        time_str = time_str + ":" + (*rtc).getMinutes();
    }
    if ((*rtc).getSeconds() < 10) {
        time_str = time_str + ":0" + (*rtc).getSeconds();
    }
    else {
        time_str = time_str + ":" + (*rtc).getSeconds();
    }
    return time_str;
}

String hours_withoutpoints(RTCZero *rtc){
    String time_str;
    if ((*rtc).getHours() < 10) {
        time_str = "0" + (*rtc).getHours();
    }
    else {
        time_str = (*rtc).getHours();
    }
    if ((*rtc).getMinutes() < 10) {
        time_str = time_str + "0" + (*rtc).getMinutes();
    }
    else {
        time_str = time_str + (*rtc).getMinutes();
    }
    if ((*rtc).getSeconds() < 10) {
        time_str = time_str + "0" + (*rtc).getSeconds();
    }
    else {
        time_str = time_str  + (*rtc).getSeconds();
    }
    return time_str;
}

String date_withbracket(RTCZero *rtc){
    String date_str = "20";
        date_str = date_str + (*rtc).getYear();
    
    if ((*rtc).getMonth()< 10) {
        date_str = date_str + "/0" + (*rtc).getMonth();
    }
    else {
        date_str = date_str + "/" + (*rtc).getMonth();
    }
    if ((*rtc).getDay() < 10) {
        date_str = date_str + "/0" + (*rtc).getDay();
    }
    else {
        date_str = date_str + "/" + (*rtc).getDay();
    }
    return date_str;
}

String date_withoutbracket(RTCZero *rtc){
    String date_str;
    date_str = (*rtc).getYear();
    
    if ((*rtc).getMonth()< 10) {
        date_str = date_str + "0" + (*rtc).getMonth();
    }
    else {
        date_str = date_str  + (*rtc).getMonth();
    }
    if ((*rtc).getDay() < 10) {
        date_str = date_str + "0" + (*rtc).getDay();
    }
    else {
        date_str = date_str  + (*rtc).getDay();
    }
    return date_str;
}
