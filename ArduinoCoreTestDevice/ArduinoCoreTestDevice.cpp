///////////////////////////////////////////////////////////////////////////////
// FILE:          ArduinoCoreTestDevice.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   ArduinoCoreTestDevice adapter.  Needs accompanying firmware
// COPYRIGHT:     University of California, San Francisco, 2008
// LICENSE:       LGPL
//
// AUTHOR:        Nico Stuurman, nico@cmp.ucsf.edu 11/09/2008
//                automatic device detection by Karl Hoover
//
//

#define JSONRPC_DEBUG_CLIENTSERVER 1

#include "ArduinoCoreTestDevice.h"
#include <ArduinoJson.hpp>
#include <Common.h>
#include <ModuleInterface.h>
#include <cstdio>
#include <iostream>
#include <rdl/JsonDelegate.h>
#include <rdl/JsonDispatch.h>
#include <rdl/Logger.h>
#include <rdl/SlipInPlace.h>
#include <rdlmm/DeviceError.h>
#include <rdlmm/DeviceProp.h>
#include <rdlmm/DevicePropHelpers.h>
#include <rdlmm/LocalProp.h>
#include <sstream>

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif
#include "FixSnprintf.h"

#define SERIALIZER json::serializeJson
#define DESERIALIZER json::deserializeJson

namespace json = ARDUINOJSON_NAMESPACE;

const char* g_DeviceNameArduinoCoreTestDeviceHub = "ArduinoCoreTestDevice-Hub";

const char* g_FirmwareName       = "MM-Ardulingua";
const int g_MinFirmwareVersion   = 1;
const int g_MaxFirmwareVersion   = 2;
const char* g_KeywordTest        = "Test";
const char* g_TestResultsUnknown = "Not Run";
const char* g_TestResultsRun     = "Run";
const char* g_TestResultsFailed  = "Failed";
const char* g_TestResultsPassed  = "Passed";

const char* g_On  = "On";
const char* g_Off = "Off";

// static lock
MMThreadLock CArduinoCoreTestDeviceHub::lock_;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData() {
    RegisterDevice(g_DeviceNameArduinoCoreTestDeviceHub, MM::HubDevice,
                   "Hub (required)");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName) {
    if (deviceName == 0) return 0;

    if (strcmp(deviceName, g_DeviceNameArduinoCoreTestDeviceHub) == 0) {
        CArduinoCoreTestDeviceHub* newHub = new CArduinoCoreTestDeviceHub;
        return newHub;
    }
    return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice) { delete pDevice; }

///////////////////////////////////////////////////////////////////////////////
// CArduinoCoreTestDeviceHUb implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
CArduinoCoreTestDeviceHub::CArduinoCoreTestDeviceHub()
    : initialized_(false), serial_(this), client_(serial_, serial_) {
    portAvailable_ = false;
    serial_.setTimeout(5000);

    InitializeDefaultErrorMessages();
    rdlmm::InitCommonErrors(this, g_FirmwareName, g_MinFirmwareVersion);

    logger_ = LoggerT(this, true);
    client_.logger(&logger_);

    port_.create(this, g_infoPort);
}

CArduinoCoreTestDeviceHub::~CArduinoCoreTestDeviceHub() { Shutdown(); }

void CArduinoCoreTestDeviceHub::GetName(char* name) const {
    CDeviceUtils::CopyLimitedString(name, g_DeviceNameArduinoCoreTestDeviceHub);
}

bool CArduinoCoreTestDeviceHub::Busy() { return false; }

// private and expects caller to:
// 1. guard the port
// 2. purge the port
int CArduinoCoreTestDeviceHub::GetControllerVersion(int& version) {
    version = 0;
    try {
        std::string fname;
        int fver  = 0;
        int error = client_.call_get<rdl::RetT<std::string>>("?fname", fname);
        if (error) {
            LogMessage("json-rpc failed: ", error);
            return error;
        }
        bool found = fname == "MM-Ardulingua";
        if (!found) {
            return ERR_FIRMWARE_NOT_FOUND;
        }
        error = client_.call_get<rdl::RetT<int>, std::string>("?fver", fver, fname);
        if (error) {
            LogMessage("json-rpc failed: ", error);
            return error;
        }
        version = fver;
        return DEVICE_OK;
    } catch (...) {
        LogMessage("Exception in GetControllerVersion!", false);
    }
    return ERR_FIRMWARE_NOT_FOUND;
}

bool CArduinoCoreTestDeviceHub::SupportsDeviceDetection(void) {
    return true;
}

MM::DeviceDetectionStatus CArduinoCoreTestDeviceHub::DetectDevice(void) {
    if (initialized_) return MM::CanCommunicate;

    // all conditions must be satisfied...
    MM::DeviceDetectionStatus result = MM::Misconfigured;
    char answerTO[MM::MaxStrLength];

    try {
        std::string portLowerCase = port();
        for (std::string::iterator its = portLowerCase.begin();
             its != portLowerCase.end(); ++its) {
            *its = (char)tolower(*its);
        }
        if (0 < portLowerCase.length() && 0 != portLowerCase.compare("undefined") &&
            0 != portLowerCase.compare("unknown")) {
            result = MM::CanNotCommunicate;
            // record the default answer time out
            GetCoreCallback()->GetDeviceProperty(port().c_str(), "AnswerTimeout",
                                                 answerTO);

            // device specific default communication parameters
            // for ArduinoCoreTestDevice Duemilanova
            GetCoreCallback()->SetDeviceProperty(port().c_str(),
                                                 MM::g_Keyword_Handshaking, g_Off);
            GetCoreCallback()->SetDeviceProperty(port().c_str(),
                                                 MM::g_Keyword_BaudRate, "57600");
            GetCoreCallback()->SetDeviceProperty(port().c_str(),
                                                 MM::g_Keyword_StopBits, "1");
            // ArduinoCoreTestDevice timed out in GetControllerVersion even if
            // AnswerTimeout  = 300 ms
            GetCoreCallback()->SetDeviceProperty(port().c_str(), "AnswerTimeout",
                                                 "500.0");
            GetCoreCallback()->SetDeviceProperty(port().c_str(), "DelayBetweenCharsMs",
                                                 "0");
            MM::Device* pS = GetCoreCallback()->GetDevice(this, port().c_str());
            pS->Initialize();
            // The first second or so after opening the serial port, the
            // ArduinoCoreTestDevice is waiting for firmwareupgrades.  Simply sleep 2
            // seconds.
            CDeviceUtils::SleepMs(2000);
            MMThreadGuard myLock(lock_);
            PurgeComPort(port().c_str());
            int v   = 0;
            int ret = GetControllerVersion(v);
            // later, Initialize will explicitly check the version #
            if (DEVICE_OK != ret) {
                LogMessageCode(ret, true);
            } else {
                // to succeed must reach here....
                result = MM::CanCommunicate;
            }
            pS->Shutdown();
            // always restore the AnswerTimeout to the default
            GetCoreCallback()->SetDeviceProperty(port().c_str(), "AnswerTimeout",
                                                 answerTO);
        }
    } catch (...) {
        LogMessage("Exception in DetectDevice!", false);
    }

    return result;
}

int CArduinoCoreTestDeviceHub::Initialize() {
    // Name
    try {
        // Simple name property
        ASSERT_OK(CreateProperty(MM::g_Keyword_Name, g_deviceNameHub, MM::String, true));

    } catch (DeviceResultException deviceError) {
        LogMessage(deviceError.format(this));
        //std::string lastTrans = getLastLog();
        //if (!lastTrans.empty()) {
        //    LogMessage(std::string("Last Transaction: ") + lastTrans);
        //}
        return deviceError.error;
    }
    //int ret =
    //    CreateProperty(MM::g_Keyword_Name, g_DeviceNameArduinoCoreTestDeviceHub,
    //                   MM::String, true);
    //if (DEVICE_OK != ret) return ret;

    // The first second or so after opening the serial port, the
    // ArduinoCoreTestDevice is waiting for firmwareupgrades.  Simply sleep 1
    // second.
    CDeviceUtils::SleepMs(2000);

    MMThreadGuard myLock(lock_);

    // Check that we have a controller:
    PurgeComPort(port().c_str());
    int ret = GetControllerVersion(version_);
    if (DEVICE_OK != ret) return ret;

    if (version_ < g_MinFirmwareVersion || version_ > g_MaxFirmwareVersion)
        return ERR_VERSION_MISMATCH;

    ret = foo_.create(this, &client_, g_infoFoo);
    if (DEVICE_OK != ret) return ret;

    ret = barA_.create(this, &client_, g_infoBarA, 0);
    if (DEVICE_OK != ret) return ret;

    ret = barB_.create(this, &client_, g_infoBarB, 1);
    if (DEVICE_OK != ret) return ret;

    CPropertyAction* pAct =
        new CPropertyAction(this, &CArduinoCoreTestDeviceHub::OnVersion);
    std::ostringstream sversion;
    sversion << version_;
    CreateProperty(g_versionProp, sversion.str().c_str(), MM::Integer, true,
                   pAct);

    pAct = new CPropertyAction(this, &CArduinoCoreTestDeviceHub::OnTest);
    CreateProperty(g_KeywordTest, g_TestResultsUnknown, MM::String, false, pAct,
                   true);

    ret = UpdateStatus();
    if (ret != DEVICE_OK) return ret;

    // turn off verbose serial debug messages
    // GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "0");

    initialized_ = true;
    return DEVICE_OK;
}

int CArduinoCoreTestDeviceHub::DetectInstalledDevices() {
    if (MM::CanCommunicate == DetectDevice()) {
        // std::vector<std::string> peripherals;
        // peripherals.clear();
        // peripherals.push_back(g_DeviceNameArduinoCoreTestDeviceSwitch);
        // peripherals.push_back(g_DeviceNameArduinoCoreTestDeviceShutter);
        // peripherals.push_back(g_DeviceNameArduinoCoreTestDeviceInput);
        // peripherals.push_back(g_DeviceNameArduinoCoreTestDeviceDA1);
        // peripherals.push_back(g_DeviceNameArduinoCoreTestDeviceDA2);
        // for (size_t i=0; i < peripherals.size(); i++)
        //{
        //   MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
        //   if (pDev)
        //   {
        //      AddInstalledDevice(pDev);
        //   }
        //}
    }

    return DEVICE_OK;
}

int CArduinoCoreTestDeviceHub::Shutdown() {
    initialized_ = false;
    return DEVICE_OK;
}

//int CArduinoCoreTestDeviceHub::OnPort(MM::PropertyBase* pProp,
//                                      MM::ActionType pAct) {
//    if (pAct == MM::BeforeGet) {
//        pProp->Set(port_.c_str());
//    } else if (pAct == MM::AfterSet) {
//        pProp->Get(port_);
//        portAvailable_ = true;
//    }
//    return DEVICE_OK;
//}

int CArduinoCoreTestDeviceHub::OnVersion(MM::PropertyBase* pProp,
                                         MM::ActionType pAct) {
    if (pAct == MM::BeforeGet) {
        pProp->Set((long)version_);
    }
    return DEVICE_OK;
}

int CArduinoCoreTestDeviceHub::OnTest(MM::PropertyBase* pProp,
                                      MM::ActionType pAct) {
    using namespace std;
    stringstream msg;
    if (pAct == MM::BeforeGet) {
        msg.clear();
        msg << "OnTest MM::BeforeGet";
        LogMessage(msg.str());
        // if (invertedLogic_)
        //    pProp->Set(g_invertedLogicString);
        // else
        //    pProp->Set(g_normalLogicString);
    } else if (pAct == MM::AfterSet) {
        msg.clear();
        string val;
        bool testPassed = false;
        pProp->Get(val);
        msg << "OnTest MM::AfterSet, Prop: " << val;
        LogMessage(msg.str());

        if (val == g_TestResultsRun) {
            cout << "=== TESTING ===" << endl;
            int version;
            int ret = GetControllerVersion(version);
            arduino::delay(1);
            if (ret == DEVICE_OK) {
                cout << "Found controller version: " << version << endl;
            } else {
                char text[MM::MaxStrLength];
                GetErrorText(ret, text);
                cout << "Version call error " << ret << ": " << text << endl;
            }

            long ival;
            SetProperty(foo_.name().c_str(), "100");
            GetProperty(foo_.name().c_str(), ival);

            ClearPropertySequence(foo_.name().c_str());
            for (long seq = 0; seq < 12; seq++) {
                std::string seqstr = ToString(seq);
                AddToPropertySequence(foo_.name().c_str(), seqstr.c_str());
            }
            SendPropertySequence(foo_.name().c_str());
            StartPropertySequence(foo_.name().c_str());
            StopPropertySequence(foo_.name().c_str());

            SetProperty(barA_.name().c_str(), "4.5678901");
            SetProperty(barA_.name().c_str(), "5.6789012");
            SetProperty(barA_.name().c_str(), "6.7890123");
            double dval;
            GetProperty(barA_.name().c_str(), dval);
            ClearPropertySequence(barA_.name().c_str());
            for (long seq = 0; seq < 12; seq++) {
                dval        = 10.0 * (seq / 11.0);
                std::string seqstr = ToString(dval);
                AddToPropertySequence(barA_.name().c_str(), seqstr.c_str());
            }
            SendPropertySequence(barA_.name().c_str());
            StartPropertySequence(barA_.name().c_str());
            StopPropertySequence(barA_.name().c_str());

            cout << "=== TESTING DONE ===" << endl;
            testPassed = true;
        }

        pProp->Set(testPassed ? g_TestResultsPassed : g_TestResultsFailed);

        // std::string logic;
        // pProp->Get(logic);
        // if (logic.compare(g_invertedLogicString) == 0)
        //    invertedLogic_ = true;
        // else invertedLogic_ = false;
    }
    return DEVICE_OK;
}
