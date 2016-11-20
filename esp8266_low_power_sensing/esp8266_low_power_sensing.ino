
// SDK headers are in C, include them extern or else they will throw an error compiling/linking
// all SDK .c files required can be added between the { }

extern "C" {
#include "user_interface.h"
}

#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


#define _DEBUG_
#define ALIVEPIN 2  //GPIO2 Pin

#define RTCMEMORYSTART 64
#define RTCMEMORYLENGTH 128

#define SECOND       1000000
#define MINUTE         (60*SECOND)
#define HOUR           (60*MINUTE)
#define HOURS_DAY       24
#define STATUS_HOUR    4

#define NO_EVENT 0
#define SEND_STATUS_EVENT 0x1
#define SEND_SENSOR_EVENT 0x2
#define SEND_START_EVENT  0x4
#define MAX_EVENT (SEND_STATUS_EVENT | SEND_SENSOR_EVENT| SEND_START_EVENT)

// Forward declarations
void sleep_rf();
void sleep_no_rf();
bool setup_wifi();
void reset_and_goto_sleep();

typedef struct {
  int counter;
  int event;
  int battery;
} rtcStore;

char ssid[] = mySSID;
char password[] = myPassword;

WiFiClient espClient;
PubSubClient client(espClient);

rtcStore rtcMem;
struct rst_info *infoPtr;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

ADC_MODE(ADC_VCC);

void setup() {

  pinMode(ALIVEPIN, OUTPUT);
  digitalWrite(ALIVEPIN, LOW); // Signal to Tiny that i'm  alive

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  Serial.println("");
  Serial.print("Wake reason: ");
  Serial.println(ESP.getResetReason());

  // Check why we was woken up
  infoPtr = ESP.getResetInfoPtr();

  // Read persistent storage
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));

  // Check for strange start or first time events
  if ( (infoPtr->reason != REASON_DEFAULT_RST) && (infoPtr->reason != REASON_DEEP_SLEEP_AWAKE) ) {

    Serial.println("DETECTED_START EVENT");
    rtcMem.event = SEND_START_EVENT; // Clear all bits except the START bit
    rtcMem.counter = HOURS_DAY;
    system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));
    sleep_rf(); // Sleep and reboot
  }

  // If no previous EVENTs has been detected, go check what this wakeup is all about
  // A comment on the test. If I experience a PIR sensor event the ESP will be reset and loose RTC memory.
  // In that case I can not look at rtcMem, so instead I interpret all REASON_DEFAULT_RST as PIR sensor events
  if ( (rtcMem.event ==  NO_EVENT) || (infoPtr->reason ==  REASON_DEFAULT_RST) ) {

    if (infoPtr->reason ==  REASON_DEFAULT_RST) {
      // PIR wakeup´
      Serial.println("******** SENSOR EVENT");

      rtcMem.event = SEND_SENSOR_EVENT; // Clear all bits except the SENSOR bit
      rtcMem.counter = HOURS_DAY ; // The counter is lost after a PIR sensor event, so just restart it
      system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));
      sleep_rf(); // Sleep and reboot

    } if (infoPtr->reason ==  REASON_DEEP_SLEEP_AWAKE) {
      // Deep sleep wakeup
      Serial.println("******** ONE HOUR EVENT");

      rtcMem.counter--;
      if (rtcMem.counter <= 0) {

        Serial.println("******** STATUS EVENT");
        rtcMem.event |= SEND_STATUS_EVENT;
        rtcMem.counter = HOURS_DAY;
        system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));
        sleep_rf(); // Sleep and reboot

      } else {
        char count_str[10];
        sprintf (count_str, "%ld", rtcMem.counter);
        Serial.println("");
        Serial.print("Hours to go: ");
        Serial.println(count_str);

        system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));
        sleep_no_rf(); // Sleep and reboot
      }
    }
  }

  printMacAddress();
  readCredentials(ssid, password);

  if (!setup_wifi()) {
    reset_and_goto_sleep();
  }

  iotUpdater(true);

  setup_mqtt();

  timeClient.begin();
}


void loop()
{

  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()) {
    if (rtcMem.event & SEND_SENSOR_EVENT) {

      Serial.println("MQTT:SEND_SENSOR_EVENT");
      char str[10];
      char result[50];
      float vdd = ESP.getVcc() / 1000.0 ;
      dtostrf( vdd, 4, 2, str);
      client.publish("home/topfloor/livingroom/motion", "motion");
      client.publish("home/topfloor/livingroom/motion/voltage", str, true);

      rtcMem.event &= ~SEND_SENSOR_EVENT;

    }

    if (rtcMem.event & SEND_STATUS_EVENT) {

      Serial.println("MQTT:SEND_STATUS_EVENT");

      char str[10];
      char result[50];
      float vdd = ESP.getVcc() / 1000.0 ;
      dtostrf( vdd, 4, 2, str);
      client.publish("home/topfloor/livingroom/motion/voltage", str, true);

      rtcMem.event &= ~SEND_STATUS_EVENT;

    }

    if (rtcMem.event & SEND_START_EVENT) {

      Serial.println("MQTT:SEND_START_EVENT");

      char str[10];
      char result[50];
      float vdd = ESP.getVcc() / 1000.0 ;
      dtostrf( vdd, 4, 2, str);
      client.publish("home/topfloor/livingroom/motion/voltage", str, true);

      rtcMem.event &= ~SEND_START_EVENT;
    }

    system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));

    adjust_time(&timeClient);
  }
  sleep_no_rf();
}



void sleep_rf() {

  Serial.println("BOOT TO RF");
  delay(100);

  digitalWrite(ALIVEPIN, HIGH); // Signal to Tiny that i'm  going to sleep

  system_deep_sleep_set_option(WAKE_RF_DEFAULT);
  system_deep_sleep(100000); //sleep time 100 ms
  delay(100); // Neccessary for the deep_sleep to work
}



void sleep_no_rf() {
  sleep_no_rf(HOUR);
}

void sleep_no_rf(uint32 sleep_time) {

  Serial.println("BOOT TO NO RF");
  delay(100);

  digitalWrite(ALIVEPIN, HIGH); // Signal to Tiny that i'm  going to sleep

  system_deep_sleep_set_option(WAKE_RF_DISABLED);

  system_deep_sleep(sleep_time); //sleep time

  delay(100); // Neccessary for the deep_sleep to work

}




bool setup_wifi() {

  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int i = 20;
  while (WiFi.status() != WL_CONNECTED && i > 0) {
    delay(500);
    i--;
    Serial.print(".");
  }

  if (i == 0) {
    return false;
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}





void reset_and_goto_sleep() {

  rtcMem.event = NO_EVENT;
  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcStore));
  sleep_no_rf();

}

