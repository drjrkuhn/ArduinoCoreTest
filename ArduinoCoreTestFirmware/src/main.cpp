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

int firmware_version(String name) {
    return (g_firmware_name == name) ? g_firmware_version : -1;
}

long foo = 10;;
constexpr size_t FOOSIZE = 200;
long fooarray[FOOSIZE];
size_t nfoo = 0;
bool foo_started = false;

     /*      {"?prop", call<int>(     [&prop_]()->int            { return prop_; })},
     *      {"!prop", call<void,int>([&prop_](int val)          { prop_ = val; })},
     *      {"^prop", call<int>(     [&pseq_max_]()->int        { return pseq_max_; })},
     *      {"#prop", call<int>(     [&pseq_count_]()->int      { return pseq_count_; })},
     *      {"0prop", call<int>(     [&pseq_count_]()->int      { pseq_count_ = 0; return 0; })},
     *      {"+prop", call<void,int>([&pseq_,&acount_](int val) { pseq_[pseq_count_++] = val; })}
    */

DispatchMapT dispatch_map {
    {"fname?", json_delegate::of<RetT<String>>::create([](){return g_firmware_name;})},
    {"fver?", json_delegate::of<RetT<int>,String>::create<firmware_version>()},
    {"?foo", json_delegate::of<RetT<long>>::create([]{return foo;})},
    {"!foo", json_delegate::of<RetT<void>,long>::create([](long v){foo = v;})},
    {"^foo", json_delegate::of<RetT<size_t>>::create([]{return FOOSIZE;})},
    {"#foo", json_delegate::of<RetT<size_t>>::create([]{return nfoo;})},
    {"+foo", json_delegate::of<RetT<void>,long>::create([](long v){fooarray[nfoo++] = v;})},
    {"?foo", json_delegate::of<RetT<long>>::create([]{return foo;})},
    {"*foo", json_delegate::of<RetT<void>>::create([]{foo_started = true;})},
    {"~foo", json_delegate::of<RetT<void>>::create([]{foo_started = false;})}
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
