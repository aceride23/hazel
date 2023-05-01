

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "eprom.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_NeoPixel.h>



void neoFriendFade();

#define timeSeconds 10

// Set GPIOs for LED and PIR Motion Sensor

const int motionSensor = D6;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;

// Checks if motion was detected, sets LED HIGH and starts a timer
ICACHE_RAM_ATTR void detectsMovement() {
  Serial.println("MOTION DETECTED!!!");
  startTimer = true;
  lastTrigger = millis();
  //detachInterrupt(motionSensor);
}





#define PIN          D5
#define PIN_VIB          D6
  // How many NeoPixels LEDs are attached to the ESP32?
#define NUMPIXELS  12

// We define birghtness of NeoPixel LEDs
#define BRIGHTNESS  2
// User stub
Adafruit_NeoPixel matrix = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);



/* Fill-in your Template ID (only if using Blynk.Cloud) */
//#define BLYNK_TEMPLATE_ID   "YourTemplateID"


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "AamirButt"
#define AIO_KEY         "aio_lTlv40EBoNLYdfepycGBSRJJN3Kz"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "nCqY0XeADSEhWHjL37mXNgZg7DfCG1p"; //Blynk Auth Token
String calendarData = "";
int remMin=0;
int F_R=210;
int F_B=165;
int F_G=0;
void heartBeat();

//Connection Settings
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort = 443;
unsigned long epochTime ;
unsigned long entryCalender, entryPrintStatus, entryInterrupt, heartBeatEntry, heartBeatLedEntry;
String url;
bool eventHere(int task);

#define UPDATETIME 10000

#ifdef CREDENTIALS
const char*  ssid = mySSID;
const char* password = myPASSWORD;
const char *GScriptIdRead = GoogleScriptIdRead;
const char *GScriptIdWrite = GoogleScriptIdWrite;
#else
//Network credentials
const char* ssid = "LJL"; //replace with you ssid, no apostrophes 
const char* password = "123456789"; //replace with your password
//Google Script ID
//script.google.com/macros/s//exec
const char *GScriptIdRead = "AKfycbxpUnGulsE1xUu_a4C_sT-SNzHSY1PBxgqwLubzd3Wo_U9ki-pdF50YebSkxGWhciwv8A"; //replace with you gscript id for reading the calendar
const char *GScriptIdWrite = "..........."; //replace with you gscript id for writing the calendar
#endif

#define NBR_EVENTS 4
int currentHour;
 int currentMinute ;

String  possibleEvents[NBR_EVENTS] = {"Laundry", "Meal",  "Telephone", "Shop"};
byte  LEDpins[NBR_EVENTS]    = {D2, D7, D4, D8};  // connect LEDs to these pins or change pin number here
byte  switchPins[NBR_EVENTS] = {D1, D3, D0, D6};  // connect switches to these pins or change pin number here
bool switchPressed[NBR_EVENTS];
boolean beat = false;
int beatLED = 0;

enum taskStatus {
  none,
  due,
  done
};

taskStatus taskStatus[NBR_EVENTS];
HTTPSRedirect* client = nullptr;


bool calenderUpToDate;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
//Connect to wifi
WidgetLCD lcd(V1);

void neoOnEvent()
{
matrix.setBrightness(200);
  for (uint16_t i = 0; i < matrix.numPixels(); i++)
  {
    matrix.setPixelColor(i, matrix.Color(255, 0, 0));
  }
  matrix.show();
}
void connectToWifi() {
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    ESP.reset();
  }
  Serial.println("Connected to Google");
}

void printStatus() {
  for (int i = 0; i < NBR_EVENTS; i++) {
    Serial.print("Task ");
    Serial.print(i);
    Serial.print(" Status ");
    Serial.println(taskStatus[i]);
  }
  Serial.println("----------");
}

void getCalendar() {
  //  Serial.println("Start Request");
  // HTTPSRedirect client(httpsPort);
  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    ESP.reset();
  }
  //Fetch Google Calendar events
  String url = String("/macros/s/") + GScriptIdRead + "/exec";
  client->GET(url, host);
  calendarData = client->getResponseBody();
  Serial.print("Calendar Data---> ");
  Serial.println(calendarData);
  calenderUpToDate = true;
  yield();
}

void createEvent(String title) {
  // Serial.println("Start Write Request");

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }
  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    ESP.reset();
  }
  // Create event on Google Calendar
  String url = String("/macros/s/") + GScriptIdWrite + "/exec" + "?title=" + title;
  client->GET(url, host);
  //  Serial.println(url);
  Serial.println("Write Event created ");
  calenderUpToDate = false;
}

void manageStatus() {
  for (int i = 0; i < NBR_EVENTS; i++) {
    switch (taskStatus[i]) {
      case none:
        if (switchPressed[i]) {
          digitalWrite(LEDpins[i], HIGH);
          while (!calenderUpToDate) getCalendar();
          if (!eventHere(i)) createEvent(possibleEvents[i]);
          Serial.print(i);
          Serial.println(" 0 -->1");
          //getCalendar();
          taskStatus[i] = due;
        } else {
          if (eventHere(i)) {
            digitalWrite(LEDpins[i], HIGH);
            Serial.print(i);
            Serial.println(" 0 -->1");
            taskStatus[i] = due;
          }
        }
        break;
      case due:
        if (switchPressed[i]) {
          digitalWrite(LEDpins[i], LOW);
          Serial.print(i);
          Serial.println(" 1 -->2");
          taskStatus[i] = done;
        }
        break;
      case done:
        if (calenderUpToDate && !eventHere(i)) {
          digitalWrite(LEDpins[i], LOW);
          Serial.print(i);
          Serial.println(" 2 -->0");
          taskStatus[i] = none;
        }
        break;
      default:
        break;
    }
    switchPressed[i] = false;
  }
  yield();
}

bool eventHere(int task) {
  if (calendarData.lastIndexOf(possibleEvents[task], 0) >= 0 ) {
    //    Serial.print("Task found ");
    //    Serial.println(task);
    return true;
  } else {
    //   Serial.print("Task not found ");
    //   Serial.println(task);
    return false;
  }
}

ICACHE_RAM_ATTR void handleInterrupt() {
  if (millis() > entryInterrupt + 100) {
    entryInterrupt = millis();
    for (int i = 0; i < NBR_EVENTS; i++) {
      if (digitalRead(switchPins[i]) == LOW) {
        switchPressed[i] = true;
      }
    }
  }
}

void getNTP() {
  timeClient.update();

  epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  

   currentHour = timeClient.getHours();
  Serial.print("Hour: ");
  Serial.println(currentHour);  

  currentMinute = timeClient.getMinutes();
  Serial.print("Minutes: ");
  Serial.println(currentMinute); 
  
}

void  offNeo() {
uint16_t i, j;

for (i = 0; i <=matrix.numPixels(); i++) {
matrix.setPixelColor(i,0,0,0);
}
matrix.setBrightness(0);
matrix.show();

}

void  brighten() {
uint16_t i, j;

for (j = 0; j < 255; j++) {
for (i = 0; i <=matrix.numPixels(); i++) {
matrix.setPixelColor(i,255,255,0); // i= number of pixels, r, g, b
}
matrix.setBrightness(j);
if(startTimer==true)
break;
matrix.show();
delay(10);
if(startTimer==true)
break;
}
//delay(1500);

}

// 255 to 0
void darken() {
uint16_t i, j;

for (j = 255; j > 0; j--) {
for (i = 1; i <=matrix.numPixels(); i++) {
matrix.setPixelColor(i,255,255,0); // yellow with a little extra red to make it warmer. Instead of “strip.setPixelColor(i, 0, 0, j);”
}
matrix.setBrightness(j);
if(startTimer==true)
break;
matrix.show();
delay(5);
if(startTimer==true)
break;
}
//delay(1500);

}

void neoCriticalFade()
{

uint16_t i, j;
matrix.setBrightness(200);

for (j = 0; j < 255; j++) {
for (i = 0; i <=matrix.numPixels(); i++) {

matrix.setPixelColor(i,255,255-j,0);
}
if(j<200)
{
  if(startTimer==true)
break;
matrix.show();
}
delay(10);
if(startTimer==true)
break;
}

}

void neoFriendFade()
{

uint16_t i, j;

for (j = 0; j < 255; j++) {
for (i = 0; i <=matrix.numPixels(); i++) {
matrix.setPixelColor(i,F_R,F_G,F_B);
}
matrix.setBrightness(j);
matrix.show();
delay(10);
}

for (j = 255; j > 0; j--) {
for (i = 1; i <=matrix.numPixels(); i++) {
matrix.setPixelColor(i,F_R,F_G,F_B); // yellow with a little extra red to make it warmer. Instead of “strip.setPixelColor(i, 0, 0, j);”
}
matrix.setBrightness(j);
matrix.show();
delay(5);

}

}


void neoSafetyFade()
{
brighten();
darken();

}
void scheduleNEO()
{

  String up_event1=calendarData.substring(0,calendarData.lastIndexOf("S"));
  Serial.println("EVENT_1: "+up_event1);

 double event_st=calendarData.substring(calendarData.lastIndexOf("S")+1,calendarData.lastIndexOf("E")).toDouble();
double event_en=calendarData.substring(calendarData.lastIndexOf("E")+1,calendarData.lastIndexOf("L")).toDouble();
event_st=event_st/1000;
event_en=event_en/1000;
Serial.println("Current EPOCH Time"+(String)(epochTime));
Serial.println("EVENT start Time: "+(String)event_st);
Serial.println("EVENT end Time: "+(String)event_en);
double remEpoch=(remMin*60000)/1000;
Serial.println("EVENT Reminder Time: "+(String)remEpoch);
if(epochTime>(event_st-remEpoch) && epochTime<(event_st-(remEpoch)/2))
{
  Serial.println("Start Safety Time Neo Yellow");
  neoSafetyFade();
}
else if (epochTime>(event_st-(remEpoch)/2) && epochTime<event_st)
{
   Serial.println("Start Critcial Time Neo Yellow to Orange");
  neoCriticalFade();
}
else if (epochTime>(event_st) && epochTime<(event_en))
{
   Serial.println("Start on Event Neo Red");
  neoOnEvent();
}
}

void uploadData()
{
  String up_event1=calendarData.substring(0,calendarData.lastIndexOf("S"));
  Serial.println("EVENT_1: "+up_event1);

lcd.clear();
int sc1_h=up_event1.substring(up_event1.lastIndexOf(";")+1,up_event1.lastIndexOf(":")).toInt();
int sc1_m=up_event1.substring(up_event1.lastIndexOf(":")+1,up_event1.lastIndexOf("S")).toInt();
Serial.println("SC1_h: "+(String)sc1_h);
Serial.println("SC1_m: "+(String)sc1_m);

lcd.print(0,0,up_event1);


scheduleNEO();
}
WiFiClient clientWifi;
Adafruit_MQTT_Client mqtt(&clientWifi, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/color");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/color");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();



void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}






void setup() {
  Serial.begin(115200);
  Serial.println("HELLO");
  pinMode(PIN_VIB,INPUT_PULLUP);
matrix.begin();
EEPROM.begin(50);
remMin=readIntFromEEPROM(0);
 Blynk.begin(auth, ssid, password);
  Serial.println("ReminderTime: "+(String)remMin);
  Serial.println("Reminder_V2");
  for (int i = 0; i < NBR_EVENTS; i++) {
    pinMode(LEDpins[i], OUTPUT);
    taskStatus[i] = none;  // Reset all LEDs
    pinMode(switchPins[i], INPUT_PULLUP);
    switchPressed[i] = false;
    attachInterrupt(digitalPinToInterrupt(switchPins[i]), handleInterrupt, FALLING);
  }
  connectToWifi();
  getCalendar();
  entryCalender = millis();

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600*5);
  getNTP();
 
    
 // matrix.setBrightness(BRIGHTNESS);
  neoSafetyFade();
neoCriticalFade();
mqtt.subscribe(&onoffbutton);

  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

}


void loop() {
   Blynk.run();
  if (millis() > entryCalender + UPDATETIME) {
    getCalendar();
    entryCalender = millis();
    getNTP();
     Blynk.run();

  scheduleNEO();
    uploadData();
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      String MQTTpayload=(char *)onoffbutton.lastread;
      Serial.println(MQTTpayload);
      F_R=MQTTpayload.substring(MQTTpayload.lastIndexOf("R")+1,MQTTpayload.lastIndexOf("G")).toInt();
     F_G=MQTTpayload.substring(MQTTpayload.lastIndexOf("G")+1,MQTTpayload.lastIndexOf("B")).toInt();
      F_B=MQTTpayload.substring(MQTTpayload.lastIndexOf("B")+1,MQTTpayload.lastIndexOf("E")).toInt();
  neoFriendFade();
    }
  }

  }
  manageStatus();
  if(startTimer==true)
  {
    neoFriendFade();
    startTimer=false;
   
  }

}



BLYNK_WRITE(V0)
{
  remMin = param.asInt(); // Get value as integer
  Serial.println("Reminder Minutes: "+(String)remMin);
writeIntIntoEEPROM(0,remMin);

}
BLYNK_WRITE(V2)
{
  F_R = param[0].asInt(); // Get value as integer
    F_G = param[1].asInt(); // Get value as integer
      F_B = param[2].asInt(); // Get value as integer
  Serial.println("Frined Color: "+(String)F_R+(String)F_G+(String)F_B);

neoFriendFade();


String color="R"+(String)(F_R)+"G"+(String)(F_G)+"B"+(String)(F_B)+"E";
  // Now we can publish stuff!
  Serial.print(F("\nSending photocell val "));
  Serial.print(color);
  Serial.print("...");
  if (! photocell.publish(color.c_str())) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
}
