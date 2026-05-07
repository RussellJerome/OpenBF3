#pragma once
#include "FRDSockets.h"
#include "engine/mem.h"

enum FRDNATType : __int32
{
	FRDNATType_Open = 0x0,
	FRDNATType_Moderate = 0x1,
	FRDNATType_Strict = 0x2,
	FRDNATType_Max = 0x3,
};

union $86F42592D46BAB39BF3744EC664D567B
{
	unsigned int ip;
	unsigned __int8 ip_inBytes[4];
};

struct __declspec(align(4)) AddressID
{
	$86F42592D46BAB39BF3744EC664D567B ___u0;
	unsigned __int16 port;
};

struct __declspec(align(4)) ConnectionID
{
	FRDNATType nat;
	AddressID privateAddress;
	AddressID publicAddress;
	bool sendDirect;
};

enum ConnectionType : __int32
{
	ConnectionTypeNone = 0x0,
	ConnectionTypeClientServer = 0x1,
	ConnectionTypeClientClient = 0x2,
};

enum PacketReliability : __int32
{
	ReliabilityUnreliable = 0x0,
	ReliabilityUnreliableOrdered = 0x1,
	ReliabilityReliable = 0x2,
	ReliabilityReliableOrdered = 0x3,
	ReliabilityMax = 0x4,
};

struct PacketCounter
{
	unsigned __int16 count;
};

struct Connection
{
	struct RChannel
	{
		PacketCounter recvCounter;
		PacketCounter sendCounter;
		dlinklistdef_s recvList;
		dlinklistdef_s sendList;
	};

	struct __declspec(align(4)) ROChannel
	{
		PacketCounter recvCounter;
		PacketCounter sendCounter;
		dlinklistdef_s recvList;
		dlinklistdef_s sendList;
		bool receivedListStalled;
	};

	struct UChannel
	{
		PacketCounter recvCounter;
		PacketCounter sendCounter;
	};

	bool m_accepted;
	bool m_sendingPrivateAddress;
	float m_timeSinceConnectionRequest;
	ConnectionID m_connectionId;
	ConnectionType m_type;
	float m_timeSinceRecv;
	float m_timeSincePing;
	int m_sendBufferSize;
	unsigned __int8 m_sendBuffer[1264];
	int m_connectionAttemptCount;
	float m_timeSinceSendBufferSend;
	float m_msgTravelTime[5];
	unsigned __int8 m_msgTravelTimeIndex;
	float m_minLatency;
	float m_maxLatency;
	float m_aveLatency;
	PacketReliability m_acknowledgeReliability;
	int m_acknowledgeChannel;
	bool m_acknowledgeSend;
	int m_copyToIndex;
	int m_numPlayers;
	int m_slotNum;
	Connection::RChannel rchannel;
	Connection::ROChannel rochannel[32];
	Connection::UChannel uchannel[2048];
};

struct Packet
{
	AddressID address;
	PacketReliability reliability;
	unsigned __int16 channel;
	unsigned int dataSize;
	unsigned __int8* data;
};

class PacketHeader
{
public:
	PacketReliability reliability;
	PacketCounter count;
	unsigned __int16 channel;
	bool connected;
	bool ackincluded;
	bool acksend;
	PacketReliability ackreliability;
	PacketCounter ackcount;
	unsigned __int16 ackchannel;
};

class PacketPoolItem : public Packet
{
public:
	bool resendrequested;
	bool datavalid;
	bool allocatedReliable;
	int allocatedSize;
	float timer;
	dlinkdef_s link;
	PacketHeader header;
	unsigned __int8 dataArray[1264];
};

struct FRDNetStats
{
	unsigned int bytesRecv;
	unsigned int gameBytesSent;
	unsigned int repeatBytesSent;
	unsigned int systemBytesSent;
	unsigned int voiceBytesSent;
	unsigned int mergedPacketsSent;
	unsigned int mergedPacketsRecv;
	unsigned int totalPacketsSent;
	unsigned int totalPacketsRecv;
	float recvSpeed;
	float gameSendSpeed;
	float repeatSendSpeed;
	float systemSendSpeed;
	float voiceSendSpeed;
	float mergedPacketsSendSpeed;
	float mergedPacketsRecvSpeed;
	float totalPacketsSendSpeed;
	float totalPacketsRecvSpeed;
	unsigned int packetSizesSend[8];
	unsigned int packetSizesRecv[8];
	unsigned int packetSizesSendCurrent[8];
	unsigned int packetSizesRecvCurrent[8];
};

class FRDNetPeer
{
public:
	struct PeerInfo
	{
		float time;
		unsigned int bytesRecv;
		unsigned int gameBytesSent;
		unsigned int repeatBytesSent;
		unsigned int systemBytesSent;
		unsigned int voiceBytesSent;
		unsigned int mergedPacketsSent;
		unsigned int mergedPacketsRecv;
		unsigned int totalPacketsSent;
		unsigned int totalPacketsRecv;
		unsigned int packetSizesSend[8];
		unsigned int packetSizesRecv[8];
	};
	int m_peerID;
	int m_maxNumPeers;
	unsigned int m_recvQueue;
	dlinklistdef_s m_receivedList;
	ConnectionID m_myPublicAddress;
	Connection* m_connections;
	bool m_server;
	bool m_dedicated;
	bool m_lookingforserver;
	PacketPoolItem* m_connectionPacket;
	bool m_allowServerMigration;
	bool m_ignoreLostConnection;
	FRDNetPeer::PeerInfo m_laststats;
	FRDNetStats m_stats;
};

class FRDPeerHandler
{
public:
	struct PacketSizes
	{
		int size;
		int maxNum;
		int numAllocated;
		int numReliableAllocated;
		int maxNumAllocated;
		int maxNumReliableAllocated;
	};

	bool(__cdecl* m_externalSDKProcessPacketFuncs[2])(unsigned int, unsigned __int16, const char*, int);
	FRDNetPeer* m_peers[2];
	FRDSocket m_socket;
	bool m_threadNeeded;
	bool m_threadActive;
	float m_threadSleepTimer;
	int m_numAllocated;
	int m_maxNum;
	CAutoExtendingPool<PacketPoolItem> m_packetPool;
	FRDPeerHandler::PacketSizes m_sizes[2];
};