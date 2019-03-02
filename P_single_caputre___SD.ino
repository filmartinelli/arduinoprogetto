#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTCZero.h>
#include "memorysaver.h"

// This code is optimized for OV2640_MINI_2MP
#if !(defined OV2640_MINI_2MP)
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

//******** CAMERA VARIABLES **********
const int CAMERA_CS = 7;
ArduCAM myCAM( OV2640, CAMERA_CS);
bool capturing = false;
uint32_t length = 0;

void start_capture_picture();
void save_to_SD();
String day_str ();
String time_str ();
bool is_header = false;

//******** CLOCK **********

RTCZero rtc;

//******** MAIN CODE **********

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
  rtc.begin();
  Serial.print("Current hour: ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
  Wire.begin();
  Serial.begin(115200);
  Serial.println(F("ArduCAM Start!"));
  //set the CS as an output:
  pinMode(CAMERA_CS, OUTPUT);
  digitalWrite(CAMERA_CS, HIGH);
  // initialize SPI:
  SPI.begin();

  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while (1) {
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);

    if (temp != 0x55) {
      Serial.println(F("SPI interface Error!"));
      delay(1000); continue;
    } else {
      Serial.println(F("SPI interface OK.")); break;
    }
  }
  //Initialize SD Card
  while (!SD.begin()) {
    Serial.println(F("SD Card Error!")); delay(1000);
  }
  Serial.println(F("SD Card detected."));

  while (1) {
    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);

    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial.println(F("Can't find OV2640 module!"));
      delay(1000);
      continue;
    }
    else {
      Serial.println(F("OV2640 detected."));
      break;
    }
  }

  myCAM.set_format(JPEG);
  myCAM.InitCAM();

  // Camera format is set
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  delay(1000);
}


void loop() {

  // If my cam is not capturing, I start taking a picture
  if (!capturing) {
    Serial.println();
    Serial.print("Current hour: ");
    print2digits(rtc.getHours());
    Serial.print(":");
    print2digits(rtc.getMinutes());
    Serial.print(":");
    print2digits(rtc.getSeconds());
    Serial.println();
    start_capture_picture();
  }

  // When the picture is taken, I enter here
  if (myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK)) {
    Serial.println(F("Capture Done."));
    length = myCAM.read_fifo_length();
    Serial.print(F("The fifo length is :"));
    Serial.println(length, DEC);
    if (length >= MAX_FIFO_SIZE) //384K
    {
      Serial.println(F("Over size."));
    }
    else if (length == 0 ) //0 kb
    {
      Serial.println(F("Size is 0."));
    } else {
      save_to_SD();
    }
  }

  delay(5000);
}

void start_capture_picture() {
  //Flush the FIFO
  myCAM.flush_fifo();
  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  //Start capture
  myCAM.start_capture();
  Serial.println(F("start Capture"));
  capturing = true;
}

void save_to_SD() {
  File image;
  String file_name;
  uint8_t temp = 0, temp_last = 0;
  byte buf[256];
  static int i = 0;

  Serial.println("Saving the file in the SD card...");

  // Creating the file name (hours & minutes of the picture);
  if (rtc.getHours() < 10) {
    file_name = "0" + rtc.getHours();
  }
  else {
    file_name = rtc.getHours();
  }
  if (rtc.getMinutes() < 10) {
    file_name = file_name + "_0" + rtc.getMinutes();
  }
  else {
    file_name = file_name + "_" + rtc.getHours();
  }
if (rtc.getSeconds() < 10) {
    file_name = file_name + "_0" + rtc.getSeconds();
  }
  else {
    file_name = file_name + "_" + rtc.getSeconds();
  }
  file_name = file_name + ".jpg";

  // Checking if the directory exists
  if (!SD.exists("Arduino/" + day_str("n") + "/pictures")) {
    SD.mkdir("Arduino/" + day_str("n") + "/pictures");
  }

  // Opening the file in the SD card
  image = SD.open("Arduino/" + day_str("n") + "/pictures/" + file_name, O_WRITE | O_CREAT | O_TRUNC);
  if (!image) {
    Serial.println("FILE OPEN FAILED");
    return;
  }

  // Starting the writing
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  // DO NOT USE ANY OTHER SPI COMMANDS IN BURST OPERTATION
  while (length--)
  {
    temp_last = temp;

    // I start the communication
    temp =  SPI.transfer(0x00);

    //Read JPEG data from FIFO

    //If find the end, break while
    if ( (temp == 0xD9) && (temp_last == 0xFF) )
    {
      buf[i++] = temp;  //save the last  0XD9
      //Write the remain bytes in the buffer
      myCAM.CS_HIGH();
      image.write(buf, i);

      //Close the file
      image.close();
      Serial.println(F("Image save OK."));
      capturing = false;
      is_header = false;
      i = 0;
    }

    if (is_header == true)
    {
      //Write image data to buffer if not full
      if (i < 256)
        buf[i++] = temp;

      //Write 256 bytes image data to file
      else
      {
        myCAM.CS_HIGH();
        image.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
      }
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      buf[i++] = temp_last;
      buf[i++] = temp;
    }
  }
}



// *************** TIME STRINGS

String time_str () {
  String time_str;
  if (rtc.getHours() < 10) {
    time_str = "0" + rtc.getHours();
  }
  else {
    time_str = rtc.getHours();
  }
  if (rtc.getMinutes() < 10) {
    time_str = time_str + ":0" + rtc.getMinutes();
  }
  else {
    time_str = time_str + ":" + rtc.getMinutes();
  }
  if (rtc.getSeconds() < 10) {
    time_str = time_str + ":0" + rtc.getSeconds();
  }
  else {
    time_str = time_str + ":" + rtc.getSeconds();
  }
  return time_str;
}


String day_str (char* c) {
  String day_str;

  if (c == "/") {
    if (rtc.getDay() < 10) {
      day_str = "0" + rtc.getDay();
    }
    else {
      day_str = rtc.getDay();
    }
    if (rtc.getMonth() < 10) {
      day_str = day_str + "/0" + rtc.getMonth();
    }
    else {
      day_str = day_str + "/" + rtc.getMonth();
    }
    day_str = day_str + "/" + rtc.getYear();
    return day_str;
   
  } else {

    if (rtc.getDay() < 10) {
      day_str = "0" + rtc.getDay();
    }
    else {
      day_str = rtc.getDay();
    }
    if (rtc.getMonth() < 10) {
      day_str = day_str + "0" + rtc.getMonth();
    }
    else {
      day_str = day_str  + rtc.getMonth();
    }
    day_str = day_str  + rtc.getYear();
    return day_str;
  }
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}
