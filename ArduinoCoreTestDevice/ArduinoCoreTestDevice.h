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

#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <map>
#include <deque>

#include <Stream.h>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_POSITION 101
#define ERR_INITIALIZE_FAILED 102
#define ERR_WRITE_FAILED 103
#define ERR_CLOSE_FAILED 104
#define ERR_BOARD_NOT_FOUND 105
#define ERR_PORT_OPEN_FAILED 106
#define ERR_COMMUNICATION 107
#define ERR_NO_PORT_SET 108
#define ERR_VERSION_MISMATCH 109

//class ArduinoCoreTestDeviceInputMonitorThread;



template <class HUB>
class HubStreamAdapter : public Stream {
    HubStreamAdapter(HUB& hub) : hub_(hub) {}

    virtual int timedRead() override {
        // let SerialManager handle timeouts
        return read();
    }
    virtual int timedPeek() override {
        // let SerialManager handle timeouts
        return peek();
    }

    virtual size_t write(const uint8_t byte) override {
        return write(&byte, 1);
    }

    virtual size_t write(const uint8_t* str, size_t n) override {
        const char* src = reinterpret_cast<const char*>(str);
        std::vector<char> out(src, src + n);
        return hub_.WriteToComPort(hub_.port_.c_str(), str, n) == 0 ? n : 0;
    }

    virtual int availableForWrite() override {
        return std::numeric_limits<int>::max();
    }

    virtual int available() override {
        getNextChar();
        return static_cast<int>(rdbuf_.size());
    }

    virtual int read() override {
        getNextChar();
        if (rdbuf_.empty())
            return -1;
        int front = rdbuf_.front();
        rdbuf_.pop();
        return front;
    }
    virtual int peek() override {
        getNextChar();
        return rdbuf_.empty() ? -1 : rdbuf_.front();
    }

protected:
    // buffer the next character because MMCore doesn't have a peek function for serial
    void getNextChar() {
        unsigned char buf;
        if (rdbuf_.empty()) {
            unsigned long read;
            hub_.ReadFromComPort(hub_.port_.c_str(), &buf, 1, read);
            if (read > 0) {
                rdbuf_.push_back(buf);
            }
        }
    }

    std::deque<unsigned char> rdbuf_;
    HUB& hub_;
};

class CArduinoCoreTestDeviceHub : public HubBase<CArduinoCoreTestDeviceHub>  
{
protected:
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
   int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnLogic(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);

   // custom interface for child devices
   bool IsPortAvailable() {return portAvailable_;}
   bool IsLogicInverted() {return invertedLogic_;}
   bool IsTimedOutputActive() {return timedOutputActive_;}
   void SetTimedOutput(bool active) {timedOutputActive_ = active;}

   int PurgeComPortH() {return PurgeComPort(port_.c_str());}
   int WriteToComPortH(const unsigned char* command, unsigned len) {return WriteToComPort(port_.c_str(), command, len);}
   int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
   {
      return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
   }
   static MMThreadLock& GetLock() {return lock_;}

private:
   int GetControllerVersion(int&);
   std::string port_;
   bool initialized_;
   bool portAvailable_;
   bool invertedLogic_;
   bool timedOutputActive_;
   int version_;
   static MMThreadLock lock_;
};

#endif //_ArduinoCoreTestDevice_H_
