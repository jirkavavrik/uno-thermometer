//#define USE_DHT22
#define USE_SHT31

#include "U8g2lib.h"
#if defined USE_DHT22
#include <DHT.h>
#elif defined USE_SHT31
#include "Adafruit_SHT31.h"
#endif
#include "Arduino.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C mujOled(U8G2_R0);

#if defined USE_DHT22
DHT dht(A0, DHT22);
#elif defined USE_SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

int x;
int y;

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

void setup() {
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
  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  #endif

  mujOled.begin();
  
  delay(1000);
  }

void loop() {
  if(millis() - last_random > 10000 || last_random == 0 || millis() < last_random) {
    #if defined USE_DHT22
    float t = dht.readTemperature();
    delay(1000);
    #elif defined USE_SHT31
    float t = sht31.readTemperature();
    #endif

    #if defined USE_DHT22
    float h = dht.readHumidity();
    delay(1000);
    #elif defined USE_SHT31
    float h = sht31.readHumidity();
    #endif
    
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
}
