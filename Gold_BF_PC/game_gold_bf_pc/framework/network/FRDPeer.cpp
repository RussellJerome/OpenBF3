#include "FRDPeer.h"
#include "logger/Log.h"
#include "EngineNetDataStream.h"
#include "engine/engineinit.h"

#include <cstdio>
#include <direct.h>
#define DumpPacket

void Serialise(
    EngineNetDataStream* stream,
    PacketReliability* reliability,
    PacketCounter* count,
    unsigned short* channel);

FRDNetPeer::FRDNetPeer()
{
    m_myPublicAddress.privateAddress.___u0.ip = -1;
    m_myPublicAddress.privateAddress.port = -1;
    m_myPublicAddress.publicAddress.___u0.ip = -1;
    m_myPublicAddress.publicAddress.port = -1;
    m_myPublicAddress.nat = FRDNATType_Open;

    m_maxNumPeers = 0;
    m_connections = 0;
    m_server = 1;
    m_lookingforserver = 0;

    m_recvQueue = tsQueueCreate(0x100, 0x4FC);

    m_receivedList.head = (dlinkdef_s*)&m_receivedList;
    m_receivedList.tail = (dlinkdef_s*)&m_receivedList;
    m_receivedList.offset = -36;

    memset(&m_stats, 0, 0xC8);
    memset(&m_laststats, 0, 0x68);

    m_connectionPacket = 0;
    m_allowServerMigration = 1;
    m_ignoreLostConnection = 0;
}

void FRDPeerHandler::FreePacketPoolItem(PacketPoolItem* item)
{
    // Get size index from allocatedSize field
    int sizeIdx = item->allocatedSize;

    // Decrement global allocated count
    s_peerHandler->m_numAllocated--;

    // Decrement per-size allocated count
    PacketSizes* sizes = &s_peerHandler->m_sizes[sizeIdx];
    sizes->numAllocated--;

    // If this was a reliable packet, decrement reliable count too
    if (item->allocatedReliable)
        sizes->numReliableAllocated--;

    // Push back onto free list
    *(void**)item = s_peerHandler->m_packetPool.m_poolae.state.free;
    s_peerHandler->m_packetPool.m_poolae.state.free = (poolObject*)item;
    s_peerHandler->m_packetPool.m_poolae.state.freeCount++;
}

void FRDNetPeer::FreeUnreliablePackets(int numkeep)
{
    dlinklistdef_s* list = &m_receivedList;
    int             kept = 0;
    dlinkdef_s* node = list->tail;

    if ((dlinklistdef_s*)node == list)
        return;

    do
    {
        dlinkdef_s* prev = node->prev;
        PacketPoolItem* item = (PacketPoolItem*)((char*)node + list->offset);

        if (!item->allocatedReliable)
        {
            if (kept >= numkeep)
            {
                // Unlink from list
                node->next->prev = prev;
                node->prev->next = node->next;
                node->next = node;
                node->prev = node;

                FRDPeerHandler::s_peerHandler->FreePacketPoolItem(item);
            }
            else
            {
                kept++;
            }
        }

        node = prev;
    } while ((dlinklistdef_s*)node != list);
}

void FRDNetPeer::InitialiseConnection(Connection* connection)
{
    connection->m_timeSincePing = 0.0f;
    connection->m_timeSinceRecv = 0.0f;
    connection->m_timeSinceSendBufferSend = 0.0f;
    connection->m_acknowledgeReliability = ReliabilityReliable;
    connection->m_type = ConnectionTypeNone;
    connection->m_acknowledgeSend = 1;
    connection->m_accepted = 0;
    connection->m_sendingPrivateAddress = 0;
    connection->m_sendBufferSize = 0;
    connection->m_connectionAttemptCount = 0;
    connection->m_acknowledgeChannel = 0;

    memset(connection->m_msgTravelTime, 0, 0x14);

    connection->m_minLatency = 0.0f;
    connection->m_maxLatency = 0.0f;
    connection->m_aveLatency = 0.0f;
    connection->m_msgTravelTimeIndex = 0;
    connection->m_numPlayers = 0;
    connection->m_copyToIndex = -1;

    // Init reliable channel recv/send lists
    {
        dlinklistdef_s* rl = &connection->rchannel.recvList;
        rl->head = (dlinkdef_s*)rl;
        rl->tail = (dlinkdef_s*)rl;
        rl->offset = -36;

        dlinklistdef_s* sl = &connection->rchannel.sendList;
        sl->head = (dlinkdef_s*)sl;
        sl->tail = (dlinkdef_s*)sl;
        sl->offset = -36;

        connection->rchannel.recvCounter.count = 0;
        connection->rchannel.sendCounter.count = 0;
    }

    // Init 32 unreliable ordered channels
    for (int i = 0; i < 32; i++)
    {
        dlinklistdef_s* recvList = &connection->rochannel[i].recvList;
        recvList->head = (dlinkdef_s*)recvList;
        recvList->tail = (dlinkdef_s*)recvList;
        recvList->offset = -36;

        dlinklistdef_s* sendList = &connection->rochannel[i].sendList;
        sendList->head = (dlinkdef_s*)sendList;
        sendList->tail = (dlinkdef_s*)sendList;
        sendList->offset = -36;

        connection->rochannel[i].recvCounter.count = 0;
        connection->rochannel[i].sendCounter.count = 0;
    }

    // Init 2048 unreliable channels
    for (int i = 0; i < 2048; i++)
    {
        connection->uchannel[i].recvCounter.count = 0;
        connection->uchannel[i].sendCounter.count = 0;
    }
}

void FRDNetPeer::CopyConnection(Connection* to, Connection* from)
{
    // Copy all plain data fields
    *to = *from;

    // Fix up rchannel recv list pointers
    {
        dlinklistdef_s* toList = &to->rchannel.recvList;
        dlinklistdef_s* fromList = &from->rchannel.recvList;

        if (fromList->head == (dlinkdef_s*)fromList)
        {
            toList->head = (dlinkdef_s*)toList;
            toList->tail = (dlinkdef_s*)toList;
        }
        else
        {
            toList->head->prev = (dlinkdef_s*)toList;
            toList->tail->next = (dlinkdef_s*)toList;
        }
    }

    // Fix up rchannel send list pointers
    {
        dlinklistdef_s* toList = &to->rchannel.sendList;
        dlinklistdef_s* fromList = &from->rchannel.sendList;

        if (fromList->head == (dlinkdef_s*)fromList)
        {
            toList->head = (dlinkdef_s*)toList;
            toList->tail = (dlinkdef_s*)toList;
        }
        else
        {
            toList->head->prev = (dlinkdef_s*)toList;
            toList->tail->next = (dlinkdef_s*)toList;
        }
    }

    // Fix up 32 unreliable ordered channel list pointers (4 lists per channel, 8 channels per iteration)
    for (int i = 0; i < 32; i++)
    {
        // recvList
        {
            dlinklistdef_s* toList = &to->rochannel[i].recvList;
            dlinklistdef_s* fromList = &from->rochannel[i].recvList;

            if (fromList->head == (dlinkdef_s*)fromList)
            {
                toList->head = (dlinkdef_s*)toList;
                toList->tail = (dlinkdef_s*)toList;
            }
            else
            {
                toList->head->prev = (dlinkdef_s*)toList;
                toList->tail->next = (dlinkdef_s*)toList;
            }
        }

        // sendList
        {
            dlinklistdef_s* toList = &to->rochannel[i].sendList;
            dlinklistdef_s* fromList = &from->rochannel[i].sendList;

            if (fromList->head == (dlinkdef_s*)fromList)
            {
                toList->head = (dlinkdef_s*)toList;
                toList->tail = (dlinkdef_s*)toList;
            }
            else
            {
                toList->head->prev = (dlinkdef_s*)toList;
                toList->tail->next = (dlinkdef_s*)toList;
            }
        }
    }
}

int FRDNetPeer::GetFreeConnection()
{
    int maxPeers = m_maxNumPeers;
    if (maxPeers <= 0)
        return -1;

    Connection* conn = m_connections;
    for (int i = 0; i < maxPeers; i++, conn++)
    {
        // A free slot has ip == -1 AND port == 0xFFFF
        bool free = (conn->m_connectionId.publicAddress.___u0.ip == (unsigned int)-1 &&
            conn->m_connectionId.publicAddress.port == 0xFFFF);

        if (free)
        {
            InitialiseConnection(conn);
            return i;
        }
    }

    return -1;
}

void FRDNetPeer::KillConnection(Connection* connection)
{
    // Invalidate connection ID
    connection->m_connectionId.privateAddress.___u0.ip = (unsigned int)-1;
    connection->m_connectionId.privateAddress.port = 0xFFFF;
    connection->m_connectionId.publicAddress.___u0.ip = (unsigned int)-1;
    connection->m_connectionId.publicAddress.port = 0xFFFF;

    // Helper lambda to drain a packet list and free all items
    auto drainList = [](dlinklistdef_s* list)
        {
            dlinkdef_s* node = list->head;
            while ((dlinklistdef_s*)node != list)
            {
                dlinkdef_s* next = node->next;
                PacketPoolItem* item = (PacketPoolItem*)((char*)node + list->offset);

                // Unlink
                node->next->prev = node->prev;
                node->prev->next = node->next;
                node->next = node;
                node->prev = node;
                
                // Free to pool
                FRDPeerHandler::s_peerHandler->m_numAllocated--;
                FRDPeerHandler::PacketSizes* sizes = &FRDPeerHandler::s_peerHandler->m_sizes[item->allocatedSize];
                sizes->numAllocated--;
                if (item->allocatedReliable)
                    sizes->numReliableAllocated--;

                *(void**)item = FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.free;
                FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.free = (poolObject*)item;
                FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.freeCount++;

                node = next;
            }
        };

    // Drain reliable channel recv and send lists
    drainList(&connection->rchannel.recvList);
    drainList(&connection->rchannel.sendList);

    // Drain 32 unreliable ordered channels (recv and send each)
    for (int i = 0; i < 32; i++)
    {
        drainList(&connection->rochannel[i].recvList);
        drainList(&connection->rochannel[i].sendList);
    }

    // Remove any received packets from this connection's peer in the global received list
    dlinkdef_s* node = m_receivedList.head;
    while ((dlinklistdef_s*)node != &m_receivedList)
    {
        dlinkdef_s* next = node->next;
        PacketPoolItem* item = (PacketPoolItem*)((char*)node + m_receivedList.offset);

        if (item->address.___u0.ip == connection->m_connectionId.privateAddress.___u0.ip &&
            item->address.port == connection->m_connectionId.privateAddress.port)
        {
            // Unlink
            node->next->prev = node->prev;
            node->prev->next = node->next;
            node->next = node;
            node->prev = node;

            // Free to pool
            FRDPeerHandler::s_peerHandler->m_numAllocated--;
            FRDPeerHandler::PacketSizes* sizes = &FRDPeerHandler::s_peerHandler->m_sizes[item->allocatedSize];
            sizes->numAllocated--;
            if (item->allocatedReliable)
                sizes->numReliableAllocated--;

            *(void**)item = FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.free;
            FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.free = (poolObject*)item;
            FRDPeerHandler::s_peerHandler->m_packetPool.m_poolae.state.freeCount++;
        }

        node = next;
    }

    connection->m_copyToIndex = -1;
}

Connection* FRDNetPeer::GetConnectionFromPrivateID(AddressID* id)
{
    if (m_maxNumPeers <= 0)
        return nullptr;

    for (int i = 0; i < m_maxNumPeers; i++)
    {
        Connection* conn = &m_connections[i];
        if (conn->m_connectionId.privateAddress.___u0.ip == id->___u0.ip &&
            conn->m_connectionId.privateAddress.port == id->port)
            return conn;
    }

    return nullptr;
}

Connection* FRDNetPeer::GetConnectionFromPublicID(AddressID* id)
{
    if (m_maxNumPeers <= 0)
        return nullptr;

    for (int i = 0; i < m_maxNumPeers; i++)
    {
        Connection* conn = &m_connections[i];
        if (conn->m_connectionId.publicAddress.___u0.ip == id->___u0.ip &&
            conn->m_connectionId.publicAddress.port == id->port)
            return conn;
    }

    return nullptr;
}

Connection* FRDNetPeer::GetConnectionFromPublicID(ConnectionID* id)
{
    if (m_maxNumPeers <= 0)
        return nullptr;

    for (int i = 0; i < m_maxNumPeers; i++)
    {
        Connection* conn = &m_connections[i];
        if (conn->m_connectionId.publicAddress.___u0.ip == id->publicAddress.___u0.ip &&
            conn->m_connectionId.publicAddress.port == id->publicAddress.port)
            return conn;
    }

    return nullptr;
}

void WriteHeaderData(PacketHeader* header, unsigned char** data, unsigned char* dataArray)
{
    EngineNetDataStream stream;
    stream.m_read = false;
    stream.m_data = dataArray;
    stream.m_databitsize = 0x2780;
    stream.m_bytenum = 0;
    stream.m_bitnum = 0;

    Serialise(&stream, &header->reliability, &header->count, &header->channel);

    stream.Serialise(&header->connected);
    stream.Serialise(&header->ackincluded);

    if (header->ackincluded)
    {
        stream.Serialise(&header->acksend);
        Serialise(&stream, &header->ackreliability, &header->ackcount, &header->ackchannel);
    }

    int bytesWritten = stream.m_bytenum;
    if (stream.m_bitnum)
        bytesWritten++;

    *data = dataArray + bytesWritten;
}

void FRDNetPeer::AddAcknowledge(Connection* connection, PacketHeader* header)
{
    header->ackincluded = true;

    PacketReliability ackRel = connection->m_acknowledgeReliability;

    if (connection->m_acknowledgeSend)
    {
        header->acksend = true;

        if (ackRel == ReliabilityReliable)
        {
            header->ackreliability = ReliabilityReliable;
            header->ackcount.count = connection->rchannel.sendCounter.count;
            connection->m_acknowledgeChannel = 0;
            connection->m_acknowledgeReliability = ReliabilityReliableOrdered;
        }
        else
        {
            header->ackreliability = ReliabilityReliableOrdered;

            int ch = (unsigned short)connection->m_acknowledgeChannel;
            header->ackchannel = (unsigned short)ch;
            header->ackcount.count = connection->rochannel[ch].sendCounter.count;

            connection->m_acknowledgeChannel++;
            if (connection->m_acknowledgeChannel >= 32)
            {
                connection->m_acknowledgeSend = false;
                connection->m_acknowledgeReliability = ReliabilityReliable;
            }
        }
    }
    else
    {
        header->acksend = false;

        if (ackRel == ReliabilityReliable)
        {
            header->ackreliability = ReliabilityReliable;
            header->ackcount.count = connection->rchannel.recvCounter.count;
            connection->m_acknowledgeChannel = 0;
            connection->m_acknowledgeReliability = ReliabilityReliableOrdered;
        }
        else
        {
            header->ackreliability = ReliabilityReliableOrdered;

            int ch = (unsigned short)connection->m_acknowledgeChannel;
            header->ackchannel = (unsigned short)ch;
            header->ackcount.count = connection->rochannel[ch].recvCounter.count;

            connection->m_acknowledgeChannel++;
            if (connection->m_acknowledgeChannel >= 32)
            {
                connection->m_acknowledgeSend = true;
                connection->m_acknowledgeReliability = ReliabilityReliable;
            }
        }
    }
}

void FRDNetPeer::SendSendBuffer(Connection* connection)
{
    int bufSize = connection->m_sendBufferSize;

    if (FRDPeerHandler::s_peerHandler->m_socket.protocol == FRDSocket::PROTOCOL_VDP)
    {
        // VDP header: size field = bufSize - 2 (excludes the 2-byte size field itself)
        *(unsigned short*)&connection->m_sendBuffer[0] = (unsigned short)(bufSize - 2);
    }
    else
    {
        // Standard UDP header: 0xF0 0x0D + checksum over bytes 6+
        connection->m_sendBuffer[0] = 0xF0;
        connection->m_sendBuffer[1] = 0x0D;

        unsigned int checksum = 0;
        int dataLen = bufSize - 6;
        if (dataLen > 0)
        {
            unsigned char* p = &connection->m_sendBuffer[6];
            for (int i = 0; i < dataLen; i++)
            {
                unsigned char b = p[i];
                checksum ^= (checksum << 5) + (checksum >> 2) + b;
            }
        }

        connection->m_sendBuffer[2] = (unsigned char)(checksum >> 24);
        connection->m_sendBuffer[3] = (unsigned char)(checksum >> 16);
        connection->m_sendBuffer[4] = (unsigned char)(checksum >> 8);
        connection->m_sendBuffer[5] = (unsigned char)(checksum >> 0);
    }

    if (FRDPeerHandler::s_peerHandler->m_socket.s != -1)
    {
        FRDSockets::m_instance->SendTo(
            &FRDPeerHandler::s_peerHandler->m_socket,
            (const char*)connection->m_sendBuffer,
            bufSize,
            connection->m_connectionId.privateAddress.___u0.ip,
            connection->m_connectionId.privateAddress.port);
    }

    connection->m_timeSinceSendBufferSend = 0.0f;
    connection->m_sendBufferSize = 0;
    m_stats.mergedPacketsSent++;
}

void FRDNetPeer::AddToSendBuffer(PacketPoolItem* p, bool voice)
{
    Connection* conn = GetConnectionFromPrivateID(&p->address);

    unsigned char* data = p->data;
    unsigned int   dataSize = (unsigned int)((char*)data - (char*)p + p->dataSize - 0x40);
    unsigned short pktSize = (unsigned short)dataSize;

    // Voice packets go straight out via VDP
    if (voice && FRDPeerHandler::s_peerHandler->m_socket.protocol == FRDSocket::PROTOCOL_VDP)
    {
        // VDP header: [size(2)] [peerID(1)] [data...]
        unsigned char buf[1280];
        *(unsigned short*)buf = (unsigned short)(pktSize + 1);
        buf[2] = (unsigned char)m_peerID;
        memcpy(buf + 3, p->dataArray, pktSize);

        if (FRDPeerHandler::s_peerHandler->m_socket.s != -1)
            FRDSockets::m_instance->SendTo(
                &FRDPeerHandler::s_peerHandler->m_socket,
                (const char*)buf,
                pktSize + 3,
                p->address.___u0.ip,
                p->address.port);
        return;
    }
    unsigned char packetType = data[0];

    // Connection-less or special packet types go straight out
    if (!conn || packetType == 3 || packetType == 4 || packetType == 10)
    {
        if (FRDPeerHandler::s_peerHandler->m_socket.protocol == FRDSocket::PROTOCOL_VDP)
        {
            // VDP format: [size(2)] [peerID(1)] [data...]
            unsigned char buf[1280];
            *(unsigned short*)buf = (unsigned short)(pktSize + 1);
            buf[2] = (unsigned char)m_peerID;
            memcpy(buf + 3, p->dataArray, pktSize);

            if (FRDPeerHandler::s_peerHandler->m_socket.s != -1)
                FRDSockets::m_instance->SendTo(&FRDPeerHandler::s_peerHandler->m_socket,
                    (const char*)buf, pktSize + 3,
                    p->address.___u0.ip, p->address.port);
        }
        else
        {
            // Standard UDP format: [0xF0][0x0D][checksum(4)][peerID(1)][data...]
            unsigned char buf[1280];
            buf[0] = 0xF0;
            buf[1] = 0x0D;
            buf[6] = (unsigned char)m_peerID;
            memcpy(buf + 7, p->dataArray, pktSize);

            // Compute checksum over peerID + data
            unsigned int checksum = 0;
            unsigned char* cs = &buf[6];
            for (unsigned int i = 0; i < pktSize + 1; i++)
            {
                unsigned char b = cs[i];
                checksum ^= (checksum << 5) + (checksum >> 2) + b;
            }
            buf[2] = (unsigned char)(checksum >> 24);
            buf[3] = (unsigned char)(checksum >> 16);
            buf[4] = (unsigned char)(checksum >> 8);
            buf[5] = (unsigned char)(checksum >> 0);

            if (FRDPeerHandler::s_peerHandler->m_socket.s != -1)
                FRDSockets::m_instance->SendTo(&FRDPeerHandler::s_peerHandler->m_socket,
                    (const char*)buf, pktSize + 7,
                    p->address.___u0.ip, p->address.port);
        }

        m_stats.mergedPacketsSent++;
        return;
    }

    // Buffered send -- accumulate into connection send buffer
    if (pktSize + conn->m_sendBufferSize + 3 > 1264)
        SendSendBuffer(conn);

    if (!conn->m_sendBufferSize)
    {
        // Initialize send buffer header
        PacketHeader ackHeader;
        memset(&ackHeader, 0, sizeof(ackHeader));
        ackHeader.reliability = ReliabilityUnreliable;
        ackHeader.connected = true;
        AddAcknowledge(conn, &ackHeader);

        unsigned char* headerStart;
        unsigned char* writePtr;

        if (FRDPeerHandler::s_peerHandler->m_socket.protocol == FRDSocket::PROTOCOL_VDP)
        {
            conn->m_sendBuffer[2] = (unsigned char)m_peerID;
            headerStart = &conn->m_sendBuffer[3];
        }
        else
        {
            conn->m_sendBuffer[0] = 0xF0;
            conn->m_sendBuffer[1] = 0x0D;
            conn->m_sendBuffer[6] = (unsigned char)m_peerID;
            headerStart = &conn->m_sendBuffer[7];
        }

        WriteHeaderData(&ackHeader, &writePtr, headerStart);

        int headerSize = (int)(writePtr - (unsigned char*)conn - 0x2F);
        *writePtr = 1; // mark as valid
        conn->m_sendBufferSize = headerSize;
        m_stats.systemBytesSent += headerSize;
    }

    // Append sub-packet: [size(2)][data...]
    *(unsigned short*)&conn->m_sendBuffer[conn->m_sendBufferSize] = pktSize;
    conn->m_sendBufferSize += 2;

    memcpy(&conn->m_sendBuffer[conn->m_sendBufferSize], p->dataArray, pktSize);
    conn->m_sendBufferSize += pktSize;

    m_stats.systemBytesSent += 2;
}

float totalLatency = 0.0; // idb
unsigned __int64 numLatency = 0uLL; // idb
unsigned int count_4 = 0u; // idb
unsigned __int64 numDelayInSecs = 0uLL; // idb

void FRDNetPeer::ProcessNetworkPacket(
    unsigned int    binaryAddress,
    unsigned short  port,
    unsigned char* data,
    int             length,
    PacketHeader* header,
    unsigned char* pdata,
    int             packetsize)
{
    // Set up read stream from packet data
    EngineNetDataStream readStream;
    readStream.m_read = true;
    readStream.m_data = pdata;
    readStream.m_databitsize = packetsize * 8;
    readStream.m_bytenum = 1;
    readStream.m_bitnum = 0;

    // Store sender address
    AddressID senderAddr;
    senderAddr.___u0.ip = binaryAddress;
    senderAddr.port = port;

    RecvQueueItem qitem;
    qitem.address = senderAddr;

    unsigned char packetType = pdata[0];

    if (packetType == 3)
    {
        // Ping request -- build pong response
        EngineNetDataStream writeStream;
        unsigned char       writeBuf[256];
        writeStream.m_read = false;
        writeStream.m_data = writeBuf;
        writeStream.m_databitsize = 256 * 8;
        writeStream.m_bytenum = 1;
        writeStream.m_bitnum = 0;
        writeBuf[0] = 4; // pong type

        // Read timestamp from ping then write it into pong
        double timestamp = 0.0;
        readStream.Serialise(&timestamp);
        writeStream.Serialise(&timestamp);

        bool hasGameTime = false;
        if (m_server)
        {
            hasGameTime = true;
            writeStream.Serialise(&hasGameTime);
            double gameTime = timerGameTime;
            writeStream.Serialise(&gameTime);
        }
        else
        {
            writeStream.Serialise(&hasGameTime);
        }

        int responseSize = writeStream.m_bytenum;
        if (writeStream.m_bitnum)
            responseSize++;

        Connection* conn = GetConnectionFromPrivateID(&senderAddr);

        PacketPoolItem response;
        memset(&response, 0, sizeof(response));
        response.reliability = ReliabilityMax;
        response.datavalid = true;
        response.data = response.dataArray;
        response.dataSize = responseSize;
        response.header.reliability = ReliabilityUnreliable;
        response.header.connected = (conn != nullptr);
        response.address = senderAddr;

        WriteHeaderData(&response.header, &response.data, response.dataArray);
        memcpy(response.data, writeBuf, responseSize);
        AddToSendBuffer(&response, false);

        m_stats.totalPacketsSent++;
        m_stats.systemBytesSent += responseSize;
        m_stats.totalPacketsRecv++;
    }
    else if (packetType == 4)
    {
        // Pong response -- update latency stats
        Connection* conn = GetConnectionFromPrivateID(&senderAddr);

        double immediateTime = timerGetImmediateTime();
        double sentTime = 0.0;
        readStream.Serialise(&sentTime);

        float travelTime = (float)(immediateTime - sentTime);

        if (conn && header->connected)
        {
            conn->m_msgTravelTime[conn->m_msgTravelTimeIndex] = travelTime * 0.5f;
            conn->m_msgTravelTimeIndex = (conn->m_msgTravelTimeIndex + 1) % 5;

            conn->m_minLatency = 3.4028e38f;
            conn->m_maxLatency = 0.0f;
            conn->m_aveLatency = 0.0f;

            for (int i = 0; i < 5; i++)
            {
                float t = conn->m_msgTravelTime[i];
                if (t < conn->m_minLatency) conn->m_minLatency = t;
                if (t > conn->m_maxLatency) conn->m_maxLatency = t;
                conn->m_aveLatency += t;
            }
            conn->m_aveLatency *= 0.2f;

            if (!m_server && conn->m_type == ConnectionTypeClientServer)
            {
                bool hasTargetTime = false;
                readStream.Serialise(&hasTargetTime);

                totalLatency += travelTime;
                numLatency++;

                if (hasTargetTime)
                {
                    double targetTime = 0.0;
                    readStream.Serialise(&targetTime);

                    if (count_4 % 5 == 0)
                    {
                        timerTargetGameTime = targetTime;
                        numDelayInSecs++;
                    }
                    else
                    {
                        count_4++;
                    }
                }
            }
        }

        // Write pong reply into queue item
        EngineNetDataStream writeStream;
        writeStream.m_read = false;
        writeStream.m_data = qitem.data;
        writeStream.m_databitsize = sizeof(qitem.data) * 8;
        writeStream.m_bytenum = 1;
        writeStream.m_bitnum = 0;
        qitem.data[0] = 4;

        unsigned int   addrVal = binaryAddress;
        unsigned short portVal = port;
        unsigned int   timeVal = 0;
        memcpy(&timeVal, &travelTime, sizeof(float));

        writeStream.Serialise(&addrVal, 0u, 0xFFFFFFFFu);
        writeStream.Serialise(&portVal, (unsigned short)0, (unsigned short)0xFFFF);
        writeStream.Serialise(&timeVal, 0u, 0xFFFFFFFFu);

        int writeSize = writeStream.m_bytenum;
        if (writeStream.m_bitnum)
            writeSize++;

        qitem.dataSize = writeSize;
        tsQueueAdd(m_recvQueue, (char*)&qitem, false);
        m_stats.totalPacketsRecv++;
    }
    else
    {
        // Regular data packet -- copy into queue item and enqueue
        qitem.dataSize = length;
        memcpy(qitem.data, data, length);
        tsQueueAdd(m_recvQueue, (char*)&qitem, false);
    }
}

bool FRDNetPeer::Initialize(
    int            peerid,
    unsigned int   maxNumPeers,
    unsigned short localPort,
    bool           isServer,
    bool           isDedicated,
    bool           allowServerMigration,
    bool           online)
{
    if (!m_maxNumPeers)
    {
        m_maxNumPeers = maxNumPeers;

        Connection* conns = nullptr;
        unsigned int allocSize = maxNumPeers * sizeof(Connection);

        if (maxNumPeers <= 0x62B2E && (int)allocSize > 0)
        {
            conns = (Connection*)memAllocAlignCore(
                allocSize, 0, 0,
                "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/boss/common.h",
                598,
                "array memalloc no group",
                4);
        }

        if (conns)
        {
            for (unsigned int i = 0; i < maxNumPeers; i++)
            {
                Connection* conn = &conns[i];

                conn->rchannel.recvCounter.count = 0;
                conn->rchannel.sendCounter.count = 0;

                // Init 32 unreliable ordered channels
                memset(conn->rochannel, 0, sizeof(conn->rochannel));

                // Init 2048 unreliable channels
                memset(conn->uchannel, 0, sizeof(conn->uchannel));
            }
        }

        m_connections = conns;

        // Zero out connection slot memory
        memset(conns, 0, maxNumPeers * 4);

        // Initialise all connection IDs to invalid
        for (unsigned int i = 0; i < m_maxNumPeers; i++)
        {
            Connection* conn = &m_connections[i];
            conn->m_connectionId.privateAddress.___u0.ip = (unsigned int)-1;
            conn->m_connectionId.privateAddress.port = 0xFFFF;
            conn->m_connectionId.publicAddress.___u0.ip = (unsigned int)-1;
            conn->m_connectionId.publicAddress.port = 0xFFFF;
            conn->m_slotNum = i;
        }
    }

    m_peerID = peerid;
    m_server = isServer;
    m_dedicated = isDedicated;
    m_allowServerMigration = allowServerMigration;

    return FRDPeerHandler::s_peerHandler->AddPeer(this, peerid, localPort, online);
}

void Serialise(
    EngineNetDataStream* stream,
    PacketReliability* reliability,
    PacketCounter* count,
    unsigned short* channel)
{
    unsigned int rel = 0;

    if (!stream->m_read)
        rel = (unsigned int)*reliability;

    stream->Serialise(&rel, 0, 3);

    if (stream->m_read)
        *reliability = (PacketReliability)rel;

    switch (*reliability)
    {
    case ReliabilityUnreliableOrdered:
        stream->Serialise(&count->count, 0, 0x3FF);
        stream->Serialise(channel, 0, 0x7FF);
        break;

    case ReliabilityReliable:
        stream->Serialise(&count->count, 0, 0x3FF);
        break;

    case ReliabilityReliableOrdered:
        stream->Serialise(&count->count, 0, 0x3FF);
        stream->Serialise(channel, 0, 0x1F);
        break;

    case ReliabilityUnreliable:
    default:
        // No count or channel for unreliable packets
        break;
    }
}

void ReadHeaderData(PacketHeader* header, unsigned char** data, unsigned char* dataArray)
{
    EngineNetDataStream stream;
    stream.m_read = true;
    stream.m_data = dataArray;
    stream.m_databitsize = 0x2780;
    stream.m_bytenum = 0;
    stream.m_bitnum = 0;

    // Read reliability, count, channel
    Serialise(&stream, &header->reliability, &header->count, &header->channel);

    // Read connected and ackincluded bools
    stream.Serialise(&header->connected);
    stream.Serialise(&header->ackincluded);

    if (header->ackincluded)
    {
        stream.Serialise(&header->acksend);
        Serialise(&stream, &header->ackreliability, &header->ackcount, &header->ackchannel);
    }
    else
    {
        header->acksend = false;
        header->ackreliability = ReliabilityUnreliable;
        header->ackchannel = 0;
        header->ackcount.count = 0;
    }

    // Advance data pointer past the header bytes consumed
    int bytesRead = stream.m_bytenum;
    if (stream.m_bitnum)
        bytesRead++;

    *data = dataArray + bytesRead;
}

void FRDPeerHandler::ProcessNetworkPacket(
    unsigned int binaryAddress,
    unsigned short port,
    const char* data,
    int length)
{
#ifdef DumpPacket
    // Dump every raw packet received
    {
        static int packetDumpIndex = 0;

        _mkdir("packet_dumps");

        char filename[256];
        sprintf_s(
            filename,
            sizeof(filename),
            "packet_dumps/packet_%06d.bin",
            packetDumpIndex++
        );

        FILE* f = nullptr;
        if (fopen_s(&f, filename, "wb") == 0 && f)
        {
            fwrite(data, 1, length, f);
            fclose(f);
        }
    }

#endif // DumpPacket

    PacketHeader header;
    memset(&header, 0, sizeof(header));

    unsigned char* pdata = nullptr;
    // Check external SDK packet handlers first
    for (int i = 0; i < 2; i++)
    {
        if (m_externalSDKProcessPacketFuncs[i] &&
            m_externalSDKProcessPacketFuncs[i](binaryAddress, port, data, length))
            return;
    }

    if (m_socket.protocol == FRDSocket::PROTOCOL_VDP)
    {
        // VDP (Xbox Live) packet format
        unsigned short packetSize = ntohs(*(unsigned short*)data); // big-endian from 360
        unsigned char  peerIdx = (unsigned char)data[2];

        if (peerIdx > 1)
            return;

        FRDNetPeer* peer = m_peers[peerIdx];
        if (!peer)
            return;

        // First sub-packet
        if (packetSize - 1 > 0)
        {
            ReadHeaderData(&header, &pdata, (unsigned char*)data + 3);
            int dataLen = packetSize - 1;
            int packetLen = dataLen - (int)(unsigned int)pdata;
            peer->ProcessNetworkPacket(
                binaryAddress, port,
                (unsigned char*)data + 3, dataLen,
                &header, pdata, packetLen);
        }

        // Second sub-packet
        int remaining = length - packetSize - 2;
        unsigned char* next = (unsigned char*)data + packetSize + 2;
        if (remaining > 0)
        {
            ReadHeaderData(&header, &pdata, next);
            int packetLen = remaining - (int)(unsigned int)pdata;
            peer->ProcessNetworkPacket(
                binaryAddress, port,
                next, remaining,
                &header, pdata, packetLen);
        }

        peer->m_stats.bytesRecv += length;
        peer->m_stats.mergedPacketsRecv++;
    }
    else
    {
        // Standard UDP packet -- check magic header 0xF0 0x0D
        if ((unsigned char)data[0] != 0xF0 || (unsigned char)data[1] != 0x0D)
            return;

        // Verify checksum (bytes 2-5)
        unsigned int storedChecksum = ((unsigned int)(unsigned char)data[2] << 24)
            | ((unsigned int)(unsigned char)data[3] << 16)
            | ((unsigned int)(unsigned char)data[4] << 8)
            | (unsigned int)(unsigned char)data[5];

        unsigned int checksum = 0;
        const char* p = data + 6;
        for (int i = 0; i < length - 6; i++)
        {
            unsigned char b = (unsigned char)p[i];
            checksum ^= (checksum << 5) + (checksum >> 2) + b;
        }

        if (checksum != storedChecksum)
            return;

        // Peer index at byte 6
        unsigned char peerIdx = (unsigned char)data[6];
        if (peerIdx > 1)
            return;

        FRDNetPeer* peer = m_peers[peerIdx];
        if (!peer)
            return;

        unsigned char* payload = (unsigned char*)data + 7;
        int payloadLen = length - 7;

        ReadHeaderData(&header, &pdata, payload);
        int packetLen = payloadLen - (int)(unsigned int)pdata;
        peer->ProcessNetworkPacket(
            binaryAddress, port,
            payload, payloadLen,
            &header, pdata, packetLen);

        peer->m_stats.bytesRecv += length;
        peer->m_stats.mergedPacketsRecv++;
    }
}

DWORD WINAPI FRDNetLoop(void* arg)
{
    FRDPeerHandler* handler = (FRDPeerHandler*)arg;

    handler->m_threadActive = true;

    if (!handler->m_threadNeeded)
    {
        handler->m_threadActive = false;
        return 0;
    }

    char           data[0x520];
    unsigned int   binaryAddress = 0;
    unsigned short port = 0;

    do
    {
        if (handler->m_socket.s != -1)
        {
            int received = FRDSockets::m_instance->RecvFrom(
                &handler->m_socket, data, &binaryAddress, (unsigned int*)&port);

            if (received > 0)
                handler->ProcessNetworkPacket(binaryAddress, port, data, received);

            if (received != -1)
                continue;
        }

        // Socket error -- deinitialize all peers
        for (int i = 0; i < 2; i++)
        {
            if (handler->m_peers[i])
                handler->m_peers[i]->Deinitialize();
        }
    } while (handler->m_threadNeeded);

    handler->m_threadActive = false;
    return 0;
}

void FRDNetPeer::Deinitialize()
{
    STUB_STATIC();
    //Disconnect();

    //s_peerHandler.RemovePeer(m_peerID);

    //// Drain the receive queue
    //unsigned char item[0x520];
    //while (tsQueueRemove(m_recvQueue, item, false))
    //    ;

    //// Free connection array
    //memFreeFlags((char*)m_connections, 4);
    //m_maxNumPeers = 0;
    //m_connections = nullptr;
}

bool FRDPeerHandler::AddPeer(FRDNetPeer* peer, int id, unsigned short localPort, bool online)
{
    if (!m_threadActive)
    {
        // Find max packet count across all size categories
        int maxNum = 0;
        for (int i = 0; i < (int)(sizeof(m_sizes) / sizeof(m_sizes[0])); i++)
        {
            if (m_sizes[i].maxNumAllocated > maxNum)
                maxNum = m_sizes[i].maxNumAllocated;

            m_sizes[i].numAllocated = 0;
            m_sizes[i].numReliableAllocated = 0;
            m_sizes[i].maxNumAllocated = 0;
            m_sizes[i].maxNumReliableAllocated = 0;
        }

        // Init packet pool
        poolInitFromHeap(&m_packetPool.m_poolae.state, 0, sizeof(PacketPoolItem), maxNum);
        m_packetPool.m_poolae.state.objectSize |= 0x20000000;
        m_packetPool.m_poolae.extendCount = 128;
        m_packetPool.m_poolae.heap = 0;
        m_packetPool.m_poolae.addObjArrays = 0;
        m_numAllocated = 0;
        m_maxNum = maxNum;

        // Init sockets if not already started
        if (!FRDSockets::m_started)
        {
            WSAStartup(MAKEWORD(2, 2), &FRDSockets::m_winsockinfo);
            FRDSockets::m_started = true;
        }

        // Create bound UDP socket
        FRDSockets::m_instance->CreateBoundSocket(&m_socket, localPort, online);
        if (m_socket.s == -1)
            return false;

        // Start network thread
        m_threadNeeded = true;
        HANDLE thread = CreateThread(nullptr, 0, FRDNetLoop, this, CREATE_SUSPENDED, nullptr);
        if (thread)
            taskmanStartThreadExHW(thread);

        // Wait for thread to become active
        while (!m_threadActive)
        {
            Sleep(9);
        }
    }

    m_peers[id] = peer;
    return true;
}

FRDPeerHandler* FRDPeerHandler::s_peerHandler = nullptr;
