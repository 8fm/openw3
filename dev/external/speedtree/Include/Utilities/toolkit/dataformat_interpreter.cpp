/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "toolkit.h"
#include "dataformat_interpreter.h"
#include "floating_point.h"
#include <algorithm>
using namespace sce;

struct SurfaceFormatInfo
{
	Gnm::SurfaceFormat m_format;
	uint8_t m_channels;
	uint8_t m_bitsPerElement;
	uint8_t m_bits[4];
	void (*m_encoder)(const SurfaceFormatInfo *restrict info, uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat);
	void (*m_decoder)(const SurfaceFormatInfo *restrict info, Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict src, const Gnm::DataFormat dataFormat);
	uint8_t m_offset[4];
	double m_ooMaxUnormValue[4]; 
	double m_ooMaxSnormValue[4]; 
	inline uint32_t maxUnormValue(uint32_t channel) const {return (uint64_t(1) << (m_bits[channel]-0)) - 1;}
	inline uint32_t maxSnormValue(uint32_t channel) const {return (uint64_t(1) << (m_bits[channel]-1)) - 1;}
	inline  int32_t minSnormValue(uint32_t channel) const {return -maxSnormValue(channel) - 1;}
};

struct FloatInfo
{
	uint8_t m_signBits;
	uint8_t m_exponentBits;
	uint8_t m_mantissaBits;
};

static const FloatInfo floatInfo[] =
{
	{}, // 0
	{}, // 1
	{}, // 2
	{}, // 3
	{}, // 4
	{}, // 5
	{}, // 6
	{}, // 7
	{}, // 8
	{}, // 9
	{0, 5, 5}, // 10
	{0, 5, 6}, // 11
	{}, // 12
	{}, // 13
	{}, // 14
	{}, // 15
	{1, 5, 10}, // 16
	{}, // 17
	{}, // 18
	{}, // 19
	{}, // 20
	{}, // 21
	{}, // 22
	{}, // 23
	{}, // 24
	{}, // 25
	{}, // 26
	{}, // 27
	{}, // 28
	{}, // 29
	{}, // 30
	{}, // 31
	{1, 8, 23}, // 32
};

namespace
{
	inline double linear2sRgb(double linear)
	{
		return (linear <= 0.00313066844250063) ? linear * 12.92 : 1.055 * pow(linear, 1 / 2.4) - 0.055;
	}

	inline double sRgb2Linear(double sRgb)
	{
		return (sRgb <= 0.0404482362771082) ? sRgb / 12.92 : pow((sRgb + 0.055) / 1.055, 2.4);
	}

	inline double clamp(double a, double lo, double hi)
	{
		return fmax(lo, fmin(hi, a));
	}

	inline uint32_t decodeU(uint32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // zero-extend a <=32 unsigned integer to 32 bits.
	{
		SCE_GNM_UNUSED(info);
		SCE_GNM_UNUSED(channel);
		return value;
	}

	inline int32_t decodeS(uint32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // sign-extend a <=32 signed integer to 32 bits.
	{
		if(value & info->minSnormValue(channel)) // if the sign bit is on
			value |= ~info->maxUnormValue(channel); // extend it to the top of the integer
		return value;
	}

	inline int32_t decodeUB(uint32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // bias a <=32 unsigned integer to signed, then sign-extend to 32 bits.
	{
		return value + info->minSnormValue(channel);
	}

	inline uint32_t encodeU(uint32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // encode a 32 bit unsigned integer to <=32 bits.
	{
		return value & info->maxUnormValue(channel);
	}

	inline uint32_t encodeS(int32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // encode a 32 bit signed integer to <=32 bits.
	{
		return value & info->maxUnormValue(channel);
	}

	inline uint32_t encodeUB(int32_t value, const SurfaceFormatInfo * restrict info, uint32_t channel) // encode a 32 bit signed integer to <=32 bits, then bias to unsigned.
	{
		return (value - info->minSnormValue(channel)) & info->maxUnormValue(channel);
	}

	inline int32_t roundDoubleToInt(double value)
	{
		return static_cast<int32_t>(floor(value + 0.5));
	}

	inline uint32_t roundDoubleToUint(double value)
	{
		return static_cast<uint32_t>(floor(value + 0.5));
	}

	void encodeTextureChannelSharedExponent(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict source, const SurfaceFormatInfo * restrict info)
	{
		Gnmx::Toolkit::Reg32 temp[3] = {source[0], source[1], source[2]};
		const int32_t sharedExponent = std::max(temp[0].bits.m_exponent, std::max(temp[1].bits.m_exponent, temp[2].bits.m_exponent)) - 127;
		temp[0].bits.m_exponent = temp[0].bits.m_exponent ? std::max(0, std::min(255, temp[0].bits.m_exponent - 127 + (info->m_bits[0] - 1) - sharedExponent + 127)) : 0;
		temp[1].bits.m_exponent = temp[1].bits.m_exponent ? std::max(0, std::min(255, temp[1].bits.m_exponent - 127 + (info->m_bits[1] - 1) - sharedExponent + 127)) : 0;
		temp[2].bits.m_exponent = temp[2].bits.m_exponent ? std::max(0, std::min(255, temp[2].bits.m_exponent - 127 + (info->m_bits[2] - 1) - sharedExponent + 127)) : 0;
		dest[0] = std::max(int32_t(0), std::min(int32_t(info->maxUnormValue(0)), roundDoubleToInt(temp[0].f)));
		dest[1] = std::max(int32_t(0), std::min(int32_t(info->maxUnormValue(1)), roundDoubleToInt(temp[1].f)));
		dest[2] = std::max(int32_t(0), std::min(int32_t(info->maxUnormValue(2)), roundDoubleToInt(temp[2].f)));
		const int32_t minExponent = -info->maxSnormValue(3);
		const int32_t maxExponent = info->maxSnormValue(3) + 1;
		dest[3] = std::max(minExponent, std::min(maxExponent, sharedExponent)) - minExponent;
	}

	void decodeTextureChannelSharedExponent(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict source, const SurfaceFormatInfo * restrict info)
	{
		const int32_t sharedExponent = source[3] - info->maxSnormValue(3);
		dest[0].f = source[0] * pow(2, sharedExponent - info->m_bits[0] + 1);
		dest[1].f = source[1] * pow(2, sharedExponent - info->m_bits[1] + 1);
		dest[2].f = source[2] * pow(2, sharedExponent - info->m_bits[2] + 1);
		dest[3].f = 0.f;
	}

	void decodeTextureChannelsUNorm(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = decodeU(value[0], info, 0) * info->m_ooMaxUnormValue[0];
		dest[1].f = decodeU(value[1], info, 1) * info->m_ooMaxUnormValue[1];
		dest[2].f = decodeU(value[2], info, 2) * info->m_ooMaxUnormValue[2];
		dest[3].f = decodeU(value[3], info, 3) * info->m_ooMaxUnormValue[3];
	}

	void decodeTextureChannelsSNorm(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = fmax(-1.0, decodeS(value[0], info, 0) * info->m_ooMaxSnormValue[0]);
		dest[1].f = fmax(-1.0, decodeS(value[1], info, 1) * info->m_ooMaxSnormValue[1]);
		dest[2].f = fmax(-1.0, decodeS(value[2], info, 2) * info->m_ooMaxSnormValue[2]);
		dest[3].f = fmax(-1.0, decodeS(value[3], info, 3) * info->m_ooMaxSnormValue[3]);
	}

	void decodeTextureChannelsUScaled(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = decodeU(value[0], info, 0);
		dest[1].f = decodeU(value[1], info, 1);
		dest[2].f = decodeU(value[2], info, 2);
		dest[3].f = decodeU(value[3], info, 3);
	}

	void decodeTextureChannelsSScaled(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = decodeS(value[0], info, 0);
		dest[1].f = decodeS(value[1], info, 1);
		dest[2].f = decodeS(value[2], info, 2);
		dest[3].f = decodeS(value[3], info, 3);
	}

	void decodeTextureChannelsUInt(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].u = decodeU(value[0], info, 0);
		dest[1].u = decodeU(value[1], info, 1);
		dest[2].u = decodeU(value[2], info, 2);
		dest[3].u = decodeU(value[3], info, 3);
	}

	void decodeTextureChannelsSInt(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].i = decodeS(value[0], info, 0);
		dest[1].i = decodeS(value[1], info, 1);
		dest[2].i = decodeS(value[2], info, 2);
		dest[3].i = decodeS(value[3], info, 3);
	}

	void decodeTextureChannelsSNormNoZero(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = ((decodeS(value[0], info, 0) - info->minSnormValue(0)) * info->m_ooMaxUnormValue[0]) * 2.0 - 1.0;
		dest[1].f = ((decodeS(value[1], info, 1) - info->minSnormValue(1)) * info->m_ooMaxUnormValue[1]) * 2.0 - 1.0;
		dest[2].f = ((decodeS(value[2], info, 2) - info->minSnormValue(2)) * info->m_ooMaxUnormValue[2]) * 2.0 - 1.0;
		dest[3].f = ((decodeS(value[3], info, 3) - info->minSnormValue(3)) * info->m_ooMaxUnormValue[3]) * 2.0 - 1.0;
	}

	void decodeTextureChannelsFloat(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = Gnmx::Toolkit::unpackFloat(value[0], floatInfo[info->m_bits[0]].m_signBits, floatInfo[info->m_bits[0]].m_exponentBits, floatInfo[info->m_bits[0]].m_mantissaBits);
		dest[1].f = Gnmx::Toolkit::unpackFloat(value[1], floatInfo[info->m_bits[1]].m_signBits, floatInfo[info->m_bits[1]].m_exponentBits, floatInfo[info->m_bits[1]].m_mantissaBits);
		dest[2].f = Gnmx::Toolkit::unpackFloat(value[2], floatInfo[info->m_bits[2]].m_signBits, floatInfo[info->m_bits[2]].m_exponentBits, floatInfo[info->m_bits[2]].m_mantissaBits);
		dest[3].f = Gnmx::Toolkit::unpackFloat(value[3], floatInfo[info->m_bits[3]].m_signBits, floatInfo[info->m_bits[3]].m_exponentBits, floatInfo[info->m_bits[3]].m_mantissaBits);
	}

	void decodeTextureChannelsSrgb(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = sRgb2Linear(decodeU(value[0], info, 0) * info->m_ooMaxUnormValue[0]);
		dest[1].f = sRgb2Linear(decodeU(value[1], info, 1) * info->m_ooMaxUnormValue[1]);
		dest[2].f = sRgb2Linear(decodeU(value[2], info, 2) * info->m_ooMaxUnormValue[2]);
		dest[3].f =             decodeU(value[3], info, 3) * info->m_ooMaxUnormValue[3];
	}

	void decodeTextureChannelsUBNorm(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = fmax(-1.0, decodeUB(value[0], info, 0) * info->m_ooMaxSnormValue[0]);
		dest[1].f = fmax(-1.0, decodeUB(value[1], info, 1) * info->m_ooMaxSnormValue[1]);
		dest[2].f = fmax(-1.0, decodeUB(value[2], info, 2) * info->m_ooMaxSnormValue[2]);
		dest[3].f = fmax(-1.0, decodeUB(value[3], info, 3) * info->m_ooMaxSnormValue[3]);
	}

	void decodeTextureChannelsUBNormNoZero(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = ((decodeUB(value[0], info, 0) - info->minSnormValue(0)) * info->m_ooMaxUnormValue[0]) * 2.0 - 1.0;
		dest[1].f = ((decodeUB(value[1], info, 1) - info->minSnormValue(1)) * info->m_ooMaxUnormValue[1]) * 2.0 - 1.0;
		dest[2].f = ((decodeUB(value[2], info, 2) - info->minSnormValue(2)) * info->m_ooMaxUnormValue[2]) * 2.0 - 1.0;
		dest[3].f = ((decodeUB(value[3], info, 3) - info->minSnormValue(3)) * info->m_ooMaxUnormValue[3]) * 2.0 - 1.0;
	}

	void decodeTextureChannelsUBInt(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].i = decodeUB(value[0], info, 0);
		dest[1].i = decodeUB(value[1], info, 1);
		dest[2].i = decodeUB(value[2], info, 2);
		dest[3].i = decodeUB(value[3], info, 3);
	}

	void decodeTextureChannelsUBScaled(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info)
	{
		dest[0].f = decodeUB(value[0], info, 0);
		dest[1].f = decodeUB(value[1], info, 1);
		dest[2].f = decodeUB(value[2], info, 2);
		dest[3].f = decodeUB(value[3], info, 3);
	}
}

typedef void (*DecoderFunction)(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo *restrict info);
static const DecoderFunction decoderFunction[] =
{
	decodeTextureChannelsUNorm,       
	decodeTextureChannelsSNorm,       
	decodeTextureChannelsUScaled,     
	decodeTextureChannelsSScaled,     
	decodeTextureChannelsUInt,        
	decodeTextureChannelsSInt,        
	decodeTextureChannelsSNormNoZero, 
	decodeTextureChannelsFloat,       
	0,
	decodeTextureChannelsSrgb,        
	decodeTextureChannelsUBNorm,      
	decodeTextureChannelsUBNormNoZero,
	decodeTextureChannelsUBInt,       
	decodeTextureChannelsUBScaled,    
};

namespace 
{
	void decodeTextureChannels(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict value, const SurfaceFormatInfo * restrict info, const Gnm::TextureChannelType textureChannelType)
	{
		SCE_GNM_ASSERT(textureChannelType < sizeof(decoderFunction)/sizeof(decoderFunction[0]));
		SCE_GNM_ASSERT(decoderFunction[textureChannelType] != 0);
		decoderFunction[textureChannelType](dest, value, info);
	}

	void encodeTextureChannelsUNorm(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeU(roundDoubleToUint(clamp((double)value[0].f, 0.0, 1.0) * info->maxUnormValue(0)), info, 0);
		dest[1] = encodeU(roundDoubleToUint(clamp((double)value[1].f, 0.0, 1.0) * info->maxUnormValue(1)), info, 1);
		dest[2] = encodeU(roundDoubleToUint(clamp((double)value[2].f, 0.0, 1.0) * info->maxUnormValue(2)), info, 2);
		dest[3] = encodeU(roundDoubleToUint(clamp((double)value[3].f, 0.0, 1.0) * info->maxUnormValue(3)), info, 3);
	}

	void encodeTextureChannelsSNorm(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeS(roundDoubleToInt(clamp((double)value[0].f, -1.0, 1.0) * info->maxSnormValue(0)), info, 0);
		dest[1] = encodeS(roundDoubleToInt(clamp((double)value[1].f, -1.0, 1.0) * info->maxSnormValue(1)), info, 1);
		dest[2] = encodeS(roundDoubleToInt(clamp((double)value[2].f, -1.0, 1.0) * info->maxSnormValue(2)), info, 2);
		dest[3] = encodeS(roundDoubleToInt(clamp((double)value[3].f, -1.0, 1.0) * info->maxSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsUScaled(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeU(roundDoubleToUint(clamp((double)value[0].f, 0.0, (double)info->maxUnormValue(0))), info, 0);
		dest[1] = encodeU(roundDoubleToUint(clamp((double)value[1].f, 0.0, (double)info->maxUnormValue(1))), info, 1);
		dest[2] = encodeU(roundDoubleToUint(clamp((double)value[2].f, 0.0, (double)info->maxUnormValue(2))), info, 2);
		dest[3] = encodeU(roundDoubleToUint(clamp((double)value[3].f, 0.0, (double)info->maxUnormValue(3))), info, 3);
	}

	void encodeTextureChannelsSScaled(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeS(roundDoubleToInt(clamp((double)value[0].f, (double)info->minSnormValue(0), (double)info->maxSnormValue(0))), info, 0);
		dest[1] = encodeS(roundDoubleToInt(clamp((double)value[1].f, (double)info->minSnormValue(1), (double)info->maxSnormValue(1))), info, 1);
		dest[2] = encodeS(roundDoubleToInt(clamp((double)value[2].f, (double)info->minSnormValue(2), (double)info->maxSnormValue(2))), info, 2);
		dest[3] = encodeS(roundDoubleToInt(clamp((double)value[3].f, (double)info->minSnormValue(3), (double)info->maxSnormValue(3))), info, 3);
	}

	void encodeTextureChannelsUInt(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeU(std::min(value[0].u, info->maxUnormValue(0)), info, 0);
		dest[1] = encodeU(std::min(value[1].u, info->maxUnormValue(1)), info, 1);
		dest[2] = encodeU(std::min(value[2].u, info->maxUnormValue(2)), info, 2);
		dest[3] = encodeU(std::min(value[3].u, info->maxUnormValue(3)), info, 3);
	}

	void encodeTextureChannelsSInt(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeS(clamp((double)value[0].i, (double)info->minSnormValue(0), (double)info->maxSnormValue(0)), info, 0);
		dest[1] = encodeS(clamp((double)value[1].i, (double)info->minSnormValue(1), (double)info->maxSnormValue(1)), info, 1);
		dest[2] = encodeS(clamp((double)value[2].i, (double)info->minSnormValue(2), (double)info->maxSnormValue(2)), info, 2);
		dest[3] = encodeS(clamp((double)value[3].i, (double)info->minSnormValue(3), (double)info->maxSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsSNormNoZero(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeS(roundDoubleToInt((clamp((double)value[0].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(0) + info->minSnormValue(0)), info, 0);
		dest[1] = encodeS(roundDoubleToInt((clamp((double)value[1].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(1) + info->minSnormValue(1)), info, 1);
		dest[2] = encodeS(roundDoubleToInt((clamp((double)value[2].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(2) + info->minSnormValue(2)), info, 2);
		dest[3] = encodeS(roundDoubleToInt((clamp((double)value[3].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(3) + info->minSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsFloat(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = Gnmx::Toolkit::packFloat(value[0].f, floatInfo[info->m_bits[0]].m_signBits, floatInfo[info->m_bits[0]].m_exponentBits, floatInfo[info->m_bits[0]].m_mantissaBits);
		dest[1] = Gnmx::Toolkit::packFloat(value[1].f, floatInfo[info->m_bits[1]].m_signBits, floatInfo[info->m_bits[1]].m_exponentBits, floatInfo[info->m_bits[1]].m_mantissaBits);
		dest[2] = Gnmx::Toolkit::packFloat(value[2].f, floatInfo[info->m_bits[2]].m_signBits, floatInfo[info->m_bits[2]].m_exponentBits, floatInfo[info->m_bits[2]].m_mantissaBits);
		dest[3] = Gnmx::Toolkit::packFloat(value[3].f, floatInfo[info->m_bits[3]].m_signBits, floatInfo[info->m_bits[3]].m_exponentBits, floatInfo[info->m_bits[3]].m_mantissaBits);
	}

	void encodeTextureChannelsSrgb(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeU(roundDoubleToUint(linear2sRgb(clamp((double)value[0].f, 0.0, 1.0)) * info->maxUnormValue(0)), info, 0);
		dest[1] = encodeU(roundDoubleToUint(linear2sRgb(clamp((double)value[1].f, 0.0, 1.0)) * info->maxUnormValue(1)), info, 1);
		dest[2] = encodeU(roundDoubleToUint(linear2sRgb(clamp((double)value[2].f, 0.0, 1.0)) * info->maxUnormValue(2)), info, 2);
		dest[3] = encodeU(roundDoubleToUint(            clamp((double)value[3].f, 0.0, 1.0)  * info->maxUnormValue(3)), info, 3);
	}

	void encodeTextureChannelsUBNorm(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeUB(roundDoubleToInt(clamp((double)value[0].f, -1.0, 1.0) * info->maxSnormValue(0)), info, 0);
		dest[1] = encodeUB(roundDoubleToInt(clamp((double)value[1].f, -1.0, 1.0) * info->maxSnormValue(1)), info, 1);
		dest[2] = encodeUB(roundDoubleToInt(clamp((double)value[2].f, -1.0, 1.0) * info->maxSnormValue(2)), info, 2);
		dest[3] = encodeUB(roundDoubleToInt(clamp((double)value[3].f, -1.0, 1.0) * info->maxSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsUBNormNoZero(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeUB(roundDoubleToInt((clamp((double)value[0].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(0) + info->minSnormValue(0)), info, 0);
		dest[1] = encodeUB(roundDoubleToInt((clamp((double)value[1].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(1) + info->minSnormValue(1)), info, 1);
		dest[2] = encodeUB(roundDoubleToInt((clamp((double)value[2].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(2) + info->minSnormValue(2)), info, 2);
		dest[3] = encodeUB(roundDoubleToInt((clamp((double)value[3].f, -1.0, 1.0) * 0.5 + 0.5) * info->maxUnormValue(3) + info->minSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsUBInt(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeUB(clamp((double)value[0].i, (double)info->minSnormValue(0), (double)info->maxSnormValue(0)), info, 0);
		dest[1] = encodeUB(clamp((double)value[1].i, (double)info->minSnormValue(1), (double)info->maxSnormValue(1)), info, 1);
		dest[2] = encodeUB(clamp((double)value[2].i, (double)info->minSnormValue(2), (double)info->maxSnormValue(2)), info, 2);
		dest[3] = encodeUB(clamp((double)value[3].i, (double)info->minSnormValue(3), (double)info->maxSnormValue(3)), info, 3);
	}

	void encodeTextureChannelsUBScaled(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = encodeUB(roundDoubleToInt(clamp((double)value[0].f, (double)info->minSnormValue(0), (double)info->maxSnormValue(0))), info, 0);
		dest[1] = encodeUB(roundDoubleToInt(clamp((double)value[1].f, (double)info->minSnormValue(1), (double)info->maxSnormValue(1))), info, 1);
		dest[2] = encodeUB(roundDoubleToInt(clamp((double)value[2].f, (double)info->minSnormValue(2), (double)info->maxSnormValue(2))), info, 2);
		dest[3] = encodeUB(roundDoubleToInt(clamp((double)value[3].f, (double)info->minSnormValue(3), (double)info->maxSnormValue(3))), info, 3);
	}
}

typedef void (*EncoderFunction)(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info);
static const EncoderFunction encoderFunction[] =
{
	encodeTextureChannelsUNorm,       
	encodeTextureChannelsSNorm,       
	encodeTextureChannelsUScaled,     
	encodeTextureChannelsSScaled,     
	encodeTextureChannelsUInt,        
	encodeTextureChannelsSInt,        
	encodeTextureChannelsSNormNoZero, 
	encodeTextureChannelsFloat,       
	0,
	encodeTextureChannelsSrgb,        
	encodeTextureChannelsUBNorm,      
	encodeTextureChannelsUBNormNoZero,
	encodeTextureChannelsUBInt,       
	encodeTextureChannelsUBScaled,    
};

namespace 
{
	void encodeTextureChannels(uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict value, const SurfaceFormatInfo *restrict info, const Gnm::TextureChannelType textureChannelType)
	{
		SCE_GNM_ASSERT(textureChannelType < sizeof(encoderFunction)/sizeof(encoderFunction[0]));
		SCE_GNM_ASSERT(encoderFunction[textureChannelType] != 0);
		encoderFunction[textureChannelType](dest, value, info);
	}

	void swizzleRegs(Gnmx::Toolkit::Reg32 *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat)
	{
		const float swizzle[] = {0.f, 1.f, 0.f, 0.f, src[0].f, src[1].f, src[2].f, src[3].f};
		dest[0].f = swizzle[dataFormat.getChannel(0)];
		dest[1].f = swizzle[dataFormat.getChannel(1)];
		dest[2].f = swizzle[dataFormat.getChannel(2)];
		dest[3].f = swizzle[dataFormat.getChannel(3)];
	}

	void packBitfields(uint32_t *restrict dest, const uint32_t *restrict src, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = dest[1] = dest[2] = dest[3] = 0;
		dest[info->m_offset[0] / 32] |= src[0] << (info->m_offset[0] % 32);
		dest[info->m_offset[1] / 32] |= src[1] << (info->m_offset[1] % 32);
		dest[info->m_offset[2] / 32] |= src[2] << (info->m_offset[2] % 32);
		dest[info->m_offset[3] / 32] |= src[3] << (info->m_offset[3] % 32);
	}

	void unpackBitfields(uint32_t *restrict dest, const uint32_t *restrict src, const SurfaceFormatInfo *restrict info)
	{
		dest[0] = (src[info->m_offset[0] / 32] >> (info->m_offset[0] % 32)) & info->maxUnormValue(0);
		dest[1] = (src[info->m_offset[1] / 32] >> (info->m_offset[1] % 32)) & info->maxUnormValue(1);
		dest[2] = (src[info->m_offset[2] / 32] >> (info->m_offset[2] % 32)) & info->maxUnormValue(2);
		dest[3] = (src[info->m_offset[3] / 32] >> (info->m_offset[3] % 32)) & info->maxUnormValue(3);
	}

	void broadcastElement(uint32_t *restrict dest, const SurfaceFormatInfo *restrict info)
	{
		switch(info->m_bitsPerElement)
		{
		case 8:
			dest[0] |= dest[0] << 8;
		case 16:
			dest[0] |= dest[0] << 16;
		default:
			break;
		}
	}

	void simpleEncoder(const SurfaceFormatInfo *restrict info, uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat)
	{
		Gnmx::Toolkit::Reg32 tempSrc[4];
		uint32_t tempDest[4];

		swizzleRegs(tempSrc, src, dataFormat);
		encodeTextureChannels(tempDest, tempSrc, info, dataFormat.getTextureChannelType());
		packBitfields(dest, tempDest, info);
		broadcastElement(dest, info);
	}

	void simpleDecoder(const SurfaceFormatInfo *restrict info, Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *src, const Gnm::DataFormat dataFormat)
	{
		uint32_t tempSrc[4];
		Gnmx::Toolkit::Reg32 tempDest[4];

		unpackBitfields(tempSrc, src, info);
		decodeTextureChannels(tempDest, tempSrc, info, dataFormat.getTextureChannelType());
		swizzleRegs(dest, tempDest, dataFormat);
	}

	void sharedExponentEncoder(const SurfaceFormatInfo *restrict info, uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat)
	{
		Gnmx::Toolkit::Reg32 tempSrc[4];
		uint32_t tempDest[4];

		swizzleRegs(tempSrc, src, dataFormat);
		encodeTextureChannelSharedExponent(tempDest, tempSrc, info);
		packBitfields(dest, tempDest, info);
		broadcastElement(dest, info);
	}

	void sharedExponentDecoder(const SurfaceFormatInfo *restrict info, Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *src, const Gnm::DataFormat dataFormat)
	{
		uint32_t tempSrc[4];
		Gnmx::Toolkit::Reg32 tempDest[4];

		unpackBitfields(tempSrc, src, info);
		decodeTextureChannelSharedExponent(tempDest, tempSrc, info);
		swizzleRegs(dest, tempDest, dataFormat);
	}

	void sharedChromaEncoder(const SurfaceFormatInfo *restrict info, uint32_t *restrict dest, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat)
	{
		Gnmx::Toolkit::Reg32 tempSrc[4];
		uint32_t tempDest[4];

		swizzleRegs(tempSrc, src, dataFormat);
		encodeTextureChannels(tempDest, tempSrc, info, dataFormat.getTextureChannelType());
		packBitfields(dest, tempDest, info);
		switch(info->m_format)
		{
		case Gnm::kSurfaceFormatBG_RG:
			dest[0] = ((dest[0] & 0xFFFFFF) << 8) | (((dest[0] >> 8) & 0xFF)      );
			break;
		case Gnm::kSurfaceFormatGB_GR:
			dest[0] = ((dest[0] & 0xFFFFFF)     ) | (((dest[0] >> 8) & 0xFF) << 24);
			break;
		default:
			break;
		}
		broadcastElement(dest, info);
	}

	void sharedChromaDecoder(const SurfaceFormatInfo *restrict info, Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *src, const Gnm::DataFormat dataFormat)
	{
		uint32_t tempSrc[4];
		Gnmx::Toolkit::Reg32 tempDest[4];

		unpackBitfields(tempSrc, src, info);
		decodeTextureChannels(tempDest, tempSrc, info, dataFormat.getTextureChannelType());
		swizzleRegs(dest, tempDest, dataFormat);
	}
}

#define NONZERO(X) ((X) ? 1 : 0)
#define SILENCE_DIVIDE_BY_ZERO_WARNING(X) ((X) ? (X) : 1)
#define MAXUNORM(X) ((uint64_t(1) << (X))-1)
#define MAXSNORM(X) (MAXUNORM(X) >> 1)
#define OOMAXUNORM(X) (1.0 / SILENCE_DIVIDE_BY_ZERO_WARNING(MAXUNORM(X)))
#define OOMAXSNORM(X) (1.0 / SILENCE_DIVIDE_BY_ZERO_WARNING(MAXSNORM(X)))
#define DEFINE_SURFACEFORMATINFO(S,X,Y,Z,W,E,D) \
	{(S), NONZERO(X)+NONZERO(Y)+NONZERO(Z)+NONZERO(W), (X)+(Y)+(Z)+(W), {(X), (Y), (Z), (W)}, (E), (D), {0, (X), (X)+(Y), (X)+(Y)+(Z)}, {OOMAXUNORM(X), OOMAXUNORM(Y), OOMAXUNORM(Z), OOMAXUNORM(W)}, {OOMAXSNORM(X), OOMAXSNORM(Y), OOMAXSNORM(Z), OOMAXSNORM(W)}}

static const SurfaceFormatInfo g_surfaceFormatInfo[] =
{
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatInvalid    ,  0,  0,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8          ,  8,  0,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16         , 16,  0,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_8        ,  8,  8,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32         , 32,  0,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16_16      , 16, 16,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat10_11_11   , 11, 11, 10,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat11_11_10   , 10, 11, 11,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat10_10_10_2 ,  2, 10, 10, 10, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat2_10_10_10 , 10, 10, 10,  2, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_8_8_8    ,  8,  8,  8,  8, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32      , 32, 32,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat16_16_16_16, 16, 16, 16, 16, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32_32   , 32, 32, 32,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat32_32_32_32, 32, 32, 32, 32, simpleEncoder, simpleDecoder),
	{},
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_6_5      ,  5,  6,  5,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat1_5_5_5    ,  5,  5,  5,  1, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_5_5_1    ,  1,  5,  5,  5, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat4_4_4_4    ,  4,  4,  4,  4, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat8_24       , 24,  8,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat24_8       ,  8, 24,  0,  0, simpleEncoder, simpleDecoder),
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatGB_GR      ,  8,  8,  8,  8, sharedChromaEncoder, sharedChromaDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormatBG_RG      ,  8,  8,  8,  8, sharedChromaEncoder, sharedChromaDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat5_9_9_9    ,  9,  9,  9,  5, sharedExponentEncoder, sharedExponentDecoder),
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat4_4        ,  4,  4,  0,  0, simpleEncoder, simpleDecoder),
	DEFINE_SURFACEFORMATINFO(Gnm::kSurfaceFormat6_5_5      ,  5,  5,  6,  0, simpleEncoder, simpleDecoder), 
};

void Gnmx::Toolkit::dataFormatEncoder(uint32_t *restrict dest, uint32_t *restrict destDwords, const Gnmx::Toolkit::Reg32 *restrict src, const Gnm::DataFormat dataFormat)
{
	Gnm::SurfaceFormat surfaceFormat = dataFormat.getSurfaceFormat();
	SCE_GNM_ASSERT(surfaceFormat < sizeof(g_surfaceFormatInfo)/sizeof(g_surfaceFormatInfo[0]));
	const SurfaceFormatInfo *info = &g_surfaceFormatInfo[dataFormat.getSurfaceFormat()];
	SCE_GNM_ASSERT(info->m_format == surfaceFormat);

	info->m_encoder(info, dest, src, dataFormat);
	*destDwords = info->m_bitsPerElement <= 32 ? 1 : info->m_bitsPerElement / 32;
}

void Gnmx::Toolkit::dataFormatDecoder(Gnmx::Toolkit::Reg32 *restrict dest, const uint32_t *restrict src, const Gnm::DataFormat dataFormat)
{
	Gnm::SurfaceFormat surfaceFormat = dataFormat.getSurfaceFormat();
	SCE_GNM_ASSERT(surfaceFormat < sizeof(g_surfaceFormatInfo)/sizeof(g_surfaceFormatInfo[0]));
	const SurfaceFormatInfo *info = &g_surfaceFormatInfo[dataFormat.getSurfaceFormat()];
	SCE_GNM_ASSERT(info->m_format == surfaceFormat);

	info->m_decoder(info, dest, src, dataFormat);
}

