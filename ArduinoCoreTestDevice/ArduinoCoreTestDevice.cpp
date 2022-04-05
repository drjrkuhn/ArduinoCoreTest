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

#include "ArduinoCoreTestDevice.h"
#include "DeviceError.h"
#include "DeviceProp.h"
#include "DevicePropHelpers.h"
#include "LocalProp.h"
#include "ModuleInterface.h"
#include <ArduinoJson.hpp>
#include <Common.h>
#include <cstdio>
#include <iostream>
#include <SlipInPlace.h>
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

// Global info about the state of the ArduinoCoreTestDevice.  This should be
// folded into a class
const int g_Min_MMVersion        = 1;
const int g_Max_MMVersion        = 2;
const char* g_KeywordTest        = "Test";
const char* g_TestResultsUnknown = "Not Run";
const char* g_TestResultsRun     = "Run";
const char* g_TestResultsFailed  = "Failed";
const char* g_TestResultsPassed  = "Passed";

const char* g_On = "On";
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
    : initialized_(false), serial_(this) {
    portAvailable_ = false;
    serial_.setTimeout(5000);

    InitializeDefaultErrorMessages();
    dprop::initCommonErrors("ArduinoCoreTestDevice", 1, [this](int err, const char* txt) {
        SetErrorText(err, txt);
    });
    portProp_.createLocalProp(this, g_infoPort);

    //SetErrorText(ERR_PORT_OPEN_FAILED, "Failed opening ArduinoCoreTestDevice USB device");
    //SetErrorText(ERR_BOARD_NOT_FOUND, "Did not find an ArduinoCoreTestDevice board with the correct firmware.  "
    //                                  "Is the ArduinoCoreTestDevice board connected to this serial port?");
    //SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The ArduinoCoreTestDevice Hub device is "
    //                              "needed to create this device");
    //std::ostringstream errorText;
    //errorText << "The firmware version on the ArduinoCoreTestDevice is not "
    //             "compatible with this adapter.  Please use firmware version ";
    //errorText << g_Min_MMVersion << " to " << g_Max_MMVersion;
    //SetErrorText(ERR_VERSION_MISMATCH, errorText.str().c_str());

    //CPropertyAction* pAct =
    //    new CPropertyAction(this, &CArduinoCoreTestDeviceHub::OnPort);
    //CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct,
    //               true);
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
        const size_t pktbuf_size = 512;
        uint8_t pktbuf[pktbuf_size];

        using encoder = slip::null_encoder;
        using decoder = slip::null_decoder;

        serial_.clear();
        json::StaticJsonDocument<200> command;
        command.add("Version");
        size_t size  = SERIALIZER(command, pktbuf, pktbuf_size);
        size_t esize = encoder::encode(pktbuf, pktbuf_size, pktbuf, size);
        serial_.write(pktbuf, esize);

        size_t nreceived =
            serial_.readBytesUntil(encoder::end_code(), pktbuf, pktbuf_size);
        size_t nread = decoder::decode(pktbuf, pktbuf_size, pktbuf, nreceived);
        json::StaticJsonDocument<200> reply;
        json::DeserializationError error =
            DESERIALIZER(reply, (char*)pktbuf, nread);
        // Test if parsing succeeds.
        if (error) {
            LogMessage("deserializeJson() failed: ", error.c_str());
            return ERR_FIRMWARE_NOT_FOUND;
        }

        bool found = reply[0] == "MM-Ard"; // serial_.findUntil("MM-Ard", "\r\n");
        if (!found) {
            return ERR_FIRMWARE_NOT_FOUND;
        }
        version = reply[1];
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
        std::string portLowerCase = port_;
        for (std::string::iterator its = portLowerCase.begin();
             its != portLowerCase.end(); ++its) {
            *its = (char)tolower(*its);
        }
        if (0 < portLowerCase.length() && 0 != portLowerCase.compare("undefined") &&
            0 != portLowerCase.compare("unknown")) {
            result = MM::CanNotCommunicate;
            // record the default answer time out
            GetCoreCallback()->GetDeviceProperty(port_.c_str(), "AnswerTimeout",
                                                 answerTO);

            // device specific default communication parameters
            // for ArduinoCoreTestDevice Duemilanova
            GetCoreCallback()->SetDeviceProperty(port_.c_str(),
                                                 MM::g_Keyword_Handshaking, g_Off);
            GetCoreCallback()->SetDeviceProperty(port_.c_str(),
                                                 MM::g_Keyword_BaudRate, "57600");
            GetCoreCallback()->SetDeviceProperty(port_.c_str(),
                                                 MM::g_Keyword_StopBits, "1");
            // ArduinoCoreTestDevice timed out in GetControllerVersion even if
            // AnswerTimeout  = 300 ms
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout",
                                                 "500.0");
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs",
                                                 "0");
            MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());
            pS->Initialize();
            // The first second or so after opening the serial port, the
            // ArduinoCoreTestDevice is waiting for firmwareupgrades.  Simply sleep 2
            // seconds.
            CDeviceUtils::SleepMs(2000);
            MMThreadGuard myLock(lock_);
            PurgeComPort(port_.c_str());
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
            GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout",
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
    PurgeComPort(port_.c_str());
    int ret = GetControllerVersion(version_);
    if (DEVICE_OK != ret) return ret;

    if (version_ < g_Min_MMVersion || version_ > g_Max_MMVersion)
        return ERR_VERSION_MISMATCH;

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
        pProp->Get(val);
        msg << "OnTest MM::AfterSet, Prop: " << val;
        LogMessage(msg.str());

        bool testPassed = false;
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
            cout << "=== TESTING DONE ===" << endl;
            testPassed = false;
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
