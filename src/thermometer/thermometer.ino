#include "arduino_secrets.h"

//choose one
//#define USE_DHT22
#define USE_SHT31

#define USE_GSM //comment out to disable GSM IoT functionality
//#define DEBUGSERIAL

#include "U8g2lib.h"
#if defined USE_DHT22
#include <DHT.h>
#elif defined USE_SHT31
#include "Adafruit_SHT31.h"
#endif
//#include "Arduino.h"
#include <avr/dtostrf.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C mujOled(U8G2_R0);

#if defined USE_DHT22
DHT dht(A0, DHT22);
#elif defined USE_SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

int x;
int y;

#ifdef USE_GSM
#define TINY_GSM_MODEM_SIM900
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <string.h>
TinyGsm modem(Serial1);
const char apn[] = "internet.t-mobile.cz";
const char user[] = "gprs"; const char pass[] = "gprs";
TinyGsmClient gsm_client(modem);
const char server[]   = "io.adafruit.com";
PubSubClient mqttclient(gsm_client);
unsigned long last_adafruit_sent = 0;
#endif

unsigned long last_random = millis();

void update_lcd(char* row1, char* row2) {
     mujOled.firstPage();
    do {
        mujOled.setFont(u8g_font_10x20);  
        mujOled.setCursor(x,y);
        mujOled.print(row1);
        mujOled.setCursor(x,y+14);
        mujOled.print(row2);
    } while( mujOled.nextPage() );
}

#ifdef USE_GSM
int reconnect_mqtt() {
  int i = 0;
  while (!mqttclient.connected()) {
    #ifdef DEBUGSERIAL
    Serial.print("Attempting MQTT connection...");
    #endif

    if (mqttclient.connect("arduino",IO_USERNAME,IO_KEY)) {
      #ifdef DEBUGSERIAL
      Serial.println("connected");
      #endif
      //client.subscribe("inTopic");
    } else {
      #ifdef DEBUGSERIAL
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      #endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
    i++;
    if(i>4) {
      #ifdef DEBUGSERIAL
      Serial.println("MQTT (re)connection failed after reaching maximum number of attempts, exiting from reconnect_mqtt();");
      #endif
      return 1;
    }
  }
  return 0;
}
#endif

#ifdef USE_GSM
void send_to_adafruit_io(float val, char* key) {

if (!mqttclient.connected()) {
    analogWrite(LED_BUILTIN, 50); //turn on the led as an error sign
    if(reconnect_mqtt() != 0) {
      return;
    }
  }

  #ifdef DEBUGSERIAL
  Serial.println("Connected to Adafruit IO and sending data......");
  #endif
  
  char payload[8];
  sprintf(payload,"%.2f", val);
  #ifdef DEBUGSERIAL
  Serial.println("Publishing message... topic:");
  Serial.println(key);
  Serial.println("payload:");
  Serial.println(payload);
  #endif
  
  mqttclient.publish(key, payload, true);

}
#endif

void setup() {
  Serial1.begin(19200);
  #ifdef DEBUGSERIAL
  Serial.begin(9600);
  #endif
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, HIGH);
  delay(1000);
  digitalWrite(A1, LOW);

  /*
    if ( mujOled.getMode() == U8G_MODE_R3G3B2 ) 
    mujOled.setColorIndex(255);     // white
  else if ( mujOled.getMode() == U8G_MODE_GRAY2BIT )
    mujOled.setColorIndex(3);         // max intensity
  else if ( mujOled.getMode() == U8G_MODE_BW )
    mujOled.setColorIndex(1);         // pixel on
    */
  randomSeed(analogRead(2));

  #if defined USE_DHT22
  dht.begin();
  #elif defined USE_SHT31
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    //while (1) delay(1);
    delay(1000);return;
  }
  #endif

  mujOled.begin();
  delay(1000);

  #ifdef USE_GSM
  Serial.println("starting gsm...");
  modem.init();
  #ifdef DEBUGSERIAL
  Serial.println("waitfornetwork...");
  #endif
  modem.waitForNetwork(600000L);
  #ifdef DEBUGSERIAL
  Serial.println("gprs connect...");
  #endif
  modem.gprsConnect(apn, user, pass);
  mqttclient.setServer(server, 1883);
  reconnect_mqtt();
  #endif
  #ifdef DEBUGSERIAL
  Serial.println("entering loop now...");
  #endif
  }

void loop() {
  #ifdef USE_GSM
  mqttclient.loop();
  delay(1000);
  #endif
  static float t, h;
  if(millis() - last_random > 10000 || last_random == 0 || millis() < last_random) {
    #if defined USE_DHT22
    t = dht.readTemperature();
    #elif defined USE_SHT31
    t = sht31.readTemperature();
    #endif
    delay(500);

    #if defined USE_DHT22
    h = dht.readHumidity();
    #elif defined USE_SHT31
    h = sht31.readHumidity();
    #endif
    delay(500);
    
    char temperature[10];
    char buff1[5];
    if(t > -10.0f) {
      dtostrf(t, 4, 1, buff1);
    } else {
      dtostrf(t, 4, 0, buff1);
    }
    sprintf(temperature, "T: %s C", buff1);  
  
    char humidity[10];
    char buff2[5];
    dtostrf(h, 4, 1, buff2);
    sprintf(humidity, "V: %s %c", buff2, 0x25);

    x = random(0,38);
    y = random(13, 50);//13,50
    update_lcd(temperature,humidity);
    
    if(t >= 30.0f) {
      digitalWrite(A1, HIGH);
    } else {
      digitalWrite(A1, LOW);
    }
    last_random = millis();
  }
  #ifdef USE_GSM
  if(millis() - last_adafruit_sent > 30000 || last_adafruit_sent == 0 || millis() < last_adafruit_sent) {
    if (!isnan(t)) {
      send_to_adafruit_io(t, IO_USERNAME"/feeds/"IO_FEED_TEMP);
    } else {return;}
    if (!isnan(h)) {
      send_to_adafruit_io(h, IO_USERNAME"/feeds/"IO_FEED_HUM);
      } else {return;}
    last_adafruit_sent = millis();
  }
  #endif
  delay(10);
}
