#include "PacketParser.h"
#include "Util.h"
#include <UdpLayer.h>
#include <PayloadLayer.h>
#include <IPv4Layer.h>
#include <iostream>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

using namespace pcpp;
using namespace std;
using namespace util;

bool Parser::onPacket(Packet packet) {
	
	resetAll();

	UdpLayer* udpLayer = packet.getLayerOfType<UdpLayer>();
	IPv4Layer* ipv4Layer = packet.getLayerOfType<IPv4Layer>();
	PayloadLayer* payloadLayer = packet.getLayerOfType<PayloadLayer>();
	
	if (udpLayer == nullptr || payloadLayer == nullptr || ipv4Layer == nullptr)
		return false;

	udpInfo.message_len = payloadLayer->getPayloadLen();
	uint8_t* message_pointer = payloadLayer->getData();

	udpInfo.srcIP = ntohl(ipv4Layer->getSrcIpAddress().toInt());
	udpInfo.dstIP = ntohl(ipv4Layer->getDstIpAddress().toInt());
	udpInfo.srcPort = (int)udpLayer->getUdpHeader()->portSrc;
	udpInfo.dstPort = (int)udpLayer->getUdpHeader()->portDst;
	
	printf("len: %d", udpInfo.message_len);

	//Initialize raw with the payload data
	raw = vector<uint8_t>(udpInfo.message_len, 0); //doing raw.resize() crashes for some reason
	for (int i = 0; i < udpInfo.message_len; i++) {
		raw[i] = *(message_pointer + i);
	}

	switch (raw[0])
	{
	case PIA_MSG:
		parsePia(raw);
		break;
	case BROWSE_REQUEST:
		message.payload = raw;
		message.payload_size = raw.size();
		break;
	case BROWSE_REPLY:
		parseBrowseReply(); //Used to get session key (used for encryption/decryption)
		printf("Browse Reply from %x\n", udpInfo.srcIP);
		break;
	}
	return true;
}

bool Parser::parsePia(vector<uint8_t> raw) {
	//Check if header matches
	for (int i = 0x00; i < 0x04; i++) {
		if (raw[i] != header.magic[i])
			return false;
	}

	vector<uint8_t>::iterator iter = raw.begin() + 5;
	iter = header.fill(iter);

	//The encrypted packet without unencrypted header
	vector<uint8_t> enc;
	while (iter < raw.end())
		enc.push_back(*iter++);


	vector<uint8_t> dec;
	if(DecryptPia(enc, &dec))
		message.setMessage(dec);
	
	return true;
}

vector<uint8_t>::iterator Parser::PIAHeader::fill(vector<uint8_t>::iterator iter) {
	connID = *iter++;
	packetID = convertType(iter, 2); iter += 2;
	copy(iter, iter + 8, nonceCounter.data()); iter += 8;
	copy(iter, iter + 16, tag.data()); iter += 16;

	return iter;
}

int Parser::Message::setMessage(vector<uint8_t> data) {
	vector<uint8_t>::iterator iter = data.begin();
	
	
	field_flags = *iter++;
	//set all message header values according to the field flags
	if (field_flags & 1)
		msg_flag = *iter++;
	if (field_flags & 2) {
		payload_size = convertType(iter, 2);
		iter += 2;
	}
	if (field_flags & 4) {
		protocol_type = *iter++;
		copy(iter, iter + 3, protocol_port);
		iter += 3;
	}
	if (field_flags & 8) {
		destination = convertType(iter, 8);
		iter += 8;
	}
	if (field_flags & 16) {
		source_station_id = convertType(iter, 8);
		iter += 8;
	}

	payload.resize(payload_size);
	for (int i = 0; i < payload_size; i++)
		payload.push_back(*iter++);

	return iter - data.begin();
}

bool Parser::parseBrowseReply() {

	if (udpInfo.message_len != 1360) { return false; } //Safety check; all browse reply packets are 1360 bytes long
										 //Checking for a matching session id is not yet implemented, so some errors may arise when attempting to use this program in a room with more than two switches

	uint8_t session_param[32];
	for (int i = 0; i < 32; i++) {
		session_param[i] = raw[i + 1270];
	}
	session_param[31] += 1;
	
	setSessionKey(session_param); //This is all we care about

	for (int i = 0; i < 4; i++) {
		sessionID[i] = raw[i + 9];
	}
	return true;
}

void Parser::setSessionKey(const uint8_t mod_param[]) //creates hash of the given array and sets session key to it
{
	HMAC_CTX* ctx = HMAC_CTX_new();
	unsigned int hmac_len;
	uint8_t session_key_ext[32] = {};
	HMAC_Init_ex(ctx, GAME_KEY, 16, EVP_sha256(), nullptr);

	HMAC_Update(ctx, mod_param, 32);
	HMAC_Final(ctx, session_key_ext, &hmac_len);

	//set actual sessionKey equal to first 16 bytes of the full key.
	for (int i = 0; i < 16; i++) {
		sessionKey[i] = session_key_ext[i];
	}
	decryptable = true;
}

bool Parser::DecryptPia(const std::vector<uint8_t> encrypted, std::vector<uint8_t> *decrypted) {
	
	//If this bit is set the packet isn't encrypted
	if (header.version >> 7 == 0)
		return true;
	else if (!decryptable)
		return false; //Cannot decrypt if sessionKey isn't set

	uint8_t nonce[12];

	//Set the nonce with the source ip and nonce counter
	for (int i = 0; i < 4; i++)
		nonce[i] = (udpInfo.srcIP >> (24 - i * 8) & 0xFF);
	nonce[4] = header.connID;
	for (int i = 1; i < 8; i++) {
		nonce[i + 4] = header.nonceCounter[i];
	}

	int decrypted_len;
	decrypted->resize(encrypted.size(), 0);

	//Start decryption
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr);
	EVP_DecryptInit_ex(ctx, nullptr, nullptr, sessionKey.data(), nonce);
	EVP_DecryptUpdate(ctx, decrypted->data(), &decrypted_len, encrypted.data(), encrypted.size());
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, header.tag.data());


	if (EVP_DecryptFinal_ex(ctx, decrypted->data() + decrypted_len, &decrypted_len) == 0) {
		EVP_CIPHER_CTX_free(ctx);
		return false;
	}

	EVP_CIPHER_CTX_free(ctx);
	return true;

}

void Parser::resetAll() {
	raw.resize(1);
	udpInfo = udpInfoReset;
	header = headerReset;
	message = messageReset;
}