#include <Arduino.h>

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 1
#include <Logger.h>
#include <JsonDispatch.h>
#include <unordered_map>
#include <string>

using namespace rdl;

#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL)
    using LoggerT = logger_base<Stream, std::string>;
    LoggerT logger(&SerialUSB1);
    void logger_begin() { SerialUSB1.begin(9600); while (!SerialUSB1) /*noop*/; }
#else
    using LoggerT = logger_base<Print_null<String>>;
    Print_null<String> nullptrinter;
    LoggerT logger(&nullprinter);
    void logger_begin() {}
#endif

String g_firmware_name("MM-Ard");
const int g_firmware_version = 1;

using DispatchMapT = std::unordered_map<std::string, rdl::json_delegate>;

int firmware_version(const char* name) {
    return (g_firmware_name == name) ? g_firmware_version : -1;
}

DispatchMapT dispatch_map {
    {"fname?", json_delegate::of<RetT<String>>::create([](){return g_firmware_name;})},
    {"fver?", json_delegate::of<RetT<int>,const char*>::create<firmware_version>()}
};

using ServerT = jsonserver<Stream, DispatchMapT, std::string, BUFFER_SIZE, LoggerT>;
ServerT server(Serial, Serial, dispatch_map);

void setup() {
    Serial.begin(9600);
    Serial.setTimeout(5000); // longer timeout on virtual machine
    logger_begin();
    server.logger(logger);

    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    logger.println("log started for ArduinoCoreTestFirmware");
}

void loop() {
    server.check_messages();
    delay(1);
}
