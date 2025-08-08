/*
 * MIT License
 * Copyright (c) 2025 Wolfgang Jung
 */
#include "NmeaClock.h"

namespace ace_time {
namespace clock {

    void NmeaClock::parse(int nmeaChar)
    {
        if (nmeaChar <= 0 || nmeaChar >= 0x7f) {
            return;
        }
        if (nmeaChar == '$' || nmeaChar == '!') {
            // start of message
            m_recvState = IN_MESSAGE;
            // start of message
            m_runningCheckSum = 0;
            m_checkSum = 0;
            m_writePosition = 0;
            return;
        } else if (nmeaChar == '*') {
            // start of checksum
            m_recvState = IN_CHECKSUM;
        } else if (m_writePosition >= 100) {
            // message longer, than expected, ignore
            m_recvState = WAIT_FOR_START;
            return;
        } else if (nmeaChar == '\n' || nmeaChar == '\r') {
            // end of line, can occur twice (if windows line endings were sent)
            if (m_recvState == IN_CHECKSUM) {
                // parse the complete NMEA sentence
                parseMessage();
            }
            m_recvState = WAIT_FOR_START;
        }

        if (m_recvState == IN_MESSAGE) {
            // only xor the current char, if it is after the $ at the start and before the * mark for the hash
            m_runningCheckSum ^= (uint8_t)nmeaChar;
        } else if (m_recvState == IN_CHECKSUM && nmeaChar != '*') {
            // parse the two digit checksum
            m_checkSum = m_checkSum * 16 + parseHex(nmeaChar);
        }
        m_messageBuffer[m_writePosition++] = nmeaChar;
        m_messageBuffer[m_writePosition] = '\0';
    }

    void NmeaClock::parseMessage()
    {
#ifdef NMEA_CLOCK_STATS
        m_stats.m_messages++;
#endif
        if (m_checkSum != m_runningCheckSum) {
#ifdef NMEA_CLOCK_STATS
            m_stats.m_checksumFailed++;
#endif
            return;
        }
        // ignore non $GPRMC messages
        if (strncmp(m_messageBuffer, "GPRMC", 5) != 0) {
            return;
        }

        char* endOfCurrentToken = m_messageBuffer;
        char* token = m_messageBuffer;

        uint8_t field_count = 0;

        char* time_str = nullptr;
        char* date_str = nullptr;
        bool valid_message = false;

#ifdef NMEA_CLOCK_STATS
        m_stats.m_gprmc++;
#endif
        while (*token != '\0' && field_count < 12) {
            // strtok implementation, that handles empty fields
            while (*endOfCurrentToken != '\0') {
                if (*endOfCurrentToken == ',' || *endOfCurrentToken == '*') {
                    *endOfCurrentToken = '\0';
                    break;
                }
                endOfCurrentToken++;
            }
            // handle the three fields of interest
            if (field_count == 1) {
                time_str = token;
            } else if (field_count == 2) {
                // valid 'A' and invalid messages 'V'
                valid_message = (token[0] == 'A');
            } else if (field_count == 9) {
                date_str = token;
            }
            // advance to the next token
            token = endOfCurrentToken + 1;
            endOfCurrentToken = token;
            field_count++;
        }

        // Only process if message is valid and we have both time and date
        if (!valid_message || time_str == nullptr || date_str == nullptr || strlen(time_str) < 6 || strlen(date_str) < 6 || field_count != 12) {
#ifdef NMEA_CLOCK_STATS
            m_stats.m_missingFields++;
#endif
            return;
        }

        // parse the date and time fields from the NMEA message
        uint8_t hour = intValue(time_str + 0);
        uint8_t minute = intValue(time_str + 2);
        uint8_t second = intValue(time_str + 4);
        uint8_t day = intValue(date_str + 0);
        uint8_t month = intValue(date_str + 2);
        uint16_t year = intValue(date_str + 4) + 2000; // pre 2000 messages will no longer be sent

        // Validate ranges. And, yes, seconds can go from 0 to 60 inclusive
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60 || day < 1 || day > 31 || month < 1 || month > 12) {
#ifdef NMEA_CLOCK_STATS
            m_stats.m_invalidData++;
#endif
            return;
        }
        // convert the GPS time to UTC
        ZonedDateTime zdt = ZonedDateTime::forComponents(year, month, day, hour, minute, second, TimeZone::forUtc());
        m_lastSyncedGpsTime = zdt.toEpochSeconds();
        m_millisAtLastSync = millis() - m_msOffsetPPStoMessage;
    }
}
}
