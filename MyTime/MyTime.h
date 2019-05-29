//
//  MyTime.h
//  Arduino
//  Here there are my function to create file names base on time and date.
//  Created by Filippo Martinelli on 02/03/2019.
//

#ifndef MyTime_h
#define MyTime_h
    #include <RTCZero.h>
    #include "Arduino.h"
    #include <pins_arduino.h>

void print2digits(int number);

String hours_withpoints(RTCZero *rtc);

String hours_withoutpoints(RTCZero *rtc);

String date_withbracket(RTCZero *rtc);

String date_withoutbracket(RTCZero *rtc);

// add day

#endif /* MyTime_h */
