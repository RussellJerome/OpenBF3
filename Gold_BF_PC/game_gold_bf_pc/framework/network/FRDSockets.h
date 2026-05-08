#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


//Many may wonder (why the hell do we care about sockets this early on)
//the answer is simple
//Because networking helps stress test core systems, so while rewriting the game is important
//Recreating a DEDICATED SERVER for the existing r7 build also is important
//Sort of

struct FRDSocket
{
	enum Protocol : __int32
	{
		PROTOCOL_UDP = 0x0,
		PROTOCOL_VDP = 0x1,
	};
	unsigned int s;
	FRDSocket::Protocol protocol;
};

class FRDSockets
{
public:
	int SendTo(FRDSocket* socket, const char* data, int length, unsigned int binaryAddress, unsigned short port);
	int RecvFrom(FRDSocket* socket, char* data, unsigned int* binaryAddress, unsigned int* port);
	void CreateBoundSocket(FRDSocket* sock, unsigned short port, bool online);

	static bool m_started;
	static FRDSockets* m_instance;
	static WSAData m_winsockinfo;
};
