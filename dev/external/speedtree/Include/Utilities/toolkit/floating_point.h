/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNM_TOOLKIT_FLOATING_POINT_H
#define _SCE_GNM_TOOLKIT_FLOATING_POINT_H

namespace sce
{
	namespace Gnmx
	{
		namespace Toolkit
		{
			/** @brief Packs a 32-bit floating point value in IEEE 754 format into a value in a packed format, with a user-specified number of sign, exponent, and mantissa bits.
			   @param value                The floating point value to pack.
			   @param signBits             The number of sign bits in the packed value. Zero means "no sign bit" and >1 is not useful.
			   @param exponentBits         The number of exponent bits in the packed value. 
			   @param mantissaBits         The number of mantissa bits in the packed value.
			   */
			uint32_t packFloat(float value, uint32_t signBits, uint32_t exponentBits, uint32_t mantissaBits);

			/** @brief Unpacks a value in a packed format with an arbitrary number of sign, exponent, and mantissa bits into a 32-bit floating point value in IEEE 754 format.
			   @param value                The packed format value to unpack.
			   @param signBits             The number of sign bits in the packed value. Zero means "no sign bit" and >1 is not useful.
			   @param exponentBits         The number of exponent bits in the packed value. 
			   @param mantissaBits         The number of mantissa bits in the packed value.
			   */
			float unpackFloat(uint32_t value, uint32_t signBits, uint32_t exponentBits, uint32_t mantissaBits);

			/** @brief Converts a floating point value into the equivalent signed integer value, using rounding.
			   @param value                The floating point value to convert to a signed integer.
			   */
			int32_t convertFloatToInt(float value);

			/** @brief Converts a floating point value into the equivalent unsigned integer value, using rounding.
			   @param value                The floating point value to convert to an unsigned integer.
			   */
			uint32_t convertFloatToUint(float value);

			/** @brief Converts a floating point value into the 10-bit float format used by the GPU.
			   @param value                The floating point value to convert to a 10-bit float.
			   */
			uint32_t floatToFloat10(float value);

			/** @brief Converts a floating point value into the 11-bit float format used by the GPU.
			   @param value                The floating point value to convert to a 11-bit float.
			   */
			uint32_t floatToFloat11(float value);

			/** @brief Converts a floating point value into the 16-bit float format used by the GPU.
			   @param value                The floating point value to convert to a 16-bit float.
			   */
			uint32_t floatToFloat16(float value);

			/** @brief Converts a floating point value into the 32-bit float format used by the GPU (these formats are identical.)
			   @param value                The floating point value to convert to a 32-bit float (these formats are identical.)
			   */
			uint32_t floatToFloat32(float value);

			/** @brief Converts a 10-bit float in the format used by the GPU into a plain-old float.
			   @param value                The 10-bit float to convert to a plain-old float.
			   */
			float float10ToFloat(uint32_t value);

			/** @brief Converts an 11-bit float in the format used by the GPU into a plain-old float.
			   @param value                The 11-bit float to convert to a plain-old float.
			   */
			float float11ToFloat(uint32_t value);

			/** @brief Converts a 16-bit float in the format used by the GPU into a plain-old float.
			   @param value                The 16-bit float to convert to a plain-old float.
			   */
			float float16ToFloat(uint32_t value);

			/** @brief Converts a 32-bit float in the format used by the GPU into a plain-old float (these formats are identical.)
			   @param value                The 32-bit float to convert to a plain-old float (these formats are identical.)
			   */
			float float32ToFloat(uint32_t value);
		}
	}
}
#endif /* _SCE_GNM_TOOLKIT_FLOATING_POINT_H*/
