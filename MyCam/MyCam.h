//
//  MyCam.h
//  Arduino
//
//  Created by Filippo Martinelli on 04/03/2019.
//

#ifndef MyCam_h
#define MyCam_h
    #include "Arduino.h"
    #include <pins_arduino.h>
    #include "GDriveUpload.h"
    #include <SPI.h>
    #include <WiFiNINA.h>
    #include <Wire.h>
    #include <SPI.h>
    #include <SD.h>
    #include <ArduCAM.h>
    #include "memorysaver.h"


// This code is optimized for OV2640_MINI_2MP
#if !(defined OV2640_MINI_2MP)
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif


ArduCAM camera_setup(int camera_cs);

void power_mode(ArduCAM *myCAM, bool high);

void start_capture_picture(ArduCAM *myCAM);

bool save_to_SD (ArduCAM *myCAM, String dir, String file_name);

bool httpsUploadFromArducam(ArduCAM *myCAM, String file_name, class Token token);

#endif /* MyCam_h */
