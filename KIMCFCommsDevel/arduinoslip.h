#pragma once

#ifndef __ARDUINOSLIP_H__
    #define __ARDUINOSLIP_H__

    #include "slipstream.h"
    #include <Stream.h>

namespace sproto {

    /**
     * @brief Arduino/Teensy specific SLIP + CRC protocol implementation.
     *
     * @tparam S Stream class to use. Usually <Serial>, <Serial1>, <Serial2>, etc.
     */
    template <class S>
    class ArduinoSlipStream : public SlipStream<ArduinoSlipStream<S>> {
        typedef SlipStream<ArduinoSlipStream<S>> base_t;
        friend base_t;

     public:
        /**
         * @brief Construct a new Arduino Slip Protocol object.
         *
         * **Implementation notes**: Arduino Stream::readBytesUntil discards the terminator
         * character. So there is no simple way to tell if the terminator character
         * was actually received by reading the buffer. But readBytesUntil will also return
         * after a timeout. We keep track of a slightly shorter timeout than the natural
         * Arduino Stream read timeout. If the readBytesUntil took longer than this, we
         * assume the Stream::read encountered a timeout and that no terminator character was
         * found. We use that to return an error condition.
         *
         * @param stream Usually Serial, Serial1, Serial2, etc
         * @param timeout readBytesUntil timeout.
         */
        ArduinoSlipStream(S& stream, unsigned long timeout = 990)
            : base_t(), stream_(stream), timeout_(timeout) {
        }

        /** Start the output stream */
        void begin() {
            stream_.begin(115200);
            while (!stream_) {
                ; // wait for serial port to connect. Needed for native USB port only
            }
            stream_.setTimeout(timeout_ + 10);
        }

        /** Stop the output stream */
        void end() {
            stream_.end();
        }

     protected:
        /**
         * @copydoc SlipProtocolBase::writeBytes
         * @details CRTP implementation.
         */
        size_t writeBytes_impl(const uint8_t* buffer, size_t size) {
            return stream_.write(reinterpret_cast<const char*>(buffer), size);
        }

        /**
         * @copydoc SlipProtocolBase::readBytesUntil
         * @details CRTP implementation.
         */
        error_t readBytesUntil_impl(uint8_t* buffer, const size_t size, const char terminator, size_t& nread) {
            const unsigned long startMillis = millis();
            // Arduino specific implementation. Could have its own timeout.
            // Kludg: Keep our timeout shorter.
            // calls Serial.readBytesUntil(terminator, buffer, size) or similar.
            nread = stream_.readBytesUntil(terminator, reinterpret_cast<char*>(buffer), size);
            if (millis() - startMillis >= timeout_) {
                return ERROR_TIMEOUT;
            }
            return NO_ERROR;
        }

        /**
         * @copydoc SlipProtocolBase::hasBytes
         * @details CRTP implementation.
         */
        bool hasBytes_impl() {
            return stream_.available();
        }

        /**
         * @copydoc SlipProtocolBase::writeNow
         * @details CRTP implementation.
         */
        void writeNow_impl() {
            stream_.flush();
            return true;
        }

        /**
         * @copydoc SlipProtocolBase::clearInput
         * @details implementation.
         */
        void clearInput_impl() {
            Serial.clear();
        }

        /**
         * @copydoc SlipProtocolBase::isStreamReady
         * @details implementation
         */
        bool isStreamReady_impl() {
            return stream_;
        }

        S& stream_;             ///< Aruino stream to write to
        unsigned long timeout_; ///< Terminated read timeout in msec
    };

}; // namespace

#endif // #ifndef __ARDUINOSLIP_H__