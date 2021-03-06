#pragma once
#include <vector>
#include <string>
#include <array>

namespace util {

	//Converts [size] ints at a pointer into a uint64_t.  
	uint64_t convertType(std::vector<uint8_t>::iterator iter, int size);

	//Converts a given number into a vector
	std::vector<uint8_t> NumToVector(size_t in, int size);

	//Fills the input vector with the input hex string
	void HexToVector(const std::string hex, std::vector<uint8_t>* in);
}