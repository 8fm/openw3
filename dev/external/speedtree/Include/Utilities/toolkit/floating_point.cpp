/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <algorithm>
#include <gnmx.h>
#include "floating_point.h"
using namespace sce;

namespace
{
	union f32
	{
		enum {kBias = 127};
		struct 
		{
			uint32_t m_mantissa:23;
			uint32_t m_exponent:8;
			uint32_t m_sign:1;
		} bits;
		uint32_t u;
		float f;
	};

	uint32_t packBits(uint32_t value, uint32_t offset, uint32_t count, uint32_t field)
	{
		const uint32_t mask = ((1 << count) - 1) << offset;
		return (value & ~mask) | ((field << offset) & mask);
	}

	uint32_t unpackBits(uint32_t value, uint32_t offset, uint32_t count)
	{
		return (value >> offset) & ((1<<count)-1);
	}

	uint32_t expandIntegerFraction(uint32_t value, uint32_t oldBits, uint32_t newBits)
	{
		const uint32_t shift = (newBits - oldBits);
		uint32_t result = value << shift;
		result |= result >> (oldBits* 1);
		result |= result >> (oldBits* 2);
		result |= result >> (oldBits* 4);
		result |= result >> (oldBits* 8);
		result |= result >> (oldBits*16);
		return result;
	}
}

uint32_t Gnmx::Toolkit::packFloat(float value, uint32_t signBits, uint32_t exponentBits, uint32_t mantissaBits)
{
	if(signBits == 0)
		value = std::max(0.f, value);
	f32 in;
	in.f = value;
	const int32_t maxExponent = (1 << exponentBits) - 1;
	const uint32_t bias        = maxExponent >> 1;
	const uint32_t sign        = in.bits.m_sign;
	uint32_t mantissa          = in.bits.m_mantissa >> (23 - mantissaBits);
	int32_t exponent;
	switch(in.bits.m_exponent)
	{
	case 0x00:
		exponent = 0;
		break;
	case 0xFF:
		exponent = maxExponent;
		break;
	default:
		exponent = in.bits.m_exponent - 127 + bias;
		if(exponent < 1 )
		{
			exponent = 1;
			mantissa = 0;
		}
		if(exponent > maxExponent - 1)
		{
			exponent = maxExponent - 1;
			mantissa = (1 << 23) - 1;
		}
	}
	uint32_t result = 0;
	result = packBits(result, 0                          , mantissaBits, mantissa);
	result = packBits(result, mantissaBits               , exponentBits, exponent);
	result = packBits(result, mantissaBits + exponentBits, signBits,     sign    );
	return result;
}

float Gnmx::Toolkit::unpackFloat(uint32_t value, uint32_t signBits, uint32_t exponentBits, uint32_t mantissaBits)
{
	f32 out;
	const uint32_t maxExponent = (1 << exponentBits) - 1;
	const uint32_t bias        = maxExponent >> 1;
	const uint32_t mantissa    = unpackBits(value, 0                          , mantissaBits);
	const uint32_t exponent    = unpackBits(value, mantissaBits               , exponentBits);
	const uint32_t sign        = unpackBits(value, mantissaBits + exponentBits, signBits    );
	out.bits.m_mantissa = expandIntegerFraction(mantissa, mantissaBits, 23);
	out.bits.m_exponent = (exponent == 0) ? 0 : (exponent == maxExponent) ? 0xFF : exponent - bias + 127;
	out.bits.m_sign     = sign;
	return out.f;
}

int32_t Gnmx::Toolkit::convertFloatToInt(float value)
{
	return static_cast<int32_t>(floorf(value + 0.5f));
}

uint32_t Gnmx::Toolkit::convertFloatToUint(float value)
{
	return static_cast<uint32_t>(floorf(value + 0.5f));
}

uint32_t Gnmx::Toolkit::floatToFloat10(float value)
{
	return packFloat(value, 0, 5, 5);
}

uint32_t Gnmx::Toolkit::floatToFloat11(float value)
{
	return packFloat(value, 0, 5, 6);
}

uint32_t Gnmx::Toolkit::floatToFloat16(float value)
{
	return packFloat(value, 1, 5, 10);
}

uint32_t Gnmx::Toolkit::floatToFloat32(float value)
{
	return packFloat(value, 1, 8, 23);
}

float Gnmx::Toolkit::float10ToFloat(uint32_t value)
{
	return unpackFloat(value, 0, 5, 5);
}

float Gnmx::Toolkit::float11ToFloat(uint32_t value)
{
	return unpackFloat(value, 0, 5, 6);
}

float Gnmx::Toolkit::float16ToFloat(uint32_t value)
{
	return unpackFloat(value, 1, 5, 10);
}

float Gnmx::Toolkit::float32ToFloat(uint32_t value)
{
	return unpackFloat(value, 1, 8, 23);
}

