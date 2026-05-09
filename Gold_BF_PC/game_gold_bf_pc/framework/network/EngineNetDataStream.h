#pragma once

union char16_s;

class EngineNetDataStream
{
public:
	virtual ~EngineNetDataStream() {}
	virtual void SerialiseString(char*, int, bool) {}

	void WriteBits(const unsigned char* data, int numbits);
	void ReadBits(unsigned char* data, int numbits);

	void Serialise(bool* data);
	void Serialise(unsigned char* data, unsigned char min, unsigned char max);
	void Serialise(unsigned short* data, unsigned short min, unsigned short max);
	void Serialise(unsigned int* data, unsigned int min, unsigned int max);
	void Serialise(unsigned long long* data, unsigned long long min, unsigned long long max);
	void Serialise(double* data);
	void Serialise(char16_s* data);
	void SerialiseBuffer(char* src, unsigned int size);
	//EngineNetDataStream::SerialiseString
	void SerialiseUniCharString(char* string, int maxlen);

	bool m_read;
	unsigned __int8* m_data;
	int m_databitsize;
	int m_bytenum;
	int m_bitnum;
};