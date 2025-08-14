/*
 * MIT License
 * Copyright (c) 2025 Wolfgang Jung
 */
#ifndef ACE_TIME_NMEA_CLOCK_H
#define ACE_TIME_NMEA_CLOCK_H

#include <stdint.h>

#include "Clock.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace ace_time {
namespace clock {

#ifdef NMEA_CLOCK_STATS
    /** Stats from the GPS driven clock. */
    class NmeaClockStats {
    public:
        friend class NmeaClock;

        const uint32_t getMessages() { return m_messages; }
        const uint32_t getChecksumFailed() { return m_checksumFailed; }
        const uint32_t getGprmcMessages() { return m_gprmc; }
        const uint32_t getMessagesWithMissingFields() { return m_missingFields; }
        const uint32_t getMessagesWithMissingInvalidData() { return m_invalidData; }

    private:
        uint32_t m_messages;
        uint32_t m_checksumFailed;
        uint32_t m_gprmc;
        uint32_t m_missingFields;
        uint32_t m_invalidData;
    };
#endif
    /** A clock, that synchronizes to an external NMEA compliant GPS receiver.
     *
     * It can be used e.g. with a SoftwareSerial connection to the GPS receiver:
     *
     * static NmeaClock nmeaClock;
     *  static SystemClockLoop systemClock(&nmeaClock, (Clock*)0);
     *
     *  SoftwareSerial gps(D7, -1);
     *  void setup()
     *  {
     *      gps.begin(9600);
     *      gps.listen();
     *      systemClock.setup();
     *  }
     *  void loop()
     *  {
     *      systemClock.loop();
     *      if (gps.available()) {
     *          nmeaClock.parse(gps.read());
     *      }
     * ...
     *  }
     */

    extern IRAM_ATTR volatile uint32_t _nmeaMillisAtLastPPS;
    extern IRAM_ATTR volatile uint32_t _nmeaSecondsFromPPS;
    IRAM_ATTR void _nmeaPPShandler();

    class NmeaClock : public Clock {
    public:
        /**
         * @param msOffsetPPStoMessage Number of milliseconds after the second switch until the NMEA message is read.
         * @param ppsPin Pin on which a PPS signal (rising edge) is applied to (optional)
         */
        explicit NmeaClock(const uint16_t msOffsetPPStoMessage = 250, const int8_t ppsPin = -1)
            : m_msOffsetPPStoMessage(msOffsetPPStoMessage)
#ifdef NMEA_CLOCK_STATS
            , m_stats(NmeaClockStats())
#endif
        {
            if (ppsPin >= 0) {
                attachInterrupt(ppsPin, _nmeaPPShandler, RISING);
            }
        }

        /** parse a single char from the NMEA stream. */
        void parse(const int nmeaChar);
#ifdef NMEA_CLOCK_STATS
        /** Stats about the clock. */
        const NmeaClockStats& getStats() { return m_stats; }
#endif

        /** The expected current time accoroding to the last GPS sync, the time of that GPS sync
         * and the current millis().
         */
        acetime_t getNow() const override
        {
            if (m_lastSyncedGpsTime == kInvalidSeconds) {
                return kInvalidSeconds;
            }
            uint32_t ppsAge = millis() - _nmeaMillisAtLastPPS;
            if (ppsAge < 1010) { // even a fast running local clock should not run faster than 101%
                return m_lastSyncedGpsTime + _nmeaSecondsFromPPS;
            }
            uint32_t syncAge = millis() - m_millisAtLastSync;
            return m_lastSyncedGpsTime + (syncAge / 1000);
        };
        /** milis() when the last successful sync happened. */
        const uint32_t getMillisOfLastSync() { return ::max(_nmeaMillisAtLastPPS, m_millisAtLastSync); }
        const bool isPPSsynced() { return (millis() - _nmeaMillisAtLastPPS) < 1100; }

    private:
        /** buffer, pointers and state for incoming messages */
        char m_messageBuffer[100];
        uint8_t m_writePosition = 0;
        uint8_t m_checkSum;
        uint8_t m_runningCheckSum;
        enum RecvState {
            WAIT_FOR_START,
            IN_MESSAGE,
            IN_CHECKSUM,
        };
        RecvState m_recvState;

        /** average time between PPS and the receiving of the GPRMC message. */
        const uint16_t m_msOffsetPPStoMessage;
        /** Last known, valid, GPS time. */
        acetime_t m_lastSyncedGpsTime = kInvalidSeconds;
        /** System time of the the last GPS time to us, if GPS receiption is missing. */
        uint32_t m_millisAtLastSync = 0;

/** Some stats, enable with NMEA_CLOCK_STATS */
#ifdef NMEA_CLOCK_STATS
        NmeaClockStats m_stats;
#endif

        uint8_t intValue(const char* twoDigitStr)
        {
            return (twoDigitStr[0] - '0') * 10 + (twoDigitStr[1] - '0');
        }

        void parseMessage();

        uint8_t parseHex(char c)
        {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            } else if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            return 0;
        }
    };

}
}

#endif