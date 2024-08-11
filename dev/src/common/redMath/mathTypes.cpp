#include "mathTypes.h"

namespace RedMath
{
	namespace SIMD
	{
		namespace _Internal
		{
			RED_ALIGNED_VAR( const unsigned int, 16 ) X_MASKDATA[4] = { 0xFFFFFFFF, 0, 0, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) Y_MASKDATA[4] = { 0, 0xFFFFFFFF, 0, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) Z_MASKDATA[4] = { 0, 0, 0xFFFFFFFF, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) W_MASKDATA[4] = { 0, 0, 0, 0xFFFFFFFF };
			RED_ALIGNED_VAR( const unsigned int, 16 ) XY_MASKDATA[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) XZ_MASKDATA[4] = { 0xFFFFFFFF, 0, 0xFFFFFFFF, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) XW_MASKDATA[4] = { 0xFFFFFFFF, 0, 0, 0xFFFFFFFF };
			RED_ALIGNED_VAR( const unsigned int, 16 ) YZ_MASKDATA[4] = { 0, 0xFFFFFFFF, 0xFFFFFFFF, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) YW_MASKDATA[4] = { 0, 0xFFFFFFFF, 0, 0xFFFFFFFF };
			RED_ALIGNED_VAR( const unsigned int, 16 ) ZW_MASKDATA[4] = { 0, 0, 0xFFFFFFFF, 0xFFFFFFFF };
			RED_ALIGNED_VAR( const unsigned int, 16 ) XYZ_MASKDATA[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 };
			RED_ALIGNED_VAR( const unsigned int, 16 ) YZW_MASKDATA[4] = { 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		}

		const SIMDVector SIGN_MASK = _mm_set1_ps( -0.0f ); 
		const SIMDVector X_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::X_MASKDATA ) );
		const SIMDVector Y_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::Y_MASKDATA ) );
		const SIMDVector Z_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::Z_MASKDATA ) );
		const SIMDVector W_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::W_MASKDATA ) );
		const SIMDVector XY_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::XY_MASKDATA ) );
		const SIMDVector XZ_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::XZ_MASKDATA ) );
		const SIMDVector XW_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::XW_MASKDATA ) );
		const SIMDVector YZ_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::YZ_MASKDATA ) );
		const SIMDVector YW_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::YW_MASKDATA ) );
		const SIMDVector ZW_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::ZW_MASKDATA ) );
		const SIMDVector XYZ_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::XYZ_MASKDATA ) );
		const SIMDVector YZW_MASK = _mm_load_ps( reinterpret_cast< const float* >( _Internal::YZW_MASKDATA ) );
		const SIMDVector EPSILON_VALUE = _mm_set1_ps( RED_EPSILON ); 
	}
}
