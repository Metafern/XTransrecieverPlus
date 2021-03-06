#pragma once
#include <iostream>
#include <fstream>
#include <string>
namespace cfg {

	const static bool is_live = true; //if false, reads + parses a file. Otherwise, does a live connection
	const std::string fileName = "XTransreciever_19_sw.pcap";
	const std::string searchfilter = "((src or dst portrange 49151-49156) or (src or dst port 30000)) and (udp)";
	const std::string interfaceIPAddr = "10.0.0.224"; //Your computer local IP addr
	const std::string switchIPAddr = "10.0.0.61"; //Your switch local IP addr
	const uint32_t switchIPAddrInt = 0x0a00003d;
	const uint32_t selfIPAddrInt = 0x0a0000e0;
};
