//#include <neotimer.h>
#include "MyCam.h"
#include <SPI.h>
#include <DHT.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <SD.h>
#include "Sensors.h"
#include "MyTime.h"
#include <ArduCAM.h>
#include "memorysaver.h"
#include <RTCZero.h>
#include "GDriveUpload.h"
#include <LiquidCrystal.h>
#include <NTPClient.h>
#include <time.h>
#include "Settings.h"

RTCZero rtc;
int next_hours = 0;
int next_minutes = 0;
int minutes = 0;

int button1 = A2;

int interval_minutes = TAKE_PICTURE_INTERVAL;

void take_picture();
void dateSetup();

void directory_change();
void data_saving();

int count = 0;

//**********************      SENSORS     ********************
// Definisce il tipo di sensore Temp
#define DHTTYPE DHT22
const int DataPinSens = A0;
DHT dht(DataPinSens, DHT22);
const int pinLight = A1;
const int pinISR = 8;
int light;
int Lmin = 0;
int Lmax = 1023;
int lightint;
LiquidCrystal lcd(14, 6, 5, 4, 3, 2);
Measurement measurement;
void start_measure();
void lcd_data_print();

// **********************    WIFI     ********************
//char ssid[] = "Rete-Laura";
//char pass[] = "ViaMolini9cCodroipo33033";
//char ssid[] = "TP-Link_00FB";
//char pass[] = "31870094";
//char ssid[] = "SCALA";
//char pass[] = "casoretto452";
char ssid[] = SSid;
char pass[] = PASSWORD;

int status = WL_IDLE_STATUS;
WiFiSSLClient client;

void connectToAP();
void printWifiStatus();

int varIndex;
int timer1_counter;

// **********************    STATES     ********************
// These are the different states in which our controller will work
#define SD_WIFI           0
#define SD_ONLY           1
#define WIFI_ONLY         2
#define NO_SD_NO_WIFI     3

int state = NO_SD_NO_WIFI;

bool sd = 0;
bool data_available = false;

// **********************    SD     ********************
void sd_init();

String directory;

String txt_name;              // Name of the data file

String jpg_name;              // Name of the image jpg file

const int SD_REMOVING = 1;

void sd_removing();

void sd_button();

bool removing = false;

// **********************    G API     ********************
TokenRequestData tokenrequest(DEVICE_CODE, USER_CODE, 1800, 5, "https://www.google.com/device" , client);
Token token(tokenrequest, ACCESS_TOKEN, REFRESH_TOKEN, 3600);

//********************** CAMERA VARIABLES ********************
const int CAMERA_CS = 7;
ArduCAM myCAM;
bool capturing = false;
int seconds;
bool takePicture = false;

//******************TIME***************
int GMT = 2;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.inrim.it");

int hours;
int minute;
int second;
String dataodierna;
String anno;
int year;
String giorno;
int day;
String Date;
String mese;
int month;


//*************************************

void setup() {
  pinMode(13, OUTPUT);
  pinMode(button1, INPUT) ;
  pinMode(SD_REMOVING, INPUT);
  Serial.begin(9600);
  delay(3000);
  Serial.println("START SETUP\n");


  Wire.begin();
  SPI.begin();
  rtc.begin();

  // Connecting to Wifi
  connectToAP(); // connection to the WIFI
  if (WiFi.status() == WL_CONNECTED) {
    printWifiStatus(); // print WIFI status & infos
    digitalWrite(13, HIGH);
  }

  // CAMERA SETUP
  pinMode(CAMERA_CS, OUTPUT);
  myCAM = camera_setup(CAMERA_CS);

  // SD
  sd_init();

  Serial.println("AM2302 Humidity - Temperature Sensor");
  //INIT SENSORE
  dht.begin();
  //CONFIGURAZIONE INPUT SENSORE
  pinMode(DataPinSens, INPUT);
  //INIT LCD
  lcd.begin(16, 2); // 16 colonne e 2 righe

  //SETTING THE DIRECTORY OF THE DAY AND THE PICTURE TIMER

  int minutes = rtc.getMinutes();
  int hours = rtc.getHours();
  next_minutes = (minutes + interval_minutes) % 60;
  Serial.println(next_minutes);
  next_hours = (hours + ((minutes + interval_minutes) / 60)) % 24;
  Serial.println(next_hours);
  directory = date_withoutbracket(&rtc);
  Serial.println("DIRECTORY: " + directory);
  txt_name = directory;
  Serial.println("FILE NAME: " + txt_name);
  // Alarm.timerRepeat(120, data_saving); // ogni 2 minuti esegue data_saving

  // If the next alarm is the next day, set the change of directory
  if (next_hours - hours < 0) {
    rtc.detachInterrupt();
    rtc.setAlarmTime(0, 0, 0);
    rtc.enableAlarm(rtc.MATCH_HHMMSS);
    rtc.attachInterrupt(directory_change);
    Serial.println("Set alarm: directory_change at 00:00:00");

  } else {    // else set the next alarm
    rtc.setAlarmHours(next_hours);
    rtc.setAlarmMinutes(next_minutes);
    rtc.setAlarmSeconds(0);
    rtc.enableAlarm(rtc.MATCH_HHMMSS);
    rtc.attachInterrupt(take_picture);
    Serial.print("Set alarm: take_picture at: ");
    print2digits(next_hours);
    Serial.print(":");
    print2digits(next_minutes);
    Serial.println(":00");
  }

  //Timing intialization
  timeClient.begin();
  attachInterrupt(digitalPinToInterrupt(SD_REMOVING), sd_button, CHANGE);


  Serial.print(date_withoutbracket(&rtc));
  //date_withoutbracket(&rtc) == "00101"
  if (date_withoutbracket(&rtc) == "00101")
  {
    dateSetup();

  }

  Serial.println("\nSETUP COMPLETE. START LOOP\n\n");
}

void loop() {

  //  if (mytimer.repeat()) {
  //    delay(500);
  //    data_saving();
  //    delay(500);
  //  }

  // WIFI CONNECTION
  //****** SE NON E' CONNESSO, AGGIUNGERE UN MODO PER EVITARE CHE SI CONNETTA OGNI VOLTA
  Serial.println("________________________________________________________________________\nLoop\nORARIO: " + hours_withpoints(&rtc));
  Serial.println(".");

  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(13, LOW);
    Serial.println("Trying to connect...");

    detachInterrupt(digitalPinToInterrupt(SD_REMOVING));
    connectToAP(); // connecition to the WIFI
    if (WiFi.status() == WL_CONNECTED) {
      printWifiStatus(); // print WIFI status & infos
      digitalWrite(13, HIGH);
    }
    attachInterrupt(digitalPinToInterrupt(SD_REMOVING), sd_button, CHANGE);
  }

  Serial.println(".");

  // Checking if SD is available
  if (!sd) {
    sd_init();
  }

  // Checking the state
  Serial.println(".");
  if (sd) {
    if (WiFi.status() == WL_CONNECTED) {
      state = SD_WIFI;
      Serial.println("State: SD & WIFI");
    } else {
      state = SD_ONLY;
      Serial.println("State: SD ONLY");
    }
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      state = WIFI_ONLY;
      Serial.println("State: WIFI ONLY");
    } else {
      state = NO_SD_NO_WIFI;
      Serial.println("State: NO SD NO WIFI");
    }
  }
  detachInterrupt(digitalPinToInterrupt(SD_REMOVING));

  //Measure Temperature, Humidity, Light Int
  Serial.println(".");
  start_measure();

  //Print data on LCD screen
  Serial.println(".");
  lcd_data_print();
  delay(500);

  //Save data
  Serial.println(".");
  data_saving();

  //Taking picture
  if (takePicture) {
    takePicture = false;
    capturing = true;
    int minutes = rtc.getMinutes();
    int hours = rtc.getHours();

    // Start taking the picture
    //power_mode(&myCAM, 1); // I set the CMOS in high power mode
    Serial.print("Taking the picture at: ");
    jpg_name = hours_withoutpoints(&rtc) + ".JPG";
    Serial.println(hours_withpoints(&rtc));
    start_capture_picture(&myCAM);

    // Set the next alarm
    next_minutes = (minutes + interval_minutes) % 60;
    next_hours = (hours + ((minutes + interval_minutes) / 60)) % 24;

    // If the next alarm is the next day, set the change of directory
    if (next_hours - hours < 0) {
      rtc.detachInterrupt();
      rtc.setAlarmTime(0, 0, 0);
      rtc.enableAlarm(rtc.MATCH_HHMMSS);
      rtc.attachInterrupt(directory_change);
      Serial.println("Set alarm: directory_change at 00:00:00");
    } else {    // else set the next alarm
      rtc.setAlarmHours(next_hours);
      rtc.setAlarmMinutes(next_minutes);
      rtc.setAlarmSeconds(0);
      rtc.enableAlarm(rtc.MATCH_HHMMSS);
      rtc.attachInterrupt(take_picture);
      Serial.print("Set alarm: take_picture at ");
      Serial.println(String(next_hours) + ":" + String(next_minutes) + ":" + String(00));
    }
  }

  Serial.println(".");
  if (myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK) && capturing) {
    //    mytimer.stop();
    capturing = false;
    Serial.println(F("CAPTURE DONE."));

    int length = myCAM.read_fifo_length();
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

      // FUNZIONE PER LO SPAZIO IN SD E NEL CASO CAMBIARE STATO

      Serial.println("\nSAVING/UPLOADING");
      switch (state) {
        case SD_WIFI:
          save_to_SD ( &myCAM, directory, jpg_name);                        // Saving JPG on SD
          token.httpsTokenRefresh();
          httpsUploadFromSD(token, "ARDUINO/" + directory + "/PICTURES/" + jpg_name);     // Saving JPG on Google Drive
          break;
        case SD_ONLY:
          save_to_SD ( &myCAM, directory, jpg_name);                         // Saving JPG on SD
          break;
        case WIFI_ONLY:
          token.httpsTokenRefresh();
          httpsUploadFromArducam(&myCAM, jpg_name, token);        // Saving JPG on Google Drive
          break;
        default:
          break;
      }
      Serial.println("\nSAVING/UPLOADING ENDED");
      //power_mode(&myCAM, 0);
    }
    //    mytimer.restart();

  }

  Serial.println(".");
  if (digitalRead(SD_REMOVING) == 1) {
    sd_removing();
    removing = false;
  }

  attachInterrupt(digitalPinToInterrupt(SD_REMOVING), sd_button, CHANGE);


}

// ********************** WIFI CONNECTION ********************
void connectToAP() {
  // Function to connect to the access point

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    return;
  }

  Serial.print("Stato WIFI AP funct: ");
  Serial.println(WiFi.status());

  // attempt to connect to Wifi network (for 2 minutes):
  long unsigned startTime = millis();
  status = WiFi.status();

  while (status != WL_CONNECTED && millis() - startTime < 1200) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 2 second for connection:
    delay(2000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to the access point");


    //***************************configura RTC*************************************************************
    timeClient.update();
    Serial.println("----------------------------------------------------------------------------------------------------");
    Serial.println("***********************************************");
    //Serial.println(timeClient.getFullFormattedTime());
    dataodierna = timeClient.getFullFormattedTime();
    Serial.println(dataodierna);
    anno = dataodierna.substring(2, 4);
    mese = dataodierna.substring(5, 7);
    giorno = dataodierna.substring(8, 10);
    year = anno.toInt();
    month = mese.toInt();
    day = giorno.toInt();
    Serial.println(year);
    Serial.println(month);
    Serial.println(day);
    hours = timeClient.getHours() + GMT;
    if (hours == 24) {
      hours = 0;
    }
    else if (hours == 25) {
      hours = 1;
    }
    Serial.println(hours);
    minute = timeClient.getMinutes();
    second = timeClient.getSeconds();
    Serial.println("***********************************************");
    rtc.setYear(year);
    rtc.setMonth(month);
    rtc.setDay(day);
    rtc.setHours(hours);
    rtc.setMinutes(minute);

    //*******************************************************************************************************

  } else {
    Serial.println("Connection to the access point failed");
  }
  return;
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

// **********************    MEASURE     ********************
void start_measure() {

  //-------------LOOP PER SENSORI------------------
  // leggo da sensore temperatura e umidità
  Serial.println("Hum\t\tTemp (C)");//\tHeat Index (C)\tHeat Index (F)
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  //CONTROLLO SENSORISTICA Temperatura e Umidità AM2302
  if (isnan(t) || isnan(h)) {
    Serial.println("Il sensore ha sbagliato la lettura");
    return;
  }
  else {
    Serial.print(h); Serial.print(" %\t\t");
    Serial.print(t); Serial.println(" *C\t");
  }

  //-------------LOOP PER FOTORESISTENZA------------
  light = analogRead(pinLight);
  delay(10);

  light = (light + analogRead(pinLight)) / 2;

  if (light <= Lmin) {
    Lmin = light;
  }
  if (Lmax <= light) {
    Lmax = light;
  }

  // calcolo quanta Luce c'è nella stanza
  lightint = (((float)light) / Lmax) * 100;


  Serial.print("Luminosità percepita: ");
  Serial.print(light);
  Serial.print("  ||  Soglia minima:");
  Serial.print(Lmin);
  Serial.print("  ||  Soglia massima:");
  Serial.print(Lmax);
  Serial.print("  ||  Il percentuale di luce è: ");
  Serial.print(lightint);
  Serial.println("%");

  measurement.time = hours_withpoints(&rtc);
  measurement.temperature = t;
  measurement.humidity = h;
  measurement.lightint = lightint;

  data_available = true;
}

// **********************    LCD     ********************
void lcd_data_print() {
  //  //FONDAMENTALE PER PULIRE IL DISPLAY QUANDO LUCE ARRIVA A 100%, ALTRIMENTI CELLA 15,1 RIMANE AL VALORE PRECEDENTE
  lcd.clear();

  /////Temperatura
  lcd.setCursor(0, 0);
  lcd.print("Temp");
  lcd.setCursor(0, 1);
  lcd.print(measurement.temperature);
  lcd.setCursor(4, 1);
  lcd.print("C");

  /////Umidità
  lcd.setCursor(5, 0);
  lcd.print("Hum %");
  lcd.setCursor(6, 1);
  lcd.print(measurement.humidity);
  lcd.setCursor(10, 1);
  lcd.print("%");

  //  /////Luminosità
  lcd.setCursor(11, 0);
  lcd.print("Lum %");
  lcd.setCursor(12, 1);
  lcd.print(measurement.lightint);
  lcd.setCursor(15, 1);
  lcd.print("%");
}

// **********************    SD     ********************

void sd_init() {
  long unsigned startTime = millis();

  while (!SD.begin() && (millis() - startTime < 1200)) {
    Serial.println(F("SD Card Error!"));
    delay(1000);
  }

  if (!SD.begin()) {
    Serial.println(F("SD Card not detected."));
    sd = false;
  } else {
    Serial.println(F("SD Card detected."));
    sd = true;
  }
  return;
}

void sd_button() {
  removing = true;
}

void sd_removing() {
  if (sd == false) {
    delay(50);
    return;
  }
  long unsigned startTime = millis();
  delay(50);
  Serial.println("SD removing procedure start\n\n");
  while (digitalRead(SD_REMOVING) && millis() - startTime < 4000) {

  }

  // If the button has been pressed for more than 5 seconds, you can remove SD.
  if (millis() - startTime > 3000) {
    Serial.println("YOU HAVE 10 SECONDS TO REMOVE SD CARD\n\n");

    // Printing instructions on the LCD
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("REMOVE");
    lcd.setCursor(7, 1);
    lcd.print("SD");

    // Removing procedure
    SD.end();
    sd = false;
    delay(10000);

    // Print the T, H, Light, values on the LCD
    lcd_data_print();
    return;

  } else {
    Serial.println("SD removing failed\n\n");
    delay(50);
    return;
  }
}


void take_picture() {
  takePicture = true;
}

void directory_change() {

  // function to change the directory every day

  directory = date_withoutbracket(&rtc);
  Serial.println("DIRECTORY: " + directory);
  txt_name = directory;
  Serial.println("FILE NAME: " + txt_name);

  // SETTING THE NEXT PICTURE ALARM
  rtc.detachInterrupt();
  if (next_hours == 0 & next_minutes == 0) {
    next_minutes ++;
  }
  rtc.setAlarmHours(next_hours);
  rtc.setAlarmMinutes(next_minutes);
  rtc.setAlarmSeconds(0);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(take_picture);
  Serial.println("Set alarm: take_picture at ");
  Serial.println(String(next_hours) + ":" + String(next_minutes) + ":" + String(00));
  return;
}


void data_saving() {
  Serial.println("Data saving starts");
  if (data_available) {
    Serial.println("Data available");
    switch (state) {
        Serial.println("Checking the state");
      case SD_WIFI:
        measurement.httpsDataSend(client);              // Saving data on Google Drive Spreadsheet
        measurement.saveDataSD(directory, txt_name);     // Saving data on SD
        break;
      case SD_ONLY:
        measurement.saveDataSD(directory, txt_name);     // Saving data on SD
        break;
      case WIFI_ONLY:
        measurement.httpsDataSend(client);              // Saving data on Google Drive Spreadsheet
        break;
      default:
        break;
    }
    data_available = false;
    Serial.println("Data saving finished");
  }
}

void dateSetup() {
  // print data dafault

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("01/01/2019");
  lcd.setCursor(4, 1);
  lcd.print("00:00:00");

  int temp = 19;
  int days = 30;

  // YEAR
  while (digitalRead(SD_REMOVING) == 0) {

    if (digitalRead(button1) == HIGH) {
      delay(200);
      temp = temp + 1;
      lcd.setCursor(11, 0);
      lcd.print(String(temp));
    }
  }
  delay(200);
  rtc.setYear(temp);
  temp = 0;

  // MONTH
  while (digitalRead(SD_REMOVING) == 0) {

    if (digitalRead(button1) == 1) {
      delay(200);
      temp = temp + 1;
      if ((temp % 12) + 1 >= 10) {
        lcd.setCursor(6, 0);
      } else {
        lcd.setCursor(7, 0);
      }
      lcd.print((temp % 12) + 1);

    }

  }
  delay(200);
  rtc.setMonth((temp % 12) + 1);

  switch ((temp % 12) + 1) {
    case 1 && 3 && 5 && 7 && 8 && 10 && 12:
      days = 31;
      break;
    case 2:
      days = 28;
      break;
    default:
      days = 30;
      break;
  }

  temp = 0;

  // DAY
  while (digitalRead(SD_REMOVING) == 0) {

    if (digitalRead(button1) == 1) {
      delay(200);
      temp = temp + 1;
      if ((temp % days) + 1 >= 10) {
        lcd.setCursor(3, 0);
      } else {
        lcd.setCursor(4, 0);
      }
      lcd.print((temp % days) + 1);
    }

  }
  delay(200);
  rtc.setDay((temp % days) + 1);
  temp = 0;

  // TIME

  // HOURS

  while (digitalRead(SD_REMOVING) == 0) {

    if (digitalRead(button1) == HIGH) {
      delay(200);
      temp = temp + 1;
      if ((temp % 24) >= 10) {
        lcd.setCursor(4, 1);
      } else {
        lcd.setCursor(5, 1);
      }
      lcd.print((temp % 24));
    }
  }
  delay(200);
  rtc.setHours(temp);
  temp = 0;

  // MINUTES
  while (digitalRead(SD_REMOVING) == 0) {

    if (digitalRead(button1) == HIGH) {
      delay(200);
      temp = temp + 1;
      if ((temp % 60) + 1 >= 10) {
        lcd.setCursor(7, 1);
      } else {
        lcd.setCursor(8, 1);
      }
      lcd.print((temp % 60) + 1);
    }
  }
  delay(200);
  rtc.setMinutes(temp);



  return;
}
