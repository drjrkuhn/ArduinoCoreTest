//#include "DeviceError.h"
//#include "DeviceProp.h"
//#include "DevicePropHelpers.h"
//#include "LocalProp.h"

#include <string>
#include <type_traits>

template <class T,
          typename std::enable_if<
              std::is_integral<T>::value,
              bool>::type = true>
inline std::string TestTemplate(T& value) {
    return std::to_string(/*static_cast<long>*/ (value));
}

template <class T,
          typename std::enable_if<
              std::is_same<T, std::string>::value,
              bool>::type = true>
inline std::string TestTemplate(T& value) {
    return value;
}

template <typename T>
std::string TestTemplateParam(typename std::enable_if<std::is_integral<T>::value, T>::type& value) {
    return std::to_string(value);
}

template <typename T>
std::string TestTemplateParam(typename std::enable_if<std::is_same<T, std::string>::value, T>::type& value) {
    return (value);
}

void TestToString() {

    std::string res;
    int intVal = 0;
    res        = TestTemplate(res);
    res        = TestTemplate(intVal);
    res        = TestTemplateParam(res);
    res        = TestTemplateParam(intVal);
}
