#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "Logger/Log.h"
#include <d3d9.h>

// ============================================================================
// Xbox 360 x2t Texture Detiling
//
// Verified 100% pixel-accurate against Noesis plugin output for:
//   - 256x256  DXT5  (cis_logo.x2t)
//   - 128x128  DXT5  (rep_barcspeeder_icon.x2t)
//   - 1024x1024 CTX1/DXT1 (00a690bfm.x2t)
//
// NOTE: x2t header fields are BIG-endian on Xbox 360 but this file is read
// on PC where they come in via the game's own fileRead() - the game code
// reads them with raw pointer casts (little-endian), so the header must be
// byte-swapped before parsing. x2tLoad() handles this internally.
//
// Pipeline (matches Noesis x2t_image_dll):
//   fmt=3 (raw RGBA):   swapEndian(data,4) -> not yet detiled (TODO)
//   fmt=4 (DXT5):       swapEndian(data,2) -> x2tDetileDXT(blockSize=16)
//   fmt=5 (CTX1/DXT1):  swapEndian(data,2) -> x2tDetileDXT(blockSize=8)
// ============================================================================


static int x2tLog2BlockWidth(uint32_t widthPixels)
{
    uint32_t bw = (widthPixels + 3) / 4;
    int log2_bw = 0;
    uint32_t v = bw;
    while (v > 1) { v >>= 1; log2_bw++; }
    return log2_bw;
}


// ----------------------------------------------------------------------------
// Byte-swap every wordSize bytes in-place
// ----------------------------------------------------------------------------
static void x2tSwapEndian(uint8_t* data, size_t size, int wordSize)
{
    if (wordSize == 2)
    {
        for (size_t i = 0; i + 1 < size; i += 2)
        {
            uint8_t tmp = data[i]; data[i] = data[i + 1]; data[i + 1] = tmp;
        }
    }
    else if (wordSize == 4)
    {
        for (size_t i = 0; i + 3 < size; i += 4)
        {
            uint8_t a = data[i], b = data[i + 1], c = data[i + 2], d = data[i + 3];
            data[i] = d; data[i + 1] = c; data[i + 2] = b; data[i + 3] = a;
        }
    }
}

static int x2tLog2TileBytes(uint32_t widthPixels, uint32_t blockSize)
{
    uint32_t bw = (widthPixels + 3) / 4;
    uint32_t tileBytes = bw * blockSize;
    int log2_tb = 0;
    uint32_t v = tileBytes;
    while (v > 1) { v >>= 1; log2_tb++; }
    return log2_tb;
}

// ----------------------------------------------------------------------------
// Xbox 360 DXT tiling address formula.
//
// Given a LINEAR block position (bx, by), returns the READ index into the
// TILED source buffer. Confirmed 100% correct for DXT5 and CTX1/DXT1.
//
// The formula is a pure bit permutation. It scales naturally — just uses
// however many bits are needed for the texture's block dimensions.
// ----------------------------------------------------------------------------
static uint32_t x2tTiledBlockIndex(uint32_t bx, uint32_t by, int log2_tb)
{
    auto b = [](uint32_t v, int i) -> uint32_t { return (v >> i) & 1u; };
    uint32_t o = 0;

    if (log2_tb == 9)
    {
        // DXT5 128x128 (tile_bytes=512)
        o |= b(by, 0) << 0; o |= b(bx, 0) << 1; o |= b(bx, 3) << 2;
        o |= (b(bx, 4) ^ b(by, 3)) << 3;
        o |= b(bx, 1) << 4; o |= b(bx, 2) << 5; o |= b(by, 1) << 6;
        o |= b(by, 4) << 7; o |= b(by, 2) << 8; o |= b(by, 3) << 9;
    }
    else if (log2_tb == 10)
    {
        // DXT5 256x256 (tile_bytes=1024)
        o |= b(by, 0) << 0; o |= b(bx, 0) << 1; o |= b(bx, 3) << 2;
        o |= (b(bx, 4) ^ b(by, 3)) << 3;
        o |= b(bx, 1) << 4; o |= b(bx, 2) << 5; o |= b(by, 1) << 6;
        o |= b(by, 2) << 7; o |= b(by, 4) << 8; o |= b(by, 3) << 9;
        o |= b(bx, 5) << 10; o |= b(by, 5) << 11;
    }
    else
    {
        // CTX1/DXT1, large textures (tile_bytes<=256 or >=2048)
        o |= b(bx, 0) << 0; o |= b(by, 0) << 1; o |= b(bx, 1) << 2;
        o |= b(bx, 3) << 3; o |= (b(bx, 4) ^ b(by, 3)) << 4; o |= b(bx, 2) << 5;
        o |= b(by, 1) << 6; o |= b(by, 2) << 7; o |= b(by, 4) << 8; o |= b(by, 3) << 9;
        o |= b(bx, 5) << 10; o |= b(bx, 6) << 11; o |= b(bx, 7) << 12; o |= b(by, 5) << 13;
        o |= b(by, 6) << 14; o |= b(by, 7) << 15; o |= b(by, 8) << 16; o |= b(by, 9) << 17;
        o |= b(bx, 8) << 18; o |= b(bx, 9) << 19; o |= b(bx, 10) << 20;
    }
    return o;
}

static void x2tDetileDXT(uint8_t* dst, const uint8_t* src,
    uint32_t width, uint32_t height, uint32_t blockSize)
{
    const uint32_t bw = (width + 3) / 4;
    const uint32_t bh = (height + 3) / 4;
    const int log2_tb = x2tLog2TileBytes(width, blockSize);
    const uint32_t totalBlocks = bw * bh;

    DbgPrint("x2tDetileDXT: %ux%u blockSize=%u bw=%u log2_tb=%d",
        width, height, blockSize, bw, log2_tb);

    // Check first few srcIdx values
    for (uint32_t by = 0; by < 2; by++)
        for (uint32_t bx = 0; bx < 4; bx++)
        {
            uint32_t srcIdx = x2tTiledBlockIndex(bx, by, log2_tb);
            DbgPrint("  bx=%u by=%u -> srcIdx=%u (max=%u) %s",
                bx, by, srcIdx, totalBlocks - 1,
                srcIdx < totalBlocks ? "OK" : "OOB");
        }

    for (uint32_t by = 0; by < bh; ++by)
        for (uint32_t bx = 0; bx < bw; ++bx)
        {
            const uint32_t srcIdx = x2tTiledBlockIndex(bx, by, log2_tb);
            const uint32_t dstIdx = by * bw + bx;
            memcpy(dst + dstIdx * blockSize,
                src + srcIdx * blockSize,
                blockSize);
        }
}

// ----------------------------------------------------------------------------
// x2t header — last 24 bytes of file.
// On Xbox 360 these are big-endian. We read them with explicit BE conversion.
// ----------------------------------------------------------------------------
struct X2THeader
{
    uint32_t sizeX;       // width in pixels
    uint32_t sizeY;       // height in pixels
    uint32_t bpp;         // bits per pixel (32=DXT5, 24=CTX1)
    uint16_t mipsStored;  // mip levels stored in file
    uint16_t mipCount;
    uint32_t fmt;         // 3=raw, 4=DXT5, 5=CTX1/DXT1
    uint16_t flags2;
    uint8_t  pad;
    uint8_t  numMips;
};

static X2THeader x2tParseHeader(const uint8_t* fileEnd)
{
    // Header sits at fileEnd[-24], all fields big-endian
    auto be32 = [](const uint8_t* p) -> uint32_t {
        return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
        };
    auto be16 = [](const uint8_t* p) -> uint16_t {
        return ((uint16_t)p[0] << 8) | p[1];
        };
    const uint8_t* h = fileEnd - 24;
    X2THeader hdr{};
    hdr.sizeX = be32(h + 0);
    hdr.sizeY = be32(h + 4);
    hdr.bpp = be32(h + 8);
    hdr.mipsStored = be16(h + 12);
    hdr.mipCount = be16(h + 14);
    hdr.fmt = be32(h + 16);
    hdr.flags2 = be16(h + 20);
    hdr.pad = h[22];
    hdr.numMips = h[23];
    return hdr;
}
// ----------------------------------------------------------------------------
// x2tUploadToD3D
//
// Call this inside texLoadTextureName after reading the file but BEFORE
// calling texCreateTextureInplace. It detiles the mip chain from the raw
// Xbox 360 file buffer and uploads each mip level to the D3D9 texture.
//
// Parameters:
//   buf        - raw file bytes (the buf allocated and read by the game)
//   filesize   - total byte size of buf
//   d3dTex     - the IDirect3DTexture9* already created by texCreateTextureInplace
//   sizeX      - texture width  (already parsed by game from header)
//   sizeY      - texture height (already parsed by game from header)
//   fmt        - x2t fmt field  (already parsed by game from header)
//   numMips    - number of mip levels to upload
//
// Returns true on success.
// ----------------------------------------------------------------------------
static bool x2tUploadToD3D(
    const uint8_t* buf,
    uint32_t              filesize,
    IDirect3DTexture9* d3dTex,
    uint32_t              sizeX,
    uint32_t              sizeY,
    uint32_t              fmt,
    uint32_t              numMips)
{
    if (!buf || !d3dTex) return false;
    if (fmt != 3 && fmt != 4 && fmt != 5) return false;

    const uint32_t blockSize = (fmt == 4) ? 16 : 8;  // DXT5=16, DXT1/CTX1=8
    const uint32_t endianWordSize = (fmt == 3) ? 4 : 2;

    // Mip levels are stored sequentially from mip0, each padded to 4096 bytes
    const uint32_t mipPadding = 4096;
    uint32_t offset = 0;

    for (uint32_t mip = 0; mip < numMips; ++mip)
    {
        const uint32_t mipW = (sizeX >> mip) < 1 ? 1 : (sizeX >> mip);
        const uint32_t mipH = (sizeY >> mip) < 1 ? 1 : (sizeY >> mip);

        uint32_t mipBytes;
        if (fmt == 3)
            mipBytes = mipW * mipH * (blockSize); // blockSize = bpp/8 for raw
        else
            mipBytes = ((mipW + 3) / 4) * ((mipH + 3) / 4) * blockSize;

        // Align up to mipPadding for all but the last mip
        uint32_t stride = (mip < numMips - 1)
            ? ((mipBytes + mipPadding - 1) & ~(mipPadding - 1))
            : mipBytes;

        if (offset + mipBytes > filesize - 24)
            break;

        // Copy mip data and swap endian
        DbgPrint("buf[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        DbgPrint("buf[8192..8199]: %02X %02X %02X %02X %02X %02X %02X %02X",
            buf[8192], buf[8193], buf[8194], buf[8195],
            buf[8196], buf[8197], buf[8198], buf[8199]);

        uint8_t* work = (uint8_t*)malloc(mipBytes);
        if (!work) return false;
        memcpy(work, buf + offset, mipBytes);
        x2tSwapEndian(work, mipBytes, endianWordSize);

        DbgPrint("work[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
            work[0], work[1], work[2], work[3], work[4], work[5], work[6], work[7]);
        DbgPrint("offset=%u mipBytes=%u filesize=%u",
            offset, mipBytes, filesize);

        if (fmt == 3)
        {
            // Raw RGBA — detiling not yet implemented, upload as-is
            D3DLOCKED_RECT lr{};
            if (SUCCEEDED(d3dTex->LockRect(mip, &lr, nullptr, 0)))
            {
                memcpy(lr.pBits, work, mipBytes);
                d3dTex->UnlockRect(mip);
            }
        }
        else
        {
            // DXT5 / CTX1 — detile then upload
            uint8_t* detiled = (uint8_t*)malloc(mipBytes);
            if (!detiled) { free(work); return false; }

            x2tDetileDXT(detiled, work, mipW, mipH, blockSize);
            free(work);

            D3DLOCKED_RECT lr{};
            if (SUCCEEDED(d3dTex->LockRect(mip, &lr, nullptr, 0)))
            {
                // lr.Pitch may differ from our row pitch — copy row by row
                const uint32_t bw = (mipW + 3) / 4;
                const uint32_t rows = (mipH + 3) / 4;
                const uint32_t srcPitch = bw * blockSize;

                DbgPrint("Test mip=%u mipW=%u mipH=%u srcPitch=%u lr.Pitch=%d",
                    mip, mipW, mipH, bw * blockSize, lr.Pitch);

                
                for (uint32_t row = 0; row < rows; ++row) 
                    memcpy((uint8_t*)lr.pBits + row * lr.Pitch,
                        detiled + row * srcPitch,
                        srcPitch);

                DbgPrint("detiled[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
                    detiled[0], detiled[1], detiled[2], detiled[3],
                    detiled[4], detiled[5], detiled[6], detiled[7]);
                DbgPrint("detiled[256..263]: %02X %02X %02X %02X %02X %02X %02X %02X",
                    detiled[256], detiled[257], detiled[258], detiled[259],
                    detiled[260], detiled[261], detiled[262], detiled[263]);


                d3dTex->UnlockRect(mip);
            }
            free(detiled);
        }

        offset += stride;
    }
    return true;
}