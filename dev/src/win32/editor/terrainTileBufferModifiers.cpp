/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "terrainBrush.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/terrainUtils.h"
#include "../../common/redSystem/numericalLimits.h"

// Operators

//////////////////////////////////////////////////////////////////////////
// Control map

class SetBitsNoCheck
{
public:
	RED_INLINE Uint16 operator()( Float /*bias*/, Float /*scale*/, Int32 /*mask*/, Int32 value, TControlMapType dstValue, const TControlMapType /*srcValue*/, const TControlMapType /*currentValue*/, Float falloffValue )
	{
		// Can paint over terrain holes
		if ( falloffValue == 0.0f )
		{
			return dstValue;
		}

		return value;
	}
};

class SetBitsMasked
{
public:
	RED_INLINE Uint16 operator()( Float /*bias*/, Float /*scale*/, Int32 mask, Int32 value, TControlMapType dstValue, const TControlMapType /*srcValue*/, const TControlMapType /*currentValue*/, Float falloffValue )
	{
		// Can't paint over terrain holes
		if ( falloffValue == 0.0f || !dstValue )
		{
			return dstValue;
		}

		// Assume that value is only set on the masked bits.
		return ( dstValue & ~mask ) | value;
	}
};


class MixBitsMasked
{
public:
	RED_INLINE Uint16 operator()( Float /*bias*/, Float /*scale*/, Int32 packedParams, Int32 value, TControlMapType dstValue, const TControlMapType /*srcValue*/, const TControlMapType /*currentValue*/, Float falloffValue )
	{
		// Can't paint over terrain holes
		if ( falloffValue == 0.0f || !dstValue )
		{
			return dstValue;
		}

		// Unpack mask parameter. Assume these were set by CTerrainBrush::SetSlopeThresholdParams().
		Int8	postShift	= ( packedParams >> 16 ) & 0xff;
		Uint16	maxVal		= ( packedParams >> 8 ) & 0xff;
		Int32	offset		= packedParams & 0xff;

		Uint16 mask = maxVal << offset;

		Uint16 currVal = ( dstValue >> offset ) & maxVal;
		Uint16 newVal = ( Uint16 )Clamp< Int32 >( ( currVal + value ) >> postShift, 0, maxVal ) << offset;

		return ( dstValue & ~mask ) | newVal;
	}
};


//////////////////////////////////////////////////////////////////////////
// Height map


class Noise
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float /*srcValue*/, const Float /*currentValue*/, Float falloffValue )
	{
		if ( falloffValue == 0.f )
		{
			return dstValue;
		}
		return dstValue + scale * GEngine->GetRandomNumberGenerator().Get< Float >( -1.f , 1.f );
	}
};


class Mix
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float srcValue, const Float /*currentValue*/, Float falloffValue )
	{
		Float alpha	= scale * falloffValue;
		return dstValue + ( srcValue - dstValue ) * alpha;
	}
};

class MixInt
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float srcValue, const Float /*currentValue*/, Float falloffValue )
	{
		Float alpha = scale * falloffValue;
		return MFloor( dstValue + ( srcValue - dstValue ) * alpha );
	}
};

class AddFalloff
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float /*srcValue*/, const Float /*currentValue*/, Float falloffValue )
	{
		return dstValue + falloffValue * scale;
	}
};

class Add
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float srcValue, const Float /*currentValue*/, Float /*falloffValue*/ )
	{
		return dstValue + srcValue * scale;
	}
};

class AddInt
{
public:
	RED_INLINE Float operator()( Float /*bias*/, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float srcValue, const Float /*currentValue*/, Float /*falloffValue*/ )
	{
		return MFloor( dstValue + srcValue * scale );
	}
};

class LerpFunc
{
public:
	RED_INLINE Float operator()( Float bias, Float scale, Int32 /*mask*/, Int32 /*offset*/, Float dstValue, const Float /*srcValue*/, const Float /*currentValue*/, Float falloffValue )
	{
		Float alpha = scale * falloffValue;
		return dstValue + ( bias - dstValue ) * alpha;
	}
};


//////////////////////////////////////////////////////////////////////////


RED_INLINE Bool CheckProbability( Float probability )
{
	return GEngine->GetRandomNumberGenerator().Get< Float >() <= probability;
}

RED_INLINE Bool CheckHeightLimit( Float height, Float lowLimit, Float highLimit )
{
	return height >= lowLimit && height <= highLimit;
}

RED_INLINE Bool CheckHTexMask( TControlMapType control, Uint32 textureMask )
{
	const Bool invertHorizontalMask	= ( ( textureMask >> 3 ) & 1 ) == 1;
	const Uint16 horizontalIndex	= ( textureMask >> 5 ) & 0x1F;

	// Check horizontal texture value
	Bool htexMatch = horizontalIndex == ( control & 0x1F );
	if ( invertHorizontalMask )
	{
		htexMatch = !htexMatch;
	}

	return htexMatch;
}

RED_INLINE Bool CheckVTexMask( TControlMapType control, Uint32 textureMask )
{
	const Bool invertVerticalMask	= ( ( textureMask >> 4 ) & 1 ) == 1;
	const Uint16 verticalIndex		= ( textureMask >> 10 ) & 0x1F;

	// Check vertical texture value
	Bool vtexMatch = verticalIndex == ( ( control >> 5 ) & 0x1F );
	if ( invertVerticalMask )
	{
		vtexMatch = !vtexMatch;
	}

	return vtexMatch;
}


template< Bool CHECK_PROB, Bool CHECK_HEIGHT, Bool CHECK_HTEXMASK, Bool CHECK_VTEXMASK >
Bool ShouldApplyBrush( Float probability, const TControlMapType* control, Uint32 textureMask, const Float* height, Float lowLimit, Float highLimit )
{
	if ( CHECK_PROB && !CheckProbability( probability ) )
	{
		return false;
	}
	if ( CHECK_HEIGHT && !CheckHeightLimit( *height, lowLimit, highLimit ) )
	{
		return false;
	}
	if ( CHECK_HTEXMASK && !CheckHTexMask( *control, textureMask ) )
	{
		return false;
	}
	if ( CHECK_VTEXMASK && !CheckVTexMask( *control, textureMask ) )
	{
		return false;
	}

	return true;
}


static ApplyBrushFilterFunc ApplyFilterFuncs[] = {
	ShouldApplyBrush< false, false, false, false >,
	ShouldApplyBrush< false, false, false, true  >,
	ShouldApplyBrush< false, false, true , false >,
	ShouldApplyBrush< false, false, true , true  >,
	ShouldApplyBrush< false, true , false, false >,
	ShouldApplyBrush< false, true , false, true  >,
	ShouldApplyBrush< false, true , true , false >,
	ShouldApplyBrush< false, true , true , true  >,
	ShouldApplyBrush< true , false, false, false >,
	ShouldApplyBrush< true , false, false, true  >,
	ShouldApplyBrush< true , false, true , false >,
	ShouldApplyBrush< true , false, true , true  >,
	ShouldApplyBrush< true , true , false, false >,
	ShouldApplyBrush< true , true , false, true  >,
	ShouldApplyBrush< true , true , true , false >,
	ShouldApplyBrush< true , true , true , true  >,
};

ApplyBrushFilterFunc GetApplyBrushFilterFunc( Bool haveHeight, Bool haveControl, Float probability, Uint32 textureMask, const TOptional< Float >& lowLimit, const TOptional< Float >& highLimit )
{
	Uint32 idx = 0;

	if ( probability < 1.0f )
	{
		idx |= 8;
	}
	if ( haveHeight && ( lowLimit.IsInitialized() || highLimit.IsInitialized() ) )
	{
		idx |= 4;
	}
	if ( haveControl )
	{
		const Bool useHorizontalMask	= ( ( textureMask >> 1 ) & 1 ) == 1;
		const Bool useVerticalMask		= ( ( textureMask >> 2 ) & 1 ) == 1;
		if ( useHorizontalMask )
		{
			idx |= 2;
		}
		if ( useVerticalMask )
		{
			idx |= 1;
		}
	}

	return ApplyFilterFuncs[idx];
}


//////////////////////////////////////////////////////////////////////////


template< typename DST_TYPE, typename SRC1_TYPE, typename SRC2_TYPE, typename FALLOFF_TYPE >
struct ApplyFunctorParams
{
	Float bias;
	Float scale;
	Int32 mask;
	Int32 offset;

	Float* heightMap;
	DST_TYPE* dstBuffer;
	const SRC1_TYPE* src1Buffer;
	const SRC2_TYPE* src2Buffer;
	const FALLOFF_TYPE* falloffBuffer;
	const TControlMapType* controlMap;

	size_t hmPitch;
	size_t destPitch;
	size_t src1Pitch;
	size_t src2Pitch;
	size_t falloffPitch;
	size_t cmPitch;

	Rect targetRect;

	ptrdiff_t offsetMinX, offsetMinY;
	ptrdiff_t offsetMaxX, offsetMaxY;

	float probability;
	Uint32 textureMask;

	TOptional< Float > lowLimit;
	TOptional< Float > highLimit;

	ApplyFunctorParams()
		: dstBuffer( nullptr )
		, src1Buffer( nullptr )
		, src2Buffer( nullptr )
		, falloffBuffer( nullptr )
		, controlMap( nullptr )
		, heightMap( nullptr )
		, offsetMinX( 0 )
		, offsetMinY( 0 )
		, offsetMaxX( 0 )
		, offsetMaxY( 0 )
	{}

	void SetParams( Float bias, Float scale, Int32 mask, Int32 offset )
	{
		this->bias = bias;
		this->scale = scale;
		this->mask = mask;
		this->offset = offset;
	}

	void SetBuffers( DST_TYPE* dst, size_t dstPitch, const SRC1_TYPE* src1, size_t src1Pitch, const SRC2_TYPE* src2, size_t src2Pitch, const FALLOFF_TYPE* falloff, size_t falloffPitch, const TControlMapType* cm, size_t cmPitch )
	{
		dstBuffer = dst;
		src1Buffer = src1;
		src2Buffer = src2;
		falloffBuffer = falloff;
		controlMap = cm;

		this->destPitch = dstPitch;
		this->src1Pitch = src1Pitch;
		this->src2Pitch = src2Pitch;
		this->falloffPitch = falloffPitch;
		this->cmPitch = cmPitch;
	}

	void SetHeightLimits( Float* heightMap, size_t hmPitch, TOptional< Float > lowLimit, TOptional< Float > highLimit )
	{
		this->heightMap = heightMap;
		this->hmPitch   = hmPitch;
		this->lowLimit  = lowLimit;
		this->highLimit = highLimit;
	}
};


template< typename Func, typename DST_TYPE, typename SRC1_TYPE, typename SRC2_TYPE, typename FALLOFF_TYPE >
void ApplyFunctorRange( ApplyFunctorParams< DST_TYPE, SRC1_TYPE, SRC2_TYPE, FALLOFF_TYPE >& params )
{
	ASSERT( params.dstBuffer );
	ASSERT( params.src1Buffer );
	ASSERT( params.src2Buffer );

	ApplyBrushFilterFunc shouldApply = GetApplyBrushFilterFunc( params.heightMap != nullptr, params.controlMap != nullptr, params.probability, params.textureMask, params.lowLimit, params.highLimit );

	Float lowLimit = params.lowLimit.IsInitialized() ? params.lowLimit.Get() : -Red::NumericLimits< Float >::Max();
	Float highLimit = params.highLimit.IsInitialized() ? params.highLimit.Get() : Red::NumericLimits< Float >::Max();


	// For area filters, we shrink the filter width towards the edge of the brush. This is the minimum width allowed. At the edges the further samples will have a
	// scaled offset of 1.
	const Float minSampleWidth = 1.0f / Max( ( params.offsetMaxX - params.offsetMinX ) / 2, ( params.offsetMaxY - params.offsetMinY ) / 2 );

	ptrdiff_t leftEdge = params.targetRect.m_left;
	ptrdiff_t rightEdge = params.targetRect.m_right;

	Func functor;
	for ( ptrdiff_t iRow = params.targetRect.m_top; iRow < params.targetRect.m_bottom; ++iRow )
	{
		DST_TYPE* dst				= params.dstBuffer + leftEdge + params.destPitch * iRow;
		const SRC1_TYPE* src1		= params.src1Buffer + leftEdge + params.src1Pitch * iRow;
		const TControlMapType* cm	= params.controlMap ? params.controlMap + leftEdge + params.cmPitch * iRow : nullptr;
		Float* hm					= params.heightMap  ? params.heightMap  + leftEdge + params.hmPitch * iRow : nullptr;
		const FALLOFF_TYPE* falloff	= params.falloffBuffer + leftEdge + params.falloffPitch * iRow;

		ptrdiff_t sz = rightEdge - leftEdge;

		for ( ptrdiff_t iCol = 0; iCol < sz; ++iCol )
		{
			Bool applyTheFunc = shouldApply( params.probability, cm, params.textureMask, hm, lowLimit, highLimit );

			if ( applyTheFunc )
			{
				FALLOFF_TYPE fo = params.falloffBuffer ? *falloff : (FALLOFF_TYPE)0;

				// As we get farther from the center of the brush, the offset positions will be closer together. This keeps the samples mostly inside the actual brush,
				// to prevent weird things happening with the Smooth/Melt tools.
				Float midX = ( sz - 1 ) * 0.5f;
				Float distX = Abs( midX - iCol ) / midX;
				Float midY = ( params.targetRect.Height() - 1 ) * 0.5f;
				Float distY = Abs( midY - iRow - params.targetRect.m_top ) / midY;

				Float sampleWidth = Lerp( MSqrt( distX*distX + distY*distY ), 1.0f, minSampleWidth );
				sampleWidth = Clamp( sampleWidth, 0.0f, 1.0f );

				for ( ptrdiff_t dy = params.offsetMinY; dy <= params.offsetMaxY; ++dy )
				{
					ptrdiff_t y = iRow + (Int32)MRound( dy * sampleWidth );

					for ( ptrdiff_t dx = params.offsetMinX; dx <= params.offsetMaxX; ++dx )
					{
						ptrdiff_t x = leftEdge + iCol + (ptrdiff_t)MRound( dx * sampleWidth );
						const SRC2_TYPE* src2 = params.src2Buffer + params.src2Pitch * y + x;

						*dst = functor( params.bias, params.scale, params.mask, params.offset, *src1, *src2, *dst, fo );
					}
				}
			}

			++dst;
			++src1;
			if ( cm )
			{
				++cm;
			}
			if ( hm )
			{
				++hm;
			}
			++falloff;
		}
	}
}


template< typename Func, typename DST_TYPE, typename SRC1_TYPE, typename SRC2_TYPE, typename FALLOFF_TYPE >
void ApplyFunctor( ApplyFunctorParams< DST_TYPE, SRC1_TYPE, SRC2_TYPE, FALLOFF_TYPE >& params )
{
	if ( params.offsetMinX != params.offsetMaxX || params.offsetMinY != params.offsetMaxY )
	{
		ApplyFunctorRange< Func, DST_TYPE, SRC1_TYPE, SRC2_TYPE, FALLOFF_TYPE >( params );
		return;
	}

	ASSERT( params.dstBuffer );
	ASSERT( params.src1Buffer );
	ASSERT( params.src2Buffer );

	ApplyBrushFilterFunc shouldApply = GetApplyBrushFilterFunc( params.heightMap != nullptr, params.controlMap != nullptr, params.probability, params.textureMask, params.lowLimit, params.highLimit );

	Float lowLimit = params.lowLimit.IsInitialized() ? params.lowLimit.Get() : -Red::NumericLimits< Float >::Max();
	Float highLimit = params.highLimit.IsInitialized() ? params.highLimit.Get() : Red::NumericLimits< Float >::Max();

	ptrdiff_t leftEdge = params.targetRect.m_left;
	ptrdiff_t rightEdge = params.targetRect.m_right;

	Func functor;
	for ( ptrdiff_t iRow = params.targetRect.m_top; iRow < params.targetRect.m_bottom; ++iRow )
	{
		DST_TYPE* dst				= params.dstBuffer + leftEdge + params.destPitch * iRow;
		const SRC1_TYPE* src1		= params.src1Buffer + leftEdge + params.src1Pitch * iRow;
		const TControlMapType* cm	= params.controlMap ? params.controlMap + leftEdge + params.cmPitch * iRow : nullptr;
		Float* hm					= params.heightMap  ? params.heightMap  + leftEdge + params.hmPitch * iRow : nullptr;
		const FALLOFF_TYPE* falloff	= params.falloffBuffer + leftEdge + params.falloffPitch * iRow;

		ptrdiff_t x = leftEdge + params.offsetMinX;
		ptrdiff_t y = iRow + params.offsetMinY;
		const SRC2_TYPE* src2 = params.src2Buffer + params.src2Pitch * y + x;

		ptrdiff_t sz = rightEdge - leftEdge;

		for ( ptrdiff_t iCol = 0; iCol < sz; ++iCol )
		{
			Bool applyTheFunc = shouldApply( params.probability, cm, params.textureMask, hm, lowLimit, highLimit );

			if ( applyTheFunc )
			{
				FALLOFF_TYPE fo = params.falloffBuffer ? *falloff : (FALLOFF_TYPE)0;
				*dst = functor( params.bias, params.scale, params.mask, params.offset, *src1, *src2, *dst, fo );
			}

			++dst;
			++src1;
			++src2;
			if ( cm )
			{
				++cm;
			}
			if ( hm )
			{
				++hm;
			}
			++falloff;
		}
	}
}


struct PaintOnTextureParams
{
	TColorMapType color; 
	TColorMapType* colorMapData;
	Rect colorMapRect;
	Uint32 colorMapDataPitch;
	Uint32 colorWidth; 
	const Float* falloff;
	Rect brushRect;
	Uint32 brushWidth;
	Float strength; 
	Uint32 colorMask;

	const Float* heightMap;
	Uint32 hmPitch;
	TOptional< Float > lowLimit;
	TOptional< Float > highLimit;
};

void PaintOnTexture( const PaintOnTextureParams& params )
{
	ASSERT( params.colorMapData );

	if ( params.colorMapRect.IsEmpty() )
	{
		LOG_EDITOR( TXT( "Color painting: ColorMap rectangle is empty. Exiting.." ) );
		return;
	}
	
	const Uint32 currValMask = ~params.colorMask;

	const Uint32 maskedColor = params.color & params.colorMask;

	const Int32 width = params.colorMapRect.Width();
	const Int32 height = params.colorMapRect.Height();

	const Float colorMid = ( params.colorWidth - 1 ) / 2.0f;
	const Float brushMid = ( params.brushWidth - 1 ) / 2.0f;
	const Float ratio = (Float)params.brushWidth / (Float)params.colorWidth;

	ApplyBrushFilterFunc shouldApply = GetApplyBrushFilterFunc( params.heightMap != nullptr, false, 1.0f, 0, params.lowLimit, params.highLimit );

	Float lowLimit = params.lowLimit.IsInitialized() ? params.lowLimit.Get() : -Red::NumericLimits< Float >::Max();
	Float highLimit = params.highLimit.IsInitialized() ? params.highLimit.Get() : Red::NumericLimits< Float >::Max();

	TerrainUtils::StampMixColor colorMixer;

	for ( ptrdiff_t row = 0; row < height; ++row )
	{
		Uint32 colorRow = params.colorMapRect.m_top + row;
		Uint32 brushRow = Clamp( ( Uint32 )( ( colorRow - colorMid ) * ratio + brushMid ), 0u, params.brushWidth - 1 );

		Uint32 index = params.colorMapDataPitch * colorRow + params.colorMapRect.m_left;
		TColorMapType* cmData = params.colorMapData + index;

		for ( ptrdiff_t col = 0; col < width; ++col )
		{
			Uint32 colorCol = params.colorMapRect.m_left + col;
			Uint32 brushCol = Clamp( ( Uint32 )( ( colorCol - colorMid ) * ratio + brushMid ), 0u, params.brushWidth - 1 );

			Uint32 heightIndex = brushRow * params.hmPitch + brushCol;

			const Float* height = params.heightMap ? &params.heightMap[heightIndex] : nullptr;

			if ( shouldApply( 1.0f, nullptr, 0, height, lowLimit, highLimit ) )
			{
				Uint32 falloffIndex = brushRow * params.brushWidth + brushCol;
				Float falloffVal = params.strength * ( params.falloff ? params.falloff[falloffIndex] : 1.0f );

				TColorMapType currVal = *cmData;
				TColorMapType newVal = maskedColor | ( currVal & currValMask );

				*cmData = colorMixer( currVal, newVal, 0, 0, falloffVal );
			}

			++cmData;
		}
	}
}


// Smooth and Melt brushes do the same thing, except with different add/mix functors. So... yeah...
template< typename AddFunc, typename MixFunc >
static void ApplyBlurBrush( const CTerrainBrush& brush )
{
	Int32 nx = brush.m_tileBorderSize;
	Int32 ny = brush.m_tileBorderSize;

	//
	// Vertical blur pass. Width of the buffer needs to be able to hold the full expanded source. Height can be just the target size.
	//
	size_t tempWidth1 = brush.m_targetRect.Width() + 2*brush.m_tileBorderSize;
	size_t tempHeight1 = brush.m_targetRect.Height();

	Float* tempBuffer1 = ( Float* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, tempWidth1 * tempHeight1 * sizeof( Float ) );
	Red::System::MemoryZero( tempBuffer1, tempWidth1 * tempHeight1 * sizeof( Float ) );

	// Need to offset from tempBuffer1, so that targetRect addresses the correct region.
	// First we offset the the border size, since the left/right edges of tempBuffer1 are still outside the targetRect.
	// Then by the targetRect's left & top, so that tempBuffer1 is correctly positioned within the brush.
	Float* tempBuffer1Start = tempBuffer1 + brush.m_tileBorderSize - brush.m_targetRect.m_left - brush.m_targetRect.m_top * tempWidth1;

	ApplyFunctorParams<Float, Float, Float, Float> vParams;
	vParams.SetParams( 0.0f, 1.0f / ( ny * 2 + 1 ), 0, 0 );
	vParams.SetBuffers( tempBuffer1Start, tempWidth1, tempBuffer1Start, tempWidth1, brush.m_tileData, brush.m_tileWidth, NULL, 0, NULL, 0 );
	vParams.targetRect = brush.m_targetRect;
	vParams.targetRect.m_left -= brush.m_tileBorderSize;
	vParams.targetRect.m_right += brush.m_tileBorderSize;
	vParams.probability = 1.0f;
	vParams.textureMask = 0;
	vParams.offsetMinY = -ny;
	vParams.offsetMaxY = ny;
	ApplyFunctor<AddFunc, Float, Float, Float, Float>( vParams );


	//
	// Horizontal blur pass. Size just needs to fit the final target area.
	//
	size_t tempWidth2 = brush.m_targetRect.Width();
	size_t tempHeight2 = brush.m_targetRect.Height();

	Float* tempBuffer2 = ( Float* )RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, tempWidth2 * tempHeight2 * sizeof( Float ) );
	Red::System::MemoryZero( tempBuffer2, tempWidth2 * tempHeight2 * sizeof( Float ) );

	// As with tempBuffer1, offset from tempBuffer2. Since we're at the final size now, we just offset by targetRect left&top to position
	// the buffer correctly.
	Float* tempBuffer2Start = tempBuffer2 - brush.m_targetRect.m_left - brush.m_targetRect.m_top * tempWidth2;

	ApplyFunctorParams<Float, Float, Float, Float> hParams;
	hParams.SetParams( 0.0f, 1.0f / ( nx * 2 + 1 ), 0, 0 );
	hParams.SetBuffers( tempBuffer2Start, tempWidth2, tempBuffer2Start, tempWidth2, tempBuffer1Start, tempWidth1, NULL, 0, NULL, 0 );
	hParams.targetRect = brush.m_targetRect;
	hParams.probability = 1.0f;
	hParams.textureMask = 0;
	hParams.offsetMinX = -nx;
	hParams.offsetMaxX = nx;
	ApplyFunctor<AddFunc, Float, Float, Float, Float>( hParams );

	//
	// Mix the blurred result with the original.
	//
	ApplyFunctorParams<Float, Float, Float, Float> mixParams;
	mixParams.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
	mixParams.SetBuffers( brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, tempBuffer2Start, tempWidth2, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
	mixParams.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
	mixParams.targetRect = brush.m_targetRect;
	mixParams.probability = brush.m_probability;
	mixParams.textureMask = brush.m_textureMask;
	ApplyFunctor<MixFunc, Float, Float, Float, Float>( mixParams );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, tempBuffer2 );
	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, tempBuffer1 );
}

void ApplyBrush( const CTerrainBrush& brush )
{
	if ( !brush.m_falloffData )
	{
		WARN_EDITOR( TXT( "ApplyBrush: falloff data not present" ) );
		return;
	}

	if ( brush.m_paintMode == TPM_BitSetNoCheck )
	{
		if ( !brush.m_tileCMData )
		{
			WARN_EDITOR( TXT( "ApplyBrush: control data not present" ) );
			return;
		}

		TControlMapType* tileBuffer	= brush.m_tileCMData;
		
		ApplyFunctorParams<TControlMapType, TControlMapType, TControlMapType, Float> params;
		params.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
		params.SetBuffers( tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
		params.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
		params.targetRect = brush.m_targetRect;
		params.probability = brush.m_probability;
		params.textureMask = brush.m_textureMask;

		ApplyFunctor<SetBitsNoCheck, TControlMapType, TControlMapType, TControlMapType, Float >( params );
	}
	else if ( brush.m_paintMode == TPM_BitSetAndMix )
	{
		if ( !brush.m_tileCMData )
		{
			WARN_EDITOR( TXT( "ApplyBrush: control data not present" ) );
			return;
		}

		TControlMapType* tileBuffer	= brush.m_tileCMData;

		// Set bits first
		if ( brush.m_param3[0] != 0 )
		{
			ApplyFunctorParams<TControlMapType, TControlMapType, TControlMapType, Float> setBitsParams;
			setBitsParams.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
			setBitsParams.SetBuffers( tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
			setBitsParams.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
			setBitsParams.targetRect = brush.m_targetRect;
			setBitsParams.probability = brush.m_probability;
			setBitsParams.textureMask = brush.m_textureMask;

			ApplyFunctor<SetBitsMasked, TControlMapType, TControlMapType, TControlMapType, Float >( setBitsParams );
		}

		// Mix step
		if ( brush.m_param3[1] != 0 )
		{
			ApplyFunctorParams<TControlMapType, TControlMapType, TControlMapType, Float> mixBitsParams;
			mixBitsParams.SetParams( 0, 0, brush.m_param3[1], brush.m_param4[1] );
			mixBitsParams.SetBuffers( tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, tileBuffer, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
			mixBitsParams.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
			mixBitsParams.targetRect = brush.m_targetRect;
			mixBitsParams.probability = brush.m_probability;
			mixBitsParams.textureMask = brush.m_textureMask;

			ApplyFunctor<MixBitsMasked, TControlMapType, TControlMapType, TControlMapType, Float >( mixBitsParams );
		}
	}
	else if ( brush.m_paintMode == TPM_ColorMap )
	{
		if ( !brush.m_tileColorData )
		{
			WARN_EDITOR( TXT( "ApplyBrush: color data not present" ) );
			return;
		}

		PaintOnTextureParams params;

		params.color = ( Uint32 )brush.m_param4[0];
		params.colorMapData = brush.m_tileColorData;
		params.colorMapRect = brush.m_colorMapRect;
		params.colorMapDataPitch = brush.m_tileColorWidth;
		params.colorWidth = brush.m_brushColorWidth;
		params.falloff = brush.m_falloffData;
		params.brushRect = brush.m_targetRect;
		params.brushWidth = brush.m_brushWidth;
		params.strength = brush.m_param2[0];
		params.colorMask = ( Uint32 )brush.m_param3[0];

		params.heightMap = brush.m_tileData;
		params.hmPitch = brush.m_tileWidth;
		params.lowLimit = brush.m_lowLimit;
		params.highLimit = brush.m_highLimit;

		PaintOnTexture( params );
	}
	else
	{
		if ( !brush.m_tileData )
		{
			WARN_EDITOR( TXT( "ApplyBrush: tile data not present" ) );
			return;
		}
		if ( !brush.m_tileCMData )
		{
			WARN_EDITOR( TXT( "ApplyBrush: control data not present" ) );
			return;
		}

		// Apply function
		switch( brush.m_paintMode )
		{
		case TPM_Noise:
			{
				ApplyFunctorParams<Float, Float, Float, Float> params;
				params.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
				params.SetBuffers( brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
				params.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
				params.targetRect = brush.m_targetRect;
				params.probability = brush.m_probability;
				params.textureMask = brush.m_textureMask;
				ApplyFunctor<Noise,Float, Float, Float, Float>( params );
				break;
			}

		case TPM_Add:
			{
				ApplyFunctorParams<Float, Float, Float, Float> params;
				params.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
				params.SetBuffers( brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
				params.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
				params.targetRect = brush.m_targetRect;
				params.probability = brush.m_probability;
				params.textureMask = brush.m_textureMask;
				ApplyFunctor<AddFalloff, Float, Float, Float, Float>( params );
				break;
			}

		case TPM_Lerp:
			{
				ApplyFunctorParams<Float, Float, Float, Float> params;
				params.SetParams( brush.m_param1[0], brush.m_param2[0], brush.m_param3[0], brush.m_param4[0] );
				params.SetBuffers( brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_tileData, brush.m_tileWidth, brush.m_falloffData, brush.m_brushWidth, brush.m_tileCMData, brush.m_tileWidth );
				params.SetHeightLimits( brush.m_tileData, brush.m_tileWidth, brush.m_lowLimit, brush.m_highLimit );
				params.targetRect = brush.m_targetRect;
				params.probability = brush.m_probability;
				params.textureMask = brush.m_textureMask;
				ApplyFunctor<LerpFunc,Float,Float,Float, Float>( params );
				break;
			}

		case TPM_Smooth:
			{
				ApplyBlurBrush< Add, Mix >( brush );
				break;
			}

		case TPM_Melt:
			{
				ApplyBlurBrush< AddInt, MixInt >( brush );
				break;
			}

		case TPM_Slope:
			{
				// Get transformation data
				const Uint32	width		= brush.m_targetRect.Width();
				const Uint32	height		= brush.m_targetRect.Height();
				const Vector2	dirUp		= brush.m_slopeParams.GetUpDir2D();
				const Float		heightRatio	= brush.m_slopeParams.GetHeightRatio();
				const Float		range		= brush.m_slopeParams.highestElevation - brush.m_slopeParams.lowestElevation;

				const Vector2	posOffset = brush.m_slopeParams.paintOrigin - ( brush.m_brushWidth / 2 );
				const Vector2	ijToDistanceFromReferencePos = posOffset - brush.m_slopeParams.referencePos;

				ApplyBrushFilterFunc shouldApply = GetApplyBrushFilterFunc( brush.m_tileData != nullptr, brush.m_tileCMData != nullptr, brush.m_probability, brush.m_textureMask, brush.m_lowLimit, brush.m_highLimit );

				Float lowLimit = brush.m_lowLimit.IsInitialized() ? brush.m_lowLimit.Get() : -Red::NumericLimits< Float >::Max();
				Float highLimit = brush.m_highLimit.IsInitialized() ? brush.m_highLimit.Get() : Red::NumericLimits< Float >::Max();

				// Get data
				for ( Int32 j = brush.m_targetRect.m_top; j < brush.m_targetRect.m_bottom; ++j )
				{
					// Get data pointers
					Float* tileData				= brush.m_tileData + brush.m_tileWidth * j + brush.m_targetRect.m_left;
					const Float* falloffData	= brush.m_falloffData + brush.m_brushWidth * j + brush.m_targetRect.m_left;
					const TControlMapType* cm	= brush.m_tileCMData ? brush.m_tileCMData + brush.m_tileWidth * j + brush.m_targetRect.m_left : nullptr;

					for ( Int32 i = brush.m_targetRect.m_left; i < brush.m_targetRect.m_right;  ++i )
					{
						Bool applyBrush = shouldApply( brush.m_probability, cm, brush.m_textureMask, tileData, lowLimit, highLimit );
						if ( applyBrush )
						{
							// Calculate coordinates
							Vector2 distanceFromReferencePos = Vector2( i, j ) + ijToDistanceFromReferencePos;
							Float heightDifferenceFactor = dirUp.Dot( distanceFromReferencePos ) / brush.m_texelsPerUnit;

							// Calculate desired height (in meters)
							Float desiredHeight = heightDifferenceFactor * heightRatio + brush.m_slopeParams.referencePos.Z;

							// calculate meters to texels
							Float metersToTexels = desiredHeight - brush.m_slopeParams.lowestElevation;
							if ( range >= 1.f )
							{
								metersToTexels /= range;
							}
							Float desiredHeightInTexels = metersToTexels * 65536.f;
					
							Float currentHeightInTexels = *tileData;

							// Blend factor
							Float t = Clamp( brush.m_param2[0] * ( *falloffData ), 0.f, 1.f );

							*tileData = Lerp( t, currentHeightInTexels, desiredHeightInTexels );
						}

						++tileData;
						++falloffData;
						if ( cm )
						{
							++cm;
						}
					}
				}
				break;
			}
		}

	}
}
