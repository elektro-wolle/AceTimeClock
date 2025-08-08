/*
 * A program to demonstrate the use of the HelloNmeaClock<T> class. It should print
 * the following on the SERIAL_PORT_MONITOR port every 2 seconds, if there is 
 * a GPS receiver connected to pin 2:
 *
 *   2021-10-18T10:28:00
 *   2021-10-18T10:28:02
 *   2021-10-18T10:28:04
 *   ...
 */

#include <Arduino.h>
#include <AceTime.h> // TimeZone, LocalDateTime
#include <AceTimeClock.h> // DS3231Clock
#include <AceWire.h> // TwoWireInterface
#include <Wire.h> // TwoWire, Wire

#include <SoftwareSerial.h>

using ace_time::acetime_t;
using ace_time::TimeZone;
using ace_time::ZonedDateTime;
using ace_time::clock::NmeaClock;
using ace_time::zonedb::kZoneAmerica_Los_Angeles;
using ace_time::BasicZoneProcessor;

// ESP32 does not define SERIAL_PORT_MONITOR
#ifndef SERIAL_PORT_MONITOR
#define SERIAL_PORT_MONITOR Serial
#endif

SoftwareSerial gpsSerial(13, -1);

static BasicZoneProcessor losAngelesProcessor;

TimeZone losAngelesTz = TimeZone::forZoneInfo(
    &kZoneAmerica_Los_Angeles,
    &losAngelesProcessor);

//-----------------------------------------------------------------------------

NmeaClock nmeaClock;

void printCurrentTime() {
  acetime_t now = nmeaClock.getNow();
  ZonedDateTime zdt = ZonedDateTime::forEpochSeconds(now, losAngelesTz);
  zdt.printTo(SERIAL_PORT_MONITOR);
  SERIAL_PORT_MONITOR.println();
}

//-----------------------------------------------------------------------------

void setup() {
#if ! defined(EPOXY_DUINO)
  delay(1000);
#endif
  SERIAL_PORT_MONITOR.begin(115200);
  gpsSerial.begin(9600);
  gpsSerial.listen();
  while (!SERIAL_PORT_MONITOR); // Wait until ready - Leonardo/Micro
}

// Do NOT use delay() here.
void loop() {
  if (gpsSerial.available()) {
    nmeaClock.parse(gpsSerial.read());
  }

  static uint16_t prevMillis;
  uint16_t nowMillis = millis();
  if ((uint16_t) (nowMillis - prevMillis) >= 2000) {
    printCurrentTime();
    prevMillis = nowMillis;
  }
}
