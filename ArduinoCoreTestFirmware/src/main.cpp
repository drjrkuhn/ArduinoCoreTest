#include <Arduino.h>

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 1
#include <Logger.h>
#include <JsonDispatch.h>
#include <map>
#include <unordered_map>
#include <string>
#include <utility>

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

using DispatchMapT = std::unordered_map<std::string, rdl::json_stub>;

int firmware_version(String name) {
    return (g_firmware_name == name) ? g_firmware_version : -1;
}


template<typename T, long MAX_SIZE=64>
class sequence {
  public:
    
    sequence(const char* _brief, const T _initial) 
    : brief_(_brief), value_(_initial), size_(0), started_(false) {}

    T get() const { 
        logger.print("SEQT get "); logger.print(brief_.c_str()); logger.print("() -> "); logger.println(value_);
        return value_;
        }

    void set(const T value) {
        value_ = value; 
        logger.print("SEQT set ") ; logger.print(brief_.c_str()) ; logger.print("(") ; logger.print(value_) ; logger.println(")");
        }

    long max_size() {
        logger.print("SEQT max_size ") ; logger.print(brief_.c_str()) ; logger.print("() -> ") ; logger.println(max_size_);
        return max_size_;
    }

    long size() {
        logger.print("SEQT size ") ; logger.print(brief_.c_str()) ; logger.print("() -> ") ; logger.println(size_);
        return size_;
    }

    void clear() {
        logger.print("SEQT clear ") ; logger.println(brief_.c_str());
        size_ = 0;
    }

    void add(const T value) {
        if (size_ >= max_size_) {
            logger.print("SEQT add ") ; logger.print(brief_.c_str()) ; logger.println(" ERROR: MAXSIZE reached");
            return;
        }
        logger.print("SEQT add ") ; logger.print(brief_.c_str()) ; logger.print("["); logger.print(size_);
        logger.print("](") ; logger.print(value) ; logger.println(")");
        values_[size_++] = value;
    }

    void start() {
        logger.print("SEQT start ") ; logger.println(brief_.c_str());
        started_ = true;
    }

    void stop() {
        logger.print("SEQT stop ") ; logger.println(brief_.c_str());
        started_ = false;
    }

    std::string message(const char opcode)  {
        std::string res(1,opcode);
        res.append(brief_);
        return res;
    }

    int add_to(DispatchMapT & map);

  protected:

    std::string brief_;
    T value_;
    const long max_size_=MAX_SIZE;
    long size_;  
    volatile bool started_;
    T values_[MAX_SIZE];
};


// template<typename T, class S= sequence<T>>
// int add_to(DispatchMapT& map, S* obj) {
//     using PairT = DispatchMapT::value_type;
//     int startsize = map.size();
//     map.emplace(PairT(obj->message('?'), json_delegate<RetT<T>>::template create<S,&S::get>(obj).stub()));
//     map.emplace(PairT(obj->message('!'), json_delegate<RetT<void>, T>::template create<S,&S::set>(obj).stub()));
//     map.emplace(PairT(obj->message('^'), json_delegate<RetT<long>>::template create<S,&S::max_size>(obj).stub()));
//     map.emplace(PairT(obj->message('#'), json_delegate<RetT<long>>::template create<S,&S::size>(obj).stub()));
//     map.emplace(PairT(obj->message('0'), json_delegate<RetT<void>>::template create<S,&S::clear>(obj).stub()));
//     map.emplace(PairT(obj->message('+'), json_delegate<RetT<void>,T>::template create<S,&S::add>(obj).stub()));
//     map.emplace(PairT(obj->message('*'), json_delegate<RetT<void>>::template create<S,&S::start>(obj).stub()));
//     map.emplace(PairT(obj->message('~'), json_delegate<RetT<void>>::template create<S,&S::stop>(obj).stub()));
//     return map.size() - startsize;
// }

template<typename T, long M>
int sequence<T,M>::add_to(DispatchMapT& map) {
    using PairT = DispatchMapT::value_type;
    using S=sequence<T,M>;
    int startsize = map.size();
    map.emplace(PairT(message('?'), json_delegate<RetT<T>>::template create<S,&S::get>(this).stub()));
    map.emplace(PairT(message('!'), json_delegate<RetT<void>, T>::template create<S,&S::set>(this).stub()));
    map.emplace(PairT(message('^'), json_delegate<RetT<long>>::template create<S,&S::max_size>(this).stub()));
    map.emplace(PairT(message('#'), json_delegate<RetT<long>>::template create<S,&S::size>(this).stub()));
    map.emplace(PairT(message('0'), json_delegate<RetT<void>>::template create<S,&S::clear>(this).stub()));
    map.emplace(PairT(message('+'), json_delegate<RetT<void>,T>::template create<S,&S::add>(this).stub()));
    map.emplace(PairT(message('*'), json_delegate<RetT<void>>::template create<S,&S::start>(this).stub()));
    map.emplace(PairT(message('~'), json_delegate<RetT<void>>::template create<S,&S::stop>(this).stub()));
    return map.size() - startsize;
}


DispatchMapT dispatch_map {
    {"fname?", json_delegate<RetT<String>>::create([](){return g_firmware_name;}).stub()},
    {"fver?", json_delegate<RetT<int>,String>::create<firmware_version>().stub()},
};

// long foo = 10;;
// constexpr size_t FOOSIZE = 200;
// long fooarray[FOOSIZE];
// size_t nfoo = 0;
// bool foo_started = false;

     /*      {"?prop", call<int>(     [&prop_]()->int            { return prop_; })},
     *      {"!prop", call<void,int>([&prop_](int val)          { prop_ = val; })},
     *      {"^prop", call<int>(     [&pseq_max_]()->int        { return pseq_max_; })},
     *      {"#prop", call<int>(     [&pseq_count_]()->int      { return pseq_count_; })},
     *      {"0prop", call<int>(     [&pseq_count_]()->int      { pseq_count_ = 0; return 0; })},
     *      {"+prop", call<void,int>([&pseq_,&acount_](int val) { pseq_[pseq_count_++] = val; })}
    */

// template<typename T, size_t N>
// struct version_t {
//     version_t(T v) : v_(v) {};

//     T get(void) const {
//         return v_;
//     }

//     T v_;
// };

// typedef version_t<int,10> vint;
// vint version(1);

// auto stub = json_delegate<int>::create<vint,&vint::get>(&version).stub();

sequence<long,128> foo("foo", 100);
const int dummy_addfoo = foo.add_to(dispatch_map);

using ServerT = jsonserver<Stream, DispatchMapT, std::string, BUFFER_SIZE, LoggerT>;
ServerT server(Serial, Serial, dispatch_map);

void setup() {

    Serial.begin(9600);
    Serial.setTimeout(5000); // longer timeout on virtual machine
    logger_begin();
    // add_to<long,SequenceT<long>>(dispatch_map, &foo);

    logger.println("Map methods:");
    for (auto p : dispatch_map) {
        logger.println(p.first.c_str());
    }

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
