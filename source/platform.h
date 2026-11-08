#pragma once
#include <cstdint>
#include <kos.h>
using WORD=uint16_t;using BYTE=uint8_t;using DWORD=uint32_t;using HANDLE=void*;
using ULONGLONG=uint64_t;
union LARGE_INTEGER {long long QuadPart=0;struct{uint32_t LowPart;int32_t HighPart;};};
inline uint64_t GetTickCount64(){return timer_ms_gettime64();}
