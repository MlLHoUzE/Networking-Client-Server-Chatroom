//INFO6016 Network Programming Project 2 Buffer
//Creators: Ben MCIlmoyle, James Kelly, Eric Marcinowski (Ari okayed 3 man group due to odd number of students)

#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdint.h>
#include <string>
#include <vector>

class Buffer {
public:
	Buffer(std::size_t size);
	virtual ~Buffer();

	void writeShort16BE(std::size_t index, short int value);
	void writeShort16BE(short int value);
	void writeStringBE(std::string value);
	void writeStringBE(std::size_t index, std::string value);

	short int readShort16BE(std::size_t index);
	short int readShort16BE();
	std::string Buffer::readStringBE(std::size_t stringLength);
	std::string Buffer::readStringBE(std::size_t index, std::size_t stringLength);

	void writeUInt32BE(std::size_t index, uint32_t value);
	void writeUInt32BE(uint32_t value);
	uint32_t readUInt32BE(std::size_t index);
	uint32_t readUInt32BE();

	void writeInt32BE(std::size_t index, int32_t value);
	void writeInt32BE(int32_t value);
	int32_t readInt32BE(std::size_t index);
	int32_t readInt32BE();

	void writeUInt16BE(std::size_t index, uint16_t value);
	void writeUInt16BE(uint16_t value);
	uint16_t readUInt16BE(std::size_t index);
	uint16_t readUInt16BE();

	void writeInt16BE(std::size_t index, int16_t value);
	void writeInt16BE(int16_t value);
	int16_t readInt16BE(std::size_t index);
	int16_t readInt16BE();

	std::size_t getReadIndex();
	std::size_t getWriteIndex();

	void setReadIndex(std::size_t index);
	void setWriteIndex(std::size_t index);

	void clear();
	void growBuffer(size_t growAmount);
	void printInHex();
	void loadBuffer(char recvbuf[512], size_t size);
	const char* toCharArray();

private:
	std::vector<uint8_t> _buffer;
	int writeIndex;
	int readIndex;
	std::string sendingString; // a string used as a place holder in sending the buffer. We need the variable to stick around since we are pointing to it in a later function
};


#endif
