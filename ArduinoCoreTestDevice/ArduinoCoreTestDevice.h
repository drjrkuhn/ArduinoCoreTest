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

#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <map>

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

class ArduinoCoreTestDeviceInputMonitorThread;

class CArduinoCoreTestDeviceHub : public HubBase<CArduinoCoreTestDeviceHub>  
{
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
   void SetShutterState(unsigned state) {shutterState_ = state;}
   void SetSwitchState(unsigned state) {switchState_ = state;}
   unsigned GetShutterState() {return shutterState_;}
   unsigned GetSwitchState() {return switchState_;}

private:
   int GetControllerVersion(int&);
   std::string port_;
   bool initialized_;
   bool portAvailable_;
   bool invertedLogic_;
   bool timedOutputActive_;
   int version_;
   static MMThreadLock lock_;
   unsigned switchState_;
   unsigned shutterState_;
};

#endif //_ArduinoCoreTestDevice_H_
