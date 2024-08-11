/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "globalWaterUpdateParams.h"
#include "dynamicCollisionCollector.h"
#include "../redMath/float16compressor.h"

const Vector CGlobalWaterUpdateParams::DefaultAmplitudeScale( 600.0f, 110.0f, 20.0f, 0.01f );
const Vector CGlobalWaterUpdateParams::DefaultUVScale( 161.5f, 34.58f, 8.0f, 0.0f );

CGlobalWaterHeightmap::CGlobalWaterHeightmap()
	: m_heightmaps( nullptr )
	, m_flipped( false )
	, m_width( 0 )
	, m_height( 0 )
{
}

CGlobalWaterHeightmap::~CGlobalWaterHeightmap()
{
	if ( m_heightmaps != nullptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, m_heightmaps );
		m_heightmaps = nullptr;
	}
}

void CGlobalWaterHeightmap::Initialize( Uint32 width, Uint32 height )
{
	// Lock, in case main thread is trying to get a height value.
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

	RED_ASSERT( !IsInit(), TXT("Water heightmap already initialized!") );
	if ( m_heightmaps != nullptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, m_heightmaps );
		m_heightmaps = nullptr;
	}

	m_heightmaps = static_cast< Uint16* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_GlobalWater, 2 * width * height * sizeof( Uint16 ) ) );

	Uint16 defaultLevel = Float16Compressor::Compress( WATER_DEFAULT_LEVEL );
	// Fill will default height
	for ( Uint32 i = 0; i < width * height * 2; ++i )
	{
		m_heightmaps[i] = defaultLevel;
	}

	m_width = width;
	m_height = height;
}

Bool CGlobalWaterHeightmap::IsInit() const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	return m_heightmaps != nullptr;
}

void* CGlobalWaterHeightmap::GetWritePointer()
{
	if ( m_heightmaps == nullptr )
	{
		LOG_ENGINE( TXT("CGlobalWaterHeightmap::GetWritePointer WITH NULL HM") );
		return nullptr;
	}

	if ( m_flipped )
	{
		return m_heightmaps;
	}
	else
	{
		return m_heightmaps + m_width * m_height;
	}
}

void CGlobalWaterHeightmap::FlipBuffers()
{
	// Lock instead of using an atomic, because we still don't want to flip while main thread is getting a height value.
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	m_flipped = !m_flipped;
}


Float CGlobalWaterHeightmap::GetHeight( Float u, Float v ) const
{
	u -= MFloor( u );
	v -= MFloor( v );

	Float wU = u * m_width;
	Float hV = v * m_height;

	Int32 rx = Int32( wU );
	Int32 ry = Int32( hV );

	rx = Clamp< Int32 >( rx, 0, m_width - 1 );
	ry = Clamp< Int32 >( ry, 0, m_height - 1 );

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

		if ( m_heightmaps == nullptr )
		{
			return WATER_DEFAULT_NON_INIT_LEVEL;
		}

		const Uint16* bufferStart = m_heightmaps;
		if ( m_flipped )
		{
			bufferStart += m_width * m_height;
		}

		/* Linear interpolation :

		00 ----- 10		upper
		|		  |
		01 ----- 11		bottom

		*/

		// Subpixel sampling
		wU -= MFloor( wU );
		hV -= MFloor( hV );

		// Wrap around (precompute ry to offset)
		Uint32 _rx =   ( rx + 1 ) % m_width;
		Uint32 _ry = ( ( ry + 1 ) % m_height ) * m_width;
		ry *= m_width;
		
		// Sample 4 heights
		const Float p00 = Float16Compressor::Decompress( bufferStart[  ry +  rx ] ); 
		const Float p10 = Float16Compressor::Decompress( bufferStart[  ry + _rx ] );
		const Float p01 = Float16Compressor::Decompress( bufferStart[ _ry +  rx ] );
		const Float p11 = Float16Compressor::Decompress( bufferStart[ _ry + _rx ] );

		// Horizontal interpolation
		Float upper = p00 + ( p10 - p00 ) * wU;
		Float bottom = p01 + ( p11 - p01 ) * wU;

		// Vertical interpolation
		Float height = upper + ( bottom - upper ) * hV;

		// RED_LOG( RED_LOG_CHANNEL( TXT("WaterHeight")) , TXT("%f") , height );
		return height;
	}
}


CGlobalWaterUpdateParams::CGlobalWaterUpdateParams( CGlobalWaterHeightmap* heightmap )
	: m_heightmap( heightmap )
{
	RED_ASSERT( m_heightmap != nullptr, TXT("Missing water heightmap") );
	if ( m_heightmap != nullptr )
	{
		m_heightmap->AddRef();
	}

	m_phillipsData = Vector(0.0f, 10.0f, 1.0, 1.0f);

	m_amplitudeScale = DefaultAmplitudeScale;
	m_uvScale = DefaultUVScale;

	m_rainIntensity = 0.0f;

	m_shouldRenderWater = false;
	m_shouldRenderUnderwater = false;

	m_gameTime = 0.0f;
}

CGlobalWaterUpdateParams::~CGlobalWaterUpdateParams()
{
	SAFE_RELEASE( m_heightmap );
}


void CLocalWaterShapesParams::AllocateBuffers( Uint32 size )
{
	const size_t shapesBufferSize = WATER_LOCAL_RESOLUTION * WATER_LOCAL_RESOLUTION * size * sizeof( Float );
	const size_t shapesMatricesSize = size * 4 * sizeof( Float );

	m_shapesBuffer = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_GlobalWater, shapesBufferSize ) );
	m_matrices = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_GlobalWater, shapesMatricesSize ) );

	Red::System::MemoryZero( m_shapesBuffer, shapesBufferSize );
	Red::System::MemoryZero( m_matrices, shapesMatricesSize );
}
void CLocalWaterShapesParams::FreeBuffers()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, m_shapesBuffer );
	RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, m_matrices );
}
CLocalWaterShapesParams::CLocalWaterShapesParams( Uint32 numShapes )
	: m_numShapes( numShapes )
	, m_allocatedShapes( numShapes )
	, m_waterMaxLevel( WATER_DEFAULT_LEVEL )
	, m_shapeWaterLevel_min( 0.0f )
	, m_shapeWaterLevel_max( 0.0f )
{
	AllocateBuffers( numShapes );
}

CLocalWaterShapesParams::~CLocalWaterShapesParams()
{
	FreeBuffers();
}

void CLocalWaterShapesParams::PushNewShape()
{
	if ( m_numShapes == m_allocatedShapes )
	{
		// reallocation

		// calculate new size
		Uint32 newAllocatedShapes = (m_numShapes * 2)+1;

		// cache previous buffers (with their sizes)
		const size_t prevShapesBufferSize = WATER_LOCAL_RESOLUTION * WATER_LOCAL_RESOLUTION * m_numShapes * sizeof( Float );
		const size_t prevShapesMatricesSize = m_numShapes * 4 * sizeof( Float );
		Float* prevShapesBuffer = m_shapesBuffer;
		Float* prevMatrices = m_matrices;

		// allocate new buffers
		AllocateBuffers( newAllocatedShapes );

		// copy previous data into new buffers
		Red::System::MemoryCopy( m_shapesBuffer, prevShapesBuffer, prevShapesBufferSize );
		Red::System::MemoryCopy( m_matrices, prevMatrices, prevShapesMatricesSize );

		// free previous buffers
		RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, prevShapesBuffer );
		RED_MEMORY_FREE( MemoryPool_Default, MC_GlobalWater, prevMatrices );

		m_allocatedShapes = newAllocatedShapes;
	}
	++m_numShapes;
}
