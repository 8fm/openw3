
#include "../../../../bin/shaders/include/globalConstantsVS.fx"
#include "../../../../bin/shaders/include/globalConstantsPS.fx"

namespace GpuApi
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global static consts and GPU Renderer methods
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static const Int32	g_drawPrimitiveUPBufferSize = 3 * 1024 * 1024;

	Bool				g_IsInsideRenderBlock = false;

	static Uint32		g_frameIndex = 0;

	Int32 FrameIndex()
	{
		return g_frameIndex;
	}

#if defined( RED_PLATFORM_WINPC )

#define  SHADER_BASE_PATH_ANSI		"./shaders/"
#define  SHADER_BASE_PATH			TXT("./shaders/")

#elif defined( RED_PLATFORM_DURANGO )

#define  SHADER_BASE_PATH_ANSI		"./bin/shaders/"
#define  SHADER_BASE_PATH			TXT("./bin/shaders/")

#elif defined( RED_PLATFORM_ORBIS )

#define  SHADER_BASE_PATH_ANSI		"bin/shaders/"
#define  SHADER_BASE_PATH			TXT("bin/shaders/")

#endif

	const Char*	GetShaderRootPath()
	{
		return SHADER_BASE_PATH;
	}

	const AnsiChar*	GetShaderRootPathAnsi()
	{
		return SHADER_BASE_PATH_ANSI;
	}

	const Char* GetShaderIncludePath()
	{
		return SHADER_BASE_PATH TXT("include/");
	}

	const AnsiChar* GetShaderIncludePathAnsi()
	{
		return SHADER_BASE_PATH_ANSI "include/";
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enums and other defines
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum EConstatntBufferDirtyMask
	{
		EBDM_CustomVS = FLAG(0),
		EBDM_CustomPS = FLAG(1),
		EBDM_FrequentVS = FLAG(2),
		EBDM_FrequentPS = FLAG(3),

		EBDM_Custom = EBDM_CustomVS | EBDM_CustomPS,
		EBDM_Frequent = EBDM_FrequentVS | EBDM_FrequentPS,
			
		EBDM_Vertex = EBDM_CustomVS | EBDM_FrequentVS,
		EBDM_Pixel  = EBDM_CustomPS | EBDM_FrequentPS,

		EBDM_All = 0xFF
	};

	struct SBufferUpdateRange
	{
		Uint16		m_startIndex;
		Uint16		m_lastIndex;

		Uint8		m_dirtyMask;

		Uint16		m_defaultStartIndex;
		Uint16		m_defaultLastIndex;

		RED_INLINE SBufferUpdateRange( Uint16 start, Uint16 last, Uint8 mask )
			: m_startIndex( 0xFFFF )
			, m_lastIndex( 0 )
			, m_dirtyMask( mask )
			, m_defaultStartIndex( start )
			, m_defaultLastIndex( last )
		{}

		RED_INLINE void Reset()
		{
			m_lastIndex = 0;
			m_startIndex = 0xFFFF;
		}

		RED_INLINE Uint16	GetNumIndexes() const
		{
			return (m_lastIndex-m_startIndex) + 1;
		}

		RED_INLINE Uint16	GetFullNumIndexes() const
		{
			return (m_defaultLastIndex-m_defaultStartIndex) + 1;
		}

		RED_INLINE void AddIndex( Uint16 index )
		{
			m_startIndex = index < m_startIndex ? index : m_startIndex;
			m_lastIndex = index > m_lastIndex ? index : m_lastIndex;
		}

		RED_INLINE void AddIndex( Uint16 firstIndex , Uint16 lastIndex )
		{
			--lastIndex;
			m_startIndex = firstIndex < m_startIndex?firstIndex : m_startIndex;
			m_lastIndex = lastIndex > m_lastIndex ? lastIndex : m_lastIndex;
		}

		RED_INLINE void MapWholeRange()
		{
			m_startIndex = m_defaultStartIndex;
			m_lastIndex = m_defaultLastIndex;
		}

		RED_INLINE Bool IsValid() const
		{
			return m_startIndex <= m_lastIndex;
		}

	};

	SBufferUpdateRange	g_CustomPSRange( PSC_Custom_First , PSC_Custom_Last , EBDM_CustomPS );
	SBufferUpdateRange	g_CustomVSRange( VSC_Custom_First , VSC_Custom_Last , EBDM_CustomVS );

	SBufferUpdateRange	g_FrequentPSRange( PSC_Frequent_First , PSC_Frequent_Last , EBDM_FrequentPS );
	SBufferUpdateRange	g_FrequentVSRange( VSC_Frequent_First , VSC_Frequent_Last , EBDM_FrequentVS );

	RED_INLINE void MapWholeBuffersRange()
	{
		g_CustomPSRange.MapWholeRange();
		g_CustomVSRange.MapWholeRange();
		g_FrequentPSRange.MapWholeRange();
		g_FrequentVSRange.MapWholeRange();
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fast SSE function for memory
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Use it for reduced updates of constant buffers
// Data before copying is cheked if it was changes
#define USE_GPU_API_BUFFER_COMPARATION

// Use it if you would like to use sse instructions for copying 
#define USE_SSE_BUFFER_OPTIMIZATIONS
#define USE_FAKE_SSE

#ifdef USE_FAKE_SSE
	#define SSE128_t __fake128i

	struct __fake128i
	{
		Uint64		low;
		Uint64		hight;

		RED_FORCE_INLINE Bool operator!=( const __fake128i& o ) const { return ((low^o.low)|(hight^o.hight)) != 0ULL; }

		RED_FORCE_INLINE void operator=( const __fake128i& o ) { low=o.low; hight=o.hight; }
	};
#else
	#define SSE128_t __m128i
#endif

	// Compares full chunks of memory - fast
	RED_INLINE Bool CompareVectors128( const void* __restrict a, const void* __restrict b, unsigned int numChunks )
	{
#ifdef USE_SSE_BUFFER_OPTIMIZATIONS

#ifdef USE_FAKE_SSE
		const __fake128i * a128 = (const __fake128i*)( a );
		const __fake128i * b128 = (const __fake128i*)( b );
		const __fake128i * end = a128 + numChunks;
		while( a128 != end )
		{
			if( *a128 != *b128 ) return false;
			++a128;
			++b128;
		}
#else
		RED_ALIGNED_VAR ( const __m128i , 16 ) zero = _mm_setzero_si128();
		RED_ALIGNED_VAR ( __m128i , 16 ) c;
		const __m128i * a128 = (const __m128i*)( a );
		const __m128i * b128 = (const __m128i*)( b );
		const __m128i * end = a128 + numChunks;
		while( a128 != end )
		{
			c = _mm_xor_si128( *a128, *b128 );
			if( _mm_testc_si128(zero, c) == 0 )
				return false;
			++a128;
			++b128;
		}
#endif

		return true;
#else
		return Red::System::MemoryCompare( a , b , numChunks*16 ) == 0;
#endif
	}

	// Compares full chunks of memory - fast
	RED_INLINE Bool CompareCopyVectors128( void* __restrict a, const void* __restrict b, unsigned int numChunks )
	{
#ifdef USE_SSE_BUFFER_OPTIMIZATIONS

#ifdef USE_FAKE_SSE
		__fake128i * a128 = (__fake128i*)( a );
		const __fake128i * b128 = (const __fake128i*)( b );
		const __fake128i * end = a128 + numChunks;
		while( a128 != end )
		{
			if( *a128 != *b128 ) goto copyRest;
			*a128++ = *b128++;
		}
#else
		RED_ALIGNED_VAR ( const __m128i , 16 ) zero = _mm_setzero_si128();
		RED_ALIGNED_VAR ( __m128i , 16 ) c;
		__m128i * a128 = (__m128i*)( a );
		const __m128i * b128 = (const __m128i*)( b );
		const __m128i * end = a128 + numChunks;
		while( a128 != end )
		{
			c = _mm_xor_si128( *a128, *b128 );
			if( _mm_testc_si128(zero, c) == 0 )
			{
				goto copyRest;
			}
			*a128++ = *b128++;
		}
#endif

		return true;

copyRest:

		while( a128 != end )
			*a128++ = *b128++;
		return false;
#else
		const Bool result = Red::System::MemoryCompare( a , b , numChunks*16 ) == 0;
		if( !result ) Red::System::MemoryCopy( a , b , numChunks*16 );
		return result;
#endif
	}


	// Copy vectors fast
	RED_INLINE void CopyVectors128( void* __restrict a, const void* __restrict b, unsigned int numChunks )
	{
#ifdef USE_SSE_BUFFER_OPTIMIZATIONS
		SSE128_t * a128 = (SSE128_t*)( a );
		const SSE128_t * b128 = (const SSE128_t*)( b );
		const SSE128_t * end = a128 + numChunks;
		while( a128 != end )
		{
			*a128++ = *b128++;
		}
#else
		Red::System::MemoryCopy( a , b , numChunks*16 );
#endif
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\
// Common vertex and pixel contant shiat
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// This one uses memory comparation, so if memory didn't changed, there is no force to update buffers
	void SetVertexShaderConstF( Uint32 first, const Float* data, Uint32 num )
	{
		RED_STATIC_ASSERT( sizeof(Float) == 4 );

		SDeviceData &dd = GetDeviceData();

		Float* currentData = &dd.m_VSConstants[ first*4 ];
		Uint8 range = first <= VSC_Frequent_Last ? EBDM_FrequentVS : EBDM_CustomVS;

#ifdef USE_GPU_API_BUFFER_COMPARATION
		// If buffer isn't dirty yet
		if( (dd.m_constantBufferDirtyMask & range ) == 0 )
		{
			// Copy and compare at the same time
			if( CompareCopyVectors128( currentData, data, num ) )
				return;

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
			for (Uint32 i = 0; i<num; ++i)
			{
				dd.m_VSConstantsDebugHistogram[first+i]++;
			}
#endif

			// If buffer changed, make it dirty babe
			dd.m_constantBufferDirtyMask |= range;
			//range.AddIndex( first , first+num );
			return;
		}

		// If it's dirty, just copy straight
		CopyVectors128( currentData, data, num );
		//range.AddIndex( first , first+num );
#else
		CopyVectors128( currentData, data, num );
		dd.m_constantBufferDirtyMask |= range;
		//range.AddIndex( first , first+num );
#endif

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		for (Uint32 i = 0; i<num; ++i)
		{
			dd.m_VSConstantsDebugHistogram[first+i]++;
		}
#endif

	}

	void SetPixelShaderConstF( Uint32 first, const Float* data, Uint32 num )
	{
		RED_STATIC_ASSERT( sizeof(Float) == 4 );

		SDeviceData &dd = GetDeviceData();

		Float* currentData = &dd.m_PSConstants[ first*4 ];
		Uint8 range = first <= PSC_Frequent_Last ? EBDM_FrequentPS : EBDM_CustomPS;

#ifdef USE_GPU_API_BUFFER_COMPARATION
		// If buffer isn't dirty yet
		if( (dd.m_constantBufferDirtyMask & range) == 0 )
		{
			// Copy and compare at the same time
			if( CompareCopyVectors128( currentData, data, num ) )
				return;

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
			for (Uint32 i = 0; i<num; ++i)
			{
				dd.m_PSConstantsDebugHistogram[first+i]++;
			}
#endif

			// If buffer changed, make it dirty babe
			dd.m_constantBufferDirtyMask |= range;
			//range.AddIndex( first , first+num );
			return;
		}

		// If it's dirty, just copy straight
		CopyVectors128( currentData, data, num );
		//range.AddIndex( first , first+num );
#else
		CopyVectors128( currentData, data, num );
		//range.AddIndex( first , first+num );
		dd.m_constantBufferDirtyMask |= range;
#endif

#ifdef CONSTANT_BUFFER_DEBUG_HISTOGRAM
		for (Uint32 i = 0; i<num; ++i)
		{
			dd.m_PSConstantsDebugHistogram[first+i]++;
		}
#endif

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Some common functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Align pointer
	Uint8* AlignPtr( Uint8* ptr, size_t alignment )
	{
		uintptr_t addr = ( uintptr_t ) ptr;
		addr = (addr + (alignment - 1)) & ~( alignment - 1 );
		return (Uint8*)addr;
	}



#ifndef RED_FINAL_BUILD
	const GpuApi::SDeviceUsageStats& GetDeviceUsageStats()
	{
		SDeviceData &dd = GetDeviceData();

		return dd.m_DeviceUsageStats;
	}
#endif


	void SConstantBufferMem::Init(void* mem, Uint32 size)
	{
		m_memStart = (Uint8*)mem;
		m_memSize = size;
		m_memPtr = m_memStart;

		m_frameMemStart[0] = m_memStart;
		m_frameMemStart[1] = m_memStart;
		m_frameIndex = 0;

#ifndef RED_FINAL_BUILD
		m_debugThroughput[0] = 0;
		m_debugThroughput[1] = 0;
#endif
	}

	void SConstantBufferMem::FrameStart()
	{
		m_frameIndex++;
		m_frameMemStart [m_frameIndex % 2] = m_memPtr;
#ifndef RED_FINAL_BUILD
		m_debugThroughput[m_frameIndex % 2] = 0;
#endif
	}

#ifndef RED_FINAL_BUILD
	Uint32 GpuApi::SConstantBufferMem::GetDebugThroughput() const
	{
		return m_debugThroughput[m_frameIndex % 2];
	}
#endif

	void* SConstantBufferMem::Allocate(Uint32 size, Uint32 align)
	{
		Uint8* prevFrameMemStart = m_frameMemStart[(m_frameIndex + 1) % 2];

		Uint8* memPtrEnd = m_memStart + m_memSize;

		// align m_memPtr up to the next boundary
		Uint8* newMemPtr = AlignPtr(m_memPtr, align);

		// assert if we're crossing the boundary of prevFrameMemStart because it means we're overwriting values potentially still in use
		if ((m_memPtr < prevFrameMemStart && (newMemPtr + size) >= prevFrameMemStart))
		{
			GPUAPI_HALT("Constant buffer overflowing previous frame!");
		}

		if (newMemPtr + size > memPtrEnd)
		{
			newMemPtr = AlignPtr(m_memStart, align);

			if ((newMemPtr + size) >= prevFrameMemStart)
			{
				GPUAPI_HALT("Constant buffer overflowing previous frame!");
			}
		}

		m_memPtr = newMemPtr + size;
#ifndef RED_FINAL_BUILD
		m_debugThroughput[m_frameIndex % 2] += size;
#endif
		return newMemPtr;
	}

}


