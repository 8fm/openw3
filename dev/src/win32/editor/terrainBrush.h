#pragma once
#include "../../common/engine/terrainTile.h"

// Most things just use one. Except TPM_BitSetAndMix, which needs an extra channel, and TPM_ColorMap[Brightness], which use four...
#define MAX_TERRAIN_BRUSH_OPS 4

/// Paint mode
enum ETerrainPaintMode
{
	TPM_BitSetNoCheck,				// Overwrite value without masking, or checking for current value

	TPM_BitSetAndMix,				// Partially overwrite mask value, and then perform some sort of mixing...
									//    params3[0], params4[0] - mask and value for "set" phase
									//    params3[1] - 0x00AABBCC
									//        AA: shift applied after addition
									//        BB: bits to use for clamped addition operation (unshifted, so if 3 bits are added, BB = 7)
									//        CC: shift for addition bits
									//    params4[1] - value to add/subtract to existing bits, according to params3[1].

	TPM_Add,						// Add scaled falloff to target bitmap: BMP += Falloff * scale
	TPM_Lerp,						// Lerp to constant value: BMP = LERP(BMP, value, scale)
	TPM_Slope,						// Slope brush
	TPM_Smooth,						// Smooth scaled by falloff: BMP = SMOOTH(BMP, Falloff * scale)
	TPM_Melt,						// Smooth, with Floor ops, scaled by falloff: BMP = SMOOTH_FLOOR(BMP, Falloff * scale)
	TPM_Noise,						// Add/Subtract random scaled value: BMP = BMP + Rand(-1,1) * scale

	TPM_ColorMap,					// Set masked color, lerping according to scaled falloff: COLOR & mask = LERP(COLOR & mask, value & mask, Falloff * scale)
};


enum ETerrainPaintSlopeThresholdAction
{
	STACT_Set,
	STACT_Add,
	STACT_Subtract,
	STACT_Mix
};

/// Stamp brush parameters
struct SStampBrushParams
{
	SStampBrushParams ()
		: yawDegrees( 0.f )
		, prevYawDegrees( 1.f )
		, multiplier( 1.f )
	{}

	Float	yawDegrees;
	Float	prevYawDegrees;
	Float	multiplier;		// 1.0f - adding, -1.0f - subtracting
};

/// Slope brush parameters
struct SSlopeBrushParams
{
	SSlopeBrushParams ()
		: yawDegrees( 0 )
		, slopeDegrees( 0 )
		, offset( 0 )
		, referencePos( 0, 0, 0 )
		, paintOrigin( 0, 0, 0 )
	{}

	Vector GetUpDir2D() const
	{
		Float yawRadians = DEG2RAD( yawDegrees );
		return Vector( Red::Math::MSin( yawRadians ), Red::Math::MCos( yawRadians ), 0.0f );
	}

	Vector GetRightDir2D() const
	{
		Vector up = GetUpDir2D();
		return Vector( -up.Y, up.X, up.Z );
	}

	Vector GetUpDir3D() const
	{
		Float scaleZ		= Red::Math::MSin( DEG2RAD( slopeDegrees ) );
		Float scaleXY		= Red::Math::MCos( DEG2RAD( slopeDegrees ) );
		Float yawRadians	= DEG2RAD( yawDegrees );
		return Vector( Red::Math::MSin( yawRadians ) * scaleXY, Red::Math::MCos( yawRadians ) * scaleXY, scaleZ );
	}

	Vector GetRightDir3D() const
	{
		Vector up = GetUpDir2D();
		return Vector( -up.Y, up.X, up.Z );
	}

	Float GetHeightRatio() const
	{
		return Red::Math::MTan( DEG2RAD( slopeDegrees ) );
	}

	Float	yawDegrees;
	Float	slopeDegrees;
	Float	offset;
	Vector	referencePos;
	Vector  paintOrigin;
	Float	lowestElevation;
	Float	highestElevation;
};

struct CTerrainBrush
{
	ETerrainPaintMode	m_paintMode;		// Painting mode

	Float*				m_tileData;
	TControlMapType*	m_tileCMData;
	TColorMapType*		m_tileColorData;

	// Width of the actual tile data buffer.
	Uint32				m_tileWidth;
	Uint32				m_tileColorWidth;

	Int32				m_tileBorderSize;
	Int32				m_tileColorBorderSize;

	const Float*		m_falloffData;
	Uint32				m_brushWidth;
	Uint32				m_brushColorWidth;

	// generic params, can be used to apply
	// different types of modifications basing on the brush type
	// eg. scale, bias
	Float				m_param1[MAX_TERRAIN_BRUSH_OPS];
	Float				m_param2[MAX_TERRAIN_BRUSH_OPS];
	Int32				m_param3[MAX_TERRAIN_BRUSH_OPS];
	Int32				m_param4[MAX_TERRAIN_BRUSH_OPS];
	Int32				m_paramSize;

	Rect				m_targetRect;
	Rect				m_colorMapRect;

	Float				m_texelsPerUnit;

	SSlopeBrushParams	m_slopeParams;

	Float				m_probability;
	Uint32				m_textureMask;

	TOptional< Float >	m_lowLimit;
	TOptional< Float >	m_highLimit;

	RED_INLINE CTerrainBrush()
		: m_paintMode( TPM_Smooth )
		, m_tileData( nullptr )
		, m_tileCMData( nullptr )
		, m_tileColorData( nullptr )
		, m_falloffData( nullptr )
		, m_tileWidth( 0 )
		, m_brushWidth( 0 )
		, m_brushColorWidth( 0 )
		, m_paramSize( 1 )
		, m_texelsPerUnit( 0.0f )
		, m_probability( 1.0f )
		, m_textureMask( 0 )
		, m_tileBorderSize( 0 )
		, m_tileColorBorderSize( 0 )
	{
		for ( Int32 i=0; i<MAX_TERRAIN_BRUSH_OPS; ++i )
		{
			m_param1[ i ] = 0;
			m_param2[ i ] = 0;
			m_param3[ i ] = 0;
			m_param4[ i ] = 0;
		}
	}


	// Pack slope threshold parameters, according to how TPM_BitSetAndMix expects.
	RED_INLINE void SetSlopeThresholdParams( ETerrainPaintSlopeThresholdAction action, Int32 slopeThresholdIndex )
	{
		const Int32 NUM_BITS_FOR_SLOPE_THRESHOLD = 3;
		const Int32 SLOPE_THRESHOLD_SHIFT = 10;

		const Int32 MAX_VALUE = ( 1 << NUM_BITS_FOR_SLOPE_THRESHOLD ) - 1;

		// If we're setting, we don't have to do the mixing step, so we can just add it to the set parameters.
		if ( action == STACT_Set )
		{
			m_param3[0] |= MAX_VALUE << SLOPE_THRESHOLD_SHIFT;
			m_param4[0] |= slopeThresholdIndex << SLOPE_THRESHOLD_SHIFT;
		}
		// Otherwise, add/subtract/mix will be applied in a second mixing pass.
		else
		{
			m_param3[1] = ( MAX_VALUE << 8 ) | ( SLOPE_THRESHOLD_SHIFT );
			if ( action == STACT_Mix )
			{
				m_param3[1] |= ( 1 << 16 );
			}

			m_param4[1] = slopeThresholdIndex + 1;
			if ( action == STACT_Subtract )
			{
				m_param4[1] = -m_param4[1];
			}
		}
	}
};


void ApplyBrush( const CTerrainBrush& brush );


typedef Bool (*ApplyBrushFilterFunc)( Float, const TControlMapType*, Uint32, const Float*, Float, Float );
ApplyBrushFilterFunc GetApplyBrushFilterFunc( Bool haveHeight, Bool haveControl, Float probability, Uint32 textureMask, const TOptional< Float >& lowLimit, const TOptional< Float >& highLimit );
