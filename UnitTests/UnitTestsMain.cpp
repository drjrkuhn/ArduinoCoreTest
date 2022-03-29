// DeviceUnitTests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define NOMINMAX
#include <Stream.h>

#include <MMCore.h>
#include <MMDevice.h>
#include "DeviceBase.h"
#include "ArduinoCoreTestDevice.h"
#include <iostream>
#include <string>
#include <vector>
#include <deque>

#include <iostream>

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
	string portOutput("COM7");
	CMMCore core;
	//core.enableStderrLog(true);
	//core.enableDebugLog(true);
	string hubLabel("Hub");
	try {
		// setup the serial port from the serial manager
		core.loadDevice(portLabel.c_str(), "SerialManager", portOutput.c_str());
		core.initializeDevice(portLabel.c_str());
		// Initialize the device and set the serial port
		core.loadDevice(hubLabel.c_str(), moduleName.c_str(), deviceName.c_str());
		core.setProperty(hubLabel.c_str(), "Port", portLabel.c_str());
		core.initializeDevice(hubLabel.c_str());

		//core.initializeAllDevices();

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
