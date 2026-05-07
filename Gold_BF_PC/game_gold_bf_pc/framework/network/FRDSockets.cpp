#include "FRDSockets.h"
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

int FRDSockets::SendTo(FRDSocket* socket, const char* data, int length, unsigned int binaryAddress, unsigned short port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = binaryAddress;

    int result;
    do
    {
        result = sendto(socket->s, data, length, 0, (sockaddr*)&addr, sizeof(addr));
    } while (result == 0);

    if (result == -1)
    {
        int err = WSAGetLastError();

        // WSAEWOULDBLOCK (10035) = non-fatal, treat as success
        if (err == WSAEWOULDBLOCK)
            return 1;

        // WSAECONNRESET (10054) = fatal error
        if (err == WSAECONNRESET)
            return -1;

        return 1;
    }

    return 1;
}

int FRDSockets::RecvFrom(FRDSocket* socket, char* data, unsigned int* binaryAddress, unsigned int* port)
{
    // Set up select with 1 second timeout
    SOCKET s = socket->s;

    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set readfds;
    readfds.fd_count = 1;
    readfds.fd_array[0] = s;

    int selectResult = select((int)s + 1, &readfds, nullptr, nullptr, &timeout);

    if (selectResult == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
            return -1;
        return -1;
    }

    if (selectResult <= 0)
        return 0;

    // Data available, receive it
    sockaddr_in from;
    int fromLen = sizeof(from);
    from.sin_family = AF_INET;

    int received = recvfrom(
        s,
        data,
        0x4F0,  // 1264 bytes max
        0,
        (sockaddr*)&from,
        &fromLen);

    if (received == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
            return -1;
        return -1;
    }

    if (received < 0)
        return 0;

    *binaryAddress = from.sin_addr.s_addr;
    *port = from.sin_port;

    return received;
}

BOOL CheckSocketError()
{
    int err = WSAGetLastError();

    if (err == WSAEWOULDBLOCK)  // 10035 - non-fatal
        return FALSE;

    if (err == WSAECONNRESET)   // 10054 - non-fatal
        return FALSE;

    return TRUE;
}

void FRDSockets::CreateBoundSocket(FRDSocket* sock, unsigned short port, bool online)
{
    // Store protocol type -- online uses a different protocol
    sock->s = online ? 1 : 0;

    // Create UDP socket -- offline uses UDP (17), online uses different protocol (0xFE)
    int protocol = online ? 0xFE : IPPROTO_UDP;
    SOCKET s = socket(AF_INET, SOCK_DGRAM, protocol);
    sock->s = (int)s;

    if (s == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
            goto fail;
        goto fail;
    }

    // Set SO_REUSEADDR
    {
        int optval = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
        {
            CheckSocketError();
            goto fail;
        }
    }

    // Set non-blocking mode (FIONBIO)
    {
        u_long nonblocking = 1;
        if (ioctlsocket(s, FIONBIO, &nonblocking) == SOCKET_ERROR)
        {
            CheckSocketError();
            goto fail;
        }
    }

    // Set SO_BROADCAST for offline (LAN) sockets
    if (!online)
    {
        int optval = 1;
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
        {
            CheckSocketError();
            goto fail;
        }
    }

    // Bind to port
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = port;
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            CheckSocketError();
            goto fail;
        }
    }

    return;

fail:
    sock->s = -1;
}