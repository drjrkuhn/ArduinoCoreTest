// DeviceUnitTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define NOMINMAX

#include <MMCore.h>
#include <MMDevice.h>
#include "DeviceBase.h"
#include <iostream>
#include <string>
#include <vector>
#include <deque>

#include <iostream>
#include <rdlmm/DevicePropHelpers.h>


inline std::string getPropertyTypeVerbose(MM::PropertyType t)
{
	switch (t)
	{
	case (MM::Float):
		return std::string("Float");
	case (MM::Integer):
		return std::string("Integer");
	case (MM::String):
		return std::string("String");
	}

	// we don't know this property so we'll just use the id
	std::ostringstream os;
	os << "Property_type_" << t;
	return os.str();
}

int main()
{
	using namespace std;

	string moduleName("ArduinoCoreTestDevice");
	string deviceName("ArduinoCoreTestDevice-Hub");
	string portLabel("HubSerial");
	string portOutput("COM8");
	CMMCore core;
	core.enableStderrLog(true);
	core.enableDebugLog(true);
	string hubLabel("Hub");
	try {
		// setup the serial port from the serial manager
		core.loadDevice(portLabel.c_str(), "SerialManager", portOutput.c_str());
		//cout << "Fast USB to Serial was: " << core.getProperty(portLabel.c_str(), "Fast USB to Serial") << endl;
		core.setProperty(portLabel.c_str(), "Fast USB to Serial", "Enable");
		core.setProperty(portLabel.c_str(), "Verbose", "1");
		core.initializeDevice(portLabel.c_str());
		// Initialize the device and set the serial port
		core.loadDevice(hubLabel.c_str(), moduleName.c_str(), deviceName.c_str());
		core.setProperty(hubLabel.c_str(), "Port", portLabel.c_str());
		core.initializeDevice(hubLabel.c_str());

		cout << "==== " << hubLabel << " Properties ====" << endl;
        for (auto propName : core.getDevicePropertyNames(hubLabel.c_str())) {
            cout << rdlmm::ToString(core.getPropertyType(hubLabel.c_str(), propName.c_str())) << " " << propName;
            cout << " = " << core.getProperty(hubLabel.c_str(), propName.c_str()) << endl;
		}

		// Run unit tests
		core.setProperty(hubLabel.c_str(), "Test", "Run");
		cout << "Results: " << core.getProperty(hubLabel.c_str(), "Test") << endl << endl;

		// unload the device
		// -----------------
		core.unloadAllDevices();
	}
	catch (CMMError& err) {
		cout << err.getMsg();
		return 1;
	}
	return 0;
}
