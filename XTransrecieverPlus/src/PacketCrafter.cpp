#include "PacketCrafter.h"
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <UdpLayer.h>
#include <PayloadLayer.h>
#include "Util.h"
#include "Config.h"
using namespace std;
using namespace pcpp;
using namespace util;
using namespace cfg;

Packet crft::PiaPacket::craftPacket(vector<uint8_t> data, int srcPort, int dstPort, int dstIP) {
	Packet out;
	EthLayer eth(MacAddress("7c:bb:8a:dd:55:bb"), MacAddress("ff:ff:ff:ff:ff:ff"));
	out.addLayer(&eth);

	IPv4Layer ip;
	ip.getIPv4Header()->ipSrc = IPv4Address(string(interfaceIPAddr)).toInt();
	ip.getIPv4Header()->ipDst = dstIP;
	ip.getIPv4Header()->ipId = htons(2000);
	ip.getIPv4Header()->timeToLive = 64;
	out.addLayer(&ip);

	UdpLayer udp(srcPort, dstPort);
	out.addLayer(&udp);

	PayloadLayer payload(data.data(), data.size(), true);
	out.addLayer(&payload);
	out.computeCalculateFields();
	return out;
}

Packet crft::Lan::craftBrowseReq() {
	//lazy approach
	vector<uint8_t> data;
	HexToVector("000000023affffffff000400040101ffffffff000000010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000020201000000000000003ce0997216b848f1fb8e6a0e332dd59444fb86ab43cdc82f0076068d0107ee3a6169c973e2ad9991233df6ccacf19c6e7019377ffdeb4dae62116fc2ac74a828362949d94372d5dd1ad3875db9a613037555f05032a82c6ee1a83aa33eb7a8ccd559f86ac1ebfa1a2a1ea20bb68aa80e88775b7ba15dc09700a2902a3be5c226c51258ef2ecd49df1c0b5f6ae6ed6bae47cdd6e5604a51ffe0cb6b18163ec54d8dedaeba2dcf365ccb80c15c87861c30e43768d3289049c8df0d0d52af995749aeb23bb2e14f2ae09db4eae59627786f9e1812ae0e4182975e7152b301df91e1ad88324b103919e8b905296d869dcac01ee5bc89e7d21070d3f1807c1331982feea20672541109de9b14e31c0503913098e6fb70c92f7da1c73aedb44ff44e9e5e", &data);
	return craftPacket(data, 30000, 30000);
}