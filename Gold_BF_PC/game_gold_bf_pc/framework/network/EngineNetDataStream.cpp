#include "EngineNetDataStream.h"
#include "engine/string/engstring.h"

void EngineNetDataStream::WriteBits(const unsigned char* data, int numbits)
{
    // Clamp to available bits
    int totalBits = m_bytenum * 8 + m_bitnum + numbits;
    if (totalBits > m_databitsize)
        numbits = m_databitsize - m_bytenum * 8 - m_bitnum;

    int byteCount = (numbits + 7) >> 3;
    if (byteCount <= 0)
        return;

    for (int i = 0; i < byteCount; i++)
    {
        unsigned char src = data[i];

        if (m_bitnum)
        {
            // Mid-byte write -- OR in upper bits
            m_data[m_bytenum] |= src >> m_bitnum;

            // If remaining bits spill into next byte
            int bitsLeft = 8 - m_bitnum;
            if (bitsLeft < numbits)
                m_data[m_bytenum + 1] = src << bitsLeft;
        }
        else
        {
            // Byte-aligned write
            m_data[m_bytenum] = src;
        }

        if (numbits < 8)
        {
            // Partial byte -- advance bit pointer
            m_bitnum += numbits;
            if (m_bitnum >= 8)
            {
                m_bitnum -= 8;
                m_bytenum++;
            }
        }
        else
        {
            // Full byte -- advance byte pointer
            numbits -= 8;
            m_bytenum++;
        }
    }
}

void EngineNetDataStream::ReadBits(unsigned char* data, int numbits)
{
    // Check bounds
    if (m_bytenum * 8 + m_bitnum + numbits > m_databitsize)
        return;

    int byteCount = (numbits + 7) >> 3;
    if (byteCount <= 0)
        return;

    int remainingBits = 8 - numbits;

    for (int i = 0; i < byteCount; i++)
    {
        unsigned char val;

        if (m_bitnum)
        {
            // Mid-byte read -- shift current byte left by bitnum
            val = m_data[m_bytenum] << m_bitnum;

            int bitsLeft = 8 - m_bitnum;
            if (numbits > bitsLeft)
            {
                // Need bits from next byte too
                val = (m_data[m_bytenum + 1] >> bitsLeft) | val;
            }
        }
        else
        {
            // Byte-aligned read
            val = m_data[m_bytenum];
        }

        data[i] = val;

        if (numbits < 8)
        {
            // Mask off unused bits
            data[i] &= (0xFF << remainingBits);

            // Advance bit pointer
            m_bitnum += numbits;
            if (m_bitnum >= 8)
            {
                m_bitnum -= 8;
                m_bytenum++;
            }
        }
        else
        {
            // Full byte consumed
            numbits -= 8;
            remainingBits += 8;
            m_bytenum++;
        }
    }
}

void EngineNetDataStream::Serialise(bool* data)
{
    unsigned char buf[1];

    if (m_read)
    {
        ReadBits(buf, 1);
        // Top bit set = true
        *data = (buf[0] & 0x80) != 0;
    }
    else
    {
        // Pack bool into top bit
        buf[0] = *data ? 0x80 : 0x00;
        WriteBits(buf, 1);
    }
}

void EngineNetDataStream::Serialise(unsigned char* data, unsigned char min, unsigned char max)
{
    // Count bits needed to represent max value
    int numbits = 0;
    unsigned int val = max;
    while (val > 0)
    {
        val >>= 1;
        numbits++;
    }

    unsigned char buf[1];

    if (m_read)
    {
        ReadBits(buf, numbits);
        // Value is packed in MSB, shift down to get actual value
        *data = buf[0] >> (8 - numbits);
    }
    else
    {
        // Pack value into MSB
        buf[0] = *data << (8 - numbits);
        WriteBits(buf, numbits);
    }
}

void EngineNetDataStream::Serialise(unsigned short* data, unsigned short min, unsigned short max)
{
    // Count bits needed to represent max value
    int numbits = 0;
    unsigned int val = max;
    while (val > 0)
    {
        val >>= 1;
        numbits++;
    }

    unsigned char buf[2];

    if (m_read)
    {
        ReadBits(buf, numbits);

        // Reconstruct 16-bit value from 2 bytes (big-endian) then shift down
        unsigned short raw = ((unsigned short)buf[0] << 8) | buf[1];
        *data = raw >> (16 - numbits);
    }
    else
    {
        // Pack value into MSB of 16-bit buffer (big-endian)
        unsigned short shifted = *data << (16 - numbits);
        buf[0] = (unsigned char)(shifted >> 8);   // high byte
        buf[1] = (unsigned char)(shifted & 0xFF); // low byte
        WriteBits(buf, numbits);
    }
}

void EngineNetDataStream::Serialise(unsigned int* data, unsigned int min, unsigned int max)
{
    // Count bits needed to represent range
    int numbits = 0;
    unsigned int range = max - min;
    while (range > 0)
    {
        range >>= 1;
        numbits++;
    }

    unsigned char buf[4];

    if (m_read)
    {
        ReadBits(buf, numbits);

        // Reconstruct 32-bit value from 4 bytes (big-endian) then shift down
        unsigned int raw = ((unsigned int)buf[0] << 24)
            | ((unsigned int)buf[1] << 16)
            | ((unsigned int)buf[2] << 8)
            | (unsigned int)buf[3];

        *data = (raw >> (32 - numbits)) + min;
    }
    else
    {
        // Offset by min, pack into MSB of 32-bit buffer (big-endian)
        unsigned int shifted = (*data - min) << (32 - numbits);

        buf[0] = (unsigned char)(shifted >> 24);
        buf[1] = (unsigned char)(shifted >> 16);
        buf[2] = (unsigned char)(shifted >> 8);
        buf[3] = (unsigned char)(shifted >> 0);

        WriteBits(buf, numbits);
    }
}

void EngineNetDataStream::Serialise(unsigned long long* data, unsigned long long min, unsigned long long max)
{
    // Count bits needed to represent a full 64-bit value
    // (always uses 64 bits -- ignores min/max range, counts from 0xFFFFFFFF down)
    int numbits = 0;
    unsigned long long val = 0xFFFFFFFFULL;
    while (val > 0)
    {
        val >>= 1;
        numbits++;
    }

    unsigned char buf[8];

    if (m_read)
    {
        ReadBits(buf, numbits);

        // Reconstruct 64-bit value from 8 bytes (big-endian) then shift down
        unsigned long long raw = ((unsigned long long)buf[0] << 56)
            | ((unsigned long long)buf[1] << 48)
            | ((unsigned long long)buf[2] << 40)
            | ((unsigned long long)buf[3] << 32)
            | ((unsigned long long)buf[4] << 24)
            | ((unsigned long long)buf[5] << 16)
            | ((unsigned long long)buf[6] << 8)
            | (unsigned long long)buf[7];

        *data = raw >> (64 - numbits);
    }
    else
    {
        // Pack value into MSB of 64-bit buffer (big-endian)
        unsigned long long shifted = *data << (64 - numbits);

        buf[0] = (unsigned char)(shifted >> 56);
        buf[1] = (unsigned char)(shifted >> 48);
        buf[2] = (unsigned char)(shifted >> 40);
        buf[3] = (unsigned char)(shifted >> 32);
        buf[4] = (unsigned char)(shifted >> 24);
        buf[5] = (unsigned char)(shifted >> 16);
        buf[6] = (unsigned char)(shifted >> 8);
        buf[7] = (unsigned char)(shifted >> 0);

        WriteBits(buf, numbits);
    }
}

void EngineNetDataStream::Serialise(double* data)
{
    unsigned int buf = 0;

    if (!m_read)
    {
        // Convert double to float, store as raw uint bits for serialisation
        float f = (float)*data;
        memcpy(&buf, &f, sizeof(float));
    }

    Serialise(&buf, 0, 0xFFFFFFFF);

    if (m_read)
    {
        // Convert raw uint bits back to float then to double
        float f;
        memcpy(&f, &buf, sizeof(float));
        *data = (double)f;
    }
}

void EngineNetDataStream::Serialise(char16_s* data)
{
    unsigned int length = 0;

    if (m_read)
    {
        // Read length first (0-16)
        Serialise(&length, 0, 0x10);

        // Clear destination and read string bytes
        memset(data, 0, 0x10);
        ReadBits((unsigned char*)data, length * 8);
    }
    else
    {
        // Find string length (up to 16 chars)
        for (length = 0; length < 16; length++)
        {
            if (!data->chars[length])
                break;
        }

        // Write length then string bytes
        Serialise(&length, 0, 0x10);
        WriteBits((const unsigned char*)data, length * 8);
    }
}

void EngineNetDataStream::SerialiseBuffer(char* src, unsigned int size)
{
    // If mid-byte, align to next byte boundary first
    if (m_bitnum)
    {
        m_bytenum++;
        m_bitnum = 0;
    }

    if (m_read)
    {
        // Copy from stream into src
        memcpy(src, &m_data[m_bytenum], size);
    }
    else
    {
        // Copy from src into stream
        memcpy(&m_data[m_bytenum], src, size);
    }

    m_bytenum += size;
}

void EngineNetDataStream::SerialiseUniCharString(char* string, int maxlen)
{
    unsigned int length = 0;

    if (!m_read)
    {
        // Count string length
        while (string[length])
            length++;
    }

    // Serialise length (0 to maxlen-1)
    Serialise(&length, 0, (unsigned int)(maxlen - 1));

    // Serialise each character as a 16-bit value
    for (int i = 0; i < (int)length; i++)
        Serialise((unsigned short*)&string[i], 0, 0xFFFF);

    // Null terminate on read
    if (m_read)
        string[length] = 0;
}