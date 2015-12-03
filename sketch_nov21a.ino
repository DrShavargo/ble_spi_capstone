#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE      1

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

int32_t hsServiceId;
int32_t hsDataCharId;

float thresholdX = 135;
float thresholdY = 270; //or more
float thresholdZ = 220;
int shakes = 0;
unsigned long time = millis();

void setup()
{
  Serial.begin(115200);
  Serial.print(F("Initialising the module: "));

  if ( !ble.begin(VERBOSE_MODE) ){ error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?")); }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE ){
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  ble.info();

  // MAKE TRUE IF DEBUGGING
  ble.verbose(true);

  Serial.println(F("Setting device name to 'SmartBand B': "));
  if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=SmartBand B")) ) {
    error(F("Could not set device name?"));
  }

  Serial.println(F("Adding the Handshake Service definition "));
  int success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID128=00-77-13-12-11-00-00-00-00-00-AB-BA-0F-A1-AF-E1"), &hsServiceId);
  if (!success) {
    error(F("Could not add Handshake service"));
  }

  Serial.println(F("Adding the Handshake Data characteristic "));
  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0x4201,PROPERTIES=0x12,MIN_LEN=1,VALUE=0"), &hsDataCharId);
  if (!success) {
    error(F("Could not add Handshake characteristic"));
  }

  Serial.println(F("Adding Handshake Service UUID to the advertising payload "));
  ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-03-02-12-13") );

  Serial.println(F("Performing a SW reset (service changes require a reset) "));
  ble.reset();
}

void loop()
{
  
  // Get raw accelerometer data for each axis
  int rawX = analogRead(A3);
  int rawY = analogRead(A4);
  int rawZ = analogRead(A5);

  if(within(rawX, thresholdX)
    && withinI(rawY, thresholdY)
    && withinI(rawZ, thresholdZ)
    && millis() - time < 1000) {
    shakes++;
  }

  if(shakes > 6){  
    ble.print( F("AT+GATTCHAR=") );
    ble.print( hsDataCharId );
    ble.println( ",1" );
    ble.waitForOK();
    Serial.println("HANDSHAKE DETECTED!");
    shakes = 0;
  }

  if(millis() - time >= 1000){
    shakes = 0;
    time = millis();
   }
  
  delay(20);
}

bool within(float x, float t){
  return ((x < t) || (675-x < t));
}

bool withinI(float x, float t){
  return ((x > t) && (675-x > t));
}

