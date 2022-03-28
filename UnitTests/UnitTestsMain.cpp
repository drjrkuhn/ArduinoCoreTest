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
	string portOutput("COM3");
	CMMCore core;
	core.enableStderrLog(true);
	core.enableDebugLog(true);
	string hubLabel("Hub");
	try {
		// setup the serial port from the serial manager
		core.loadDevice(portLabel.c_str(), "SerialManager", portOutput.c_str());
		core.setProperty(portLabel.c_str(), "AnswerTimeout", "500.0");
		core.setProperty(portLabel.c_str(), "BaudRate", "115200");
		core.setProperty(portLabel.c_str(), "DelayBetweenCharsMs", "0.0");
		core.setProperty(portLabel.c_str(), "Handshaking", "Off");
		core.setProperty(portLabel.c_str(), "Parity", "None");
		core.setProperty(portLabel.c_str(), "StopBits", "1");
		core.setProperty(portLabel.c_str(), "Verbose", "1");
		core.initializeDevice(portLabel.c_str());

		// Initialize the device
		// ---------------------
		cout << "Loading " << deviceName << " from library " << moduleName << "..." << endl;
		core.loadDevice(hubLabel.c_str(), moduleName.c_str(), deviceName.c_str());
		cout << "Done." << endl;

		// set the serial port
		core.setProperty(hubLabel.c_str(), "Port", portLabel.c_str());
		//CArduinoCoreTestDeviceHub* hub = CArduinoCoreTestDeviceHub::getInstance();

		cout << "Initializing..." << endl;
		core.initializeAllDevices();
		cout << "Done." << endl;

		// Obtain device properties
		// ------------------------
		vector<string> props(core.getDevicePropertyNames(hubLabel.c_str()));
		for (unsigned i = 0; i < props.size(); i++) {
			cout << props[i] << " (" << ::getPropertyTypeVerbose(core.getPropertyType(hubLabel.c_str(), props[i].c_str())) << ") = "
				<< core.getProperty(hubLabel.c_str(), props[i].c_str()) << endl;
		}
		cout << endl;

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

	// declare success
	// ---------------
	cout << "Device " + deviceName + " PASSED" << endl;
	return 0;
}
