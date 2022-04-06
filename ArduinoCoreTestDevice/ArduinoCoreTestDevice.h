//////////////////////////////////////////////////////////////////////////////
// FILE:          ArduinoCoreTestDevice.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Adapter for ArduinoCoreTestDevice board
//                Needs accompanying firmware to be installed on the board
// COPYRIGHT:     University of California, San Francisco, 2008
// LICENSE:       LGPL
//
// AUTHOR:        Nico Stuurman, nico@cmp.ucsf.edu, 11/09/2008
//                automatic device detection by Karl Hoover
//
//

#ifndef _ArduinoCoreTestDevice_H_
#define _ArduinoCoreTestDevice_H_

#define NOMINMAX
//#include <map>
#include "DeviceBase.h"
#include "HubStreamAdapter.h"
#include <DevicePropHelpers.h>
#include <DeviceProp.h>
#include <LocalProp.h>
#include <string>

using namespace dprop;

const char* g_deviceNameHub = "ArduinoCoreTestDevice-Hub";
const char* g_versionProp   = "Version";

const char* g_intProp    = "intProp";
const char* g_longProp   = "longProp";
const char* g_stringProp = "stringProp";
//const char* g_doubleProp = "doubleProp";

const auto g_infoPort     = PropInfo<std::string>::build(MM::g_Keyword_Port, "Undefined").withIsPreInit();
const auto g_infoVersion  = PropInfo<long>::build(g_versionProp, 0);
const auto g_infoIntProp  = PropInfo<int>::build(g_intProp, 100);
const auto g_infoLongProp = PropInfo<long>::build(g_longProp, 100000);
//const auto g_infoStringProp   = PropInfo<std::string>::build(g_doubleProp, "Lorus Ipsum");

class CArduinoCoreTestDeviceHub : public HubBase<CArduinoCoreTestDeviceHub> {
 protected:
    using HUB = CArduinoCoreTestDeviceHub;
    LocalProp<std::string, HUB> port_;
    LocalProp<long, HUB> versionProp_;
    LocalProp<int, HUB> intProp_;
    LocalProp<long, HUB> longProp_;
    LocalProp<std::string, HUB> stringProp_;
    //LocalProp<double, HUB> doubleProp_;

    using StreamAdapter = HubStreamAdapter<CArduinoCoreTestDeviceHub>;
    friend StreamAdapter;

 public:
    CArduinoCoreTestDeviceHub();
    ~CArduinoCoreTestDeviceHub();

    int Initialize();
    int Shutdown();
    void GetName(char* pszName) const;
    bool Busy();

    bool SupportsDeviceDetection(void);
    MM::DeviceDetectionStatus DetectDevice(void);
    int DetectInstalledDevices();

    // property handlers
    //int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
    int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);
    int OnTest(MM::PropertyBase* pPropt, MM::ActionType eAct);

    // custom interface for child devices
    bool IsPortAvailable() { return portAvailable_; }

    // int PurgeComPortH() {return PurgeComPort(port_.c_str());}
    // int WriteToComPortH(const unsigned char* command, unsigned len) {return
    // WriteToComPort(port_.c_str(), command, len);} int
    // ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long&
    // bytesRead)
    //{
    //   return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
    //}
    static MMThreadLock& GetLock() { return lock_; }

    bool port(const std::string& port) {
        return port_.set(port);
    }
    const std::string port() const {
        std::string res;
        port_.getProperty(res);
        return res;
    }

 private:
    int GetControllerVersion(int&);
    //std::string port_;
    bool initialized_;
    bool portAvailable_;
    int version_;
    static MMThreadLock lock_;
    StreamAdapter serial_;
};

#endif //_ArduinoCoreTestDevice_H_
