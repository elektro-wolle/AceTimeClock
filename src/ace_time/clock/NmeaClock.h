/*
 * MIT License
 * Copyright (c) 2025 Wolfgang Jung
 */
#ifndef ACE_TIME_NMEA_CLOCK_H
#define ACE_TIME_NMEA_CLOCK_H

#include <stdint.h>

#include "Clock.h"

namespace ace_time {
namespace clock {

#ifdef NMEA_CLOCK_STATS
    /** Stats from the GPS driven clock. */
    class NmeaClockStats {
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
        uint32_t m_ageOfLastSync;
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
    class NmeaClock : public Clock {
    public:
        /** Number of milliseconds after the second switch until the NMEA message is read. */
        explicit NmeaClock(const uint16_t msOffsetPPStoMessage = 250)
            : m_msOffsetPPStoMessage(msOffsetPPStoMessage)
        {
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
            return m_lastSyncedGpsTime + ((millis() - m_millisAtLastSync) / 1000);
        };

        /** milis() when the last successful sync happened. */
        const uint32_t getMillisOfLastSync() { return m_millisAtLastSync; }

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
        const NmeaClockStats m_stats;
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