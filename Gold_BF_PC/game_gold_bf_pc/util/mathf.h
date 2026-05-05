#pragma once

struct $7B7AD9436E53DE341D9DDF6F0536A99B
{
    float r;
    float g;
    float b;
};

struct $91D1B2149FAC90180ECB9AC277F76009
{
    float x;
    float y;
    float z;
    float w;
};

struct $393C16A032292777F0C3725FFB2C0008
{
    float x;
    float y;
    float z;
};


union vec3_u
{
    float v[3];
    $393C16A032292777F0C3725FFB2C0008 __s1;
};

struct $63056BEC92AC0319FFAE617B1D21B026
{
    vec3_u v3;
    float w;
};

struct $681E0D15C353F796E96C24BCA986BC17
{
    float x;
    float y;
    float z;
    float a;
};

union vec4_u
{
    float v[4];
    $63056BEC92AC0319FFAE617B1D21B026 __s1;
    $681E0D15C353F796E96C24BCA986BC17 __s2;
    $7B7AD9436E53DE341D9DDF6F0536A99B __s3;
};

union $59FDB503F939769A97DB84DD0BF18FC6
{
    $91D1B2149FAC90180ECB9AC277F76009 __s0;
    float v[4];
    unsigned int u[4];
};

struct __vector4
{
    $59FDB503F939769A97DB84DD0BF18FC6 ___u0;
};

union mtx_u
{
    float f[4][4];
    vec4_u v[4];
    __vector4 sm[4];
};