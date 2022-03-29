#include "Arduino.h"
#include <ArduinoJson.h>

#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL)
#    define log_begin()             \
        {                           \
            SerialUSB1.begin(9600); \
            while (!SerialUSB1)     \
                ;                   \
        }
#    define log_print(val) \
        { SerialUSB1.print(val); }
#    define log_println(val) \
        { SerialUSB1.println(val); }
#    define log_json(doc)                   \
        {                                   \
            SerialUSB1.print("JSON: ");     \
            serializeJson(doc, SerialUSB1); \
        }
#else
#    define log_begin()
#    define log_print(val)
#    define log_println(val)
#    define log_json(doc)
#endif

void setup() {
    Serial.begin(9600);
    log_begin();

    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // prints title with ending line break

    StaticJsonDocument<200> doc;
    doc.add("Source");
    doc.add("ArduinoCoreTestFirmware");
    doc.add(millis());
    serializeJsonPretty(doc, SerialUSB1);
    log_println();
    log_println("log started for ArduinoCoreTestFirmware");
}

int loopnum = 0;

void loop() {
    StaticJsonDocument<200> doc;

    if (Serial.available() > 0) {
        DeserializationError error = deserializeJson(doc, Serial);
        // Test if parsing succeeds.
        if (error) {
            log_print(F("deserializeJson() failed: "));
            log_println(error.f_str());
            return;
        }
        String command = doc[0];
        if (command.length() > 0) {
            log_print("Received JSON command: ");
            log_println(command);
            if (command == "Version") {
                StaticJsonDocument<100> reply;
                reply.add("MM-Ard");
                reply.add(1);
                serializeJson(reply, Serial);
                log_json(reply);
            }
        }
    }
    // Serial.print("Loop ");
    // Serial.print(loopnum++);
    // Serial.println(" output");
    // delay(900);
}
