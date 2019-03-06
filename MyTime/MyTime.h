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
#endif /* MyTime_h */

void print2digits(int number);

String hours_withpoints(RTCZero rtc);

String hours_withoutpoints(RTCZero rtc);

// add day
