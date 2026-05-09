#include "FRDSockets.h"

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

/*
The devil went down to Georgia, he was lookin' for a soul to steal
He was in a bind 'cause he was way behind
And he was willin' to make a deal
When he came across this young man sawin' on a fiddle and playin' it hot
And the devil jumped up on a hickory stump
And said, "boy, let me tell you what"
"I guess you didn't know it but I'm a fiddle player too
And if you'd care to take a dare, I'll make a bet with you
Now you play pretty good fiddle, boy
But give the devil his due
I'll bet a fiddle of gold against your soul
'Cause I think I'm better than you"
*/

void FRDSockets::CreateBoundSocket(FRDSocket* sock, unsigned short port, bool online)
{
    //sock->protocol = (FRDSocket::Protocol)(online ? 1 : 0);

    //Xbox seems to always use the VDP PATH
    sock->protocol = FRDSocket::PROTOCOL_VDP;

    // On PC always use standard UDP -- 0xFE is Xbox 360 VDP only
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sock->s = (int)s;

    if (s == INVALID_SOCKET)
    {
        sock->s = -1;
        return;
    }

    // Set SO_REUSEADDR
    {
        int optval = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
            goto fail;
    }

    // Set non-blocking mode
    {
        u_long nonblocking = 1;
        if (ioctlsocket(s, FIONBIO, &nonblocking) == SOCKET_ERROR)
            goto fail;
    }

    // SO_BROADCAST for LAN
    if (!online)
    {
        int optval = 1;
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR)
            goto fail;
    }

    // Bind to port
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);  // also fix -- port needs htons()
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
            goto fail;
    }

    return;

fail:
    closesocket(s);
    sock->s = -1;
}

bool FRDSockets::m_started = false;
FRDSockets* FRDSockets::m_instance = nullptr;

WSAData FRDSockets::m_winsockinfo = { 0u, 0u, "", "", 0u, 0u, NULL }; // idb