#include "DeviceError.h"
#include "DeviceProp.h"
#include "DevicePropHelpers.h"
#include "LocalProp.h"

//#include <string>
//#include <type_traits>
//template <class T,
//          typename std::enable_if<
//              std::is_integral<T>::value,
//              bool>::type = true>
//inline std::string TestTemplate(T& value) {
//    return std::to_string(/*static_cast<long>*/ (value));
//}
//
//template <class T,
//          typename std::enable_if<
//              std::is_same<T, std::string>::value,
//              bool>::type = true>
//inline std::string TestTemplate(T& value) {
//    return value;
//}
//
//template <typename T>
//std::string TestTemplateParam(typename std::enable_if<std::is_integral<T>::value, T>::type& value) {
//    return std::to_string(value);
//}
//
//template <typename T>
//std::string TestTemplateParam(typename std::enable_if<std::is_same<T, std::string>::value, T>::type& value) {
//    return (value);
//}

#include <iostream>

//template <typename T>
//T typed_sizeof() { return T(sizeof(T)); }

static const bool g_boolConst             = true;
static const signed char g_charConst      = 10; //typed_sizeof<signed char>();
static const unsigned char g_ucharConst   = 11; //typed_sizeof<unsigned char>();
static const signed short g_shortConst    = 20; //typed_sizeof<signed short>();
static const unsigned short g_ushortConst = 21; //typed_sizeof<unsigned short>();
static const signed int g_intConst        = 30; //typed_sizeof<signed int>();
static const unsigned int g_uintConst     = 31; //typed_sizeof<unsigned int>();
static const signed long g_longConst      = 40; //typed_sizeof<signed long>();
static const unsigned long g_ulongConst   = 41; //typed_sizeof<unsigned long>();
static const float g_floatConst           = 50; //typed_sizeof<float>();
static const double g_doubleConst         = 51; //typed_sizeof<double>();
static const std::string g_stringConst    = "string";
static const char* g_charPtrConst         = "charPtr";

void Test_ToString() {
    using namespace dprop;
    using namespace std;
    cout << "\n======== Test_ToString" << endl;

    cout << "bool\t\t" << ToString(g_boolConst) << endl;
    cout << "signed char\t" << ToString(g_charConst) << endl;
    cout << "unsigned char\t" << ToString(g_ucharConst) << endl;
    cout << "signed short\t" << ToString(g_shortConst) << endl;
    cout << "unsigned short\t" << ToString(g_ushortConst) << endl;
    cout << "signed int\t" << ToString(g_intConst) << endl;
    cout << "unsigned int\t" << ToString(g_uintConst) << endl;
    cout << "signed long\t" << ToString(g_longConst) << endl;
    cout << "unsigned long\t" << ToString(g_ulongConst) << endl;
    cout << "float\t\t" << ToString(g_floatConst) << endl;
    cout << "double\t\t" << ToString(g_doubleConst) << endl;
    cout << "string\t\t" << ToString(g_stringConst) << endl;
    cout << "charPtr\t\t" << ToString(g_charPtrConst) << endl;
}

void Test_mm_property_type_of() {
    using namespace std;
    using namespace dprop;
    cout << "\n======== Test_mm_property_type_of" << endl;
    cout << "bool\t\t" << ToString(MMPropertyType_of(g_boolConst)) << endl;
    cout << "signed char\t" << ToString(MMPropertyType_of(g_charConst)) << endl;
    cout << "unsigned char\t" << ToString(MMPropertyType_of(g_ucharConst)) << endl;
    cout << "signed short\t" << ToString(MMPropertyType_of(g_shortConst)) << endl;
    cout << "unsigned short\t" << ToString(MMPropertyType_of(g_ushortConst)) << endl;
    cout << "signed int\t" << ToString(MMPropertyType_of(g_intConst)) << endl;
    cout << "unsigned int\t" << ToString(MMPropertyType_of(g_uintConst)) << endl;
    cout << "signed long\t" << ToString(MMPropertyType_of(g_longConst)) << endl;
    cout << "unsigned long\t" << ToString(MMPropertyType_of(g_ulongConst)) << endl;
    cout << "float\t\t" << ToString(MMPropertyType_of(g_floatConst)) << endl;
    cout << "double\t\t" << ToString(MMPropertyType_of(g_doubleConst)) << endl;
    cout << "string\t\t" << ToString(MMPropertyType_of(g_stringConst)) << endl;
    cout << "charPtr\t\t" << ToString(MMPropertyType_of(g_charPtrConst)) << endl;
}

void Test_Parse() {
    using namespace std;
    using namespace dprop;
    cout << "\n======== Test_Parse" << endl;
    cout << "bool\t\t" << Parse<bool>(ToString(g_boolConst)) << endl;
    cout << "signed char\t" << (int)Parse<signed char>(ToString(g_charConst)) << endl;
    cout << "unsigned char\t" << (int)Parse<unsigned char>(ToString(g_ucharConst)) << endl;
    cout << "signed short\t" << Parse<signed short>(ToString(g_shortConst)) << endl;
    cout << "unsigned short\t" << Parse<unsigned short>(ToString(g_ushortConst)) << endl;
    cout << "signed int\t" << Parse<signed int>(ToString(g_intConst)) << endl;
    cout << "unsigned int\t" << Parse<unsigned int>(ToString(g_uintConst)) << endl;
    cout << "signed long\t" << Parse<signed long>(ToString(g_longConst)) << endl;
    cout << "unsigned long\t" << Parse<unsigned long>(ToString(g_ulongConst)) << endl;
    cout << "float\t\t" << Parse<float>(ToString(g_floatConst)) << endl;
    cout << "double\t\t" << Parse<double>(ToString(g_doubleConst)) << endl;
    cout << "string\t\t" << Parse<string>(ToString(g_stringConst)) << endl;
}

void Test_Assign() {
    using namespace std;
    using namespace dprop;
    cout << "\n======== Test_Assign" << endl;

    MM::IntegerProperty intProp("Integer roperty");
    MM::FloatProperty floatProp("FloatProperty");
    MM::StringProperty stringProp("StringProperty");

    bool boolVal             = g_boolConst;
    signed char charVal      = g_charConst;
    unsigned char ucharVal   = g_ucharConst;
    signed short shortVal    = g_shortConst;
    unsigned short ushortVal = g_ushortConst;
    signed int intVal        = g_intConst;
    unsigned int uintVal     = g_uintConst;
    signed long longVal      = g_longConst;
    unsigned long ulongVal   = g_ulongConst;
    float floatVal           = g_floatConst;
    double doubleVal         = g_doubleConst;
    string stringVal         = g_stringConst;

    string getVal;

    try {
        ASSERT_OK(Assign(intProp, boolVal) && Assign(boolVal, intProp) && ToString(intProp) == ToString(boolVal));
        cout << "bool\t\tprop=" << ToString(intProp) << "\tval=" << ToString(boolVal) << endl;
        ASSERT_OK(Assign(intProp, charVal) && Assign(charVal, intProp) && ToString(intProp) == ToString(charVal));
        cout << "signed char\tprop=" << ToString(intProp) << "\tval=" << ToString(charVal) << endl;
        ASSERT_OK(Assign(intProp, ucharVal) && Assign(ucharVal, intProp) && ToString(intProp) == ToString(ucharVal));
        cout << "unsigned char\tprop=" << ToString(intProp) << "\tval=" << ToString(ucharVal) << endl;
        ASSERT_OK(Assign(intProp, intVal) && Assign(intVal, intProp) && ToString(intProp) == ToString(intVal));
        cout << "signed int\tprop=" << ToString(intProp) << "\tval=" << ToString(intVal) << endl;
        ASSERT_OK(Assign(intProp, uintVal) && Assign(uintVal, intProp) && ToString(intProp) == ToString(uintVal));
        cout << "unsigned int\tprop=" << ToString(intProp) << "\tval=" << ToString(uintVal) << endl;
        ASSERT_OK(Assign(intProp, longVal) && Assign(longVal, intProp) && ToString(intProp) == ToString(longVal));
        cout << "signed long\tprop=" << ToString(intProp) << "\tval=" << ToString(longVal) << endl;
        ASSERT_OK(Assign(intProp, ulongVal) && Assign(ulongVal, intProp) && ToString(intProp) == ToString(ulongVal));
        cout << "unsigned long\tprop=" << ToString(intProp) << "\tval=" << ToString(ulongVal) << endl;
        ASSERT_OK(Assign(floatProp, floatVal) && Assign(floatVal, floatProp) && ToString(floatProp).substr(0, 4) == ToString(floatVal).substr(0, 4));
        cout << "float\t\tprop=" << ToString(floatProp).substr(0, 4) << "\tval=" << ToString(floatVal).substr(0, 4) << endl;
        ASSERT_OK(Assign(floatProp, doubleVal) && Assign(doubleVal, floatProp) && ToString(floatProp).substr(0, 4) == ToString(doubleVal).substr(0, 4));
        cout << "double\t\tprop=" << ToString(floatProp).substr(0, 4) << "\tval=" << ToString(doubleVal).substr(0, 4) << endl;
        ASSERT_OK(Assign(stringProp, stringVal) && Assign(stringVal, stringProp) && ToString(stringProp) == ToString(stringVal));
        cout << "string\t\tprop=" << ToString(stringProp) << "\tval=" << ToString(stringVal) << endl;
    }
    catch (DeviceResultException e) {
        cerr << e.format();
    }

    #if 0
    cout << "\n======== Test_Assign" << endl;
    cout << "bool\t\t" << Parse<bool>(ToString(g_boolConst)) << endl;
    cout << "signed char\t" << (int)Parse<signed char>(ToString(g_charConst)) << endl;
    cout << "unsigned char\t" << (int)Parse<unsigned char>(ToString(g_ucharConst)) << endl;
    cout << "signed short\t" << Parse<signed short>(ToString(g_shortConst)) << endl;
    cout << "unsigned short\t" << Parse<unsigned short>(ToString(g_ushortConst)) << endl;
    cout << "signed int\t" << Parse<signed int>(ToString(g_intConst)) << endl;
    cout << "unsigned int\t" << Parse<unsigned int>(ToString(g_uintConst)) << endl;
    cout << "signed long\t" << Parse<signed long>(ToString(g_longConst)) << endl;
    cout << "unsigned long\t" << Parse<unsigned long>(ToString(g_ulongConst)) << endl;
    cout << "float\t\t" << Parse<float>(ToString(g_floatConst)) << endl;
    cout << "double\t\t" << Parse<double>(ToString(g_doubleConst)) << endl;
    cout << "string\t\t" << Parse<string>(ToString(g_stringConst)) << endl;
    #endif
}

int main() {
    Test_ToString();
    Test_mm_property_type_of();
    Test_Parse();
    Test_Assign();

    return 0;
}
