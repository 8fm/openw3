#include "build.h"
#include "toolsPanel.h"
#include "terrainEditTool.h"
#include "wxThumbnailImageLoader.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/heightmapUtils.h"
#include "../../common/engine/terrainUtils.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/vegetationBrush.h"
#include "../../common/core/depot.h"
#include "../../common/core/xmlFileWriter.h"
#include "../../common/core/xmlFileReader.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/curve.h"


#include "editorExternalResources.h"
#include "curveEditor.h"
#include "undoTerrain.h"
#include "undoManager.h"
#include "wxThumbnailImageLoader.h"
#include "textureArrayViewer.h"
#include "terrainTextureUsageTool.h"

#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/material.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/textureArray.h"

#include "../../common/engine/waterComponent.h"
#include "../../common/engine/triggerAreaComponent.h"
#include "../../common/redMath/mathfunctions_fpu.h"


#define PROCESSING_FREQUENCY 30.0f

#define DEFAULT_AUTO_COLLISION_DEPTH_LIMIT 30.0f
// For auto-collision
RED_DEFINE_STATIC_NAME( hub_exit_entity );


#define wxID_EDIT_PRESET				4500
#define wxID_SET_PRESET_FROM_CURRENT	4501
#define wxID_RESET_PRESET				4502
#define wxID_RESET_ALL_PRESETS			4503

#define wxID_MATERIAL_PARAMETER_START	4600



enum ETextureBlendParam
{
	TBP_BlendSharpness,
	TBP_LowerThreshold,
	TBP_WeakenNormal,
	TBP_Fallof,
	TBP_Specularity,
	TBP_Glossiness,
	TBP_SpecularContrast,
	TBP_EnumSize
};

static Char* TextureBlendParamNames[TBP_EnumSize] = {
	TXT("[H] Blend sharpness"),
	TXT("[V] Slope-based damp"),
	TXT("[V] Slope-based normal damp"),
	TXT("[V/H] RSpec_Scale"),
	TXT("[V/H] Specularity"),
	TXT("[V/H] RSpec_Base"),
	TXT("[V/H] Fallof")
};

static Char* TextureBlendParamTooltips[TBP_EnumSize] = {
	TXT("Horizontal texture parameter: it defines how sharp the transition to vertical texture is. For example snow would probably have quite sharp transition (slider tick on left side), while dirt can have a smooth transition (slider tick on the right side)"),
	TXT("Vertical texture parameter: makes the texture less apparent on the horizontal surfaces. The most common use case, is when you want a specific rock texture to be covered by snow on the horizontal surface, while having it quite visible on steep slopes. Left-most tick position means no damp. Right-most tick position means full damp."),
	TXT("Vertical texture parameter: makes the texture NORMAL less apparent on the horizontal surfaces. The most common use case, is when you want a specific rock texture to be covered by snow on the horizontal surface, while having it quite visible on steep slopes. Left-most tick position means no damp. Right-most tick position means full damp."),
	TXT("Both vertical and horizontal texture parameter: RSpec_Scale - pbr param"),
	TXT("Both vertical and horizontal texture parameter: Specularity of the material - pbr param"),
	TXT("Both vertical and horizontal texture parameter: RSpec_Base - pbr param"),
	TXT("Both vertical and horizontal texture parameter: Strength of the falloff effect - additional param")
};

static const wxColour DefaultColorPresets[ NUM_COLOR_PRESETS ] = {
	wxColour(   0,   0,   0 ),
	wxColour(  64,  64,  64 ),
	wxColour( 128, 128, 128 ),
	wxColour( 192, 192, 192 ),
	wxColour( 255, 255, 255 ),
	wxColour( 128,   0,   0 ),
	wxColour(   0, 128,   0 ),
	wxColour(   0,   0, 128 ),
	wxColour( 255,   0,   0 ),
	wxColour(   0, 255,   0 ),
	wxColour(   0,   0, 255 ),
	wxColour(   0, 128, 128 ),
	wxColour( 128,   0, 128 ),
	wxColour( 128, 128,   0 ),
	wxColour( 255, 128,   0 ),
	wxColour( 255,   0, 128 ),
	wxColour(   0, 255, 128 ),
	wxColour( 128, 255,   0 ),
	wxColour( 128,   0, 255 ),
	wxColour(   0, 128, 255 ),
};


static const Color TerrainTileCollisionColors[ 4 ] = {
	Color( 62, 0, 163 ),	// TTC_AutoOn
	Color( 239, 54, 0 ),	// TTC_AutoOff
	Color( 0, 169, 78 ),	// TTC_ForceOn
	Color( 239, 212, 0 )	// TTC_ForceOff
};

const Float CEdTerrainEditTool::PaintSelectionThreshold = 2;

///////////////////////////////////////////////////////////////////////////



struct SStampUtilsHM
{
	typedef TerrainUtils::FilterHeightMap SampleFilter;
	static Uint16* GetClipmapData( const SClipmapHMQueryResult& part ) { return part.m_tile->GetLevelWriteSyncHM( 0 ); }
	static const Rect& GetClipmapRect( const SClipmapHMQueryResult& part ) { return part.m_addressingRect; }
	static const Rect& GetBufferRect( const SClipmapHMQueryResult& part ) { return part.m_selectionSubRect; }
	static Uint32 GetClipmapResolution( const SClipmapHMQueryResult& part ) { return part.m_tile->GetResolution(); }

	static TControlMapType SampleControlMap( const SClipmapHMQueryResult& part, Int32 x, Int32 y )
	{
		return TerrainUtils::Sample< TControlMapType, TerrainUtils::FilterControlMap >( part.m_tile->GetLevelSyncCM( 0 ), x, y, part.m_tile->GetResolution(), part.m_tile->GetResolution() );
	}
};
struct SStampUtilsCM
{
	typedef TerrainUtils::FilterControlMap SampleFilter;
	static TControlMapType* GetClipmapData( const SClipmapHMQueryResult& part ) { return part.m_tile->GetLevelWriteSyncCM( 0 ); }
	static const Rect& GetClipmapRect( const SClipmapHMQueryResult& part ) { return part.m_addressingRect; }
	static const Rect& GetBufferRect( const SClipmapHMQueryResult& part ) { return part.m_selectionSubRect; }
	static Uint32 GetClipmapResolution( const SClipmapHMQueryResult& part ) { return part.m_tile->GetResolution(); }

	static TControlMapType SampleControlMap( const SClipmapHMQueryResult& part, Int32 x, Int32 y )
	{
		return TerrainUtils::Sample< TControlMapType, TerrainUtils::FilterControlMap >( part.m_tile->GetLevelSyncCM( 0 ), x, y, part.m_tile->GetResolution(), part.m_tile->GetResolution() );
	}
};
struct SStampUtilsColor
{
	typedef TerrainUtils::FilterColorMap SampleFilter;
	static TColorMapType* GetClipmapData( const SClipmapHMQueryResult& part ) { return static_cast< TColorMapType* >( part.m_tile->GetHighestColorMapDataWrite() ); }
	static const Rect& GetClipmapRect( const SClipmapHMQueryResult& part ) { return part.m_colorAddressingRect; }
	static const Rect& GetBufferRect( const SClipmapHMQueryResult& part ) { return part.m_colorSelectionSubRect; }
	static Uint32 GetClipmapResolution( const SClipmapHMQueryResult& part ) { return part.m_tile->GetHighestColorMapResolution(); }

	static TControlMapType SampleControlMap( const SClipmapHMQueryResult& part, Int32 x, Int32 y )
	{
		Uint32 fullRes = part.m_tile->GetResolution();
		Uint32 colRes = part.m_tile->GetHighestColorMapResolution();
		// Need to convert to full resolution. Sample at the middle of the color texel.
		Int32 cx = ( Int32 )( ( x + 0.5f ) * fullRes / colRes );
		Int32 cy = ( Int32 )( ( y + 0.5f ) * fullRes / colRes );
		return TerrainUtils::Sample< TControlMapType, TerrainUtils::FilterControlMap >( part.m_tile->GetLevelSyncCM( 0 ), cx, cy, fullRes, fullRes );
	}
};


struct SApplyStampHM : public SStampUtilsHM
{
	typedef TerrainUtils::StampMixHeightReplace MixFilter;
};
struct SApplyStampHMAdd : public SStampUtilsHM
{
	typedef TerrainUtils::StampMixHeightAdd MixFilter;
};
struct SApplyStampCM : public SStampUtilsCM
{
	typedef TerrainUtils::StampMixControl MixFilter;
};
struct SApplyStampColor : public SStampUtilsColor
{
	typedef TerrainUtils::StampMixColor MixFilter;
};



template< typename T >
struct SStampBrushData
{
	TDynArray< SClipmapHMQueryResult >* clipmapParts;

	const T* stampData;
	Uint32 stampWidth;
	Uint32 stampHeight;
	
	Float angleDegrees;
	Float brushScale;

	Float heightScale;
	Float heightOffset;
	
	const CCurve* falloffCurve;

	Uint32 textureMask;

	SStampBrushData()
		: clipmapParts( nullptr )
		, stampData( nullptr )
		, stampWidth( 0 )
		, stampHeight( 0 )
		, angleDegrees( 0 )
		, brushScale( 1.0f )
		, heightScale( 1.0f )
		, heightOffset( 0 )
		, falloffCurve( nullptr )
		, textureMask( 0 )
	{}
};



static Vector GetRotatedDataRect( Int32 srcWidth, Int32 srcHeight, Float angle )
{
	Float radians = ( 2.0f * M_PI * angle ) / 360.0f;

	Float cosine = MCos( radians );
	Float sine = MSin( radians );

	// Determine the dimensions of rotated image
	Float point1x = -(Float)srcHeight * sine; 
	Float point1y = (Float)srcHeight * cosine; 
	Float point2x = (Float)srcWidth * cosine - (Float)srcHeight * sine; 
	Float point2y = (Float)srcHeight * cosine + (Float)srcWidth * sine;
	Float point3x = (Float)srcWidth * cosine; 
	Float point3y = (Float)srcWidth * sine; 

	Float minX = Min( Min( 0.0f, point1x ), Min( point2x, point3x ) );
	Float minY = Min( Min( 0.0f, point1y ), Min( point2y, point3y ) );
	Float maxX = Max( Max( 0.0f, point1x ), Max( point2x, point3x ) );
	Float maxY = Max( Max( 0.0f, point1y ), Max( point2y, point3y ) );

	return Vector( minX, maxX, minY, maxY );
}


// Copy data from a stamp, into the terrain clipmap.
// Assume that 'parts' is the result from GetRectangleOfData(), passing in the scaled/rotated stamp rectangle.
template< typename _Utils, typename T >
static void CopyRotatedScaledToClipmap( const SStampBrushData< T >& brushData )
{
	ASSERT( brushData.clipmapParts );
	ASSERT( brushData.stampData );
	ASSERT( brushData.stampWidth > 0 && brushData.stampHeight > 0 );
	ASSERT( brushData.brushScale > 0 );
	if ( !brushData.clipmapParts || !brushData.stampData || brushData.stampWidth == 0 || brushData.stampHeight == 0 || brushData.brushScale <= 0 )
	{
		return;
	}

	_Utils::MixFilter mixFilter;


	// Pre-calculate falloff values, so we don't have to do it for every copied value.
	Uint32 falloffSize = ( Uint32 )( brushData.brushScale * ( Max( brushData.stampWidth, brushData.stampHeight ) + 1 ) / 2 );
	Float* falloffBuffer = NULL;
	if ( brushData.falloffCurve )
	{
		falloffBuffer = ( Float* )RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, falloffSize * sizeof( Float ) );
		for ( Uint32 i = 0; i < falloffSize; ++i )
		{
			falloffBuffer[i] = Clamp( brushData.falloffCurve->GetFloatValue( ( Float )i / ( falloffSize - 1 ) ), 0.0f, 1.0f );
		}
	}


	// When sampling the source data, if we're rotating we need to apply a filter to keep it looking nice.
	Bool doBilinearSample = ( brushData.angleDegrees != 0.0f || brushData.brushScale != 1.0f );

	Float radians = brushData.angleDegrees * M_PI / 180.0f;

	Float rotateCos = MCos( radians );
	Float rotateSin = MSin( radians );

	Vector rotatedRect = GetRotatedDataRect( brushData.stampWidth * brushData.brushScale, brushData.stampHeight * brushData.brushScale, brushData.angleDegrees );

	Float minX = rotatedRect.X;
	Float minY = rotatedRect.Z;
	Float rotatedWidth = rotatedRect.Y - rotatedRect.X;
	Float rotatedHeight = rotatedRect.W - rotatedRect.Z;


	const Bool doMasking =  (brushData.textureMask & 1 ) != 0;

	ApplyBrushFilterFunc shouldApply = GetApplyBrushFilterFunc( false, true, 1.0f, brushData.textureMask, TOptional<Float>(), TOptional<Float>() );


	TDynArray< SClipmapHMQueryResult >& parts = *brushData.clipmapParts;
	for ( Uint32 i = 0; i < parts.Size(); ++i )
	{
		const SClipmapHMQueryResult& part = parts[i];

		T* dstPtr = _Utils::GetClipmapData( part );
		const Rect& dstRect = _Utils::GetClipmapRect( part );
		Uint32 dstResolution = _Utils::GetClipmapResolution( part );

		const Rect& stampRect = _Utils::GetBufferRect( part );


		T* targetPtr = &dstPtr[ dstRect.m_top * dstResolution + dstRect.m_left ];

		Int32 numToFillX = dstRect.Width();
		Int32 numToFillY = dstRect.Height();

		for ( Int32 y = 0; y < numToFillY; ++y )
		{
			for ( Int32 x = 0; x < numToFillX; ++x )
			{
				// Put this into the pasted area's space, with the origin in the center of the area.
				Float cx = stampRect.m_left + x - (Float)rotatedWidth  * 0.5f;
				Float cy = stampRect.m_top  + y - (Float)rotatedHeight * 0.5f;

				// Rotate and scale
				Float srcX = ( ( cx * rotateCos + cy * rotateSin ) / brushData.brushScale );
				Float srcY = ( ( cy * rotateCos - cx * rotateSin ) / brushData.brushScale );

				// Offset so origin is back in the corner of stamp.
				srcX += brushData.stampWidth / 2.0f;
				srcY += brushData.stampHeight / 2.0f;

				// If it's outside the source data, don't have to do anything.
				if ( srcX < 0 || srcX >= brushData.stampWidth || srcY < 0 || srcY >= brushData.stampHeight )
				{
					continue;
				}

				if ( doMasking )
				{
					TControlMapType cm = _Utils::SampleControlMap( part, dstRect.m_left + x, dstRect.m_top + y );
					if ( !shouldApply( 1.0f, &cm, brushData.textureMask, nullptr, 0, 0 ) )
					{
						continue;
					}
				}

				T srcValue;
				if ( doBilinearSample )
				{
					srcValue = TerrainUtils::Sample< T, _Utils::SampleFilter >( brushData.stampData, srcX, srcY, brushData.stampWidth, brushData.stampHeight );
				}
				else
				{
					srcValue = brushData.stampData[ (Uint32)srcY * brushData.stampWidth + (Uint32)srcX ];
				}

				Float falloffWeight = 1.0f;
				if ( brushData.falloffCurve )
				{
					// Compute falloff. Even though we pre-calculated falloff values at the scaled resolution, we can still get some aliasing when the
					// brush is rotated, so do linear interpolation between the pre-calculated values.
					Float texelsToBorderX = Min<Float>( srcX, brushData.stampWidth - srcX );
					Float texelsToBorderY = Min<Float>( srcY, brushData.stampHeight - srcY );

					Float idx = Min( texelsToBorderX, texelsToBorderY ) * brushData.brushScale;
					Uint32 idx0 = Clamp( ( Uint32 )idx, 0u, falloffSize - 1 );
					Uint32 idx1 = Clamp( ( Uint32 )( idx + 1 ), 0u, falloffSize - 1 );
					falloffWeight = Lerp( MFract( idx ), falloffBuffer[ idx0 ], falloffBuffer[ idx1 ] );
				}

				targetPtr[ x ] = mixFilter( targetPtr[ x ], srcValue, brushData.heightScale, brushData.heightOffset, falloffWeight );
			}
			targetPtr += dstResolution;
		}
	}

	RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, falloffBuffer );
}


// Assume that dest is already allocated. Assume parts is result of a call to GetRectangleOfData(), with a rectangle of the destination size, rotated by the given angle.
template< typename T, typename _Utils >
static void CopyRotatedFromClipMap( T* dest, Uint32 destWidth, Uint32 destHeight, const TDynArray< SClipmapHMQueryResult >& parts, Float rotation )
{
	const Bool doRotate = rotation != 0.0f;


	T* tempBuffer = NULL;
	Uint32 tempBufferWidth, tempBufferHeight;
	// If we need to rotate, allocate a temporary buffer big enough to hold the bounding box of the rotated data.
	if ( doRotate )
	{
		// Figure out how big the buffer needs to be to fit the source tile data.
		tempBufferWidth = 0;
		tempBufferHeight = 0;
		for ( Uint32 i = 0; i < parts.Size(); ++i )
		{
			const Rect& selRect = _Utils::GetBufferRect( parts[ i ] );
			tempBufferWidth = Max( tempBufferWidth, ( Uint32 )selRect.m_right );
			tempBufferHeight = Max( tempBufferHeight, ( Uint32 )selRect.m_bottom );
		}

		tempBuffer = ( T* )RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, tempBufferWidth * tempBufferHeight * sizeof( T ) );
	}
	// Otherwise, we can just copy into the destination buffer directly.
	else
	{
		tempBufferWidth = destWidth;
		tempBufferHeight = destHeight;
		tempBuffer = dest;
	}

	// Copy from all related terrain tiles into our buffer (either the temporary one, or the output).
	for ( Uint32 i = 0; i < parts.Size(); ++i )
	{
		const SClipmapHMQueryResult& part = parts[i];

		const T* srcPtr = _Utils::GetClipmapData( part );
		const Rect& srcRect = _Utils::GetClipmapRect( part );
		const Rect& dstRect = _Utils::GetBufferRect( part );
		Uint32 srcSize = _Utils::GetClipmapResolution( part );

		const T* srcData = srcPtr + srcRect.m_top * srcSize + srcRect.m_left;
		T* dstData = tempBuffer + dstRect.m_top * tempBufferWidth + dstRect.m_left;

		for ( Int32 y = srcRect.m_top; y < srcRect.m_bottom; ++y )
		{
			Red::System::MemoryCopy( dstData, srcData, srcRect.Width() * sizeof( T ) );
			dstData += tempBufferWidth;
			srcData += srcSize;
		}
	}

	// If we need to rotate, then we can now copy from the temporary buffer into the output.
	if ( doRotate )
	{
		const Float rotateCos = MCos( rotation * M_PI / 180.0f );
		const Float rotateSin = MSin( rotation * M_PI / 180.0f );

		// Copy rotated data
		for ( Uint32 y = 0; y < destHeight; ++y )
		{
			for ( Uint32 x = 0; x < destWidth; ++x )
			{
				Float cx = ( ( Float )x - ( Float )destWidth / 2 );
				Float cy = ( ( Float )y - ( Float )destHeight / 2 );

				Int32 rx = ( Int32 )( ( cx * rotateCos - cy * rotateSin ) + tempBufferWidth / 2.0f );
				Int32 ry = ( Int32 )( ( cx * rotateSin + cy * rotateCos ) + tempBufferHeight / 2.0f );

				rx = Clamp< Int32 >( rx, 0, tempBufferWidth - 1 );
				ry = Clamp< Int32 >( ry, 0, tempBufferHeight - 1 );

				dest[ x + y * destWidth ] = TerrainUtils::Sample< T, _Utils::SampleFilter>( tempBuffer, rx, ry, tempBufferWidth, tempBufferHeight );
			}
		}

		RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, tempBuffer );
	}
}



static Float GetScaledOffset( Float fixedOffset, Float scale, Float scaleOrigin )
{
	return fixedOffset + ( 1.0f - scale ) * scaleOrigin;
}


///////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEdTerrainEditTool );

namespace
{
	void LoadSaveToolControlParams( CConfigurationManager &config, const String &toolTypeName, STerrainToolControlsParams &params, bool save )
	{
		if ( save )
		{
			config.Write( toolTypeName + TXT("/Radius"),		ToString( params.radius ) );
			config.Write( toolTypeName + TXT("/Intensity"),		ToString( params.intensity ) );
			config.Write( toolTypeName + TXT("/Control"),		ToString( params.height ) );
			config.Write( toolTypeName + TXT("/SlopeAngle"),	ToString( params.slopeAngle ) );
			config.Write( toolTypeName + TXT("/StampAngle"),	ToString( params.slopeAngle ) );
			config.Write( toolTypeName + TXT("/Slope"),			ToString( params.slope ) );
			config.Write( toolTypeName + TXT("/SlopeOffset"),	ToString( params.offset ) );
			config.Write( toolTypeName + TXT("/FilterSize"),	ToString( params.filterSize ) );
			config.Write( toolTypeName + TXT("/Heightmap"),		ToString( params.heightmapPath ) );
		}
		else
		{
			params.radius			= config.Read( toolTypeName + TXT("/Radius"),		50.0f );
			params.intensity		= config.Read( toolTypeName + TXT("/Intensity"),	1.0f );
			params.height			= config.Read( toolTypeName + TXT("/Control"),		0.0f );
			params.slopeAngle		= config.Read( toolTypeName + TXT("/SlopeAngle"),	0.0f );
			params.stampAngle		= config.Read( toolTypeName + TXT("/StampAngle"),	0.0f );
			params.slope			= config.Read( toolTypeName + TXT("/Slope"),		0.0f );
			params.offset			= config.Read( toolTypeName + TXT("/SlopeOffset"),	0.0f );
			params.filterSize		= config.Read( toolTypeName + TXT("/FilterSize"),	1.0f );
			params.heightmapPath	= config.Read( toolTypeName + TXT("/Heightmap"),	TXT("") );
		}
	}

	void LoadSaveToolControlParams( CConfigurationManager &config, STerrainToolControlsParams params[ TT_Max ], bool save )
	{
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/RiseLower" ),			params[ TT_RiseLower			], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/Flatten" ),			params[ TT_Flatten				], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/Slope" ),				params[ TT_Slope				], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/Smooth" ),				params[ TT_Smooth				], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/Stamp" ),				params[ TT_Stamp				], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/Melt" ),				params[ TT_Melt					], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/PaintTerrainHoles" ),	params[ TT_PaintTerrainHoles	], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/PaintTexture" ),		params[ TT_PaintTexture			], save );
		LoadSaveToolControlParams( config, TXT( "Tools/TerrainEdit/ToolTypes/PaintColor" ),			params[ TT_PaintColor			], save );
	}

} // end namespace (utility functions)

CTerrainEditCursor::CTerrainEditCursor()
	: m_toolType( TT_None )
	, m_position( 0, 0, 0 )
	, m_lastPosition( 0, 0, 0 )
	, m_desiredElevation( 100 )
	, m_prevDesiredElevation( 100 )
	, m_brushSize( 0 )
	, m_prevBrushSize( 0 )
	, m_intensity( 1.0f )
	, m_falloffData( NULL )
	, m_stampHeightMap( NULL )
	, m_stampControlMap( NULL )
	, m_stampColorMap( NULL )
	, m_texelsPerUnit( 0.0f )
	, m_mode( CM_Paint )
	, m_hasNonZeroValues( false )
	, m_probability( 1.0f )
	, m_falloffDataNeedsRebuild( false )
{
}

CTerrainEditCursor::~CTerrainEditCursor()
{
	ClearAllData();
}


void CTerrainEditCursor::ClearAllData()
{
	m_hasNonZeroValues = false;

	if ( m_falloffData )
	{
		RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, m_falloffData );
		m_falloffData = NULL;
	}

	ClearStamp();
}


void CTerrainEditCursor::ReallocFalloff()
{
	ClearAllData();
	m_falloffData = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, m_texelsPerEdge * m_texelsPerEdge * sizeof( Float ) ) );
	m_falloffDataNeedsRebuild = true;
}

void CTerrainEditCursor::BuildFalloff()
{
	if ( m_toolType == TT_Stamp )
	{
		return;
	}


	ASSERT( m_falloffData, TXT("Trying to build falloff data, when the buffer hasn't been allocated! I'll allocate it now, but should probably look into why it isn't created") );
	if ( !m_falloffData )
	{
		ReallocFalloff();
	}


	// If we're using falloff, create a buffer with samples taken from the falloff curve. We take enough samples to avoid most aliasing issues,
	// and then later we'll lerp these samples to make it very nice.
	Uint32 numFalloffSamples = 0;
	Float* tempFalloff = NULL;
	if ( 0 != ( m_creationFlags & BT_UseFalloffCurve ) && m_falloffCurve )
	{
		numFalloffSamples = m_texelsPerEdge;
		tempFalloff = static_cast< Float* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, ( numFalloffSamples + 1 ) * sizeof( Float ) ) );

		// If brush size > 1, get several samples from the falloff curve.
		if ( m_texelsPerEdge > 1 )
		{
			m_falloffCurve->GetApproximationSamples( tempFalloff, numFalloffSamples );
		}
		// For brush size == 1, the above ends up sampling at 0.0f. In this case, it's better to sample at 1.0f.
		else
		{
			tempFalloff[ 0 ] = m_falloffCurve->GetFloatValue( 1.0f );
		}

		// Clamp falloff samples to [0,1] range.
		for ( Uint32 i = 0; i < numFalloffSamples; ++i )
		{
			tempFalloff[ i ] = Clamp( tempFalloff[ i ], 0.0f, 1.0f );
		}
		// Duplicate the final one, so we don't have to clamp indices when we lerp these.
		tempFalloff[ numFalloffSamples ] = tempFalloff[ numFalloffSamples - 1 ];
	}


	const Float halfTexelSize = ( m_texelsPerEdge * 0.5f );
	const Vector center( ( m_texelsPerEdge - 1 ) * 0.5f, ( m_texelsPerEdge - 1 ) * 0.5f, 0.0f );

	m_hasNonZeroValues = false;

	for ( Uint32 v = 0; v < ( m_texelsPerEdge + 1 ) / 2; ++v )
	{
		Float* falloffRow = &m_falloffData[ v * m_texelsPerEdge ];
		for ( Uint32 u = 0; u < ( m_texelsPerEdge + 1 ) / 2; ++u )
		{
			Float falloff = 0.0f;

			Float distanceFactor = 1.0f - ( center.DistanceTo2D( Vector( u, v, 0.0f ) ) / halfTexelSize );
			if ( distanceFactor >= 0.0f )
			{
				if ( tempFalloff )
				{
					Float falloffIndexF = Clamp< Float >( distanceFactor * numFalloffSamples, 0.0f, numFalloffSamples - 1 );
					size_t falloffIndex = static_cast< size_t >( falloffIndexF );
					Float pct = falloffIndexF - falloffIndex;

					falloff = Lerp( pct, tempFalloff[ falloffIndex ], tempFalloff[ falloffIndex + 1 ] );
				}
				else
				{
					falloff = 1.0f;
				}
			}

			falloffRow[ u ]							= falloff;
			falloffRow[ m_texelsPerEdge - u - 1 ]	= falloff;

			m_hasNonZeroValues |= falloff != 0;
		}

		// Mirror this row to the bottom half of the buffer.
		Red::System::MemoryCopy( &m_falloffData[ ( m_texelsPerEdge - v - 1 ) * m_texelsPerEdge ], falloffRow, m_texelsPerEdge * sizeof( Float ) );
	}

	RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, tempFalloff );

	m_falloffDataNeedsRebuild = false;
}

void CTerrainEditCursor::Tick()
{
	if ( m_toolType == TT_Stamp )
	{
		// Keep prevBrushSize in sync with current brush size. We don't need to do any work when brush size changes.
		m_prevBrushSize = m_brushSize;
		return;
	}

	// If brush size has changed, we'll need to rebuild falloff buffer.
	if ( m_brushSize != m_prevBrushSize )
	{
		m_prevBrushSize = m_brushSize;
		m_texelsPerEdge = (Uint32)MRound( m_brushSize * m_texelsPerUnit * 2.0f );
		ClearAllData();
	}
}

void CTerrainEditCursor::PrepareBuffers()
{
	if ( m_toolType == TT_Stamp )
	{
		return;
	}

	if ( !m_falloffData )
	{
		ReallocFalloff();
	}

	if ( m_falloffDataNeedsRebuild )
	{
		BuildFalloff();
	}
}


void CTerrainEditCursor::SetTexelsPerUnit( Float texelsPerUnit )
{
	m_texelsPerUnit = texelsPerUnit;

	// Clear out prevBrushSize, so that the brush size will be properly updated.
	m_prevBrushSize = 0.0f;
}


void CTerrainEditCursor::SetUseFalloffCurve( Bool useFalloffCurve )
{
	if ( useFalloffCurve )
	{
		m_creationFlags |= BT_UseFalloffCurve;
	}
	else
	{
		m_creationFlags &= ~BT_UseFalloffCurve;
	}

	if ( m_toolType != TT_Stamp )
	{
		m_falloffDataNeedsRebuild = true;
	}
}

void CTerrainEditCursor::SetControlParams( const STerrainToolControlsParams& params )
{
	m_brushSize					= params.radius;
	m_desiredElevation			= params.height;
	m_intensity					= params.intensity;
	m_slopeParams.offset		= params.offset;
	m_slopeParams.yawDegrees	= params.slopeAngle;
	m_slopeParams.slopeDegrees	= params.slope;
	m_stampParams.yawDegrees	= params.stampAngle;
	m_filterSize				= params.filterSize;
	Tick();

	if ( m_toolType == TT_Stamp )
	{
		// Since stamp brush doesn't get ticked, we won't be updating prevBrushSize there. So, we set it here directly
		// so that if we switch back to the old brush it won't think the size hasn't changed.
		m_prevBrushSize = m_brushSize;
		ClearAllData();

		// Try to load stamp from heightmap. If it succeeds, our m_heightmapPath will be set. Otherwise, we've got
		// no data.
		LoadStampHeightmap( params.heightmapPath );
	}
}

void CTerrainEditCursor::UpdateScreenSpaceParams( const CMousePacket& packet )
{
	m_prevScreenSpacePos	= m_currScreenSpacePos;
	m_currScreenSpacePos.X	= packet.m_x;
	m_currScreenSpacePos.Y	= packet.m_y;
}


void CTerrainEditCursor::ClearStamp()
{
	if ( m_stampHeightMap )
	{
		RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, m_stampHeightMap );
		m_stampHeightMap = NULL;
	}
	if ( m_stampControlMap )
	{
		RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, m_stampControlMap );
		m_stampControlMap = NULL;
	}
	if ( m_stampColorMap )
	{
		RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, m_stampColorMap );
		m_stampColorMap = NULL;
	}

	m_stampDataFlags = TESDF_None;
	m_heightmapPath = String::EMPTY;
}


void CTerrainEditCursor::LoadStampFromTerrain( CClipMap* terrain, Uint32 flags )
{
	m_stampDataFlags = 0;
	m_heightmapPath = String::EMPTY;
	m_texelsPerEdge = (Uint32)( m_brushSize * m_texelsPerUnit * 2.0f );

	ClearAllData();

	if ( flags == 0 ) return;

	const Bool loadHeight = ( flags & TESDF_Height ) != 0;
	const Bool loadControl = ( flags & TESDF_Control ) != 0;
	const Bool loadColor = ( flags & TESDF_Color ) != 0;

	const Float rotation = m_stampParams.yawDegrees;

	Vector rotatedRect = GetRotatedDataRect( m_texelsPerEdge, m_texelsPerEdge, rotation );
	Uint32 rotatedWidth = ( Uint32 )MCeil( rotatedRect.Y - rotatedRect.X );
	Uint32 rotatedHeight = ( Uint32 )MCeil( rotatedRect.W - rotatedRect.Z );

	if ( rotatedWidth == 0 || rotatedHeight == 0 ) return;


	SClipmapParameters terrainParameters;
	terrain->GetClipmapParameters( &terrainParameters );

	// cursorRectangle has texel-space bounds of rotated stamp area.
	Vector2 texPos = terrain->GetTexelSpaceNormalizedPosition( m_position, false ) * terrainParameters.clipmapSize;
	Rect cursorRectangle;
	cursorRectangle.m_left = texPos.X - rotatedWidth / 2;
	cursorRectangle.m_right = cursorRectangle.m_left + rotatedWidth;
	cursorRectangle.m_top = texPos.Y - rotatedHeight / 2;
	cursorRectangle.m_bottom = cursorRectangle.m_top + rotatedHeight;
	TDynArray< SClipmapHMQueryResult > parts;
	terrain->GetRectangleOfData( cursorRectangle, parts );

	if ( loadHeight )
	{
		ASSERT( !m_stampHeightMap );
		m_stampHeightMap = static_cast< Uint16* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, m_texelsPerEdge * m_texelsPerEdge * sizeof( Uint16 ) ) );
		CopyRotatedFromClipMap< Uint16, SStampUtilsHM >( m_stampHeightMap, m_texelsPerEdge, m_texelsPerEdge, parts, rotation );
	}

	if ( loadControl )
	{
		ASSERT( !m_stampControlMap );
		m_stampControlMap = static_cast< TControlMapType* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, m_texelsPerEdge * m_texelsPerEdge * sizeof( TControlMapType ) ) );
		CopyRotatedFromClipMap< TControlMapType, SStampUtilsCM >( m_stampControlMap, m_texelsPerEdge, m_texelsPerEdge, parts, rotation );
	}

	if ( loadColor )
	{
		ASSERT( !m_stampColorMap );

		m_stampColorTexelsPerEdge = ( Uint32 )MRound( ( Float )m_texelsPerEdge * terrainParameters.highestColorRes / terrainParameters.tileRes );
		if ( m_stampColorTexelsPerEdge > 0 )
		{
			m_stampColorMap = static_cast< TColorMapType* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, m_stampColorTexelsPerEdge * m_stampColorTexelsPerEdge * sizeof( TColorMapType ) ) );
			CopyRotatedFromClipMap< TColorMapType, SStampUtilsColor >( m_stampColorMap, m_stampColorTexelsPerEdge, m_stampColorTexelsPerEdge, parts, rotation );
		}
		else
		{
			flags = flags & ~TESDF_Color;
		}
	}

	m_stampDataFlags = flags & ( ~TESDF_Additive );


	m_stampSourcePosition = m_position;
}


Bool CTerrainEditCursor::LoadStampHeightmap( const String& path )
{
	if ( path.Empty() ) return false;


	Uint32 imageWidth, imageHeight;
	SHeightmapImageEntry<Uint16> entry;
	entry.m_params.normalizeImage = true;
	if ( !SHeightmapUtils::GetInstance().LoadHeightmap( path, entry, &imageWidth, &imageHeight ) )
	{
		return false;
	}

	ASSERT( entry.m_data );

	ClearAllData();

	m_stampDataFlags = TESDF_Height | TESDF_Additive;

	// Rescale to the larger dimension.
	Uint32 size = Min<Uint32>( Max( imageWidth, imageHeight ), MAX_TEXELS_PER_EDGE );

	// If the size is different from what we already have allocated, need to reallocate.
	m_texelsPerEdge = size;
	m_stampHeightMap = static_cast< Uint16* >( RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, m_texelsPerEdge * m_texelsPerEdge * sizeof( Uint16 ) ) );

	// Do full resample if needed (if image was bigger than max size, or not square)
	if ( size != imageWidth || size != imageHeight )
	{
		for ( Uint32 j = 0; j < m_texelsPerEdge; ++j )
		{
			for ( Uint32 i = 0; i < m_texelsPerEdge; ++i )
			{
				Float si = ( ( (Float)i / (Float)m_texelsPerEdge ) * imageWidth );
				Float sj = ( ( (Float)j / (Float)m_texelsPerEdge ) * imageHeight );

				m_stampHeightMap[i + j*m_texelsPerEdge] = TerrainUtils::Sample< Uint16, TerrainUtils::FilterHeightMap >( entry.m_data, si, sj, imageWidth, imageHeight );
			}
		}
	}
	// Otherwise, a simple memcpy will do
	else
	{
		Red::System::MemoryCopy( m_stampHeightMap, entry.m_data, m_texelsPerEdge * m_texelsPerEdge * sizeof( Uint16 ) );
	}


	// Offset so minimum in the source image will be 0 in the stamp data. Also check for non-zero values.
	m_hasNonZeroValues = false;
	for ( Uint32 i = 0; i < m_texelsPerEdge * m_texelsPerEdge; ++i )
	{
		m_hasNonZeroValues |= m_stampHeightMap[ i ] > 0;
	}

	m_heightmapPath = path;

	return true;
}


Uint32 CTerrainEditCursor::GetPaintingBufferFlags() const
{
	Uint32 bufferFlags = 0;
	switch ( m_toolType )
	{
	case TT_Flatten:
	case TT_Slope:
	case TT_Smooth:
	case TT_Melt:
	case TT_RiseLower:
		// Height
		bufferFlags = PPBF_ReadHM | PPBF_ReadCM | PPBF_WriteHM;
		break;

	case TT_PaintColor:
		// Color
		bufferFlags = PPBF_ReadColor | PPBF_WriteColor;
		break;

	case TT_PaintTexture:
	case TT_PaintTerrainHoles:
		bufferFlags = PPBF_ReadCM | PPBF_WriteCM;
		break;

	case TT_Stamp:
		if ( ( m_stampDataFlags & TESDF_Height )  != 0 ) bufferFlags |= ( PPBF_ReadHM | PPBF_WriteHM );
		if ( ( m_stampDataFlags & TESDF_Control ) != 0 ) bufferFlags |= ( PPBF_ReadCM | PPBF_WriteCM );
		if ( ( m_stampDataFlags & TESDF_Color )   != 0 ) bufferFlags |= ( PPBF_ReadColor | PPBF_WriteColor );
		break;

	case TT_CollForceOn:
	case TT_CollForceOff:
	case TT_CollReset:
		bufferFlags = PPBF_CollisionType;
		break;
	}

	if ( bufferFlags != PPBF_CollisionType )
	{
		if ( m_lowLimit.IsInitialized() || m_highLimit.IsInitialized() )
		{
			// height map is required by z-limits
			bufferFlags |= PPBF_ReadHM;
		}
	}

	return bufferFlags;
}


String CTerrainEditCursor::GetToolName() const
{
	switch ( m_toolType )
	{
	case TT_Flatten:			return TXT("Flatten Terrain");
	case TT_Slope:				return TXT("Terrain Slope");
	case TT_Smooth:				return TXT("Smooth Terrain");
	case TT_RiseLower:			return TXT("Raise/Lower Terrain");
	case TT_PaintTexture:		return TXT("Paint Terrain Texture");
	case TT_PaintColor:			return TXT("Paint Terrain Color");
	case TT_PaintTerrainHoles:	return TXT("Terrain Holes");
	case TT_Stamp:				return TXT("Terrain Stamp");
	case TT_Melt:				return TXT("Melt Terrain");
	case TT_CollForceOn:		return TXT("Force Collision On");
	case TT_CollForceOff:		return TXT("Force Collision Off");
	case TT_CollReset:			return TXT("Reset Collision");
	default:
		HALT( "Invalid terrain tool type?" );
		return TXT("Unknown Terrain Tool");
	}
}


//////////////////////////////////////////////////////////////////////////
// TERRAIN EDIT TOOL
//////////////////////////////////////////////////////////////////////////

CEdTerrainEditTool::CEdTerrainEditTool() 
	: m_dialog(NULL)
	, m_anyTilesAffected( false )
	, m_paintingRequest( false )
	, m_paintingRequestFrames( 0 )
	, m_curveEditor( NULL )
	, m_falloffCurve( NULL )
	, m_isStarted( false )
	, m_selectedHorizontalTexture( 0 )
	, m_selectedVerticalTexture( 0 )
	, m_lastSelectedTexture( 0 )
	, m_slopeThresholdIndex( 0 )
	, m_verticalUVMult( 0 )
	, m_paintSelectionActivated( false )
	, m_textureMaterialParamsSliderRange( 1000000 )
	, m_textureMaterialParamsSliderToTBoxRange( 1000 )
	, m_needToUpdateCollisionOverlay( false )
	, m_presetIndex( 0 )
{
	Red::System::MemoryZero( m_toolButtons, sizeof( m_toolButtons ) );
	Red::System::MemoryZero( m_colorPresetButtons, sizeof( m_colorPresetButtons ) );
}

CEdTerrainEditTool::~CEdTerrainEditTool()
{
	//SEvents::GetInstance().UnregisterListener( CNAME( SelectionChanged ), this );
}

void CEdTerrainEditTool::ToggleUseFalloff(wxCommandEvent & event)
{
	if ( m_cursor.m_toolType == TT_PaintColor )
	{
		m_cursor.SetUseFalloffCurve( m_colorUseFalloffCheckBox->IsChecked() );
	}
	else
	{
		m_cursor.SetUseFalloffCurve( m_useFalloffCheckBox->IsChecked() );
	}
}

void CEdTerrainEditTool::OnClose( wxCloseEvent& event )
{
	RunLaterOnce( [](){ wxTheFrame->GetToolsPanel()->CancelTool(); } );
	event.Veto();
}

void CEdTerrainEditTool::OnModifySelectors(wxCommandEvent& event)
{
	if(event.GetId() == XRCID("m_InputSelectorAddButton"))
	{
		char tmp[32];
		Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), "Selector: %i", m_SelectorCounter ) ;

		// for names display only
		m_SelectorCounter++;

		m_InputDialogParentID = XRCID("m_InputSelectorAddButton");

		XRCCTRL(*m_dialog, "m_TT_InputTextCtrl", wxTextCtrl)->SetValue( ANSI_TO_UNICODE(tmp) );
		m_InputDialog->Show();
	}

	if(event.GetId() == XRCID("m_InputSelectorRemoveButton"))
	{
		RemoveSelector();
	}
}

void CEdTerrainEditTool::OnSave( wxCommandEvent& event )
{
	CClipMap* terrain = m_world->GetTerrain();
	if ( terrain )
	{
		terrain->SaveAllTiles();
	}
}

void CEdTerrainEditTool::OnSelectAll( wxCommandEvent& event )
{
	for ( Uint32 index = 0; index < m_tilesSelection.Size(); ++index )
	{
		m_tilesSelection[ index ] = true;
	}
}

void CEdTerrainEditTool::AddNewSelector( wxString selectorName )
{
	m_SelectorList->Insert( selectorName, 0 );	
}

void CEdTerrainEditTool::RemoveSelector( void )
{
	Int32 sel = m_SelectorList->GetSelection();

	if(sel >-1)
	{
		m_SelectorList->Delete( sel );		
	}
	else
		GFeedback->ShowMsg(TXT("Nothing selected!"), TEXT("Select selector first!") );
}

void CEdTerrainEditTool::OnModifyLayers(wxCommandEvent& event)
{
	// TODO TERRAIN FEATURE

	// add new layer for hmap/tex map/color map/	

	if(event.GetId() == XRCID("m_AddNewLayerButton"))
	{
		char tmp[32];
		Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), "Layer: %i", m_LayerCounter );

		// for names display only
		m_LayerCounter++;

		m_InputDialogParentID = XRCID("m_AddNewLayerButton");

		XRCCTRL(*m_dialog, "m_TT_InputTextCtrl", wxTextCtrl)->SetValue( ANSI_TO_UNICODE(tmp) );
		m_InputDialog->Show();
	}

	if(event.GetId() == XRCID("m_DeleteLayerButton"))
	{
		RemoveLayer();
	}

	if(event.GetId() == XRCID("m_MergeLayersButton"))
	{
		MergeLayers();
	}
}

void CEdTerrainEditTool::AddNewLayer( wxString layerName )
{
	m_LayerList->Insert( layerName, 0 );
	m_LayerList->Check(0);	
}

void CEdTerrainEditTool::RemoveLayer( void )
{
	const int sel = m_LayerList->GetSelection();

	if(sel > -1) m_LayerList->Delete( sel );
	else
		GFeedback->ShowMsg(TXT("Nothing selected!"), TEXT("Select layer first!") );
}

void CEdTerrainEditTool::MergeLayers( void )
{
	// update wx lists
	TDynArray<Uint32> mergeLayersIndex;
	Int32 mergeInto = 0;

	// find highest visible layer we merge into
	for(Uint32 i=0; i<m_LayerList->GetCount(); i++)
	{
		if( m_LayerList->IsChecked(i) )
		{
			if( (Int32)i > mergeInto ) mergeInto = i;
		}		
	}

	// mark for merging
	for(Uint32 i=0; i<m_LayerList->GetCount(); i++)
	{
		if( mergeInto != i && m_LayerList->IsChecked(i) ) mergeLayersIndex.PushBack(i);
	}	

	for(Int32 i=(mergeLayersIndex.SizeInt()-1); i>-1; i--)
	{
		m_LayerList->Delete( mergeLayersIndex[i] );
	}
}

void CEdTerrainEditTool::OnFalloffPreset(wxCommandEvent& event)
{
	m_cursor.m_falloffDataNeedsRebuild = true;

	/*
	if(event.GetId() == XRCID("m_InputFalloffSaveButton"))
	{
		char tmp[32];
		Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), "Preset: %i", m_FallofPresetCounter );

		// for names display only
		m_FallofPresetCounter++;

		m_InputDialogParentID = XRCID("m_InputFalloffSaveButton");

		XRCCTRL(*this, "m_TT_InputTextCtrl", wxTextCtrl)->SetValue( ANSI_TO_UNICODE(tmp) );
		m_InputDialog->Show();
	}

	if(event.GetId() == XRCID("m_InputFalloffRemoveButton"))
	{		
		RemoveFalloffPreset();
	}
	*/
}

void CEdTerrainEditTool::SaveFalloffPreset( wxString presetName )
{
	m_FalloffChoice->Append( presetName );
	m_FalloffChoice->SetSelection( m_FalloffChoice->GetCount()-1 );
}

void CEdTerrainEditTool::RemoveFalloffPreset( void )
{
	Int32 sel = m_FalloffChoice->GetCurrentSelection();
	
	if(sel >-1)
	{
		m_FalloffChoice->Delete( sel );
		m_FalloffChoice->SetSelection(0);
	}
}

void CEdTerrainEditTool::OnInputEntry(wxCommandEvent& event)
{	
	if( XRCCTRL(*m_dialog, "m_InputNameOKButton", wxButton)->GetId() == event.GetId() )
	{
		// add falloff preset here
		if(m_InputDialogParentID == XRCID("m_InputFalloffSaveButton") )
			SaveFalloffPreset( XRCCTRL(*m_dialog, "m_TT_InputTextCtrl", wxTextCtrl)->GetValue() );

		// add layer here
		if(m_InputDialogParentID == XRCID("m_AddNewLayerButton") )
			AddNewLayer( XRCCTRL(*m_dialog, "m_TT_InputTextCtrl", wxTextCtrl)->GetValue() );		
		
		// add selector here
		if(m_InputDialogParentID == XRCID("m_InputSelectorAddButton") )
			AddNewSelector( XRCCTRL(*m_dialog, "m_TT_InputTextCtrl", wxTextCtrl)->GetValue() );
	}	
		
	m_InputDialog->Hide();
}

Bool CEdTerrainEditTool::ValidateDirectory( const wxDir& directory, const String& pattern, Uint32& tileSize, Uint32& tilesPerSide, String& warningMessage )
{
	if ( !directory.IsOpened() )
	{
		warningMessage += TXT( "Cannot find directory to scan." );
		return false;
	}

	wxArrayString resources;
	if ( !directory.GetAllFiles( directory.GetName(), &resources, wxString( pattern.AsChar() ) ) )
	{
		warningMessage += String::Printf( TXT( "No files found with pattern '%s'." ), pattern.AsChar() );
		return false;
	}

	Uint32 width;
	Uint32 height;

	if ( !SHeightmapUtils::GetInstance().GetImageSize( String( resources[ 0 ] ), width, height ) )
	{
		warningMessage += String::Printf( TXT( "Error while reading file: '%s'." ), String( resources[0] ).AsChar() );
		return false;
	}

	Uint32 prevX = 0;
	Uint32 prevY = 0;
	
	Uint32 maxX = 0;
	Uint32 maxY = 0;
	for ( Uint32 i = 1; i < resources.GetCount(); ++i )
	{
		String currentFileName = resources[ i ];
		
		Uint32 currentWidth;
		Uint32 currentHeight;
		if ( !SHeightmapUtils::GetInstance().GetImageSize( currentFileName, currentWidth, currentHeight ) )
		{
			warningMessage += String::Printf( TXT( "Error while reading file: '%s'." ), currentFileName.AsChar() );
			return false;
		}
		if ( currentWidth != width || currentHeight != height )
		{
			warningMessage += String::Printf( TXT( "All tiles in directory have to be the same size (width and height)." ), currentFileName.AsChar() );
			return false;
		}

		Uint32 currentX;
		Uint32 currentY;
		if ( !CClipMap::GetCoordinatesFromWorldMachineFilename( currentFileName, currentX, currentY ) )
		{
			warningMessage += String::Printf( TXT( "Cannot determine tile coordinates from filename: '%s'." ), currentFileName.AsChar() );
			return false;
		}
		
		Bool isNextInRow = maxX ? Abs<Int32>( currentX - prevX ) % maxX > 1 : Abs<Int32>( currentX - prevX ) > 1;
		if ( isNextInRow )
		{
			warningMessage += String::Printf( TXT( "Some of tile files seem to be missing, check source directory." ) );
			return false;
		}
		isNextInRow = maxY ? Abs<Int32>( currentY - prevY ) % maxY > 1 : Abs<Int32>( currentY - prevY ) > 1;
		if ( isNextInRow ) 
		{
			warningMessage += String::Printf( TXT( "Some of tile files seem to be missing, check source directory." ) );
			return false;
		}
		
		prevX = currentX;
		prevY = currentY;

		maxX = Max( maxX, currentX );
		maxY = Max( maxY, currentY );
	}
	
	if ( maxX != maxY )
	{
		warningMessage += String::Printf( TXT( "Terrain has to have the same ammount of tiles along each axis." ) );
		return false;
	}

	// check for maxX + 1 is performed, because indices are in the range [0..pow(2) - 1]
	if ( !IsPow2( maxX + 1 ) )
	{
		warningMessage += String::Printf( TXT( "Number of tiles along the axis has to be power of 2 {1, 2, 4, 8, 16..}." ) );
		return false;
	}

	if ( width != height )
	{
		warningMessage += String::Printf( TXT( "Tiles have to be square." ) );
		return false;
	}

	if ( !IsPow2( width ) )
	{
		warningMessage += String::Printf( TXT( "Tile size has to be power of 2." ) );
		return false;
	}

	tileSize = width;
	tilesPerSide = maxX + 1;

	return true;
}

void CEdTerrainEditTool::RefillTilePathsArray( const wxDir& directory, const String& pattern )
{
	m_tilePaths.Clear();

	wxArrayString resources;
	directory.GetAllFiles( directory.GetName(), &resources, wxString( pattern.AsChar() ) );

	for ( Uint32 i = 0; i < resources.GetCount(); ++i )
	{
		m_tilePaths.PushBack( String( resources[ i ] ) );
	}
}

void CEdTerrainEditTool::OnApplySetup( wxCommandEvent& event )
{

}

void CEdTerrainEditTool::OnExportTiles( wxCommandEvent& event )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}

	Uint32 tilesPerEdge = m_world->GetTerrain()->GetNumTilesPerEdge();
	Bool anyTileSelected = false;
	for ( Uint32 i = 0; i < m_tilesSelection.Size(); ++i )
	{
		if ( m_tilesSelection[ i ] )
		{
			anyTileSelected = true;
			break;
		}
	}

	if ( !anyTileSelected )
	{
		return;
	}

	// compute bounds of tiles to export
	Uint32 minX = INT_MAX;
	Uint32 minY = INT_MAX;
	Uint32 maxX = 0;
	Uint32 maxY = 0;
	for ( Uint32 y = 0; y < tilesPerEdge; ++y )
	{
		for ( Uint32 x = 0; x < tilesPerEdge; ++x )
		{
			if ( m_tilesSelection[ y * tilesPerEdge + x ] )
			{
				minX = Min<Uint32>( minX, x );
				minY = Min<Uint32>( minY, y );
				maxX = Max<Uint32>( maxX, x );
				maxY = Max<Uint32>( maxY, y );
			}
		}
	}

	if ( maxX - minX != maxY - minY )
	{
		if ( !GFeedback->AskYesNo( TXT( "The area you are trying to export seems not rectangular - WorldMachine does not accept rectangular areas. Do you want to continue?" ) ) )
		{
			return;
		}
	}

	Bool exportHM;	// heightmap data
	Bool exportCM;	// colormap data
	GetExportConfiguration( exportHM, exportCM );


	CEdFileDialog openDlg;

	openDlg.AddFormat( TXT("csv"), TXT("CSV") );
	openDlg.SetMultiselection( false );
	openDlg.SetIniTag( TXT("CEdTerrainEditTool_ExportTiles") );

	if ( openDlg.DoSave( m_dialog->GetHandle(), nullptr, true ) )
	{
		String dirName = openDlg.GetFileDirectory();
		String filename = openDlg.GetFileName();

		String nameWithoutExtension = GetBareFilename( filename );
		
		if ( !GSystemIO.CreateDirectory( String::Printf( TXT("%s\\%s_input"), dirName.AsChar(), nameWithoutExtension.AsChar() ).AsChar() ) )
		{
			WARN_EDITOR( TXT( "Could not create 'input' directory in '%s'" ), dirName.AsChar() );
			return;
		}
		if ( !GSystemIO.CreateDirectory( String::Printf( TXT("%s\\%s_output"), dirName.AsChar(), nameWithoutExtension.AsChar() ).AsChar() ) )
		{
			WARN_EDITOR( TXT( "Could not create 'output' directory in '%s'" ), dirName.AsChar() );
			return;
		}

		TDynArray< String > exportedHeightmapFiles;
		TDynArray< String > exportedColormapFiles;
		// export tiles (from (0,0) to (n,n)) saving the offset
		for ( Uint32 y = minY; y <= maxY; ++y )
		{
			for ( Uint32 x = minX; x <= maxX; ++x )
			{
				if ( m_tilesSelection[ y * tilesPerEdge + x ] )
				{
					String hmFileName;
					String cmFileName;
					m_world->GetTerrain()->OnExportTile( x, y, minX, minY, dirName, nameWithoutExtension, exportHM, hmFileName, exportCM, cmFileName );
					if ( exportHM )
					{
						exportedHeightmapFiles.PushBack( hmFileName );
					}
					if ( exportCM )
					{
						exportedColormapFiles.PushBack( cmFileName );
					}
				}
			}
		}

		// prepare csv with description
		SaveAdditionalInfo( minX, minY, maxX - minX + 1, maxY - minY + 1, exportedHeightmapFiles, exportedColormapFiles, dirName, nameWithoutExtension );
	}
}

void CEdTerrainEditTool::OnImportTiles( wxCommandEvent& event )
{
	CEdFileDialog openDlg;

	openDlg.AddFormat( TXT("csv"), TXT("CSV") );
	openDlg.SetMultiselection( !false );
	openDlg.SetIniTag( TXT("CEdTerrainEditTool_ImportTiles") );

	if ( openDlg.DoOpen( m_dialog->GetHandle(), true ) )
	{
		Uint32 offsetX = 0;
		Uint32 offsetY = 0;
		Uint32 sizeX = 0;
		Uint32 sizeY = 0;
		TDynArray< String > tilesToImport;
		ReadAdditionalInfo( openDlg.GetFile(), offsetX, offsetY, sizeX, sizeY, tilesToImport );

		// Remove extension
		String filename = GetBareFilename( openDlg.GetFileName() );
		String directory = openDlg.GetFileDirectory();

		for ( Uint32 x = 0; x < sizeX; ++x )
		{
			for ( Uint32 y = 0; y < sizeY; ++y )
			{
				Bool importHM;
				Bool importCM;
				GetImportConfiguration( importHM, importCM );

				// for filenames like "skellige_islands_x05_y03.png"
				String importHMPathLong  = String::Printf( TXT( "%s\\%s_input\\%s_x%s%d_y%s%d.png" ),	directory.AsChar(), 
					filename.AsChar(),
					filename.AsChar(),
					x < 10 ? TXT("0") : TXT(""),
					x,
					y < 10 ? TXT("0") : TXT(""),
					y );
				// for filenames like "skellige_islands_x5_y3.png"
				String importHMPathShort  = String::Printf( TXT( "%s\\%s_input\\%s_x%d_y%d.png" ),	directory.AsChar(),
					filename.AsChar(),
					filename.AsChar(),
					x,
					y );
					
				// for filenames like "skellige_islands_cm_x05_y03.png"
				String importCMPathLong  = String::Printf( TXT( "%s\\%s_input\\%s_cm_x%s%d_y%s%d.png" ),	directory.AsChar(), 
					filename.AsChar(),
					filename.AsChar(),
					x < 10 ? TXT("0") : TXT(""),
					x,
					y < 10 ? TXT("0") : TXT(""),
					y );
				// for filenames like "skellige_islands_cm_x5_y3.png"
				String importCMPathShort  = String::Printf( TXT( "%s\\%s_input\\%s_cm_x%d_y%d.png" ),	directory.AsChar(),
					filename.AsChar(),
					filename.AsChar(),
					x,
					y );

				Bool hmOk = false;
				Bool cmOk = false;

				ImportStatus importStatus = m_world->GetTerrain()->ImportTile( x + offsetX, y + offsetY, importHM, importHMPathShort, importCM, importCMPathShort );
				if ( importHM )
				{
					if ( importStatus & EIS_HeightmapImported )
					{
						// heightmap imported, prevent from importing again
						importHM = false;
						hmOk = true;
						LOG_EDITOR( TXT("    Heightmap loaded from [%s]"), importHMPathShort.AsChar() );
					}
					else
					{
						WARN_EDITOR( TXT("    Heightmap not found under [%s]"), importHMPathShort.AsChar() );
					}
				}
				else
				{
					hmOk = true;
				}
				if ( importCM )
				{
					if ( importStatus & EIS_ColormapImported )
					{
						// colormap imported, prevent from importing again
						importCM = false;
						cmOk = true;
						LOG_EDITOR( TXT("    Colormap loaded from [%s]"), importCMPathShort.AsChar() );
					}
					else
					{
						WARN_EDITOR( TXT("    Colormap not found under [%s]"), importCMPathShort.AsChar() );
					}
				}
				else
				{
					cmOk = true;
				}
				
				if ( hmOk && cmOk )
				{
					// all ok, continue importing
					continue;
				}
				
				// short paths failed, try with long ones
				importStatus = m_world->GetTerrain()->ImportTile( x + offsetX, y + offsetY, importHM, importHMPathLong, importCM, importCMPathLong );
				if ( importHM )
				{
					if ( importStatus & EIS_HeightmapImported )
					{
						LOG_EDITOR( TXT("    Heightmap loaded from [%s]"), importHMPathLong.AsChar() );
					}
					else
					{
						WARN_EDITOR( TXT("    Heightmap not found under [%s]"), importHMPathLong.AsChar() );
					}
				}

				if ( importCM )
				{
					if ( importStatus & EIS_ColormapImported )
					{
						LOG_EDITOR( TXT("    Colormap loaded from [%s]"), importCMPathLong.AsChar() );
					}
					else
					{
						WARN_EDITOR( TXT("    Colormap not found under [%s]"), importCMPathLong.AsChar() );
					}
				}
			}
		}
	}
}

void CEdTerrainEditTool::OnResetSelection( wxCommandEvent& event )
{
	Uint32 tilesCount = m_world->GetTerrain()->GetNumTilesPerEdge() * m_world->GetTerrain()->GetNumTilesPerEdge();
	for ( Uint32 i = 0; i < tilesCount; ++i )
	{
		m_tilesSelection[ i ] = false;
	}
}

void CEdTerrainEditTool::OnSetupChanged( wxCommandEvent& event )
{
	Int32 numTilesPerEdge = m_numTilesPerEdgeChoice->GetSelection() + 1;
	Int32 lodSelection = m_lodConfigChoice->GetSelection();
	String sizeStr( m_sizeText->GetValue().wc_str() );
	String minHeightStr( m_minHeightText->GetValue().wc_str() );
	String maxHeightStr( m_maxHeightText->GetValue().wc_str() );

	Float size;
	Float minHeight, maxHeight;

	if ( numTilesPerEdge > 0 && lodSelection != -1 && FromString( sizeStr, size ) && FromString( minHeightStr, minHeight ) && FromString( maxHeightStr, maxHeight ) )
	{
		String summary;

		// Compile new terrain parameters set based on the user input
		m_terrainParameters.clipSize = g_clipMapConfig[lodSelection][0];
		m_terrainParameters.tileRes = g_clipMapConfig[lodSelection][1];
		m_terrainParameters.clipmapSize = numTilesPerEdge * m_terrainParameters.tileRes;
		m_terrainParameters.terrainSize = size;
		m_terrainParameters.lowestElevation = minHeight;
		m_terrainParameters.highestElevation = maxHeight;

		// Compile summary for user consideration
		GenerateTerrainParametersSummary( m_terrainParameters, summary );
		m_summary->SetValue( summary.AsChar() );

		// If the changes doesn't require a rebuild of terrain data, apply them now
		if ( m_world->GetTerrain() && m_world->GetTerrain()->IsCompatible( m_terrainParameters ) )
		{
			m_world->GetTerrain()->Reinit( m_terrainParameters );

			m_heightWorkingBuffers.ClearAll();
		}


		m_cursor.SetTexelsPerUnit( m_terrainParameters.clipmapSize / m_terrainParameters.terrainSize );
	}
	else
	{
		// Values not set properly
		return;
	}
}

void CEdTerrainEditTool::OnImportButtonHit( wxCommandEvent& event )
{
	// Create terrain if doesn't exist
	CClipMap* terrainClipMap = m_world->GetTerrain();
	Bool wasCreated = terrainClipMap != NULL;
	if ( !terrainClipMap )
	{
		terrainClipMap = m_world->CreateTerrain();
	}

	wxDirDialog dirDialog(m_dialog);
	dirDialog.SetPath( m_world->GetFile()->GetDirectory()->GetAbsolutePath().AsChar() );

	if ( dirDialog.ShowModal() != wxID_OK )
	{
		return;
	}

	wxDir dir;
	if ( dir.Open( dirDialog.GetPath() ) )
	{
		Uint32 tileRes;
		Uint32 tilesPerSide;
		String warningMessage;

		// Look for valid source tile set in given location
		if ( ValidateDirectory( dir, TXT("*.png"), tileRes, tilesPerSide, warningMessage ) )
		{
			// Tile set seems valid. Fill list of file paths for import.
			RefillTilePathsArray( dir, TXT( "*.png" ) );

			// Determine clipmap parameters
			Int32 numTiles = tilesPerSide * tilesPerSide;
			Int32 resolution = tilesPerSide * tileRes;

			// Check if setup is supported
			Bool supported = true;
			supported &= IsResolutionSupported( resolution );
			supported &= IsTileResolutionSupported( tileRes );
			if ( supported )
			{
				// Create new terrain parameters to match source tiles configuration
				SClipmapParameters importParams = m_terrainParameters;
				Int32 tileResIndex = GetFirstClipmapConfigIndexForTileResolution( tileRes );
				ASSERT( tileResIndex >= 0 );
				importParams.clipmapSize = tilesPerSide * tileRes;
				importParams.clipSize = g_clipMapConfig[tileResIndex][0];
				importParams.tileRes = g_clipMapConfig[tileResIndex][1];

				// Build summary for user confirmation
				String summary;
				summary += TXT( "The set of tiles seems to be ok. It will generate the terrain with the following characteristics:\n" );
				GenerateTerrainParametersSummary( importParams, summary );
				if ( wasCreated )
				{
					summary += TXT("Importing the terrain, will cause the previous terrain data to be revered and deleted. Are you sure you want to proceed? ");
				}

				// Show the summary, and import data if the confirmation is received
				Int32 answer = wxMessageBox( summary.AsChar(), TXT("Import summary"), wxYES_NO, m_dialog );
				if ( answer == wxYES )
				{
					// Setup from scratch
					if ( terrainClipMap->Reinit( importParams ) )
					{
						m_heightWorkingBuffers.ClearAll();

						// Import
						m_world->GetTerrain()->ImportTerrain( m_tilePaths );

						// Remember new settings
						m_terrainParameters = importParams;

						// Feed controls to display new settings
						UpdateTerrainParamsControls();

						// Reinitialize tile selection
						InitializeSelectionArray();
					}
					else
					{
						wxMessageBox( wxString( TXT("CEdTerrainEditTool::OnImportButtonHit() : Failed to reinit terrain clipmap. To rebuild the terrain from scratch, you need to have it fully checked out. That means both the CWorld and all CTerrainTile files.") ) );
					}
				}
			}
		}
		else
		{
			wxMessageBox( wxString( warningMessage.AsChar() ) );
		}
	}
	else
	{
		wxMessageBox( wxString( String::Printf( TXT( "Cannot open directory '%s'" ), dirDialog.GetPath() ).AsChar() ) );
	}
}

void CEdTerrainEditTool::OnDebugImportPuget( wxCommandEvent& event )
{
	CClipMap* terrainClipMap = m_world->GetTerrain();
	if ( !terrainClipMap )
	{
		terrainClipMap = m_world->CreateTerrain();
	}

	// Setup from scratch
	if ( !terrainClipMap->Reinit( m_terrainParameters ) )
	{
		wxMessageBox( wxString( TXT("CEdTerrainEditTool::OnDebugImportPuget() : Failed to reinit terrain clipmap. To rebuild the terrain from scratch, you need to have it fully checked out. That means both the CWorld and all CTerrainTile files.") ) );
		return;
	}

	m_heightWorkingBuffers.ClearAll();

	// Build tiles and Import
	m_world->GetTerrain()->Test_ImportTerrain();
}


void CEdTerrainEditTool::OnCreateTerrain( wxCommandEvent& event )
{
	CClipMap* terrainClipMap = m_world->GetTerrain();

	if ( terrainClipMap )
	{
		Int32 answer = wxMessageBox( TXT("Creating the terrain will revert and delete tiles that fall outside of the bounds of the new terrain. Are you sure you want to proceed?"), TXT("Create terrain"), wxYES_NO, m_dialog );
		if ( answer == wxNO )
		{
			return;
		}
	}

	if ( !terrainClipMap )
	{
		terrainClipMap = m_world->CreateTerrain();
	}

	// Setup from scratch
	if ( !terrainClipMap->Reinit( m_terrainParameters ) )
	{
		wxMessageBox( wxString( TXT("CEdTerrainEditTool::OnCreateFlatTerrain() : Failed to reinit terrain clipmap. To rebuild the terrain from scratch, you need to have it fully checked out. That means both the CWorld and all CTerrainTile files.") ) );
		return;
	}

	m_heightWorkingBuffers.ClearAll();

	InitializeSelectionArray();
}

void CEdTerrainEditTool::OnSetMaterial( wxCommandEvent& event )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}
	String activeMaterialName;
	if ( GetActiveResource( activeMaterialName ) )
	{
		IMaterial* material = Cast< IMaterial >( GDepot->LoadResource( activeMaterialName ) );
		if ( material )
		{
			m_world->GetTerrain()->SetMaterial( material );
			m_currentMaterialText->SetValue( material->GetDepotPath().AsChar() );
		}
	}
	UpdateMaterialTexturesGrid();
}

void CEdTerrainEditTool::OnProbabilityChanged( wxCommandEvent& event )
{
	UpdatePresetData();
}

void CEdTerrainEditTool::OnMaskCheckboxChange( wxCommandEvent& event )
{
	UpdatePresetData();
}

void CEdTerrainEditTool::OnToolButtonClicked(wxCommandEvent& event)
{
	if ( !m_world )
	{
		GFeedback->ShowMsg( TXT( "Error - No world" ), TXT( "Cannot select terrain edit tools, when there is no world loaded." ) );
		return;
	}
	if ( !m_world->GetTerrain() )
	{
		GFeedback->ShowMsg( TXT( "Error - No terrain" ), TXT( "Cannot select terrain edit tools, when there is no terrain in the current world." ) );
		return;
	}

	// Find which button we clicked.
	EToolType toolType = TT_None;
	for ( Uint32 i = 0; i < TT_Max; ++i )
	{
		if ( m_toolButtons[ i ] == event.GetEventObject() )
		{
			if ( m_toolButtons[ i ]->GetValue() )
			{
				toolType = ( EToolType )i;
			}
			break;
		}
	}

	// Wasn't one of our actual buttons, so check the "brush shape" toolbar.
	if ( toolType == TT_None )
	{
		if ( event.GetEventObject() == m_brushShapesToolbar )
		{
			// Only activate the tool if the toolbar button is on. Otherwise, we'll set TT_None to
			// deselect all tools.
			if ( m_brushShapesToolbar->GetToolState( event.GetId() ) )
			{
				toolType = static_cast< EToolType >( event.GetId() );
			}
		}
	}

	SelectTool( toolType );
}


void CEdTerrainEditTool::OnToolButtonEnter(wxCommandEvent& event)
{
	// Set a new tooltip based on which tool the mouse is over.
	Int32 tool = event.GetSelection();
	if ( tool != -1 )
	{
		ASSERT( tool >= 0 && tool < TT_Max );
		m_brushShapesToolbar->SetToolTip( GetToolTooltip( static_cast< EToolType >( tool ) ) );
	}
	else
	{
		m_brushShapesToolbar->SetToolTip( wxT("") );
	}
}


void CEdTerrainEditTool::OnChangeHeightmap(wxCommandEvent& event)
{
	CEdFileDialog openDlg;

	openDlg.AddFormat( TXT("png"), TXT("PNG Image") );
	openDlg.SetMultiselection( false );
	openDlg.SetIniTag( TXT("CEdTerrainEditTool_Heightmap") );

	if ( openDlg.DoOpen( m_dialog->GetHandle(), true ) )
	{
		String path = openDlg.GetFile();
		if ( m_cursor.LoadStampHeightmap( path ) )
		{
			SetStampOffsetControlsEnabled( false );

			m_world->GetTerrain()->SetStampData( m_cursor.m_heightmapPath );
			m_world->GetTerrain()->SetStampModeAdditive( true );
			SetBrushThumbnail();
		}
	}
}


void CEdTerrainEditTool::OnHeightmapButtonSized( wxSizeEvent& event )
{
	// When the button changes size, make sure the image is updated to match.
	SetBrushThumbnail();
}


void CEdTerrainEditTool::SelectTool( EToolType toolType )
{
	ASSERT( toolType >= 0 && toolType < TT_Max, TXT("Trying to set invalid terrain brush type: %d"), toolType );
	if ( toolType < 0 || toolType >= TT_Max )
	{
		toolType = TT_None;
	}
	m_cursor.m_limitsAvailable = ( toolType != TT_Stamp );

	if ( toolType == m_cursor.m_toolType )
	{
		return;
	}

	// Store settings for the old tool.
	m_sessionToolParams[ m_cursor.m_toolType ] = GetCurrControlParams();

	EnableDisableShapeBrushControls( toolType );

	// Clean up previous tool
	switch ( m_cursor.m_toolType )
	{
	case TT_Stamp:
		SetStampOffsetControlsEnabled( false );
		m_cursor.ClearStamp();
		m_world->GetTerrain()->ClearStampData();
		break;
	}

	// Set new tool
	m_cursor.m_toolType = toolType;

	UpdateBrushControlRanges();

	SetControlParams( m_sessionToolParams[ toolType ] );

	m_cursor.m_falloffCurve = m_falloffCurve;
	m_cursor.SetUseFalloffCurve( m_useFalloffCheckBox->GetValue() );

	switch ( m_cursor.m_toolType )
	{
	case TT_PaintColor:
		m_cursor.SetUseFalloffCurve( m_colorUseFalloffCheckBox->GetValue() );
		m_cursor.m_falloffCurve = m_colorFalloffCurve;
		break;

	case TT_PaintTexture:
		m_cursor.SetUseFalloffCurve( false );
		break;

	case TT_Stamp:
		// If we have a heightmap then it was loaded above, so we can set up the stamp.
		if ( !m_cursor.m_heightmapPath.Empty() )
		{
			m_world->GetTerrain()->SetStampData( m_cursor.m_heightmapPath );
		}
		m_world->GetTerrain()->SetStampModeAdditive( !m_cursor.m_heightmapPath.Empty() );
		SetBrushThumbnail();
		break;

	case TT_CollForceOn:
	case TT_CollForceOff:
	case TT_CollReset:
		UpdateTerrainCollisionMask();
		break;
	}

	// Switch to appropriate tab, if we aren't already there.
	if ( m_cursor.m_toolType != TT_None && m_toolButtons[ m_cursor.m_toolType ] )
	{
		size_t numPages = m_editorTabs->GetPageCount();
		for ( size_t i = 0; i < numPages; ++i )
		{
			wxWindow* page = m_editorTabs->GetPage( i );
			if ( page != m_editorTabs->GetCurrentPage() && page->IsDescendant( m_toolButtons[ m_cursor.m_toolType ] ) )
			{
				m_editorTabs->SetSelection( i );
				break;
			}
		}
	}

	// Make sure only the current tool's button is selected.
	for ( Uint32 i = 0; i < TT_Max; ++i ) 
	{
		if ( m_toolButtons[ i ] )
		{
			if ( i != toolType )
			{
				m_toolButtons[ i ]->SetValue( false );
			}
			else
			{
				m_toolButtons[ i ]->SetValue( true );
			}
		}
		else
		{
			// Also update brush shape toolbar, in case this tool is on there.
			m_brushShapesToolbar->ToggleTool( i, i == toolType );
		}
	}
}

void CEdTerrainEditTool::OnAddMaterial(wxCommandEvent& event)
{
	// add new material to atlas/array
}

void CEdTerrainEditTool::SetBrushThumbnail(void)
{
	wxSize size = m_heightmapButton->GetSize();
	Int32 imgSize = Max< Int32 >( 1, Min< Int32 >( size.GetWidth(), size.GetHeight() ) - 10 );
	wxImage b( imgSize, imgSize );

	if ( m_cursor.m_toolType == TT_Stamp && m_cursor.m_stampHeightMap && ( m_cursor.m_stampDataFlags != 0 ) )
	{
		for ( Int32 y = 0; y < imgSize; ++y )
		{
			Uint32 srcY = (Uint32)(( (Float)y / imgSize ) * m_cursor.m_texelsPerEdge);
			for ( Int32 x = 0; x < imgSize; ++x )
			{
				Uint32 srcX = (Uint32)(( (Float)x / imgSize ) * m_cursor.m_texelsPerEdge);
				Uint32 index = srcY * m_cursor.m_texelsPerEdge + srcX;
				Uint8 grey = ( m_cursor.m_stampHeightMap[ index ] / 65536.f ) * 255;

				// If we have color data as well, include it in the thumbnail...
				if ( m_cursor.m_stampColorMap )
				{
					Uint32 colX = (Uint32)(( (Float)x / imgSize ) * m_cursor.m_stampColorTexelsPerEdge);
					Uint32 colY = (Uint32)(( (Float)y / imgSize ) * m_cursor.m_stampColorTexelsPerEdge);
					Uint32 col = m_cursor.m_stampColorMap[ colY * m_cursor.m_stampColorTexelsPerEdge + colX ];
					Uint8 cr = ( col ) & 0xFF;
					Uint8 cg = ( col >> 8 ) & 0xFF;
					Uint8 cb = ( col >> 16 ) & 0xFF;

					cr *= ( Float )grey / 255.0f;
					cg *= ( Float )grey / 255.0f;
					cb *= ( Float )grey / 255.0f;
					b.SetRGB( x, y, cr, cg, cb );
				}
				else
				{
					b.SetRGB( x, y, grey, grey, grey );
				}
			}
		}
	}
	else
	{
		for ( Int32 y = 0; y < imgSize; ++y )
		{
			for ( Int32 x = 0; x < imgSize; ++x )
			{
				b.SetRGB( x, y, 0, 0, 0 );
			}
		}
	}

	wxBitmap wb = wxBitmap( b, 32 );
	m_heightmapButton->SetBitmap( wb );
}

String CEdTerrainEditTool::GetCaption() const
{
	return TXT("Terrain Edit Tools");
}

void CEdTerrainEditTool::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( OnActiveResChanged ) )
	{
		// Selection has changed
		SetBrushThumbnail();
	}
	else if ( name == CNAME( EditorPostUndoStep ) )
	{
		// EditorPostUndoStep covers both undo and redo.
		if ( GetEventData< CEdUndoManager* >( data ) == m_viewport->GetUndoManager() )
		{
			// If the undo was by our undo manager, need to refresh our cached float height buffers, in case any of them were changed...
			m_heightWorkingBuffers.RefreshFromTerrain();

			// Refresh terrain shadows...
			( new CRenderCommand_UpdateTerrainShadows( m_world->GetRenderSceneEx() ) )->Commit();

			// Also, save tiles if we're supposed to be continuously saving...
			if ( m_saveStrokesCheckBox->GetValue() )
			{
				m_world->GetTerrain()->SaveAllTiles();
			}

			{
				// If we've got the Collision tab open, update mask, in case the Undo change collision for some tiles.
				if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_collisionPanel", wxPanel ) )
				{
					UpdateTerrainCollisionMask();
				}
			}

		}
	}
}


wxString CEdTerrainEditTool::GetToolTooltip( EToolType toolType )
{
	switch ( toolType )
	{
	case TT_RiseLower:
		return
			"Rise Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Shift:\t\tSubtract\n"
			"  Ctrl+Wheel:\tHeight"
			;

	case TT_Flatten:
		return
			"Flatten Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Ctrl+Click:\tPick height\n"
			"  Ctrl+Wheel:\tHeight"
			;

	case TT_Slope:
		return
			"Slope Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Click:\tPick slope\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Ctrl:\t\tRotate brush\n"
			"  Alt:\t\tSlope angle\n"
			"  Ctrl+Alt:\tSlope offset"
			;

	case TT_Smooth:
		return
			"Smooth Brush\n"
			"  Shift:\t\tNoise\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Ctrl+Wheel:\tFilter size"
			;

	case TT_Stamp:
		return
			"Stamp Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Ctrl:\t\tRotate brush\n"
			"  Ctrl+Wheel:\tOffset\n"
			"  Shift+Wheel:\tScale\n"
			"  Ctrl+Shift+Wheel: Scaling origin\n"
			"  Tilde:\t\tToggle add/subtract (loaded heightmap image)\n"
			"  Alt+Click:\tCapture from terrain\n"
			"  Shift+Click:\tGet Scaling origin from terrain\n"
			"  /:\t\tClear stamp"
			;

	case TT_PaintTerrainHoles:
		return
			"Hole Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Wheel:\tIntensity"
			;

	case TT_Melt:
		return
			"Melt Brush\n"
			"  Wheel:\t\tBrush size\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Ctrl+Wheel:\tFilter size"
			;

	case TT_PaintColor:
		return
			"Color Paint\n"
			"  Wheel:\t\tBrush Size\n"
			"  Shift+Wheel:\tIntensity\n"
			"  Ctrl+Wheel:\tBrightness"
			;

	case TT_PaintTexture:
		return
			"Texture Paint\n"
			"  Ctrl+Click:\tPick from terrain"
			;

	case TT_CollForceOn:	return "Force terrain collision on";
	case TT_CollForceOff:	return "Force terrain collision off";
	case TT_CollReset:		return "Allow auto collision";

	default:
		HALT( "Invalid ToolType: %d", toolType );
		return "";
	}
}


Bool CEdTerrainEditTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// No world
	if ( !world )
	{
		return false;
	}

	// Remember what we are editing
	m_world = world;

	// Remember render panel containing terrain view
	m_viewport = viewport;

	// Load layout from XRC
	//m_dialog = wxXmlResource::Get()->LoadFrame( this, "m_TerrainEditorTool" );	
	m_dialog = wxXmlResource::Get()->LoadFrame( wxTheFrame, "m_TerrainEditorFrame" );	

	// Add event handler for closing the window
	m_dialog->Bind( wxEVT_CLOSE_WINDOW, &CEdTerrainEditTool::OnClose, this );
	m_dialog->Bind( wxEVT_UPDATE_UI, &CEdTerrainEditTool::OnUpdateUI, this );

	// Load layout of string input box
	m_InputDialog = wxXmlResource::Get()->LoadDialog( m_dialog, "m_TT_InputStringDialog" );
	m_presetsConfigurationDlg = wxXmlResource::Get()->LoadDialog( m_dialog, "m_TT_PresetsConfigurationDialog" );

	for ( Uint32 i = 0; i < 6; ++i )
	{
		wxString name;
		name.Printf( "m_preset%dCBox", i + 1 );
		wxCheckBox* cbox = XRCCTRL( *m_presetsConfigurationDlg, name, wxCheckBox );
		m_presetCheckBoxes.PushBack( cbox );
	}
	
	// set asset browser resource changed listener for thumbnail updates
	SEvents::GetInstance().RegisterListener( CNAME( OnActiveResChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPostUndoStep ), this );

	m_editorTabs = XRCCTRL( *m_dialog, "m_notebook1", wxNotebook );


	m_editorTabs->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING, wxCommandEventHandler( CEdTerrainEditTool::OnPageChanging ), NULL, this );
	m_editorTabs->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler( CEdTerrainEditTool::OnPageChanged ), NULL, this );

	m_useFalloffCheckBox = XRCCTRL( *m_dialog, "m_useFalloff", wxCheckBox );
	m_useFalloffCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::ToggleUseFalloff ), NULL, this );

	m_useMaterialTextureMasking = XRCCTRL( *m_dialog, "m_useMaterialTextureMasking", wxCheckBox );
	m_brushProbability = XRCCTRL( *m_dialog, "m_brushProbability", wxSlider );

	m_outsideRadiusControl.Init(	m_dialog, TXT("m_radius"),		   0.1f,	 2000,  50, &m_cursor.m_brushSize					);
	m_intensityControl.Init(		m_dialog, TXT("m_intensity"),	      0,		1,   1, &m_cursor.m_intensity					);
	m_heightControl.Init(			m_dialog, TXT("m_height"),		      0,	65535, 100, &m_cursor.m_desiredElevation			);
	m_slopeAngleControl.Init(		m_dialog, TXT("m_angle"),		      0,	  360,   0, &m_cursor.m_slopeParams.yawDegrees		);
	m_slopeControl.Init(			m_dialog, TXT("m_slope"),		      0,	   85,   0, &m_cursor.m_slopeParams.slopeDegrees	);
	m_slopeOffsetControl.Init(		m_dialog, TXT("m_slopeOffset"),	   -500,	  500,   0, &m_cursor.m_slopeParams.offset			);
	m_stampAngleControl.Init(		m_dialog, TXT("m_stampAngle"),	      0,	  360,   0, &m_cursor.m_stampParams.yawDegrees		);
	m_stampScaleOriginControl.Init(	m_dialog, TXT("m_stampOrigin"),	   -500,	  500,   0, &m_cursor.m_stampScaleOrigin			);
	m_filterSizeControl.Init(		m_dialog, TXT("m_filterSize"),	      1,	   20,   1, &m_cursor.m_filterSize,					1.0f );
	m_materialRadiusControl.Init(	m_dialog, TXT("m_materialRadius"), 0.1f,	 2000,  50, &m_cursor.m_brushSize					);
	m_colorRadiusControl.Init(		m_dialog, TXT("m_colorRadius"),	   0.1f,	 2000,  50, &m_cursor.m_brushSize					);
	m_colorIntensityControl.Init(	m_dialog, TXT("m_colorIntensity"),    0,		1,   1, &m_cursor.m_intensity					);

	m_heightmapButton = XRCCTRL( *m_dialog, "m_heightmapButton", wxBitmapButton );
	m_heightmapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnChangeHeightmap ), NULL, this );
	m_heightmapButton->Connect( wxEVT_SIZE, wxSizeEventHandler( CEdTerrainEditTool::OnHeightmapButtonSized ), NULL, this );
	m_heightmapButton->Connect( wxEVT_SIZING, wxSizeEventHandler( CEdTerrainEditTool::OnHeightmapButtonSized ), NULL, this );

	// layers stuff
	XRCCTRL( *m_dialog, "m_AddNewLayerButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnModifyLayers ), NULL, this );	
	XRCCTRL( *m_dialog, "m_DeleteLayerButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnModifyLayers ), NULL, this );
	XRCCTRL( *m_dialog, "m_MergeLayersButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnModifyLayers ), NULL, this );

	m_substepPainting = XRCCTRL( *m_dialog, "m_substepPainting", wxCheckBox );
	m_drawBrushAsOverlay = XRCCTRL( *m_dialog, "m_drawBrushOverlay", wxCheckBox );
	m_updateShadowsCheckBox = XRCCTRL( *m_dialog, "m_updateShadowsCheckBox", wxCheckBox );

	// saving stuff
	XRCCTRL( *m_dialog, "m_saveButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnSave ), NULL, this );	
	m_saveStrokesCheckBox = XRCCTRL( *m_dialog, "m_saveStrokesCheckBox", wxCheckBox );

	// saving stuff
	XRCCTRL( *m_dialog, "m_selectAllButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnSelectAll ), NULL, this );	

	// input dialog stuff
	XRCCTRL( *m_dialog, "m_InputNameOKButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnInputEntry ), NULL, this );
	XRCCTRL( *m_dialog, "m_InputNameCancelButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnInputEntry ), NULL, this );
	
	// saving falloff presets
	XRCCTRL( *m_dialog, "m_InputFalloffSaveButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnFalloffPreset ), NULL, this );
	XRCCTRL( *m_dialog, "m_InputFalloffRemoveButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnFalloffPreset ), NULL, this );

	// saving selectors
	XRCCTRL( *m_dialog, "m_InputSelectorAddButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnModifySelectors ), NULL, this );
	XRCCTRL( *m_dialog, "m_InputSelectorRemoveButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnModifySelectors ), NULL, this );	

	// button for import of the terrain tiles
	XRCCTRL( *m_dialog, "m_importButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnImportButtonHit ), NULL, this );

	// button for the DEBUG import of puget sound data
	XRCCTRL( *m_dialog, "m_debugImportButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnDebugImportPuget ), NULL, this );

	XRCCTRL( *m_dialog, "m_exportTilesButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnExportTiles ), NULL, this );
	XRCCTRL( *m_dialog, "m_importTilesButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnImportTiles ), NULL, this );
	XRCCTRL( *m_dialog, "m_resetSelectionButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnResetSelection ), NULL, this );

	XRCCTRL( *m_dialog, "m_setMaterialButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnSetMaterial ), NULL, this );

	m_paintProbabilitySlider = XRCCTRL( *m_dialog, "m_paintProbabilitySlider", wxSlider );
	m_paintProbabilitySlider->Bind( wxEVT_COMMAND_SLIDER_UPDATED, &CEdTerrainEditTool::OnProbabilityChanged, this );
	
	m_currentColorButton = XRCCTRL( *m_dialog, "m_currentColor", wxPanel );
	m_currentColorButton->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CEdTerrainEditTool::OnCurrentColorClicked ), NULL, this );
	m_currentColorButton->Connect( wxEVT_PAINT, wxPaintEventHandler( CEdTerrainEditTool::OnPaintColorPanel), NULL, this );


	wxPanel* colorPresetContainer = XRCCTRL( *m_dialog, "m_colorPresetContainer", wxPanel );
	wxSizer* colorPresetRows[2] = {
		new wxBoxSizer( wxHORIZONTAL ),
		new wxBoxSizer( wxHORIZONTAL )
	};
	colorPresetContainer->GetSizer()->Add( colorPresetRows[ 0 ], 1 );
	colorPresetContainer->GetSizer()->Add( colorPresetRows[ 1 ], 1 );
	for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
	{
		SetColorPreset( i, DefaultColorPresets[ i ] );

		m_colorPresetButtons[ i ] = new wxPanel( colorPresetContainer, wxID_ANY, wxDefaultPosition, wxSize( 32, 32 ), wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE );
		m_colorPresetButtons[ i ]->SetBackgroundStyle( wxBG_STYLE_PAINT );

		colorPresetRows[ i / ( NUM_COLOR_PRESETS / 2 ) ]->Add( m_colorPresetButtons[ i ] );

		m_colorPresetButtons[ i ]->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CEdTerrainEditTool::OnColorPresetClicked ), NULL, this );
		m_colorPresetButtons[ i ]->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CEdTerrainEditTool::OnColorPresetRightClicked ), NULL, this );
		m_colorPresetButtons[ i ]->Connect( wxEVT_PAINT, wxPaintEventHandler( CEdTerrainEditTool::OnPaintColorPanel), NULL, this );
	}

	wxButton* saveColorPresetsBtn = XRCCTRL( *m_dialog, "m_saveColorPresets", wxButton );
	saveColorPresetsBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnSaveColorPresets ), NULL, this );
	wxButton* loadColorPresetsBtn = XRCCTRL( *m_dialog, "m_loadColorPresets", wxButton );
	loadColorPresetsBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnLoadColorPresets ), NULL, this );

	m_createButton = XRCCTRL( *m_dialog, "m_createTerrainButton", wxButton );
	m_createButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnCreateTerrain ), NULL, this );

	// basic terrain config
	m_numTilesPerEdgeChoice = XRCCTRL( *m_dialog, "m_numTilesPerEdgeChoice", wxChoice );
	m_numTilesPerEdgeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnSetupChanged ), NULL, this );

	m_sizeText = XRCCTRL( *m_dialog, "m_sizeSquareMetersText", wxTextCtrl );
	m_sizeText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnSetupChanged ), NULL, this );

	m_minHeightText = XRCCTRL( *m_dialog, "m_minHeightText", wxTextCtrl );
	m_maxHeightText = XRCCTRL( *m_dialog, "m_maxHeightText", wxTextCtrl );

	m_lodConfigChoice = XRCCTRL( *m_dialog, "m_lodConfigChoice", wxChoice );
	m_lodConfigChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnSetupChanged ), NULL, this );

	m_minHeightText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnSetupChanged ), NULL, this );
	m_maxHeightText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnSetupChanged ), NULL, this );

	m_summary = XRCCTRL( *m_dialog, "m_summaryText", wxTextCtrl );
	m_currentMaterialText = XRCCTRL( *m_dialog, "m_currentMaterialText", wxTextCtrl );

	// Texture array grid
	m_textureArrayGridPanel = XRCCTRL( *m_dialog, "m_textureArrayGridPanel", wxPanel );
	m_textureArrayGrid = new CEdTextureArrayGrid( m_textureArrayGridPanel, 0 );
	m_textureArrayGrid->SetAllowSecondary( true );
	m_textureArrayGrid->SetHook( this );
	wxBoxSizer* textureArrayGridBoxSizer = new wxBoxSizer( wxVERTICAL );
	m_textureArrayGridPanel->SetSizer( textureArrayGridBoxSizer );
	textureArrayGridBoxSizer->Add( m_textureArrayGrid, 1, wxEXPAND );

	// Texture parameters
	m_textureParametersPanel = XRCCTRL( *m_dialog, "m_textureParametersPanel", wxPanel );

	// UV multipliers
	m_verticalUVScaleMask = XRCCTRL( *m_dialog, "m_verticalUVScaleMask", wxCheckBox );
	m_verticalUVScaleMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnMaskCheckboxChange ), NULL, this );
	m_uvMultVerticalSpin = XRCCTRL( *m_dialog, "m_uvMultVerticalSpin", wxSpinCtrl );
	m_uvMultVerticalSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnUVMultVerticalSpin ), NULL, this );

	m_slopeThresholdMask = XRCCTRL( *m_dialog, "m_slopeThresholdMask", wxCheckBox );
	m_slopeThresholdMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnMaskCheckboxChange ), NULL, this );
	m_paintSlopeThresholdSpin = XRCCTRL( *m_dialog, "m_paintSlopeThresholdSpin", wxSpinCtrl );
	m_paintSlopeThresholdSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnSlopeThresholdSpin ), NULL, this );
	m_slopeThresholdActionChoice = XRCCTRL( *m_dialog, "m_slopeThresholdAction", wxChoice );
	m_slopeThresholdActionChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnSlopeThresholdAction ), NULL, this );

	// Horizontal + vertical texture application masks
	m_horizontalMask = XRCCTRL( *m_dialog, "m_horizontalMask", wxCheckBox );
	m_verticalMask = XRCCTRL( *m_dialog, "m_verticalMask", wxCheckBox );
	m_horizontalMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnMaskCheckboxChange ), NULL, this );
	m_verticalMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnMaskCheckboxChange ), NULL, this );

	// Texture-based masking of the whole operation
	m_useHorizontalTextureMask = XRCCTRL( *m_dialog, "m_useHorizontalTextureMask", wxCheckBox );
	m_useVerticalTextureMask = XRCCTRL( *m_dialog, "m_useVerticalTextureMask", wxCheckBox );
	m_invertHorizontalTextureMask = XRCCTRL( *m_dialog, "m_invertHorizontalTextureMask", wxCheckBox );
	m_invertVerticalTextureMask = XRCCTRL( *m_dialog, "m_invertVerticalTextureMask", wxCheckBox );
	m_horizontalTextureMask = XRCCTRL( *m_dialog, "m_horizontalTextureMask", wxTextCtrl );
	m_verticalTextureMask = XRCCTRL( *m_dialog, "m_verticalTextureMask", wxTextCtrl );
	m_horizontalTextureMaskIndex = m_verticalTextureMaskIndex = 0;

	// Terrain tiles offsetting
	m_colOffsetSpinButton = XRCCTRL( *m_dialog, "m_colOffsetSpinButton", wxSpinButton );
	m_rowOffsetSpinButton = XRCCTRL( *m_dialog, "m_rowOffsetSpinButton", wxSpinButton );
	//XRCCTRL( *this, "m_applyOffsetButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnApplyOffsetButtonHit ), NULL, this );
	m_colOffsetSpinButton->Connect( wxEVT_SPIN_DOWN, wxSpinEventHandler( CEdTerrainEditTool::OnColSpinLeft ), NULL, this );
	m_colOffsetSpinButton->Connect( wxEVT_SPIN_UP, wxSpinEventHandler( CEdTerrainEditTool::OnColSpinRight ), NULL, this );
	m_rowOffsetSpinButton->Connect( wxEVT_SPIN_DOWN, wxSpinEventHandler( CEdTerrainEditTool::OnRowSpinDown ), NULL, this );
	m_rowOffsetSpinButton->Connect( wxEVT_SPIN_UP, wxSpinEventHandler( CEdTerrainEditTool::OnRowSpinUp ), NULL, this );

	// Fill choices
	{
		m_numTilesPerEdgeChoice->Clear();
		for ( Uint32 i=MIN_NUM_TILES_PER_EDGE; i<=MAX_NUM_TILES_PER_EDGE; ++i )
		{
			const String desc = String::Printf( TXT("%i ( %i x %i ) tiles"), i * i, i, i );
			m_numTilesPerEdgeChoice->Append( desc.AsChar() );
		}

		m_lodConfigChoice->Clear();
		for ( Uint32 i=0; i<NUM_CLIPMAP_CONFIGS; ++i )
		{
			const String desc = String::Printf( TXT("%i clip window and %i tile resolution"), g_clipMapConfig[i][0], g_clipMapConfig[i][1] );
			m_lodConfigChoice->Append( desc.AsChar() );
		}
	}
	
	// update selectors, falloff, layers count
	m_FalloffChoice = XRCCTRL( *m_dialog, "m_FalloffPresetChoice", wxChoice );
	m_FallofPresetCounter = m_FalloffChoice->GetCount();

	m_LayerList = XRCCTRL( *m_dialog, "m_LayerListBox", wxCheckListBox );
	m_LayerCounter = m_LayerList->GetCount();	

	m_SelectorList = XRCCTRL( *m_dialog, "m_SelectorsListBox", wxListBox );
	m_SelectorCounter = m_SelectorList->GetCount();

	// this id is used to identify what function needs input from dialog data
	m_InputDialogParentID = -1;


	m_stampSourceLocation = XRCCTRL( *m_dialog, "m_stampSourceLocationText", wxTextCtrl );
	m_stampOffsetX = XRCCTRL( *m_dialog, "m_stampOfsXEdit", wxTextCtrl );
	m_stampOffsetX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnStampOffsetChanged ), NULL, this );
	m_stampOffsetY = XRCCTRL( *m_dialog, "m_stampOfsYEdit", wxTextCtrl );
	m_stampOffsetY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnStampOffsetChanged ), NULL, this );
	m_stampPaste = XRCCTRL( *m_dialog, "m_pasteStampButton", wxButton );
	m_stampPaste->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnStampPasteClicked ), NULL, this );

	// check the default background layer - make it non removable?
	m_LayerList->Check(0);
	
	if ( m_world && m_world->GetTerrain() )
	{
		CClipMap* clipMap = GGame->GetActiveWorld()->GetTerrain();

		clipMap->GetClipmapParameters( &m_terrainParameters );

		UpdateTerrainParamsControls();

		InitializeSelectionArray();

		if ( clipMap->GetMaterial() )
		{
			m_currentMaterialText->SetValue( clipMap->GetMaterial()->GetDepotPath().AsChar() );
		}

		m_cursor.SetTexelsPerUnit( m_terrainParameters.clipmapSize / m_terrainParameters.terrainSize );
	}
	else
	{
		m_numTilesPerEdgeChoice->SetSelection(0);
		m_lodConfigChoice->SetSelection(0);
		m_sizeText->SetValue("10000");
		m_minHeightText->SetValue( "0" );
		m_maxHeightText->SetValue( "1000" );
	}


	m_colorUseFalloffCheckBox = XRCCTRL(*m_dialog, "m_colorUseFalloff", wxCheckBox);	
	m_colorUseFalloffCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::ToggleUseFalloff ), NULL, this );



	// create default falloff curve
	{
		m_falloffCurve = CreateObject< CCurve >();
		m_falloffCurve->GetCurveData().AddPoint( 0.0f, 0.0f, CST_BezierSmoothSymertric );
		m_falloffCurve->GetCurveData().AddPoint( 1.0f, 1.0f, CST_BezierSmoothSymertric );

		wxPanel* rp = XRCCTRL( *m_dialog, "m_FalloffPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
		m_curveEditor = new CEdCurveEditor( rp );
		sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );

		m_curveEditor->AddCurve( m_falloffCurve, TXT("falloff") );
		m_curveEditor->Connect( wxEVT_COMMAND_CURVE_CHANGED, wxCommandEventHandler( CEdTerrainEditTool::OnFalloffCurveChanged ), NULL, this );
	}


	{
		m_colorFalloffCurve = CreateObject< CCurve >();
		m_colorFalloffCurve->GetCurveData().AddPoint( 0.0f, 0.0f, CST_BezierSmoothSymertric );
		m_colorFalloffCurve->GetCurveData().AddPoint( 1.0f, 1.0f, CST_BezierSmoothSymertric );

		wxPanel* rp = XRCCTRL( *m_dialog, "m_colorFalloffPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_colorCurveEditor = new CEdCurveEditor( rp );
		sizer1->Add( m_colorCurveEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );

		m_colorCurveEditor->AddCurve( m_colorFalloffCurve, TXT("falloff") );
		m_colorCurveEditor->Connect( wxEVT_COMMAND_CURVE_CHANGED, wxCommandEventHandler( CEdTerrainEditTool::OnFalloffCurveChanged ), NULL, this );
	}


	// Setup Tool selector buttons.

	// Texture and Color painting have their own toggle buttons on their respective tabs.
	m_toolButtons[ TT_PaintTexture ] = XRCCTRL( *m_dialog, "m_paintToggle", wxToggleButton );
	m_toolButtons[ TT_PaintColor ] = XRCCTRL( *m_dialog, "m_PaintColorsButton", wxToggleButton );

	// The rest are in a toolbar on the Brush Shape tab. Due to an apparent bug with wxWidgets, we can't just use toggle buttons
	// with bitmaps attached -- the "toggled" state looks identical to "normal". On the plus side, toolbar support "radio" buttons
	// so we don't have to manually turn them off when one is selected.
	wxPanel* shapeBrushWindow = XRCCTRL(*m_dialog, "m_ShapeBrushWindow", wxPanel);
	m_brushShapesToolbar = XRCCTRL(*m_dialog, "m_brushShapes", wxToolBar);
	m_brushShapesToolbar->Connect( wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnToolButtonClicked ), nullptr, this );

	// Turn off the automatic tool tips. They show the "short" tip for each item on the toolbar, which is not long enough to
	// show our tips. Instead, we watch for TOOL_ENTER events, and set the tooltip for the toolbar based on which item the mouse
	// is over.
	// wxFormBuilder doesn't expose this flag, so we make sure it's off manually.
	if ( !m_brushShapesToolbar->HasFlag( wxTB_NO_TOOLTIPS ) )
	{
		m_brushShapesToolbar->ToggleWindowStyle( wxTB_NO_TOOLTIPS );
	}
	m_brushShapesToolbar->Connect( wxEVT_COMMAND_TOOL_ENTER, wxCommandEventHandler( CEdTerrainEditTool::OnToolButtonEnter ), nullptr, this );

	// Add tools to the brush shape toolbar.
	m_brushShapesToolbar->AddCheckTool( TT_RiseLower,			wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_RISE_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_Flatten,				wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_FLATTEN_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_Slope,				wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_SLOPE_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_Smooth,				wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_SMOOTH_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_Melt,				wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_MELT_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_Stamp,				wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_STAMP_ICON") ) );
	m_brushShapesToolbar->AddCheckTool( TT_PaintTerrainHoles,	wxT(""), SEdResources::GetInstance().LoadBitmap( wxT("IMG_TT_HOLE_ICON") ) );

	// Have to call Realize() to make the new items appear.
	m_brushShapesToolbar->Realize();



	// Collision tab
	{
		m_toolButtons[ TT_CollForceOn ] = XRCCTRL( *m_dialog, "m_collForceOn", wxToggleButton );
		m_toolButtons[ TT_CollForceOff ] = XRCCTRL( *m_dialog, "m_collForceOff", wxToggleButton );
		m_toolButtons[ TT_CollReset ] = XRCCTRL( *m_dialog, "m_collReset", wxToggleButton );

		XRCCTRL( *m_dialog, "m_collAuto", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnRefreshAutoCollision ), NULL, this );
		XRCCTRL( *m_dialog, "m_collAutoCurrent", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnRefreshAutoCollisionCurrent ), NULL, this );

		XRCCTRL( *m_dialog, "m_collColorAutoOn", wxPanel )->SetBackgroundColour( wxColour( TerrainTileCollisionColors[ TTC_AutoOn ].ToUint32() ) );
		XRCCTRL( *m_dialog, "m_collColorAutoOff", wxPanel )->SetBackgroundColour( wxColour( TerrainTileCollisionColors[ TTC_AutoOff ].ToUint32() ) );
		XRCCTRL( *m_dialog, "m_collColorForceOn", wxPanel )->SetBackgroundColour( wxColour( TerrainTileCollisionColors[ TTC_ForceOn ].ToUint32() ) );
		XRCCTRL( *m_dialog, "m_collColorForceOff", wxPanel )->SetBackgroundColour( wxColour( TerrainTileCollisionColors[ TTC_ForceOff ].ToUint32() ) );
	}


	// Set the tip for any buttons we have. We don't necessarily have a button for every tool, because most of
	// them are on the brush toolbar. Also connect to CLICKED events on those buttons.
	for ( Uint32 i = 0; i < TT_Max; ++i )
	{
		if ( m_toolButtons[ i ] )
		{
			m_toolButtons[ i ]->SetToolTip( GetToolTooltip( static_cast< EToolType >( i ) ) );
			m_toolButtons[ i ]->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnToolButtonClicked ), NULL, this );
		}
	}

	// Automatic grass brush setting
	{
		m_grassBrushLink = XRCCTRL( *m_dialog, "m_grassBrushLink", wxHyperlinkCtrl );
		m_grassBrushLink->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( CEdTerrainEditTool::OnGrassBrushHyperlinkClicked ), NULL, this );

		XRCCTRL( *m_dialog, "m_setGrassBrushButton", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnSetGrassBrushClicked ), NULL, this );
		XRCCTRL( *m_dialog, "m_removeGrassBrushButton", wxBitmapButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdTerrainEditTool::OnRemoveGrassBrushClicked ), NULL, this );
	}

	// brush z limit controls
	{
		XRCCTRL( *m_dialog, "m_lowLimitCheck" , wxCheckBox )->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdTerrainEditTool::OnLowLimitChoice, this );
		XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdTerrainEditTool::OnHighLimitChoice, this );

		XRCCTRL( *m_dialog, "m_lowLimitEdit" , wxTextCtrl )->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdTerrainEditTool::OnLowLimitText, this );
		XRCCTRL( *m_dialog, "m_highLimitEdit", wxTextCtrl )->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdTerrainEditTool::OnHighLimitText, this );

		for ( Uint32 i = 0; i < 6; ++i )
		{
			wxString name;
			name.Printf( "m_heightPreset%d", i + 1 );
			wxRadioButton* rbtn = XRCCTRL( *m_dialog, name.c_str(), wxRadioButton );
			rbtn->Bind( wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CEdTerrainEditTool::OnLimitPresetSelected, this );
			m_presetsRadioBtns.PushBack( rbtn );
		}

		XRCCTRL( *m_dialog, "m_presetsConfig", wxButton )->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdTerrainEditTool::OnConfigurePresetsBtnClicked, this );
	}

	XRCCTRL( *m_dialog, "m_textureUsageTool", wxButton )->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdTerrainEditTool::OnTextureUsageTool, this );

	// Start out with all sliders disabled.
	EnableDisableShapeBrushControls( TT_None );

	UpdateMaterialTexturesGrid();
	UpdatePresetData();
	UpdateTextureParameters();

	LoadOptionsFromConfig();

	// Make sure there is enough room for the curve
	m_dialog->SetMinClientSize( wxSize( 10, 600 ) );
	m_dialog->SetClientSize( wxSize( 1050, 950 ) );
	LayoutRecursively( m_dialog );

	SetPreset( m_presetIndex );

	m_isStarted = true;

	m_dialog->Show();

	// Start tool
	return true;
}

void CEdTerrainEditTool::End()
{
	if ( m_world && m_world->GetTerrain() && m_world->GetTerrain()->GetTerrainProxy() )
	{
		( new CRenderCommand_ClearTerrainCustomOverlay( m_world->GetTerrain()->GetTerrainProxy() ) )->Commit();
	}

	m_heightWorkingBuffers.ClearAll();


	// cleanup stamp visualization
	if ( m_world && m_world->GetTerrain() )
	{
		m_world->GetTerrain()->ClearStampData();
	}

	SaveOptionsToConfig();

	// Close dialog
	if ( m_dialog )
	{
		m_dialog->Destroy();
	}

	m_isStarted = false;

	SEvents::GetInstance().UnregisterListener( CNAME( OnActiveResChanged ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( EditorPostUndoStep ), this );

	// Clear all data from the cursor.
	m_cursor.ClearAllData();
}

Bool CEdTerrainEditTool::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	m_cursor.SetCursorMode( CM_Paint );

	if ( ProcessCursorClick( view, button, state, x, y ) )
	{
		return true;
	}

	m_paintingRequest = false;
	m_paintingRequestFrames = 0;

	// Let user move gizmo and such
	return false;
}


Bool CEdTerrainEditTool::OnViewportTick( IViewport* view, Float timeDelta )
{
	if ( m_needToUpdateCollisionOverlay )
	{
		UpdateTerrainCollisionMask();
	}

	// update cursor if needed
	m_cursor.Tick();

	// Update interface
	{
		// We don't want the terrain create button to be enabled when terrain of desired parameters is already created.
		CClipMap* terrain  = m_world->GetTerrain();
		if ( terrain )
		{
			SClipmapParameters params;
			terrain->GetClipmapParameters( &params );
			m_createButton->Enable( !terrain->IsCompatible( m_terrainParameters ) );
		}
		else
		{
			m_createButton->Enable( true );
		}
	}


	// We don't get notified when sliders are changed, so we'll just push relevant values for rendering the stamp preview
	if ( m_cursor.m_toolType == TT_Stamp )
	{
		m_world->GetTerrain()->SetStampSize( m_cursor.m_brushSize * 2.0f );
		m_world->GetTerrain()->SetStampCenter( m_cursor.m_position );

		if ( m_cursor.m_stampDataFlags & TESDF_Additive )
		{
			m_world->GetTerrain()->SetStampIntensity( m_cursor.m_intensity * m_cursor.m_stampParams.multiplier );
			m_world->GetTerrain()->SetStampOffset( HeightOffsetToTexels( m_cursor.m_desiredElevation ) / 65535.0f );
		}
		else
		{
			m_world->GetTerrain()->SetStampIntensity( m_cursor.m_intensity );

			Float texelOffset = HeightOffsetToTexels( m_cursor.m_desiredElevation );
			Float texelOrigin = HeightToTexels( m_cursor.m_stampScaleOrigin );
			m_world->GetTerrain()->SetStampOffset( GetScaledOffset( texelOffset, m_cursor.m_intensity, texelOrigin ) / 65535.0f );
		}
		m_world->GetTerrain()->SetStampRotation( m_cursor.m_stampParams.yawDegrees * M_PI / 180.0f );
	}


	Bool paintingResult = ProcessPainting( timeDelta );

	m_heightWorkingBuffers.ProcessTimeouts( timeDelta );

	return paintingResult;
}

Bool CEdTerrainEditTool::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( key == IK_MouseZ )
	{
		switch ( m_cursor.m_toolType )
		{
		case TT_Slope:
			HandleMouseZBrushSlope( view, key, action, data );
			break;
		case TT_Stamp:
			HandleMouseZBrushStamp( view, key, action, data );
			break;
		case TT_PaintColor:
			HandleMouseZBrushPaint( view, key, action, data );
			break;
		case TT_Smooth:
		case TT_Melt:
			HandleMouseZBrushSmoothMelt( view, key, action, data );
			break;
		case TT_PaintTexture:
			HandleMouseZBrushPaintTex( view, key, action, data );
			break;
		default:
			HandleMouseZBrushGeneral( view, key, action, data );
			break;
		}
	}

	switch ( m_cursor.m_toolType )
	{
	case TT_Slope:
		HandleSlopeBrushInput( view, key, action, data );
		break;
	case TT_Stamp:
		HandleStampBrushInput( view, key, action, data );
		break;
	default:
		break;
	}

	// Swap material painting pair slots
	if ( m_cursor.m_toolType == TT_PaintTexture && key == IK_X && action == IACT_Press )
	{
		Int32 newPreset = ( m_presetIndex + 1 ) % m_presetsRadioBtns.Size();
		while ( !m_presetsRadioBtns[newPreset]->IsEnabled() )
		{
			newPreset = ( newPreset + 1 ) % m_presetsRadioBtns.Size();
		}
		SetPreset( newPreset );

		return true;
	}

	// temporary, need to be mapped in shortcuts editor
	if( key == IK_R && action == IACT_Release ) SelectTool( TT_RiseLower );
	if( key == IK_F && action == IACT_Release ) SelectTool( TT_Flatten );
	if( key == IK_H && action == IACT_Release ) SelectTool( TT_Smooth );
	if( key == IK_T && action == IACT_Release ) SelectTool( TT_Stamp );
	if( key == IK_L && action == IACT_Release ) SelectTool( TT_Slope );

	return false;
}

Bool CEdTerrainEditTool::OnViewportMouseMove( const CMousePacket& packet )
{
	ProcessMouseMove( packet );
	return true;
}

Bool CEdTerrainEditTool::OnViewportTrack( const CMousePacket& packet )
{
	ProcessMouseMove( packet );
	return true;
}

Bool CEdTerrainEditTool::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	// Handled
	return true;
}

Bool CEdTerrainEditTool::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// When using the stamp tool, we need to do something about the terrain shadows. Since the live preview doesn't actually modify the terrain,
	// it is not represented in the terrain shadows. This means that anywhere where the stamp lowers the terrain, that area will be fully in
	// shadow, and difficult to see. So, rather than regenerating the terrain shadows every frame, taking into account the stamp shape, we'll
	// just disable the terrain shadows while the tool is open.
	if ( m_cursor.m_toolType == TT_Stamp )
	{
		// Only disable when we actually have data, since that's the only time the shadows can cause problems.
		if ( ( m_cursor.m_stampDataFlags & TESDF_DataTypesMask ) != 0 )
		{
			frame->GetFrameInfo().m_terrainShadowsDistance = 0;
		}
	}


	if ( m_world && m_world->GetTerrain() )
	{
		if ( m_cursor.m_toolType == TT_None )
		{
			RenderSelectedTiles( view, frame );
		}
		else if ( m_cursor.m_brushSize > 0.0f )
		{
			RenderBrush( view, frame );

			const String cursorPos = String::Printf( TXT("Cursor position: %3.2f, %3.2f, %3.2f"), m_cursor.m_position.X, m_cursor.m_position.Y, m_cursor.m_position.Z );
			frame->AddDebugScreenText( 2, frame->GetFrameOverlayInfo().m_height - 2, cursorPos );

			switch ( m_cursor.GetCursorMode() )
			{
			case CM_SlopeRotationChange:
				RenderSlopeRotationGizmo( view, frame );
				break;
			case CM_SlopeAngleChange:
				RenderSlopeSlopeGizmo( view, frame );
				break;
			case CM_SlopeOffsetChange:
				RenderSlopeOffsetGizmo( view, frame );
				break;
			case CM_StampRotationChange:
				RenderStampRotationGizmo( view, frame );
				break;
			default:
				break;
			}

			if ( m_cursor.m_toolType == TT_Stamp )
			{
				if ( ( m_cursor.m_stampDataFlags & TESDF_DataTypesMask ) == 0 )
				{
					static String warning( TXT( "Warning! No stamp data exists, so nothing will be stamped!" ) );
					Int32 x = ( view->GetWidth() - frame->GetDebugTextBounds( warning ).Width() ) / 2;
					Int32 y = view->GetHeight() / 2;
					frame->AddDebugScreenText( x, y, warning, Color::RED );
				}
			}
			else if ( m_cursor.m_toolType == TT_RiseLower )
			{
				if ( m_cursor.m_intensity == 0 )
				{
					static String warning( TXT( "Warning! Painting will not affect terrain (intensity is zero)!" ) );
					Int32 x = ( view->GetWidth() - frame->GetDebugTextBounds( warning ).Width() ) / 2;
					Int32 y = view->GetHeight() / 2;
					frame->AddDebugScreenText( x, y, warning, Color::RED );
				}
			}

			// If we're using falloff curve, check if we have any non-zero values.
			if ( m_cursor.GetUseFalloffCurve() )
			{
				Bool isNonZero = m_cursor.HasNonZeroValues();

				// If the cursor doesn't actually have the falloff buffer created (maybe brush size has changed, but no painting is done yet),
				// we'll just use the falloff curve directly to get a pretty good idea...
				if ( !m_cursor.m_falloffData || m_cursor.m_falloffDataNeedsRebuild )
				{
					// First, simple check... are any of the curve's control points non-zero?
					if ( !isNonZero )
					{
						Float curveMin, curveMax;
						m_cursor.m_falloffCurve->GetFloatValuesMinMax( curveMin, curveMax );
						isNonZero = ( curveMax > 0.0f );
					}

					// If control points were zero, we'll do a rough check through interpolated values, in case tangents are pushing values
					// above 0.
					if ( !isNonZero )
					{
						Float stepSize = 1.0f / 128.0f;
						// Start at 1 and work backwards. In general, probably the middle of the brush is most likely to be non-zero.
						for ( Float x = 1.0f; x >= 0.0f; x -= stepSize )
						{
							if ( m_cursor.m_falloffCurve->GetFloatValue( x ) > 0.0f )
							{
								isNonZero = true;
								break;
							}
						}
					}
				}

				// If we still haven't found any positive falloff values, show a warning.
				if ( !isNonZero )
				{
					static String warning( TXT( "Warning! Painting will probably not affect terrain (falloff curve appears to be zero)!" ) );
					Int32 x = ( view->GetWidth() - frame->GetDebugTextBounds( warning ).Width() ) / 2;
					Int32 y = view->GetHeight() / 2;
					frame->AddDebugScreenText( x, y, warning, Color::RED );
				}
			}

			//RenderTilesAffectedByCursor( view, frame );
		}

		/*
		static Bool generated = false;
		static TDynArray< Vector > startPositions;
		static TDynArray< Vector > normals;
		if ( !generated )
		{
			generated = true;
			for ( Int32 x = -10; x < 10; x += 2 )
				for ( Int32 y = -10; y < 10; y += 2 )
				{
					Vector sPos( (Float)x, (Float)y, 0.0f );
					sPos.Z = m_world->GetTerrain()->GetHeightForWorldPosition( sPos );
					startPositions.PushBack( sPos );
					Vector n = m_world->GetTerrain()->GetNormalForWorldPosition( sPos );
					normals.PushBack( n );
				}
		}
		for ( Uint32 i = 0; i < startPositions.Size(); ++i )
		{
			Vector endPos = startPositions[i] + normals[i] * 3.0f;
			frame->AddDebugLine( startPositions[i], endPos, Color::RED, false );
		}
		*/
	}

	return true;
}


void CEdTerrainEditTool::DrawBrushLine( CRenderFrame* frame, const Vector& p0, const Vector& p1, const Color& color, Bool asOverlay, Bool clipToZLimit )
{
	const Uint8 occludedAlpha = 31;
	const Uint8 normalAlpha = 255;

	Vector pp0 = p0;
	Vector pp1 = p1;

	if ( clipToZLimit && ( m_cursor.m_lowLimit.IsInitialized() || m_cursor.m_highLimit.IsInitialized() ) )
	{
		if ( pp0.Z > pp1.Z )
		{
			Swap( pp0, pp1 );
		}

		Float zDif = pp1.Z - pp0.Z;

		if ( m_cursor.m_lowLimit.IsInitialized() )
		{
			Float low = TexelsToHeight( m_cursor.m_lowLimit.Get() );
			if ( pp0.Z < low )
			{
				if ( pp1.Z < low )
				{
					return;
				}
				else
				{
					pp0 = Lerp( ( low - pp0.Z ) / zDif, pp0, pp1 );
				}
			}
		}

		if ( m_cursor.m_highLimit.IsInitialized() )
		{
			Float high = TexelsToHeight( m_cursor.m_highLimit.Get() );
			if ( pp1.Z > high )
			{
				if ( pp0.Z > high )
				{
					return;
				}
				else
				{
					pp1 = Lerp( ( high - pp0.Z ) / zDif, pp0, pp1 );
				}
			}
		}
	}


	if ( asOverlay )
	{
		frame->AddDebugLine( pp0, pp1, Color( color.R, color.G, color.B, normalAlpha ), true, true );
	}
	else
	{
		frame->AddDebugLine( pp0, pp1, Color( 0, 0, 255, occludedAlpha ), true, true );
		frame->AddDebugLine( pp0, pp1, Color( color.R, color.G, color.B, normalAlpha ), false, true );
	}
}

void CEdTerrainEditTool::RenderBrush( IViewport* view, CRenderFrame* frame )
{
	const Bool asOverlay = m_drawBrushAsOverlay->GetValue();

	if ( m_cursor.m_toolType == TT_CollForceOff || m_cursor.m_toolType == TT_CollForceOn || m_cursor.m_toolType == TT_CollReset )
	{
		Int32 x, y;
		if ( m_world->GetTerrain()->GetTileFromPosition( m_cursor.m_position, x, y, false ) )
		{
			Box tileBox = m_world->GetTerrain()->GetBoxForTile( x, y, 0.0f );

			Vector corners[4] = {
				Vector( tileBox.Min.X, tileBox.Min.Y, 0.0f ),
				Vector( tileBox.Max.X, tileBox.Min.Y, 0.0f ),
				Vector( tileBox.Max.X, tileBox.Max.Y, 0.0f ),
				Vector( tileBox.Min.X, tileBox.Max.Y, 0.0f )
			};

			const Uint32 numPointsPerSide = m_world->GetTerrain()->GetTilesMaxResolution();

			// Generate points
			TDynArray< Vector > ringPoints;
			for ( Uint32 i = 0; i < numPointsPerSide * 4; ++i )
			{
				Uint32 side = i / numPointsPerSide;
				Uint32 sidePt = i % numPointsPerSide;
				// Calc vertex world space xy position
				Vector pos = Lerp( sidePt / (Float)numPointsPerSide, corners[side], corners[( side + 1 )%4] );
				m_world->GetTerrain()->GetHeightForWorldPosition( pos, pos.Z );
				ringPoints.PushBack( pos );
			}

			for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
			{
				DrawBrushLine( frame, ringPoints[i], ringPoints[( i + 1 ) % ringPoints.Size()], Color::RED, true );
			}
		}
	}
	else if ( m_cursor.m_toolType == TT_Stamp )
	{
		Vector topLeft( -m_cursor.m_brushSize, -m_cursor.m_brushSize, 0.f );
		Vector topRight( m_cursor.m_brushSize, -m_cursor.m_brushSize, 0.f );
		Vector bottomLeft( -m_cursor.m_brushSize, m_cursor.m_brushSize, 0.f );
		Vector bottomRight( m_cursor.m_brushSize, m_cursor.m_brushSize, 0.f );

		Matrix rot = Matrix::IDENTITY;
		rot.SetRotZ33( DEG2RAD( m_cursor.m_stampParams.yawDegrees ) );

		Vector corners[] = {
			rot.TransformVector( topLeft )		+ m_cursor.m_position,
			rot.TransformVector( topRight )		+ m_cursor.m_position,
			rot.TransformVector( bottomRight )	+ m_cursor.m_position,
			rot.TransformVector( bottomLeft )	+ m_cursor.m_position,
		};


		// Circle subdivision based on brush radius
		const Uint32 numPointsPerSide = Max( Int32( m_cursor.m_brushSize ), 32 );

		// Generate points
		TDynArray< Vector > ringPoints;
		for ( Uint32 i = 0; i < numPointsPerSide * 4; ++i )
		{
			Uint32 side = i / numPointsPerSide;
			Uint32 sidePt = i % numPointsPerSide;
			// Calc vertex world space xy position
			Vector pos = Lerp( sidePt / (Float)numPointsPerSide, corners[side], corners[( side + 1 )%4] );
			m_world->GetTerrain()->GetHeightForWorldPosition( pos, pos.Z );
			ringPoints.PushBack( pos );
		}

		for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
		{
			DrawBrushLine( frame, ringPoints[i], ringPoints[( i + 1 ) % ringPoints.Size()], Color::RED, true );
		}
	}
	else
	{
		// Circle subdivision based on brush radius
		const Uint32 numPoints = Max( Int32( 2.0 * M_PI * m_cursor.m_brushSize ), 32 );

		// Generate points
		TDynArray< Vector > ringPoints;
		for ( Uint32 i = 0; i < numPoints; i ++ )
		{
			const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

			// Calc vertex world space xy position
			Vector pos 				
				( MCos( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.X
				, MSin( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.Y
				, 0 );


			m_world->GetTerrain()->GetHeightForWorldPosition( pos, pos.Z );
			ringPoints.PushBack( pos );
		}

		for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
		{
			DrawBrushLine( frame, ringPoints[i], ringPoints[( i + 1 ) % ringPoints.Size()], Color::YELLOW, true, m_cursor.m_limitsAvailable );
		}
	}

	// render more "pretty stuff" basing on the brush type
	switch ( m_cursor.m_toolType )
	{
	case TT_Flatten:
		{
			Vector endPoint = m_cursor.m_position;
			endPoint.Z = m_cursor.m_desiredElevation;
			DrawBrushLine( frame, m_cursor.m_position, endPoint, m_cursor.m_position.Z > endPoint.Z ? Color::BLUE : Color::GREEN, true );


			// Draw a circle at the target flatten height.
			const Uint32 numPoints = Max( Int32( 2.0 * M_PI * m_cursor.m_brushSize ), 32 );
			TDynArray< Vector > ringPoints;
			for ( Uint32 i = 0; i < numPoints; i ++ )
			{
				const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

				// Calc vertex world space xy position
				Vector pos
					( MCos( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.X
					, MSin( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.Y
					, m_cursor.m_desiredElevation );
				ringPoints.PushBack( pos );
			}
			for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
			{
				DrawBrushLine( frame, ringPoints[i], ringPoints[( i + 1 ) % ringPoints.Size()], Color::RED, asOverlay );
			}

			// Then draw a grid across the circle
			Vector center = m_cursor.m_position;
			center.Z = m_cursor.m_desiredElevation;
			const Float  radius = m_cursor.m_brushSize;
			// Minimum 4 lines. It was requested that there not just be a cross in the middle, so yeah...
			const Uint32 numLines = Max( Red::Math::MLog2( m_cursor.m_texelsPerEdge ) + 3, 4 );
			// Don't need to draw the first or last lines, since they actually have 0 length.
			for ( Uint32 i = 1; i < numLines - 1; ++i )
			{
				Float t = (Float)i / (numLines - 1.f);
				Float y = t * 2.0f - 1.0f;

				Vector midX = center + Vector( y * radius, 0, 0, 0 );
				Vector midY = center + Vector( 0, y * radius, 0, 0 );

				Float lineHL = MSqrt( 1.0f - y*y ) * radius;

				DrawBrushLine( frame, midX - Vector( 0, lineHL, 0, 0 ), midX + Vector( 0, lineHL, 0, 0 ), Color::RED, asOverlay );
				DrawBrushLine( frame, midY - Vector( lineHL, 0, 0, 0 ), midY + Vector( lineHL, 0, 0, 0 ), Color::RED, asOverlay );
			}
		}
		break;
	case TT_Slope:
		{
			Vector center = m_cursor.m_position + Vector( 0.f, 0.f, m_slopeOffsetControl.GetValue() );
			if ( m_paintingRequest )
			{
				// Get the cursor position in texel coordinates (the same space as our reference position)
				Vector cursorPos = m_world->GetTerrain()->GetTexelSpaceNormalizedPosition( m_cursor.m_position );
				cursorPos *= m_terrainParameters.clipmapSize;
				m_world->GetTerrain()->GetHeightForTexelPosition( cursorPos, cursorPos.Z );

				Vector horizontalDir = m_cursor.m_slopeParams.GetUpDir3D() * Vector( 1.f, 1.f, 0.f, 0.f );
				horizontalDir.Normalize3();
				Float heightOffset = Vector::Dot3( cursorPos - m_cursor.m_slopeParams.referencePos, horizontalDir ) / m_cursor.m_texelsPerUnit * m_cursor.m_slopeParams.GetHeightRatio();

				center = m_cursor.m_position;
				center.Z = m_cursor.m_slopeParams.referencePos.Z + heightOffset + m_slopeOffsetControl.GetValue();
			}
			const Float  radius = m_cursor.m_brushSize;

			Vector rightDir  = m_cursor.m_slopeParams.GetRightDir3D();
			Vector upDirOrig = m_cursor.m_slopeParams.GetUpDir3D();
			Float  stretch	 = 1.f / Max( 0.001f, sqrtf( upDirOrig.X * upDirOrig.X + upDirOrig.Y * upDirOrig.Y ) );
			Vector upDir	 = upDirOrig * stretch;
			Vector corner    = center - rightDir * radius - upDir * radius;

			const Uint32   numLinesX = Max<Uint32>( 10, 10 * stretch );
			const Uint32   numLinesY = 10;

			for ( Uint32 i = 0; i < numLinesX; ++i )
			{
				Float  t    = (Float)i / (numLinesX - 1.f);
				Vector pos0 = corner + upDir * ( t * 2.f * radius );
				Vector pos1 = pos0 + rightDir * ( 2.f * radius );

				Vector colorVec = Vector( 1.f, 0.f, 0.f ) * t + Vector( 0.f, 0.f, 1.f ) * ( 1.f - t );
				DrawBrushLine( frame, pos0, pos1, Color( colorVec ), asOverlay );
			}

			for ( Uint32 i = 0; i < numLinesY; ++i )
			{
				Float  t    = (Float)i / (numLinesY - 1.f);
				Vector pos0 = corner + rightDir * ( t * 2.f * radius );
				Vector pos1 = pos0 + upDir * ( 2.f * radius );
				DrawBrushLine( frame, pos0, pos1, Color::RED, asOverlay );
			}
		}
		break;
	default:
		break;
	}

#if 0
	Vector topleft		= m_cursor.m_position + Vector(-1.0f,  1.0f, 0.0f) * m_cursor.m_brushSize;
	Vector topright		= m_cursor.m_position + Vector( 1.0f,  1.0f, 0.0f) * m_cursor.m_brushSize;
	Vector bottomleft	= m_cursor.m_position + Vector(-1.0f, -1.0f, 0.0f) * m_cursor.m_brushSize;
	Vector bottomright	= m_cursor.m_position + Vector( 1.0f, -1.0f, 0.0f) * m_cursor.m_brushSize;

	frame->AddDebugBox( Box(topleft, 5.0f),		Matrix::IDENTITY, Color(  0,   0, 255), true ); // topleft		- red
	frame->AddDebugBox( Box(topright, 5.0f),	Matrix::IDENTITY, Color(  0, 255,   0), true ); // topright		- green
	frame->AddDebugBox( Box(bottomleft, 5.0f),	Matrix::IDENTITY, Color(255,   0,   0), true ); // bottomleft	- blue
	frame->AddDebugBox( Box(bottomright, 5.0f), Matrix::IDENTITY, Color(255, 255, 255), true ); // bottomright	- white

	frame->AddDebugScreenFormatedText( 10, 10, TXT( "Cursor position: [%1.3f; %1.3f; %1.3f]" ), m_cursor.m_position.X, m_cursor.m_position.Y, m_cursor.m_position.Z );
#endif
}

void CEdTerrainEditTool::RenderTilesAffectedByCursor( IViewport* view, CRenderFrame* frame )
{
	if ( !m_anyTilesAffected || !m_world || !m_world->GetTerrain() )
	{
		return;
	}

	if ( m_tilesAffectedByCursor.m_right < 0 || m_tilesAffectedByCursor.m_left > (Int32)m_world->GetTerrain()->GetNumTilesPerEdge() - 1 ||
		 m_tilesAffectedByCursor.m_top < 0 || m_tilesAffectedByCursor.m_bottom > (Int32)m_world->GetTerrain()->GetNumTilesPerEdge() - 1 )
	{
		return;
	}

	for ( Int32 x = m_tilesAffectedByCursor.m_left; x <= m_tilesAffectedByCursor.m_right; ++x )
	{
		for ( Int32 y = m_tilesAffectedByCursor.m_top; y <= m_tilesAffectedByCursor.m_bottom; ++y )
		{
			if ( x < 0 || x >= (Int32)m_world->GetTerrain()->GetNumTilesPerEdge() || y < 0 || y >= (Int32)m_world->GetTerrain()->GetNumTilesPerEdge() )
			{
				continue;
			}

			Box b = m_world->GetTerrain()->GetBoxForTile(x, y, m_cursor.m_position.Z );
			frame->AddDebugBox( b, Matrix::IDENTITY, Color::RED, true );
		}
	}
}

void CEdTerrainEditTool::RenderSelectedTiles( IViewport* view, CRenderFrame* frame )
{
	Uint32 tilesPerEdge = m_world->GetTerrain()->GetNumTilesPerEdge();
	for ( Uint32 y = 0; y < tilesPerEdge; ++y )
	{
		for ( Uint32 x = 0; x < tilesPerEdge; ++x )
		{
			if ( m_tilesSelection[ y * tilesPerEdge + x ] )
			{
				RenderSelectedTile( view, frame, x, y );
			}
		}
	}
}

void CEdTerrainEditTool::RenderSelectedTile( IViewport* view, CRenderFrame* frame, Uint32 x, Uint32 y )
{
	Uint32 tilesPerEdge = m_world->GetTerrain()->GetNumTilesPerEdge();
	Box b = m_world->GetTerrain()->GetBoxForTile( x, y, 0.f );

	// check left wall
	if ( x == 0 || !m_tilesSelection[ y * tilesPerEdge + x - 1 ] )
	{
		// draw left wall
		Vector first	( b.Min.X, b.Min.Y, m_terrainParameters.lowestElevation );
		Vector second	( b.Min.X, b.Max.Y, m_terrainParameters.lowestElevation );
		RenderBoxWall( view, frame, first, second );
	}
	// check right wall
	if ( x == tilesPerEdge - 1 || !m_tilesSelection[ y * tilesPerEdge + x + 1 ] )
	{
		// draw right wall
		Vector first	( b.Max.X, b.Min.Y, m_terrainParameters.lowestElevation );
		Vector second	( b.Max.X, b.Max.Y, m_terrainParameters.lowestElevation );
		RenderBoxWall( view, frame, first, second );
	}
	// check bottom wall
	if ( y == 0 || !m_tilesSelection[ ( y - 1 ) * tilesPerEdge + x ] )
	{
		// draw bottom wall
		Vector first	( b.Min.X, b.Min.Y, m_terrainParameters.lowestElevation );
		Vector second	( b.Max.X, b.Min.Y, m_terrainParameters.lowestElevation );
		RenderBoxWall( view, frame, first, second );
	}
	// check top wall
	if ( y == tilesPerEdge - 1 || !m_tilesSelection[ ( y + 1 ) * tilesPerEdge + x ] )
	{
		// draw top wall
		Vector first	( b.Min.X, b.Max.Y, m_terrainParameters.lowestElevation );
		Vector second	( b.Max.X, b.Max.Y, m_terrainParameters.lowestElevation );
		RenderBoxWall( view, frame, first, second );
	}
}

void CEdTerrainEditTool::RenderBoxWall( IViewport* view, CRenderFrame* frame, const Vector& first, const Vector& second )
{
	Vector height( 0.f, 0.f, m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation );
	TDynArray< Vector > worldPoints;
	worldPoints.PushBack( first );
	worldPoints.PushBack( second );

	TDynArray< DebugVertex > vertices;
	TDynArray< Uint16 > indices;
	vertices.Resize( worldPoints.Size() * 4 );
	indices.Resize( worldPoints.Size() * 12 );

	// Alpha
	Color color = Color::LIGHT_GREEN;
	color.A = 80;

	// Write
	Uint16* index = indices.TypedData();
	DebugVertex* vertex = vertices.TypedData();
	for ( Uint32 i=0; i < 2; i++, vertex+=4, index += 12 )
	{
		Uint16 base = (Uint16)(i * 4);

		// Faces
		vertex[0].Set( worldPoints[ i ], color );
		vertex[1].Set( worldPoints[ i ] + height, color );
		vertex[2].Set( worldPoints[ (i+1) % worldPoints.Size() ] + height, color );
		vertex[3].Set( worldPoints[ (i+1) % worldPoints.Size() ], color );

		// Indices
		index[0] = base + 0;
		index[1] = base + 1;
		index[2] = base + 2;
		index[3] = base + 0;
		index[4] = base + 2;
		index[5] = base + 3;
		index[6] = base + 2;
		index[7] = base + 1;
		index[8] = base + 0;
		index[9] = base + 3;
		index[10] = base + 2;
		index[11] = base + 0;
	}

	// Draw faces
	new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), RSG_DebugTransparent );
}

void CEdTerrainEditTool::UpdateCursor( const CMousePacket& packet )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}

	Vector position;
	if ( !m_world->GetTerrain()->Intersect( packet.m_rayOrigin, packet.m_rayDirection, position ) )
	{
		return;
	}

	// Always update cursor position
	m_cursor.m_position = position;

	// If we're using the stamp, and have data _not_ from a heightmap (i.e. it's copied from the terrain), update the offset fields based on current position.
	if ( m_cursor.m_toolType == TT_Stamp && m_cursor.m_heightmapPath.Empty() && ( m_cursor.m_stampDataFlags & TESDF_DataTypesMask ) != 0 )
	{
		m_stampOffsetX->ChangeValue( wxString::Format( "%f", m_cursor.m_position.X - m_cursor.m_stampSourcePosition.X ) );
		m_stampOffsetY->ChangeValue( wxString::Format( "%f", m_cursor.m_position.Y - m_cursor.m_stampSourcePosition.Y ) );
	}

	DetermineTilesAffectedByCursor( m_cursor );
}

void CEdTerrainEditTool::ProcessMouseMove( const CMousePacket& packet )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.01f : 0.1f;

	// update screenspace position
	m_cursor.UpdateScreenSpaceParams( packet );

	switch ( m_cursor.GetCursorMode() )
	{
	case CM_Paint:
		{
			UpdateCursor( packet );
		}
		break;
	case CM_SlopeAngleChange:
		{
			m_cursor.m_slopeParams.slopeDegrees += m_cursor.GetScreenSpaceDeltaY() * baseMod;
			m_cursor.m_slopeParams.slopeDegrees  = Min( m_cursor.m_slopeParams.slopeDegrees, m_slopeControl.GetMax() );
			m_slopeControl.UpdateValue( m_cursor.m_slopeParams.slopeDegrees );

			m_cursor.m_slopeParams.referencePos = GetSlopeReferencePoint();
		}
		break;
	case CM_SlopeOffsetChange:
		{
			m_cursor.m_slopeParams.offset += m_cursor.GetScreenSpaceDeltaY() * baseMod;
			m_cursor.m_slopeParams.offset  = Min( m_cursor.m_slopeParams.offset, m_slopeOffsetControl.GetMax() );
			m_slopeOffsetControl.UpdateValue( m_cursor.m_slopeParams.offset );
		}
		break;
	case CM_SlopeRotationChange:
		{
			m_cursor.m_slopeParams.yawDegrees += m_cursor.GetScreenSpaceDeltaX() * baseMod;
			if ( m_cursor.m_slopeParams.yawDegrees < m_slopeAngleControl.GetMin() )
			{
				m_cursor.m_slopeParams.yawDegrees += m_slopeAngleControl.GetMax();
			}
			if ( m_cursor.m_slopeParams.yawDegrees > m_slopeAngleControl.GetMax() )
			{
				m_cursor.m_slopeParams.yawDegrees -= m_slopeAngleControl.GetMax();
			}
			m_slopeAngleControl.UpdateValue( m_cursor.m_slopeParams.yawDegrees );

			m_cursor.m_slopeParams.referencePos = GetSlopeReferencePoint();
		}
		break;
	case CM_StampRotationChange:
		{
			m_cursor.m_stampParams.yawDegrees += m_cursor.GetScreenSpaceDeltaX() * baseMod;
			if ( m_cursor.m_stampParams.yawDegrees < m_stampAngleControl.GetMin() )
			{
				m_cursor.m_stampParams.yawDegrees += m_stampAngleControl.GetMax();
			}
			if ( m_cursor.m_stampParams.yawDegrees > m_stampAngleControl.GetMax() )
			{
				m_cursor.m_stampParams.yawDegrees -= m_stampAngleControl.GetMax();
			}
			m_stampAngleControl.UpdateValue( m_cursor.m_stampParams.yawDegrees );
		}
		break;
	}

	// After clicking, the stamp will be hidden so the user can see the new terrain. Then they can move the mouse to see the stamp again.
	if ( m_cursor.m_toolType == TT_Stamp && !RIM_IS_KEY_DOWN( IK_Alt ) && !RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		SetStampVisible( true );
	}

	// Reset mouse state
	if ( !RIM_IS_KEY_DOWN( IK_LeftMouse ) && 
		 !RIM_IS_KEY_DOWN( IK_MiddleMouse ) && 
		 !RIM_IS_KEY_DOWN( IK_RightMouse ) )
	{
		m_viewport->GetViewport()->SetMouseMode( MM_Normal );
	}
}

Bool CEdTerrainEditTool::OnViewportKillFocus( IViewport* view )
{
	if ( m_paintingRequest )
	{
		StopPainting( view );
	}

	return true;
}

Bool CEdTerrainEditTool::OnViewportSetFocus( IViewport* view )
{
	m_cursor.SetCursorMode( CM_Paint );
	return true;
}

Bool CEdTerrainEditTool::DetermineTilesAffectedByCursor( const CTerrainEditCursor& cursor )
{
	if( !m_world || !m_world->GetTerrain() )
	{
		return false;
	}

	Vector2 vmax( FLT_MIN, FLT_MIN );
	Vector2 vmin( FLT_MAX, FLT_MAX );

	for ( Uint32 i = 0; i < 4; ++i )
	{
		const Float localAngle = 2.0f * M_PI * i * 0.25f;
		Float x = MCos( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.X;
		Float y = MSin( localAngle ) * m_cursor.m_brushSize + m_cursor.m_position.Y;

		vmax.X = i == 0 ? x : Max( vmax.X, x );
		vmax.Y = i == 0 ? y : Max( vmax.Y, y );

		vmin.X = i == 0 ? x : Min( vmin.X, x );
		vmin.Y = i == 0 ? y : Min( vmin.Y, y );
	}

	m_anyTilesAffected = m_world->GetTerrain()->GetTilesInWorldArea( vmin, vmax, m_tilesAffectedByCursor );
	return m_anyTilesAffected;
}

void CEdTerrainEditTool::RenderSlopeRotationGizmo( IViewport* view, CRenderFrame* frame )
{
	const Float gizmoSize = m_cursor.m_brushSize * 0.75f;
	Color color = Color::LIGHT_YELLOW;
	color.A = 0.5f;

	// Circle subdivision based on brush radius
	const Uint32 numPoints = Max( Int32( 2.0 * M_PI * 20.f ), 32 );

	// Generate points
	TDynArray< Vector > ringPoints;
	for ( Uint32 i = 0; i < numPoints; i ++ )
	{
		const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

		// Calc vertex world space xy position
		Vector pos = m_cursor.m_position + Vector( MCos( localAngle ) * gizmoSize, MSin( localAngle ) * gizmoSize, 0.f );
		ringPoints.PushBack( pos );
	}

	for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
	{
		frame->AddDebugLine( ringPoints[i], ringPoints[ ( i + 1 ) % ringPoints.Size()], Color::YELLOW, true );
	}

	const Uint32 anglePoints = Max<Uint32>( 2, (Uint32)m_cursor.m_slopeParams.yawDegrees );
	TDynArray< DebugVertex > pieVertices;
	pieVertices.PushBack( DebugVertex( m_cursor.m_position, Color::LIGHT_YELLOW ) );
	for ( Uint32 i = 0; i < anglePoints; ++i )
	{
		const Float localAngle = 2 * M_PI * ( i / 360.f );
		Vector pos = m_cursor.m_position + Vector( MSin( localAngle ) * gizmoSize, MCos( localAngle ) * gizmoSize, 0.f );
		pieVertices.PushBack( DebugVertex( pos, color ) );
	}
	TDynArray< Uint16 > pieIndices;
	
	for ( Uint32 i = 0; i < anglePoints - 1; ++i )
	{
		pieIndices.PushBack( 0 );
		pieIndices.PushBack( i );
		pieIndices.PushBack( i + 1 );
	}
	
	frame->AddDebugTriangles( Matrix::IDENTITY, pieVertices.TypedData(), pieVertices.Size(), pieIndices.TypedData(), pieIndices.Size(), color );

	const Float highestAngle = 2 * M_PI * ( anglePoints / 360.f );
	Vector pos = m_cursor.m_position + Vector( MSin( highestAngle ) * m_cursor.m_brushSize, MCos( highestAngle ) * m_cursor.m_brushSize, 0.f );
	frame->AddDebugLine( m_cursor.m_position, pos, Color::YELLOW, false );
}

void CEdTerrainEditTool::RenderSlopeSlopeGizmo( IViewport* view, CRenderFrame* frame )
{
	Float gizmoSize = m_cursor.m_brushSize * 0.75f;

	// Circle subdivision based on brush radius
	const Uint32 numPoints = Max( Int32( 2.0 * M_PI * gizmoSize ), 32 );

	Vector up = m_cursor.m_slopeParams.GetUpDir2D();
	Vector right = m_cursor.m_slopeParams.GetRightDir2D();

	// Generate points
	TDynArray< Vector > ringPoints;
	for ( Uint32 i = 0; i < numPoints; i ++ )
	{
		const Float localAngleOverTwo = M_PI * ( i / (Float)numPoints );
		Float quatSin = MSin( localAngleOverTwo );
		Vector quat( right.X * quatSin, right.Y * quatSin, right.Z * quatSin, MCos( localAngleOverTwo ) );
		Matrix m;
		m = m.BuildFromQuaternion( quat );
		Vector pos = m.TransformVector( up ) * gizmoSize + m_cursor.m_position;

		ringPoints.PushBack( pos );
	}

	for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
	{
		frame->AddDebugLine( ringPoints[i], ringPoints[ ( i + 1 ) % ringPoints.Size()], Color::YELLOW, true );
	}

	const Uint32 anglePoints = Max<Uint32>( 2, (Uint32)m_cursor.m_slopeParams.slopeDegrees );
	Color color = Color::LIGHT_YELLOW;
	color.A = 0.5f;
	TDynArray< DebugVertex > pieVertices;
	pieVertices.PushBack( DebugVertex( m_cursor.m_position, Color::LIGHT_YELLOW ) );
	for ( Uint32 i = 0; i < anglePoints; ++i )
	{
		const Float localAngle = ( i / 90.f ) * M_PI * 0.5f;
		Float quatSin = MSin( localAngle * 0.5f );
		Vector quat( right.X * quatSin, right.Y * quatSin, right.Z * quatSin, MCos( localAngle * 0.5f ) );
		Matrix m;
		m = m.BuildFromQuaternion( quat );
		Vector pos = m.TransformVector( up ).Normalized3() * gizmoSize + m_cursor.m_position;
		pieVertices.PushBack( DebugVertex( pos, color ) );
	}
	TDynArray< Uint16 > pieIndices;
	for ( Uint32 i = 0; i < anglePoints - 1; ++i )
	{
		pieIndices.PushBack( 0 );
		pieIndices.PushBack( i );
		pieIndices.PushBack( i + 1 );

		pieIndices.PushBack( 0 );
		pieIndices.PushBack( i + 1 );
		pieIndices.PushBack( i );
	}

	frame->AddDebugTriangles( Matrix::IDENTITY, pieVertices.TypedData(), pieVertices.Size(), pieIndices.TypedData(), pieIndices.Size(), color );
}

void CEdTerrainEditTool::RenderSlopeOffsetGizmo( IViewport* view, CRenderFrame* frame )
{
	Color color = m_cursor.m_slopeParams.offset >= 0.f ? Color::LIGHT_GREEN : Color::LIGHT_RED;
	Vector end = m_cursor.m_position;
	end.Z += m_cursor.m_slopeParams.offset;

	static const Float minThickness = 0.2f;
	static const Float maxThickness = 4.0f;
	Float thickness = minThickness + ( (Float)m_cursor.m_texelsPerEdge / MAX_TEXELS_PER_EDGE ) * ( maxThickness - minThickness );

	frame->AddDebugFatLine( m_cursor.m_position, end, color, thickness, true );
}

void CEdTerrainEditTool::RenderStampRotationGizmo( IViewport* view, CRenderFrame* frame )
{
	const Float gizmoSize = m_cursor.m_brushSize * 0.75f;
	Color color = Color::LIGHT_YELLOW;
	color.A = 0.5f;

	// Circle subdivision based on brush radius
	const Uint32 numPoints = Max( Int32( 2.0 * M_PI * 20.f ), 32 );

	// Generate points
	TDynArray< Vector > ringPoints;
	for ( Uint32 i = 0; i < numPoints; i ++ )
	{
		const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

		// Calc vertex world space xy position
		Vector pos = m_cursor.m_position + Vector( MCos( localAngle ) * gizmoSize, MSin( localAngle ) * gizmoSize, 0.f );
		ringPoints.PushBack( pos );
	}

	for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
	{
		frame->AddDebugLine( ringPoints[i], ringPoints[ ( i + 1 ) % ringPoints.Size()], Color::YELLOW, true );
	}

	const Uint32 anglePoints = Max<Uint32>( 2, (Uint32)m_cursor.m_stampParams.yawDegrees );
	TDynArray< DebugVertex > pieVertices;
	pieVertices.PushBack( DebugVertex( m_cursor.m_position, Color::LIGHT_YELLOW ) );

	Matrix m = Matrix::IDENTITY;
	Vector nonTransformedPos( gizmoSize, 0.f, 0.f );
	for ( Uint32 i = 0; i < anglePoints; ++i )
	{
		const Float localAngle = 2 * M_PI * ( i / 360.f );
		m.SetRotZ33( localAngle );
		Vector pos = m_cursor.m_position + m.TransformPoint( nonTransformedPos );
		pieVertices.PushBack( DebugVertex( pos, color ) );
	}
	TDynArray< Uint16 > pieIndices;

	for ( Uint32 i = 0; i < anglePoints - 1; ++i )
	{
		pieIndices.PushBack( 0 );
		pieIndices.PushBack( i + 1 );
		pieIndices.PushBack( i );
	}

	frame->AddDebugTriangles( Matrix::IDENTITY, pieVertices.TypedData(), pieVertices.Size(), pieIndices.TypedData(), pieIndices.Size(), color );

	const Float highestAngle = 2 * M_PI * ( anglePoints / 360.f );
	m.SetRotZ33( highestAngle );
	Vector pos = m_cursor.m_position + m.TransformPoint( Vector( m_cursor.m_brushSize, 0.f, 0.f ) );
	frame->AddDebugLine( m_cursor.m_position, pos, Color::YELLOW, false );
}

void CEdTerrainEditTool::SelectTile( IViewport* view, const Vector & position, SelectMode mode )
{
	Int32 tileX;
	Int32 tileY;
	m_world->GetTerrain()->GetTileFromPosition( position, tileX, tileY );
	Int32 tilesPerEdge = (Int32)m_world->GetTerrain()->GetNumTilesPerEdge();
	if ( tileX >= 0 && tileX < tilesPerEdge && tileY >= 0 && tileY < tilesPerEdge )
	{
		Uint32 index = tileY * tilesPerEdge + tileX;

		switch ( mode )
		{
		case SM_Select:
			m_tilesSelection[ index ] = true;
			break;
		case SM_Deselect:
			m_tilesSelection[ index ] = false;
			break;
		case SM_Toggle:
			m_tilesSelection[ index ] = !m_tilesSelection[ index ];
			break;
		}
	}
}

Bool CEdTerrainEditTool::ProcessCursorClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if( !m_world || !m_world->GetTerrain() )
	{
		return false;
	}

	if (button == 0)
	{
		if (state == true) // LMB pressed
		{
			// Make sure we have a correct cursor position.
			Vector origin, dir;
			view->CalcRay( x, y, origin, dir );

			Vector position;
			if ( !m_world->GetTerrain()->Intersect( origin, dir, position ) )
			{
				// Didn't click the terrain
				return false;
			}
			
			m_cursor.m_position = position;

			StartPainting( view );
		}
		else
		{
			Bool wasPaintSelection = m_paintSelectionActivated;
			StopPainting( view );

			// handle click by brush type
			switch ( m_cursor.m_toolType )
			{
			case TT_None:
				{
					if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
					{
						if ( !wasPaintSelection )
						{
							// no brush chosen, nor paint selection activated, proceed with single-click selection
							CMousePacket dummy( view, x, y, 0, 0, 0.f );
							Vector intersection;
							m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection );
							SelectTile( view, intersection, SM_Toggle );
						}
					}
				}
				break;
			case TT_Flatten:
				{
					if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
					{
						// pick height from cursor
						CMousePacket dummy( view, x, y, 0, 0, 0.f );
						Vector intersection;
						if ( m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection ) )
						{
							m_world->GetTerrain()->GetHeightForWorldPosition( intersection, m_cursor.m_desiredElevation );
							m_heightControl.UpdateValue( m_cursor.m_desiredElevation );
							return false;
						}
					}
				}
				break;
			case TT_Slope:
				{
					if ( RIM_IS_KEY_DOWN( IK_Shift ) )
					{
						// pick slope from cursor
						CMousePacket dummy( view, x, y, 0, 0, 0.f );
						Vector intersection;
						if ( m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection ) )
						{
							Vector norm = m_world->GetTerrain()->GetNormalForWorldPosition( intersection );

							Float slope = MAcos( norm.Z );
							m_cursor.m_slopeParams.slopeDegrees = slope * 180 / M_PI;

							Float rotation = MATan2( -norm.X, -norm.Y );
							if ( rotation < 0 ) rotation += M_PI * 2;
							if ( rotation > M_PI * 2 ) rotation -= M_PI * 2;
							m_cursor.m_slopeParams.yawDegrees = rotation * 180 / M_PI;

							m_slopeControl.UpdateValue( m_cursor.m_slopeParams.slopeDegrees );
							m_slopeAngleControl.UpdateValue( m_cursor.m_slopeParams.yawDegrees );
							return false;
						}
					}
				}
				break;
			case TT_PaintTexture:
				{
					if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
					{
						// pick height from cursor
						CMousePacket dummy( view, x, y, 0, 0, 0.f );
						Vector intersection;
						if ( m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection ) )
						{
							TControlMapType controlMapValue = m_world->GetTerrain()->GetControlmapValueForWorldPosition( intersection );
							
							/*
							controlMapValue is Uint16 value:
							[#1: 3 bits] [#2: 3 bits] [#3: 5 bits] [#4: 5 bits]
							#1: vertical UV multiplier, [0 - 7]
							#2: slope threshold index, [0 - 7]
							#3: vertical texture index, [0-31], stored as a value in the range [1 - 31]
							#4: horizontal texture index, [0-31], stored as a value in the range [1 - 31]
							*/

							Uint16 horizontalIndex = ( controlMapValue & 0x1F ) - 1;
							Uint16 verticalIndex = ( ( controlMapValue >> 5 ) & 0x1F ) - 1;
							m_textureArrayGrid->SetSelected( horizontalIndex );
							m_textureArrayGrid->SetSecondary( verticalIndex );
							m_selectedHorizontalTexture = horizontalIndex;
							m_selectedVerticalTexture = verticalIndex;

							Uint8 slopeThresholdIndex = ( controlMapValue >> 10 ) & 0x07;
							m_paintSlopeThresholdSpin->SetValue( slopeThresholdIndex + 1 );
							m_slopeThresholdIndex = slopeThresholdIndex;

							Uint8 verticalUVMult = ( controlMapValue >> 13 ) & 0x07;
							m_uvMultVerticalSpin->SetValue( verticalUVMult + 1 );
							m_verticalUVMult = verticalUVMult;

							UpdatePresetData();

							return false;
						}
					}
				}
				break;
			case TT_PaintColor:
				{
					if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
					{
						// pick color from cursor
						CMousePacket dummy( view, x, y, 0, 0, 0.f );
						Vector intersection;
						if ( m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection ) )
						{
							TColorMapType colorMapValue = m_world->GetTerrain()->GetColormapValueForWorldPosition( intersection );

							SetCurrentColor( wxColour( colorMapValue ) );
							return false;
						}
					}
				}
				break;
			case TT_Stamp:
				{
					if ( RIM_IS_KEY_DOWN( IK_Alt ) )
					{
						SetStampOffsetControlsEnabled( true );
						m_stampSourceLocation->ChangeValue( wxString::Format( "%0.3f %0.3f", m_cursor.m_position.X, m_cursor.m_position.Y ) );

						m_cursor.LoadStampFromTerrain( m_world->GetTerrain(), TESDF_Height | TESDF_Control | TESDF_Color );
						m_world->GetTerrain()->SetStampData( m_cursor.m_stampHeightMap, m_cursor.m_texelsPerEdge, m_cursor.m_texelsPerEdge );
						m_world->GetTerrain()->SetStampControlData( m_cursor.m_stampControlMap, m_cursor.m_texelsPerEdge, m_cursor.m_texelsPerEdge );
						m_world->GetTerrain()->SetStampColorData( m_cursor.m_stampColorMap, m_cursor.m_stampColorTexelsPerEdge, m_cursor.m_stampColorTexelsPerEdge );
						m_world->GetTerrain()->SetStampModeAdditive( false );

						SetBrushThumbnail();

						SetStampVisible( true );
					}
					else if ( RIM_IS_KEY_DOWN( IK_Shift ) )
					{
						// pick height from cursor
						CMousePacket dummy( view, x, y, 0, 0, 0.f );
						Vector intersection;
						if ( m_world->GetTerrain()->Intersect( dummy.m_rayOrigin, dummy.m_rayDirection, intersection ) )
						{
							m_world->GetTerrain()->GetHeightForWorldPosition( intersection, m_cursor.m_stampScaleOrigin );
							m_stampScaleOriginControl.UpdateValue( m_cursor.m_stampScaleOrigin );
							return false;
						}
					}

				}
				break;
			default:
				break;
			}
		}

		return true;
	}

	return false;
}

Bool CEdTerrainEditTool::ProcessPainting( Float timeDelta )
{
	const bool justClicked = m_paintingRequest && 0==m_paintingRequestFrames;
	if ( m_paintingRequest )
	{
		++m_paintingRequestFrames;
	}

	if ( !m_paintingRequest || justClicked || !m_world || !m_world->GetTerrain() )	
	{
		return false;
	}

	if ( !m_cursor.Validate() )
	{
		return true;
	}


	switch ( m_cursor.m_toolType )
	{
	case TT_None:
		if ( RIM_IS_KEY_DOWN( IK_LControl ) )
		{
			if ( !m_paintSelectionActivated && m_cursor.GetScreenSpaceDistFromStored() >= PaintSelectionThreshold )
			{
				m_paintSelectionActivated = true;
			}

			if ( m_paintSelectionActivated )
			{
				// "paint" selection
				m_paintSelectionActivated = true;
				SelectMode mode = RIM_IS_KEY_DOWN( IK_Shift ) ? SM_Deselect : SM_Select;
				SelectTile( m_viewport->GetViewport(), m_cursor.m_position, mode );
			}
		}
		return true;

	case TT_Stamp:
		ProcessPaintingStamp();
		StopPainting( m_viewport->GetViewport() );
		return true;

	case TT_Slope:
		// Shift used for picking
		if ( RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			return false;
		}
		break;

	case TT_PaintTexture:
	case TT_Flatten:
	case TT_PaintColor:
		// Ctrl used for picking
		if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			return false;
		}
		break;

	case TT_CollForceOn:
	case TT_CollForceOff:
	case TT_CollReset:
		ProcessPaintingCollision();
		return true;
	}


	// Allow the cursor to build any internal buffers that might be out-dated.
	m_cursor.PrepareBuffers();

	CClipMap* terrain = m_world->GetTerrain();
	ASSERT( terrain );
	terrain->GetClipmapParameters( &m_terrainParameters );


	Int32 brushExpansion = 0;
	if ( m_cursor.m_toolType == TT_Smooth )
	{
		// If Shift is down, Smooth becomes Noise, which doesn't need any expansion.
		if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			brushExpansion = Min< Int32 >( m_cursor.m_filterSize, m_cursor.m_texelsPerEdge / 2 );
		}
	}
	else if ( m_cursor.m_toolType == TT_Melt )
	{
		brushExpansion = Min< Int32 >( m_cursor.m_filterSize, m_cursor.m_texelsPerEdge / 2 );
	}


	// Find _all_ tiles that will be affected by this stroke, and mark them all modified.
	Bool stopAfterThisPaint = false;
	{
		// TODO : For now, just grab the full bounds of the stroke. This isn't necessarily correct, but whatever.

		const Vector2 startPos = terrain->GetTexelSpaceNormalizedPosition( m_cursor.m_lastPosition, false );
		const Vector2 endPos = terrain->GetTexelSpaceNormalizedPosition( m_cursor.m_position, false );

		const Vector2 minCenter( Min( startPos.X, endPos.X ), Min( startPos.Y, endPos.Y ) );
		const Vector2 maxCenter( Max( startPos.X, endPos.X ), Max( startPos.Y, endPos.Y ) );

		Rect strokeRectClipmapSpace;
		strokeRectClipmapSpace.m_left = ( Int32 )MRound( minCenter.X * m_terrainParameters.clipmapSize ) - m_cursor.m_texelsPerEdge/2;
		strokeRectClipmapSpace.m_top = ( Int32 )MRound( minCenter.Y * m_terrainParameters.clipmapSize ) - m_cursor.m_texelsPerEdge/2;
		strokeRectClipmapSpace.m_right = ( Int32 )MRound( maxCenter.X * m_terrainParameters.clipmapSize ) - m_cursor.m_texelsPerEdge/2;
		strokeRectClipmapSpace.m_bottom = ( Int32 )MRound( maxCenter.Y * m_terrainParameters.clipmapSize ) - m_cursor.m_texelsPerEdge/2;

		strokeRectClipmapSpace.m_right += m_cursor.m_texelsPerEdge;
		strokeRectClipmapSpace.m_bottom += m_cursor.m_texelsPerEdge;

		strokeRectClipmapSpace.Grow( brushExpansion, brushExpansion );

		Rect strokeRectColorSpace;
		TDynArray< SClipmapHMQueryResult > clipmapParts;
		terrain->GetRectangleOfData( strokeRectClipmapSpace, clipmapParts, &strokeRectColorSpace );

		// If there are no tiles in the selection, do nothing.
		if ( clipmapParts.Size() == 0 ) return true;


		// Check if we have write access to all contributing tiles.
		for ( Uint32 p = 0; p < clipmapParts.Size(); ++p )
		{
			const SClipmapHMQueryResult& part = clipmapParts[p];

			if ( !part.m_tile->MarkModified() )
			{
				// We don't want to apply changes partially
				StopPainting( m_viewport->GetViewport() );
				return true;
			}
		}

		// While marking all tiles modified, if we brought up the popup asking the user to check out a file, then we got a "mouse up" notification
		// and stopped painting. So, let's deal with that...
		if ( !m_paintingRequest )
		{
			StartPainting( m_viewport->GetViewport() );
			stopAfterThisPaint = true;
		}
	}


	Uint32 bufferFlags = m_cursor.GetPaintingBufferFlags();
	if ( bufferFlags == 0 )
	{
		return false;
	}


	// We may be modifying a single tile multiple times. So, rather than committing each change separately, we'll accumulate
	// the entire modified area.
	struct ModifiedTile
	{
		Int32 col, row;
		Rect addressingRect;
		Rect colorAddressingRect;

		ModifiedTile( Int32 c, Int32 r )
			: col( c )
			, row( r )
			, addressingRect( Rect::EMPTY )
			, colorAddressingRect( Rect::EMPTY )
		{}
	};

	THashMap< CTerrainTile*, ModifiedTile > modifiedParts;

	const Vector applyVec = m_cursor.m_position - m_cursor.m_lastPosition;

	Uint32 numSteps = 1;
	Uint32 firstStep = 0;

	Float stepsPerBrush = 1.0f;
	const Bool doSubstepping = m_substepPainting->GetValue();
	if ( doSubstepping )
	{
		const Float texPerUnit = m_terrainParameters.clipmapSize / m_terrainParameters.terrainSize;
		const Float stepsPerUnit = texPerUnit * 2;
		stepsPerBrush = stepsPerUnit * m_cursor.m_brushSize;


		numSteps = MFloor( applyVec.Mag3() * stepsPerUnit ) + 1;
		// Skip first step, since it would have been done in the previous Processing.
		firstStep = Min( 1u, numSteps - 1 );
	}

	// HACK : Brush probability isn't updated when slider changes... so, grab the value here. We may need to tweak it too for substepping.
	Float probability = m_cursor.m_toolType == TT_PaintTexture ? m_presetsData[ m_presetIndex ].probability : static_cast< Float >( m_brushProbability->GetValue() );
	probability *= 0.01f;

	timeDelta /= ( numSteps - firstStep );

	for ( Uint32 step = firstStep; step < numSteps; ++step )
	{
		Uint32 numTimesToApply = 1;

		// If we aren't substepping the painting, then we'll go with time-based applications like before.
		if ( !doSubstepping )
		{
			// let's run with constant time step (120hz)
			static const Float dt = 1.f / PROCESSING_FREQUENCY;
			static Float accumulator = 0.f;
			accumulator += timeDelta;

			const Float timePerIteration = Min( timeDelta, dt );

			Uint32 numTimesToApply = 0;
			while ( accumulator >= dt )
			{
				// apply brush until desired FPS is reached
				accumulator -= timePerIteration;

				++numTimesToApply;
			}

			// Early-out. If not enough time has passed, we can skip all processing.
			if ( numTimesToApply == 0 )
			{
				continue;
			}

			// Since control map modifications don't really do any accumulation, we can easily just apply a single time.
			// This isn't 100% correct, since probability could be affected a bit, but that should be okay...
			if ( m_cursor.m_toolType == TT_PaintTerrainHoles || m_cursor.m_toolType == TT_PaintTexture )
			{
				numTimesToApply = 1;
			}

			m_cursor.m_intensityScale = 1.0f;
			m_cursor.m_probability = probability;
		}
		else
		{
			// Scale back the brush intensity, since we're going to be applying it a lot more.

			// How many steps are overlapping here?
			// Clamp between 1 (can't have less that 1 step overlapping, doesn't make sense) and number of steps (if there are only 1 or 2
			// steps, can't have more steps than that overlapping).
			// 4?? Because we can only really overlap up to half the brush size away (/2), and both overlaps are scaled down (/2 again)
			Float overlap = Clamp< Float >( stepsPerBrush / 4.0f, 1.0f, numSteps );
			m_cursor.m_intensityScale = 1.0f / overlap;

			// Also need to scale back the probability...
			// What's probability of not filling a point?
			Float invProb = Clamp( 1.0f - probability, 0.0f, 1.0f );
			// Repeat probability for each step that touches that point. what's probability of it not being filled N times?
			Float stepsPerPoint = Min( (Float)numSteps, stepsPerBrush );
			Float scaledInvProb = Red::Math::MPow( invProb, 1.0f / stepsPerPoint );
			// And back into probability of filling.
			m_cursor.m_probability = Clamp( 1.0f - scaledInvProb, 0.0f, 1.0f );
		}


		m_cursor.m_position = m_cursor.m_lastPosition + applyVec * ( (Float)(step+1)/(Float)numSteps );

		const Vector2 brushCenterTexelSpaceFNorm = terrain->GetTexelSpaceNormalizedPosition( m_cursor.m_position, false );

		const Int32 brushCenterTexelSpaceCol = ( Int32 )MRound( brushCenterTexelSpaceFNorm.X * m_terrainParameters.clipmapSize );
		const Int32 brushCenterTexelSpaceRow = ( Int32 )MRound( brushCenterTexelSpaceFNorm.Y * m_terrainParameters.clipmapSize );

		const Int32 brushCornerTexelSpaceCol = brushCenterTexelSpaceCol - m_cursor.m_texelsPerEdge/2;
		const Int32 brushCornerTexelSpaceRow = brushCenterTexelSpaceRow - m_cursor.m_texelsPerEdge/2;

		Rect brushRectClipmapSpace( brushCornerTexelSpaceCol, brushCornerTexelSpaceCol + m_cursor.m_texelsPerEdge, 
			brushCornerTexelSpaceRow, brushCornerTexelSpaceRow + m_cursor.m_texelsPerEdge );


		brushRectClipmapSpace.Grow( brushExpansion, brushExpansion );

		Rect brushRectColorSpace;
		TDynArray< SClipmapHMQueryResult > clipmapParts;
		terrain->GetRectangleOfData( brushRectClipmapSpace, clipmapParts, &brushRectColorSpace );

		// If there are no tiles in the selection, do nothing.
		if ( clipmapParts.Size() == 0 ) continue;


		// Accumulate modified area on each tile.
		for ( Uint32 p = 0; p < clipmapParts.Size(); ++p )
		{
			const SClipmapHMQueryResult& part = clipmapParts[p];
			RED_ASSERT( part.m_tile->CanModify() );

			ModifiedTile& mod = modifiedParts.GetRef( part.m_tile, ModifiedTile( part.m_col, part.m_row ) );
			mod.addressingRect.m_left = Min( mod.addressingRect.m_left, part.m_addressingRect.m_left );
			mod.addressingRect.m_top = Min( mod.addressingRect.m_top, part.m_addressingRect.m_top );
			mod.addressingRect.m_right = Max( mod.addressingRect.m_right, part.m_addressingRect.m_right );
			mod.addressingRect.m_bottom = Max( mod.addressingRect.m_bottom, part.m_addressingRect.m_bottom );

			mod.colorAddressingRect.m_left = Min( mod.colorAddressingRect.m_left, part.m_colorAddressingRect.m_left );
			mod.colorAddressingRect.m_top = Min( mod.colorAddressingRect.m_top, part.m_colorAddressingRect.m_top );
			mod.colorAddressingRect.m_right = Max( mod.colorAddressingRect.m_right, part.m_colorAddressingRect.m_right );
			mod.colorAddressingRect.m_bottom = Max( mod.colorAddressingRect.m_bottom, part.m_colorAddressingRect.m_bottom );
		}

		Int32 brushColorExpansion = ( Int32 )( ( ( Float )brushExpansion / brushRectClipmapSpace.Width() ) * brushRectColorSpace.Width() );


		for ( Uint32 i = 0; i < clipmapParts.Size(); ++i )
		{
			SClipmapHMQueryResult& part = clipmapParts[i];
			CTerrainTile* tile = part.m_tile;
			ASSERT( tile );

			CUndoTerrain::AddStroke( m_viewport->GetUndoManager(), tile, part.m_row, part.m_col, part.m_addressingRect, part.m_colorAddressingRect);
		}


		// If we didn't have to expand the brush at all (don't need to read data from neighbors), or the brush fits into a single
		// tile, we can do it in-place. Also, if we're using an expanded brush it cannot be touching the edge of the terrain.
		Rect terrainRect( 0, m_terrainParameters.clipmapSize, 0, m_terrainParameters.clipmapSize );
		Bool onTerrainEdge = !terrainRect.Contains( brushRectClipmapSpace );
		if ( brushExpansion == 0 || ( clipmapParts.Size() == 1 && !onTerrainEdge ) )
		{
			ProcessPaintingInPlace( numTimesToApply, bufferFlags, brushRectClipmapSpace, brushRectColorSpace, brushExpansion, brushColorExpansion, clipmapParts );
		}
		// If we need neighboring data from multiple tiles, we need to copy the full area into a temporary buffer and operate on
		// that. That way we know that all data will be available and don't have to worry about filtering over tile borders.
		else
		{
			ProcessPaintingWithBuffer( numTimesToApply, bufferFlags, brushRectClipmapSpace, brushRectColorSpace, brushExpansion, brushColorExpansion, clipmapParts );
		}
	}


	for ( auto& mod : modifiedParts )
	{
		CTerrainTile* tile = mod.m_first;

		// Copy working data back into tile.
		if ( ( bufferFlags & PPBF_WriteHM ) != 0 )
		{
			m_heightWorkingBuffers.CopyToTile( tile, mod.m_second.addressingRect );
		}

		CPathLibWorld* pathlib = m_world->GetPathLibWorld();
		if ( pathlib )
		{
			pathlib->MarkTileSurfaceModified( mod.m_second.col, mod.m_second.row );
		}

		tile->SetDirty( true );
		tile->RebuildMipmaps();
	}

	// If we modified the heightmap, regenerate terrain shadows.
	if ( ( bufferFlags & PPBF_WriteHM ) != 0 && m_updateShadowsCheckBox->GetValue() )
	{
		( new CRenderCommand_UpdateTerrainShadows( m_world->GetRenderSceneEx() ) )->Commit();
	}


	if ( stopAfterThisPaint )
	{
		StopPainting( m_viewport->GetViewport() );
	}

	m_cursor.m_lastPosition = m_cursor.m_position;

	return true;
}


void CEdTerrainEditTool::SetupPaintingBrush( CTerrainBrush& brush )
{
	switch ( m_cursor.m_toolType )
	{
	case TT_Flatten:
		{
			brush.m_param1[0] = HeightToTexels( m_cursor.m_desiredElevation );
			brush.m_param2[0] = m_cursor.m_intensity * m_cursor.m_intensityScale * MAbs( RIM_TABLET_PRESSURE );
			brush.m_paintMode = TPM_Lerp;
			brush.m_probability = m_cursor.m_probability;

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;
	case TT_Slope:
		{
			brush.m_paintMode						= TPM_Slope;
			brush.m_param2[0]						= m_cursor.m_intensity * m_cursor.m_intensityScale * MAbs( RIM_TABLET_PRESSURE );
			brush.m_slopeParams						= m_cursor.m_slopeParams;

			brush.m_slopeParams.referencePos.Z += m_cursor.m_slopeParams.offset;

			brush.m_texelsPerUnit					= m_cursor.m_texelsPerUnit;
			brush.m_slopeParams.lowestElevation		= m_terrainParameters.lowestElevation;
			brush.m_slopeParams.highestElevation	= m_terrainParameters.highestElevation;
			brush.m_probability = m_cursor.m_probability;

			brush.m_slopeParams.paintOrigin		= m_world->GetTerrain()->GetTexelSpaceNormalizedPosition( m_cursor.m_position );
			brush.m_slopeParams.paintOrigin.X	= MRound( brush.m_slopeParams.paintOrigin.X * m_terrainParameters.clipmapSize );
			brush.m_slopeParams.paintOrigin.Y	= MRound( brush.m_slopeParams.paintOrigin.Y * m_terrainParameters.clipmapSize );

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;
	case TT_Smooth:
		{
			brush.m_probability = m_cursor.m_probability;
			if ( RIM_IS_KEY_DOWN( IK_Shift ) )
			{
				brush.m_param2[0] = (m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation) * 0.1f;
				brush.m_param2[0] *= m_cursor.m_intensity * m_cursor.m_intensityScale * MAbs( RIM_TABLET_PRESSURE );
				brush.m_paintMode = TPM_Noise;
			}
			else
			{
				brush.m_param1[0] = m_cursor.m_desiredElevation;
				brush.m_param2[0] = m_cursor.m_intensity * m_cursor.m_intensityScale * MAbs( RIM_TABLET_PRESSURE );
				brush.m_paintMode = TPM_Smooth;
			}

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;
	case TT_Melt:
		{
			brush.m_param1[0] = m_cursor.m_desiredElevation;
			brush.m_param2[0] = m_cursor.m_intensity * m_cursor.m_intensityScale * MAbs( RIM_TABLET_PRESSURE );
			brush.m_paintMode = TPM_Melt;
			brush.m_probability = m_cursor.m_probability;

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;
	case TT_RiseLower:
		{
			// When we apply the brush, we're locking it at a fixed rate, so we can do the same here.
			static const Float dt = 1.f / PROCESSING_FREQUENCY;

			// When intensity is 1, the brush will raise/lower the terrain at about 400 m/s.
			const Float MAX_HEIGHT_PER_SECOND = 400.0f;

			// Apply simple curve to the intensity, so that values that have small effect cover more of the intensity range. A person
			// is more likely to need fine control for small intensities, so we can reduce the precision of high intensities and move
			// it to the low.
			Float intensity = Red::Math::MPow( m_cursor.m_intensity, 2.0f ) * m_cursor.m_intensityScale;

			// Figure out how much to change the actual heightmap values.
			Float amt = dt * intensity * MAX_HEIGHT_PER_SECOND / ( m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation );

			brush.m_param1[0] = 1.0f;
			brush.m_param2[0] = ( RIM_IS_KEY_DOWN( IK_Shift ) ? -1.0f : 1.0f ) * RIM_TABLET_PRESSURE * amt * 65535.0f;
			brush.m_paintMode = TPM_Add;
			brush.m_probability = m_cursor.m_probability;

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;


	case TT_PaintTexture:
		{
			brush.m_paramSize = 0;

			// Note: param3=mask, param4=value for masked bits

			// Paint the horizontal texture
			Uint32 texture = 0;
			Uint32 textureMask = 0;
			if ( m_horizontalMask->GetValue() )
			{
				texture |= (m_selectedHorizontalTexture + 1);
				textureMask |= 0x1F;
			}

			// Paint the vertical texture
			if ( m_verticalMask->GetValue() )
			{
				texture |= (m_selectedVerticalTexture + 1) << 5;
				textureMask |= 0x1F << 5;
			}

			// Paint the vertical texture multiplier
			if ( m_verticalUVScaleMask->GetValue() )
			{
				texture |= ( m_verticalUVMult << 13 );
				textureMask |= 0x07 << 13;
			}

			brush.m_param3[0] = textureMask;
			brush.m_param4[0] = texture;



			// Paint the slope threshold
			if ( m_slopeThresholdMask->GetValue() )
			{
				ETerrainPaintSlopeThresholdAction action;
				switch ( m_slopeThresholdAction )
				{
				case STACT_Add:
					if ( RIM_IS_KEY_DOWN( IK_Shift ) )
					{
						action = STACT_Subtract;
					}
					else
					{
						action = STACT_Add;
					}
					break;
				case STACT_Subtract:
					if ( RIM_IS_KEY_DOWN( IK_Shift ) )
					{
						action = STACT_Add;
					}
					else
					{
						action = STACT_Subtract;
					}
					break;
				default:
					action = m_slopeThresholdAction;
				}


				brush.SetSlopeThresholdParams( action, m_slopeThresholdIndex );
			}

			brush.m_probability = m_cursor.m_probability;
			brush.m_paintMode = TPM_BitSetAndMix;

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;

	case TT_PaintTerrainHoles:
		{
			Int32 cmValue = 0;
			if ( RIM_IS_KEY_DOWN( IK_Shift ) )
			{
				cmValue = ( 1 ) | ( 1 << 5 );
			}

			brush.m_param4[0] = cmValue;
			brush.m_paintMode = TPM_BitSetNoCheck; // Biased bitmask is allowed to paint over 0 values
			brush.m_probability = m_cursor.m_probability;

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;


	case TT_PaintColor:
		{
			brush.m_paintMode = TPM_ColorMap;
			brush.m_probability = 1.0f;

			// Mask used for each paint mode setting. Write into RGB, leave A (currently unused) alone
			brush.m_param3[0] = 0x00ffffff;

			Color paintColor( m_currentColor.Red(), m_currentColor.Green(), m_currentColor.Blue(), 0 );
			brush.m_param4[0] = paintColor.ToUint32();

			brush.m_param2[0] = m_cursor.m_intensity * m_cursor.m_intensityScale * fabs( RIM_TABLET_PRESSURE );

			// Setup Z limits
			brush.m_lowLimit  = m_cursor.m_lowLimit;
			brush.m_highLimit = m_cursor.m_highLimit;
		}
		break;
	}
	
	// Setup texture masking
	if ( ( m_cursor.m_toolType == TT_PaintTexture || m_useMaterialTextureMasking->GetValue() )
		&& ( m_useHorizontalTextureMask->GetValue() || m_useVerticalTextureMask->GetValue() ) )
	{
		Uint32 val = 1;

		// Use horizontal texture as mask
		if ( m_useHorizontalTextureMask->GetValue() )
		{
			val |= 0x02;
			val |= m_invertHorizontalTextureMask->GetValue() ? 0x08 : 0x00;
			val |= ( ( m_horizontalTextureMaskIndex + 1 ) << 5 );
		}

		// Use vertical texture as mask
		if ( m_useVerticalTextureMask->GetValue() )
		{
			val |= 0x04;
			val |= m_invertVerticalTextureMask->GetValue() ? 0x10 : 0x00;
			val |= ( ( m_verticalTextureMaskIndex + 1 ) << 10 );
		}

		brush.m_textureMask = val;
	}
	else
	{
		brush.m_textureMask = 0;
	}
}



static void AdjustExpandedRects( Rect& brushSubRect, Rect& tileSubRect, const Rect& brushRect, Int32 brushExpansion )
{
	// Check if the rectangle goes outside the actual (unexpanded) brush.
	if ( brushSubRect.m_left < brushExpansion )
	{
		tileSubRect.m_left += brushExpansion - brushSubRect.m_left;
		brushSubRect.m_left = brushExpansion;
	}
	if ( brushSubRect.m_right > brushRect.Width() - brushExpansion )
	{
		tileSubRect.m_right -= brushSubRect.m_right - ( brushRect.Width() - brushExpansion );
		brushSubRect.m_right = brushRect.Width() - brushExpansion;
	}
	if ( brushSubRect.m_top < brushExpansion )
	{
		tileSubRect.m_top += brushExpansion - brushSubRect.m_top;
		brushSubRect.m_top = brushExpansion;
	}
	if ( brushSubRect.m_bottom > brushRect.Height() - brushExpansion )
	{
		tileSubRect.m_bottom -= brushSubRect.m_bottom - ( brushRect.Height() - brushExpansion );
		brushSubRect.m_bottom = brushRect.Height() - brushExpansion;
	}

	brushSubRect.m_right -= brushExpansion;
	brushSubRect.m_left -= brushExpansion;
	brushSubRect.m_bottom -= brushExpansion;
	brushSubRect.m_top -= brushExpansion;

	tileSubRect.m_right -= brushSubRect.m_left;
	tileSubRect.m_left -= brushSubRect.m_left;
	tileSubRect.m_bottom -= brushSubRect.m_top;
	tileSubRect.m_top -= brushSubRect.m_top;
}

void CEdTerrainEditTool::ProcessPaintingInPlace( Uint32 numTimes, Uint32 bufferFlags, const Rect& brushRectInClipmap, const Rect& brushRectInColorMap, Int32 brushExpansion, Int32 brushColorExpansion, TDynArray< SClipmapHMQueryResult >& clipmapParts )
{
	CTerrainBrush brush;

	// Can apply in-place, without copying stuff.
	for ( Uint32 i = 0; i < clipmapParts.Size(); ++i )
	{
		SClipmapHMQueryResult& part = clipmapParts[i];
		CTerrainTile* tile = part.m_tile;
		ASSERT( tile );

		Rect selSubRect = part.m_selectionSubRect;
		Rect addrRect = part.m_addressingRect;
		AdjustExpandedRects( selSubRect, addrRect, brushRectInClipmap, brushExpansion );

		Rect colorSelSubRect = part.m_colorSelectionSubRect;
		Rect colorAddrRect = part.m_colorAddressingRect;
		AdjustExpandedRects( colorSelSubRect, colorAddrRect, brushRectInColorMap, brushColorExpansion );

		// Sanity checks. selSubRect should not be outside the brush data, and addrRect (offset'ed by the selSubRect) should not be outside the tile data.
		ASSERT( selSubRect.Width() > 0 && selSubRect.Height() > 0 );
		if ( !( selSubRect.Width() > 0 && selSubRect.Height() > 0 ) ) break;
		ASSERT( selSubRect.m_left >= 0 && selSubRect.m_top >= 0 && selSubRect.m_right <= (Int32)m_cursor.m_texelsPerEdge && selSubRect.m_bottom <= (Int32)m_cursor.m_texelsPerEdge );
		if ( !( selSubRect.m_left >= 0 && selSubRect.m_top >= 0 && selSubRect.m_right <= (Int32)m_cursor.m_texelsPerEdge && selSubRect.m_bottom <= (Int32)m_cursor.m_texelsPerEdge ) ) break;
		ASSERT( ( addrRect.m_left + selSubRect.m_left ) >= 0 && ( addrRect.m_top + selSubRect.m_top ) >= 0 && ( addrRect.m_right + selSubRect.m_left ) <= (Int32)m_terrainParameters.tileRes && ( addrRect.m_bottom + selSubRect.m_top ) <= (Int32)m_terrainParameters.tileRes );
		if ( !( ( addrRect.m_left + selSubRect.m_left ) >= 0 && ( addrRect.m_top + selSubRect.m_top ) >= 0 && ( addrRect.m_right + selSubRect.m_left ) <= (Int32)m_terrainParameters.tileRes && ( addrRect.m_bottom + selSubRect.m_top ) <= (Int32)m_terrainParameters.tileRes ) ) break;

		Int32 colorRes = tile->GetHighestColorMapResolution();
		ASSERT( colorSelSubRect.Width() >= 0 && colorSelSubRect.Height() >= 0 );
		if ( !( colorSelSubRect.Width() >= 0 && colorSelSubRect.Height() >= 0 ) ) break;
		ASSERT( ( colorAddrRect.m_left + colorSelSubRect.m_left ) >= 0 && ( colorAddrRect.m_top + colorSelSubRect.m_top ) >= 0 && ( colorAddrRect.m_right + colorSelSubRect.m_left ) <= colorRes && ( colorAddrRect.m_bottom + colorSelSubRect.m_top ) <= colorRes );
		if ( !( ( colorAddrRect.m_left + colorSelSubRect.m_left ) >= 0 && ( colorAddrRect.m_top + colorSelSubRect.m_top ) >= 0 && ( colorAddrRect.m_right + colorSelSubRect.m_left ) <= colorRes && ( colorAddrRect.m_bottom + colorSelSubRect.m_top ) <= colorRes ) ) break;


		// Setup brush
		brush.m_targetRect = selSubRect;
		brush.m_colorMapRect = colorSelSubRect;

		brush.m_tileWidth = m_terrainParameters.tileRes;
		brush.m_tileColorWidth = colorRes;
		brush.m_brushWidth = m_cursor.m_texelsPerEdge;
		brush.m_brushColorWidth = brushRectInColorMap.Width() - brushColorExpansion * 2;

		// In general, this would be a bad offset (m_left and m_top can easily be negative depending on selSubRect). But, the offset'ed pointers passed to ApplyBrush will only
		// be read/written in the region given by selSubRect, plus the border width, so those negative left/top values will cancel out. We just need to provide pointers to the
		// "theoretical" start of the brush area.
		ptrdiff_t tileOffset = ( ptrdiff_t )addrRect.m_left + ( ptrdiff_t )addrRect.m_top * brush.m_tileWidth;
		ptrdiff_t colorOffset = ( ptrdiff_t )colorAddrRect.m_left + ( ptrdiff_t )colorAddrRect.m_top * brush.m_tileColorWidth;

		if ( ( bufferFlags & ( PPBF_ReadHM | PPBF_WriteHM ) ) != 0 )
		{
			brush.m_tileData = m_heightWorkingBuffers.GetBufferForTile( tile ) + tileOffset;
		}
		if ( ( bufferFlags & ( PPBF_ReadCM | PPBF_WriteCM ) ) != 0 )
		{
			brush.m_tileCMData = tile->GetLevelWriteSyncCM( 0 ) + tileOffset;
		}
		if ( ( bufferFlags & ( PPBF_ReadColor | PPBF_WriteColor ) ) != 0 )
		{
			brush.m_tileColorData = static_cast< TColorMapType* >( tile->GetHighestColorMapDataWrite() ) + colorOffset;
		}

		brush.m_falloffData = m_cursor.m_falloffData;

		brush.m_tileBorderSize = brushExpansion;
		brush.m_tileColorBorderSize = brushColorExpansion;

		SetupPaintingBrush( brush );

		// Brush settings are the same for all iterations, so we can just use the same info.
		for ( Uint32 i = 0; i < numTimes; ++i )
		{
			ApplyBrush( brush );
		}
	}
}


void CEdTerrainEditTool::ProcessPaintingWithBuffer( Uint32 numTimes, Uint32 bufferFlags, const Rect& brushRectInClipmap, const Rect& brushRectInColorMap, Int32 brushExpansion, Int32 brushColorExpansion, TDynArray< SClipmapHMQueryResult >& clipmapParts )
{
	CTerrainBrush brush;

	Uint32 brushWidth = brushRectInClipmap.Width();
	Uint32 brushHeight = brushRectInClipmap.Height();

	Uint32 colorWidth = brushRectInColorMap.Width();
	Uint32 colorHeight = brushRectInColorMap.Height();

	// Copy data into temp buffer so that we can filter across tile boundaries.
	Float* tempHM = NULL;
	TControlMapType* tempCM = NULL;
	TColorMapType* tempColor = NULL;
	if ( ( bufferFlags & PPBF_ReadHM ) != 0 )
	{
		tempHM = (Float*)RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, brushWidth * brushHeight * sizeof( Float ) );
	}
	if ( ( bufferFlags & PPBF_ReadCM ) != 0 )
	{
		tempCM = (TControlMapType*)RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, brushWidth * brushHeight * sizeof( TControlMapType ) );
	}
	if ( ( bufferFlags & PPBF_ReadColor ) != 0 )
	{
		tempColor = (TColorMapType*)RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Temporary, colorWidth * colorHeight * sizeof( TColorMapType ) );
	}

	for ( Uint32 i = 0; i < clipmapParts.Size(); ++i )
	{
		SClipmapHMQueryResult& part = clipmapParts[i];
		CTerrainTile* tile = part.m_tile;
		ASSERT( tile );

		if ( ( bufferFlags & PPBF_ReadHM ) != 0 )
		{
			TerrainUtils::CopySubBuffer( tempHM, part.m_selectionSubRect, brushWidth, m_heightWorkingBuffers.GetBufferForTile( tile ), part.m_addressingRect, part.m_tile->GetResolution() );
		}
		if ( ( bufferFlags & PPBF_ReadCM ) != 0 )
		{
			TerrainUtils::CopySubBuffer( tempCM, part.m_selectionSubRect, brushWidth, tile->GetLevelSyncCM( 0 ), part.m_addressingRect, part.m_tile->GetResolution() );
		}
		if ( ( bufferFlags & PPBF_ReadColor ) != 0 )
		{
			TerrainUtils::CopySubBuffer( tempColor, part.m_colorSelectionSubRect, colorWidth, static_cast< const TColorMapType* >( tile->GetHighestColorMapData() ), part.m_colorAddressingRect, tile->GetHighestColorMapResolution() );
		}
	}

	// Setup brush
	brush.m_targetRect = Rect( 0, m_cursor.m_texelsPerEdge, 0, m_cursor.m_texelsPerEdge );
	brush.m_colorMapRect = Rect( 0, colorWidth - 2*brushColorExpansion, 0, colorHeight - 2*brushColorExpansion );

	brush.m_tileWidth = brushWidth;
	brush.m_tileColorWidth = colorWidth;

	// m_tileData points to start of data under brush. We're responsible for ensuring there is readable data before this point.
	if ( tempHM )
	{
		brush.m_tileData = tempHM + brushExpansion + brushExpansion * brushWidth;
	}
	if ( tempCM )
	{
		brush.m_tileCMData = tempCM + brushExpansion + brushExpansion * brushWidth;
	}
	if ( tempColor )
	{
		brush.m_tileColorData = tempColor + brushColorExpansion + brushColorExpansion * colorWidth;
	}


	brush.m_falloffData = m_cursor.m_falloffData;
	brush.m_brushWidth = m_cursor.m_texelsPerEdge;
	brush.m_brushColorWidth = colorWidth - brushColorExpansion * 2;

	brush.m_tileBorderSize = brushExpansion;
	brush.m_tileColorBorderSize = brushColorExpansion;


	SetupPaintingBrush( brush );

	// Brush settings are the same for all iterations, so we can just use the same info.
	for ( Uint32 i = 0; i < numTimes; ++i )
	{
		ApplyBrush( brush );
	}


	// Write back the new data into terrain. We can only write into a buffer if we previously read from it, so check for that too.
	for ( Uint32 i = 0; i < clipmapParts.Size(); ++i )
	{
		SClipmapHMQueryResult& part = clipmapParts[i];
		CTerrainTile* tile = part.m_tile;
		ASSERT( tile );

		if ( ( bufferFlags & PPBF_WriteHM ) != 0 )
		{
			ASSERT( ( bufferFlags & PPBF_ReadHM ) != 0 );
			if ( ( bufferFlags & PPBF_ReadHM ) != 0 )
			{
				TerrainUtils::CopySubBuffer( m_heightWorkingBuffers.GetBufferForTile( tile ), part.m_addressingRect, part.m_tile->GetResolution(), tempHM, part.m_selectionSubRect, brushWidth );
			}
		}
		if ( ( bufferFlags & PPBF_WriteCM ) != 0 )
		{
			ASSERT( ( bufferFlags & PPBF_ReadCM ) != 0 );
			if ( ( bufferFlags & PPBF_ReadCM ) != 0 )
			{
				TerrainUtils::CopySubBuffer( tile->GetLevelWriteSyncCM( 0 ), part.m_addressingRect, part.m_tile->GetResolution(), tempCM, part.m_selectionSubRect, brushWidth );
			}
		}
		if ( ( bufferFlags & PPBF_WriteColor ) != 0 )
		{
			ASSERT( ( bufferFlags & PPBF_ReadColor ) != 0 );
			if ( ( bufferFlags & PPBF_ReadColor ) != 0 )
			{
				TerrainUtils::CopySubBuffer( static_cast< TColorMapType* >( tile->GetHighestColorMapDataWrite() ), part.m_colorAddressingRect, tile->GetHighestColorMapResolution(), tempColor, part.m_colorSelectionSubRect, colorWidth );
			}
		}
	}

	RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, tempHM );
	RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, tempCM );
	RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Temporary, tempColor );
}

void CEdTerrainEditTool::ProcessPaintingStamp()
{
	if ( RIM_IS_KEY_DOWN( IK_Alt ) || RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		return;
	}

	// If we don't have any stored data, nothing to do
	if ( !( m_cursor.m_stampDataFlags & TESDF_DataTypesMask ) )
	{
		return;
	}

	CClipMap* terrain = m_world->GetTerrain();
	ASSERT( terrain );
	terrain->GetClipmapParameters( &m_terrainParameters );
	const Vector2 brushCenterTexelSpaceFNorm = terrain->GetTexelSpaceNormalizedPosition( m_cursor.m_position, false );

	const Int32 brushCenterTexelSpaceCol = brushCenterTexelSpaceFNorm.X * m_terrainParameters.clipmapSize;
	const Int32 brushCenterTexelSpaceRow = brushCenterTexelSpaceFNorm.Y * m_terrainParameters.clipmapSize;


	Uint32 scaledTexelsPerEdge = ( Uint32 )( m_cursor.m_brushSize * m_cursor.m_texelsPerUnit * 2 );
	Float scale = ( Float )scaledTexelsPerEdge / ( Float )m_cursor.m_texelsPerEdge;

	Int32 numTexelsX = m_cursor.m_texelsPerEdge;
	Int32 numTexelsY = m_cursor.m_texelsPerEdge;

	Float rotationAngle = m_cursor.m_stampParams.yawDegrees;

	// Figure out the full rectangle in the source data where we'll be pasting.
	Rect pasteAreaRect;
	{
		Vector rotatedRect = GetRotatedDataRect( scaledTexelsPerEdge, scaledTexelsPerEdge, rotationAngle );
		const Int32 destNumTexelsX = ( Int32 )MCeil( rotatedRect.Y - rotatedRect.X );
		const Int32 destNumTexelsY = ( Int32 )MCeil( rotatedRect.W - rotatedRect.Z );

		const Int32 centerTexelX = brushCenterTexelSpaceCol;
		const Int32 centerTexelY = brushCenterTexelSpaceRow;

		Int32 cornerTexelX = centerTexelX - destNumTexelsX / 2;
		Int32 cornerTexelY = centerTexelY - destNumTexelsY / 2;

		pasteAreaRect.m_left = cornerTexelX;
		pasteAreaRect.m_right = cornerTexelX + destNumTexelsX;
		pasteAreaRect.m_top = cornerTexelY;
		pasteAreaRect.m_bottom = cornerTexelY + destNumTexelsY;
	}

	// If the paste area is empty, don't have to do anything.
	if ( !pasteAreaRect.Width() || !pasteAreaRect.Height() )
	{
		return;
	}


	TDynArray< SClipmapHMQueryResult > clipmapParts;
	m_world->GetTerrain()->GetRectangleOfData( pasteAreaRect, clipmapParts );

	// Check if we have write access to all contributing tiles
	for ( Uint32 p = 0; p < clipmapParts.Size(); ++p )
	{
		// We don't want to apply changes partially
		if ( !clipmapParts[p].m_tile->MarkModified() )
		{
			return;
		}
	}


	for ( Uint32 p = 0; p < clipmapParts.Size(); ++p )
	{
		SClipmapHMQueryResult& part = clipmapParts[p];
		CUndoTerrain::AddStroke( m_viewport->GetUndoManager(), part.m_tile, part.m_row, part.m_col, part.m_addressingRect, part.m_colorAddressingRect );
	}


	Uint32 textureMask = 0;
	// Setup texture masking
	if ( m_useMaterialTextureMasking->GetValue() && ( m_useHorizontalTextureMask->GetValue() || m_useVerticalTextureMask->GetValue() ) )
	{
		Uint32 val = 1;

		// Use horizontal texture as mask
		if ( m_useHorizontalTextureMask->GetValue() )
		{
			val |= 0x02;
			val |= m_invertHorizontalTextureMask->GetValue() ? 0x08 : 0x00;
			val |= ( ( m_horizontalTextureMaskIndex + 1 ) << 5 );
		}

		// Use vertical texture as mask
		if ( m_useVerticalTextureMask->GetValue() )
		{
			val |= 0x04;
			val |= m_invertVerticalTextureMask->GetValue() ? 0x10 : 0x00;
			val |= ( ( m_verticalTextureMaskIndex + 1 ) << 10 );
		}

		textureMask = val;
	}


	CCurve* falloffCurve = m_cursor.GetUseFalloffCurve() ? m_cursor.m_falloffCurve : NULL;
	if ( m_cursor.m_stampDataFlags & TESDF_Height )
	{
		SStampBrushData< Uint16 > heightStampData;
		heightStampData.clipmapParts = &clipmapParts;
		heightStampData.stampData = m_cursor.m_stampHeightMap;
		heightStampData.stampWidth = numTexelsX;
		heightStampData.stampHeight = numTexelsY;
		heightStampData.angleDegrees = rotationAngle;
		heightStampData.brushScale = scale;
		heightStampData.heightScale = m_cursor.m_intensity;
		heightStampData.falloffCurve = falloffCurve;
		heightStampData.textureMask = textureMask;

		if ( m_cursor.m_stampDataFlags & TESDF_Additive )
		{
			heightStampData.heightOffset = HeightOffsetToTexels( m_cursor.m_desiredElevation );
			heightStampData.heightScale *= m_cursor.m_stampParams.multiplier;
			CopyRotatedScaledToClipmap< SApplyStampHMAdd >( heightStampData );
		}
		else
		{
			Float texelOffset = HeightOffsetToTexels( m_cursor.m_desiredElevation );
			Float texelOrigin = HeightToTexels( m_cursor.m_stampScaleOrigin );
			heightStampData.heightOffset = GetScaledOffset( texelOffset, m_cursor.m_intensity, texelOrigin );
			CopyRotatedScaledToClipmap< SApplyStampHM >( heightStampData );
		}
	}

	if ( m_cursor.m_stampDataFlags & TESDF_Color )
	{
		SStampBrushData< TColorMapType > colorStampData;
		colorStampData.clipmapParts = &clipmapParts;
		colorStampData.stampData = m_cursor.m_stampColorMap;
		colorStampData.stampWidth = m_cursor.m_stampColorTexelsPerEdge;
		colorStampData.stampHeight = m_cursor.m_stampColorTexelsPerEdge;
		colorStampData.angleDegrees = rotationAngle;
		colorStampData.brushScale = scale;
		colorStampData.falloffCurve = falloffCurve;
		colorStampData.textureMask = textureMask;
		CopyRotatedScaledToClipmap< SApplyStampColor >( colorStampData );
	}

	// NOTE: Control must be done LAST, so that texture masking works properly. Otherwise, masking will be done with the pasted
	// values, which isn't right...
	if ( m_cursor.m_stampDataFlags & TESDF_Control )
	{
		SStampBrushData< TControlMapType > controlStampData;
		controlStampData.clipmapParts = &clipmapParts;
		controlStampData.stampData = m_cursor.m_stampControlMap;
		controlStampData.stampWidth = numTexelsX;
		controlStampData.stampHeight = numTexelsY;
		controlStampData.angleDegrees = rotationAngle;
		controlStampData.brushScale = scale;
		controlStampData.falloffCurve = falloffCurve;
		controlStampData.textureMask = textureMask;
		CopyRotatedScaledToClipmap< SApplyStampCM >( controlStampData );
	}

	for ( Uint32 p = 0; p < clipmapParts.Size(); ++p )
	{
		SClipmapHMQueryResult& part = clipmapParts[p];

		// Make sure our working buffers are up-to-date.
		if ( m_cursor.m_stampDataFlags & TESDF_Height )
		{
			m_heightWorkingBuffers.CopyFromTile( part.m_tile, part.m_addressingRect );
		}

		part.m_tile->SetDirty( true );
		part.m_tile->LoadAllMipmapsSync();
		part.m_tile->RebuildMipmaps();
	}

	( new CRenderCommand_UpdateTerrainShadows( m_world->GetRenderSceneEx() ) )->Commit();

	// Hide the stamp preview, so user can see how it looks! It'll be reshown once the mouse is moved.
	SetStampVisible( false );
}


void CEdTerrainEditTool::OnStampOffsetChanged( wxCommandEvent& event )
{
	Float dx = 0.0f;
	float dy = 0.0f;
	FromString( m_stampOffsetX->GetValue().t_str(), dx );
	FromString( m_stampOffsetY->GetValue().t_str(), dy );

	m_cursor.m_position.X = m_cursor.m_stampSourcePosition.X + dx;
	m_cursor.m_position.Y = m_cursor.m_stampSourcePosition.Y + dy;
}

void CEdTerrainEditTool::OnStampPasteClicked( wxCommandEvent& event )
{
	ProcessPaintingStamp();
}


void CEdTerrainEditTool::SetStampOffsetControlsEnabled( Bool enabled )
{
	if ( enabled )
	{
		m_stampOffsetX->Enable();
		m_stampOffsetY->Enable();
		m_stampPaste->Enable();
	}
	else
	{
		m_stampOffsetX->ChangeValue( "" );
		m_stampOffsetY->ChangeValue( "" );
		m_stampSourceLocation->ChangeValue( "" );

		m_stampOffsetX->Disable();
		m_stampOffsetY->Disable();
		m_stampPaste->Disable();
	}
}


void CEdTerrainEditTool::OnRefreshAutoCollisionCurrent( wxCommandEvent& /*event*/ )
{
	RefreshAutoCollision( true );
}

void CEdTerrainEditTool::OnRefreshAutoCollision( wxCommandEvent& /*event*/ )
{
	RefreshAutoCollision( false );
}



void CEdTerrainEditTool::RefreshAutoCollision( Bool currentLayers )
{
	GFeedback->BeginTask( TXT("Scanning layers"), true );

	TDynArray< Box > localWaterBounds;
	TDynArray< Box > hubBorderBounds;

	// Collect everything we need to test against.
	{
		// For each tile, find the lowest water level there, and check if the tile is entirely some threshold below the surface.
		// If so, we can skip the tile. It will not need collision, it will not need rendering, etc.
		// We can also skip collision on any tiles that are outside the world's hub borders (walkable area).

		TDynArray< CLayerInfo* > layers;
		m_world->GetWorldLayers()->GetLayers( layers, false, true, true );

		for ( Uint32 i = 0; i < layers.Size(); ++i )
		{
			CLayerInfo* layerInfo = layers[i];

			if ( GFeedback->IsTaskCanceled() )
			{
				GFeedback->EndTask();
				return;
			}

			GFeedback->UpdateTaskInfo( layerInfo->GetDepotPath().AsChar() );
			GFeedback->UpdateTaskProgress( i, layers.Size() );

			Bool alreadyLoaded = layerInfo->IsLoaded();
			if ( !alreadyLoaded )
			{
				// Don't load anything if we're only doing the current layers
				if ( currentLayers )
				{
					continue;
				}

				LayerLoadingContext context;
				context.m_queueEvent = false;
				context.m_loadHidden = true;
				if ( !layerInfo->SyncLoad( context ) )
				{
					ERR_ENGINE( TXT("Failed to load layer %ls"), layerInfo->GetDepotPath().AsChar() );
					continue;
				}
			}

			TDynArray< CEntity* > entities;
			layerInfo->GetLayer()->GetEntities( entities );

			for ( CEntity* entity : entities )
			{
				if ( GFeedback->IsTaskCanceled() )
				{
					GFeedback->EndTask();
					return;
				}

				if ( ! entity )
				{
					continue;
				}

				entity->CreateStreamedComponents( SWN_NotifyWorld );

				for ( ComponentIterator< CWaterComponent > iter( entity ); iter; ++iter )
				{
					localWaterBounds.PushBack( ( *iter )->GetBoundingBox() );
				}

				if ( entity->HasTag( RED_NAME(hub_exit_entity) ) )
				{
					CTriggerAreaComponent* area = entity->FindComponent< CTriggerAreaComponent >();
					if ( area != nullptr )
					{
						hubBorderBounds.PushBack( area->GetBoundingBox() );
					}
				}
			}

			if ( !alreadyLoaded )
			{
				layerInfo->SyncUnload();
			}
		}
	}

	GFeedback->UpdateTaskName( TXT("Processing tiles") );

	const Uint32 numTiles = m_world->GetTerrain()->GetNumTilesPerEdge();
	TDynArray< ETerrainTileCollision > newStates( numTiles * numTiles );

	Float depthLimit;
	{
		wxTextCtrl* depthLimitCtrl = XRCCTRL( *m_dialog, "m_collAutoDepthLimit", wxTextCtrl );
		// If we can't get a valid value, revert to default.
		if ( !FromString( depthLimitCtrl->GetValue().wc_str(), depthLimit ) )
		{
			depthLimit = DEFAULT_AUTO_COLLISION_DEPTH_LIMIT;
			depthLimitCtrl->SetValue( ToString( depthLimit ).AsChar() );
		}
	}

	for ( Uint32 y = 0; y < numTiles; ++y )
	{
		for ( Uint32 x = 0; x < numTiles; ++x )
		{
			if ( GFeedback->IsTaskCanceled() )
			{
				GFeedback->EndTask();
				return;
			}


			GFeedback->UpdateTaskInfo( TXT("%u, %u"), x, y );
			GFeedback->UpdateTaskProgress( x + y * numTiles, numTiles * numTiles );

			CTerrainTile* tile = m_world->GetTerrain()->GetTile( x, y );
			ETerrainTileCollision& newState = newStates[ x + y * numTiles ];

			// If collision is forced, don't do anything.
			if ( tile->GetCollisionType() == TTC_ForceOn || tile->GetCollisionType() == TTC_ForceOff )
			{
				newState = tile->GetCollisionType();
				continue;
			}

			Box tileBounds = m_world->GetTerrain()->GetBoxForTile( x, y, 0.0f );

			// If it's outside all hub borders, we don't need collision.
			{
				Bool isInHub = hubBorderBounds.Empty();
				for ( const Box& bounds : hubBorderBounds )
				{
					if ( tileBounds.Touches2D( bounds ) )
					{
						isInHub = true;
						break;
					}
				}

				if ( !isInHub )
				{
					newState = TTC_AutoOff;
					continue;
				}
			}

			// If we're sufficiently below the water's lowest point, then we don't need collision.
			{
				const Float maxTerrainHeight = m_world->GetTerrain()->GetTileMaximumHeight( tile );
				
				// Definitely want collision if we're above water :) Avoid looping over all local water
				if ( maxTerrainHeight < -depthLimit )
				{
					Bool isNotDeep = false;
					for ( const Box& bounds : localWaterBounds )
					{
						if ( tileBounds.Touches2D( bounds ) && maxTerrainHeight > bounds.Min.Z - depthLimit )
						{
							isNotDeep = true;
							break;
						}
					}
					if ( !isNotDeep )
					{
						newState = TTC_AutoOff;
						continue;
					}
				}
			}

			newState = TTC_AutoOn;
		}
	}

	GFeedback->EndTask();


	// First, we need to check out everything that we're changing.
	for ( Uint32 y = 0; y < numTiles; ++y )
	{
		for ( Uint32 x = 0; x < numTiles; ++x )
		{
			CTerrainTile* tile = m_world->GetTerrain()->GetTile( x, y );
			if ( tile->GetCollisionType() != newStates[ x + y * numTiles ] )
			{
				if ( tile && !tile->MarkModified() )
				{
					return;
				}
			}
		}
	}

	GFeedback->BeginTask( TXT("Applying changes"), false );

	CUndoTerrain::CreateStep( m_viewport->GetUndoManager(), TXT("Refresh Auto Collision"), m_world->GetTerrain(), false, false, false, true );
	for ( Uint32 y = 0; y < numTiles; ++y )
	{
		for ( Uint32 x = 0; x < numTiles; ++x )
		{
			CTerrainTile* tile = m_world->GetTerrain()->GetTile( x, y );
			if ( tile && tile->GetCollisionType() != newStates[ x + y * numTiles ] )
			{
				// rects don't really matter for undo/redo collision, but we'll just say we're modifying the full tile.
				{
					Uint32 tileRes = tile->GetResolution();
					Uint32 colorRes = tile->GetHighestColorMapResolution();
					Rect tileRect( 0, tileRes, 0, tileRes );
					Rect colorRect( 0, colorRes, 0, colorRes );
					CUndoTerrain::AddStroke( m_viewport->GetUndoManager(), tile, x, y, tileRect, colorRect );
				}

				tile->SetCollisionType( newStates[ x + y * numTiles ] );
			}
		}
	}
	CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );

	if ( m_saveStrokesCheckBox->GetValue() )
	{
		GFeedback->UpdateTaskInfo( TXT("") );
		GFeedback->UpdateTaskName( TXT("Saving tiles") );
		m_world->GetTerrain()->SaveAllTiles();
	}

	UpdateTerrainCollisionMask();

	GFeedback->EndTask();
}


void CEdTerrainEditTool::ProcessPaintingCollision()
{
	Int32 x, y;
	CTerrainTile* tile = m_world->GetTerrain()->GetTileFromPosition( m_cursor.m_position, x, y, false );
	if ( !tile )
	{
		return;
	}

	// Same as is done in ProcessPainting(), we need to check out the tile we're modifying. If the user is asked to check out the tile,
	// we're going to get a mouse-up event, which will stop the current stroke before we've applied it. So we need to account for that.
	Bool stopAfterThisPaint = false;
	// Check if we have write access to all contributing tiles.
	if ( !tile->MarkModified() )
	{
		// We don't want to apply changes partially
		StopPainting( m_viewport->GetViewport() );
		return;
	}

	// While marking the tile modified, if we brought up the popup asking the user to check out a file, then we got a "mouse up" notification
	// and stopped painting. So, let's deal with that...
	if ( !m_paintingRequest )
	{
		StartPainting( m_viewport->GetViewport() );
		stopAfterThisPaint = true;
	}


	// rects don't really matter for undo/redo collision, but we'll just say we're modifying the full tile.
	Uint32 tileRes = tile->GetResolution();
	Uint32 colorRes = tile->GetHighestColorMapResolution();
	Rect tileRect( 0, tileRes, 0, tileRes );
	Rect colorRect( 0, colorRes, 0, colorRes );
	CUndoTerrain::AddStroke( m_viewport->GetUndoManager(), tile, x, y, tileRect, colorRect );

	ETerrainTileCollision typeToSet;
	switch ( m_cursor.m_toolType )
	{
	case TT_CollForceOn:	typeToSet = TTC_ForceOn;	break;
	case TT_CollForceOff:	typeToSet = TTC_ForceOff;	break;
	case TT_CollReset:		typeToSet = TTC_AutoOn;		break;
	}

	tile->SetCollisionType( typeToSet );

	UpdateTerrainCollisionMask();


	if ( stopAfterThisPaint )
	{
		StopPainting( m_viewport->GetViewport() );
	}
}

void CEdTerrainEditTool::UpdateTerrainCollisionMask()
{
	// If we don't have the world initialized yet (maybe we're just loading the world, and tool is open to begin with), we need
	// to try again, so we can be sure the collision mask overlay is set.
	if ( !m_world || !m_world->GetTerrain() || !m_world->GetTerrain()->GetTerrainProxy() )
	{
		m_needToUpdateCollisionOverlay = true;
		return;
	}

	m_needToUpdateCollisionOverlay = false;

	// Clear collision state overlay
	if ( m_editorTabs->GetCurrentPage() != XRCCTRL( *m_dialog, "m_collisionPanel", wxPanel ) )
	{
		( new CRenderCommand_ClearTerrainCustomOverlay( m_world->GetTerrain()->GetTerrainProxy() ) )->Commit();
		return;
	}

	Uint32	width = m_world->GetTerrain()->GetNumTilesPerEdge();
	Uint32	height = m_world->GetTerrain()->GetNumTilesPerEdge();
	TDynArray< Uint32 > overlay( width * height );
	for ( Uint32 y = 0; y < height; ++y )
	{
		for ( Uint32 x = 0; x < width; ++x )
		{
			CTerrainTile* tile = m_world->GetTerrain()->GetTile( x, y );
			overlay[ x + y * width ] = TerrainTileCollisionColors[ tile->GetCollisionType() ].ToUint32();
		}
	}

	( new CRenderCommand_SetTerrainCustomOverlay( m_world->GetTerrain()->GetTerrainProxy(), overlay, width, height ) )->Commit();
}

Int32 CEdTerrainEditTool::GetClipmapResolutionIndexForCurrentTerrain()
{
	if ( !m_world->GetTerrain() )
	{
		// No terrain exists
		return -1;
	}

	// Fetch settings from current terrain
	SClipmapParameters currentParams;
	m_world->GetTerrain()->GetClipmapParameters( &currentParams );

	// Find corresponding option
	Uint32 resolutionChoice = 0;
	for ( ; resolutionChoice < NUM_CLIPMAP_RESOLUTIONS; resolutionChoice++ )
	{
		if (g_clipMapResolutions[resolutionChoice] == currentParams.clipmapSize )
		{
			// Found
			break;
		}
	}
	if (resolutionChoice == NUM_CLIPMAP_RESOLUTIONS)
	{
		// Not found
		resolutionChoice = -1;
	}

	return resolutionChoice;
}

Int32 CEdTerrainEditTool::GetClipmapLODConfigIndexForCurrentTerrain()
{
	if ( !m_world->GetTerrain() )
	{
		// No terrain exists
		return -1;
	}

	// Fetch settings from current terrain
	SClipmapParameters currentParams;
	m_world->GetTerrain()->GetClipmapParameters( &currentParams );

	// Find corresponding option
	Uint32 lodChoice = 0;
	for ( ; lodChoice<NUM_CLIPMAP_CONFIGS; lodChoice++ )
	{
		if (g_clipMapConfig[lodChoice][0] == currentParams.clipSize && g_clipMapConfig[lodChoice][1] == currentParams.tileRes )
		{
			// Found
			break;
		}
	}
	if (lodChoice == NUM_CLIPMAP_CONFIGS)
	{
		// Not found
		lodChoice = -1;
	}

	return lodChoice;
}

Bool CEdTerrainEditTool::IsResolutionSupported( Int32 resolution )
{
	for ( Uint32 i = 0; i < NUM_CLIPMAP_RESOLUTIONS; ++i )
	{
		if ( g_clipMapResolutions[ i ] == resolution )
		{
			return true;
		}
	}

	// Not found supporting config
	return false;
}

Bool CEdTerrainEditTool::IsTileResolutionSupported( Int32 resolution )
{
	for ( Uint32 i = 0; i < NUM_CLIPMAP_CONFIGS; ++i )
	{
		if ( g_clipMapConfig[ i ][ 1 ] == resolution )
		{
			return true;
		}
	}

	// Not found supporting config
	return false;
}

Int32 CEdTerrainEditTool::GetClipmapResolutionIndexForResolution( int res )
{
	for ( Int32 i=0; i < NUM_CLIPMAP_RESOLUTIONS; ++i )
	{
		if (g_clipMapResolutions[i] == res )
		{
			// Found
			return i;
		}
	}

	// Not found
	return -1;
}

Int32 CEdTerrainEditTool::GetFirstClipmapConfigIndexForTileResolution( Int32 tileRes )
{
	for ( Int32 i=0; i<NUM_CLIPMAP_CONFIGS; ++i )
	{
		if ( g_clipMapConfig[i][1] == tileRes )
		{
			// Found
			return i;
		}
	}

	// Not found
	return -1;
}

Int32 CEdTerrainEditTool::GetClipmapConfigIndex( Int32 clipSize, Int32 tileRes )
{
	for ( Int32 i=0; i<NUM_CLIPMAP_CONFIGS; ++i )
	{
		if ( g_clipMapConfig[i][0] == clipSize && g_clipMapConfig[i][1] == tileRes )
		{
			// Found
			return i;
		}
	}

	// Not found
	return -1;
}


void CEdTerrainEditTool::UpdateBrushControlRanges()
{
	Float minBrushSize = 0.5f / m_cursor.m_texelsPerUnit;
	Float maxBrushSize = 0.5f * MAX_TEXELS_PER_EDGE / m_cursor.m_texelsPerUnit;

	// Set default control ranges.
	m_outsideRadiusControl.UpdateRange		(	minBrushSize,	maxBrushSize	);
	m_intensityControl.UpdateRange			(	           0,	           1	);
	m_heightControl.UpdateRange				(	           0,	       65535	);
	m_slopeAngleControl.UpdateRange			(	           0,	         360	);
	m_slopeControl.UpdateRange				(	           0,	          85	);
	m_slopeOffsetControl.UpdateRange		(	        -500,	         500	);
	m_filterSizeControl.UpdateRange			(	           1,	         100	);		// Filter size only used by smooth/melt. 0 doesn't make sense (no change)
	m_stampAngleControl.UpdateRange			(	           0,	         360	);
	m_stampScaleOriginControl.UpdateRange	(	        -500,	         500	);

	m_materialRadiusControl.UpdateRange		(	minBrushSize,	maxBrushSize	);

	m_colorRadiusControl.UpdateRange		(	minBrushSize,	maxBrushSize	);
	m_colorIntensityControl.UpdateRange		(	           0,	           1	);


	// Now make any adjustments based on current tool.
	switch ( m_cursor.m_toolType )
	{
	case TT_Stamp:
		{
			// 3 is a rather arbitrary upper bound, basically to allow copied terrain to be made bigger if wanted.
			m_intensityControl.UpdateRange( 0.0f, 3.0f );
			m_stampScaleOriginControl.UpdateRange( m_terrainParameters.lowestElevation, m_terrainParameters.highestElevation );

			Float heightRange = m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation;
			m_heightControl.UpdateRange( -heightRange, heightRange );
			break;
		}

	case TT_Flatten:
		{
			m_heightControl.UpdateRange( m_terrainParameters.lowestElevation, m_terrainParameters.highestElevation );
			break;
		}
	case TT_RiseLower:
		{
			// Allow a maximum rate of about half the terrain range per second. Should allow for building up big things pretty quick, but common values (smaller changes)
			// aren't clustered at just one end.
			Float maxAmt = ( m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation ) * 0.5f;
			m_heightControl.UpdateRange( 0, maxAmt );
			break;
		}

	case TT_PaintColor:
		{
			Float minColorBrushSize = 0.5f * ( m_terrainParameters.tileRes / m_terrainParameters.highestColorRes ) / m_cursor.m_texelsPerUnit;
			m_colorRadiusControl.UpdateRange( minColorBrushSize, maxBrushSize );
			break;
		}
	}
}


void CEdTerrainEditTool::EnableDisableShapeBrushControls( EToolType toolType )
{
	// NOTE: Some are empty, to show they do not have any sliders to be enabled/disabled here.
	// I'm _pretty sure_ C++ standard says the empty initializer gets filled with 0's, but even if not it'll just result in
	// sliders on a page other than the active one being randomly enabled/disabled. And then they'll be set up properly when
	// the user goes to that page.
	//
	// NOTE: Must be same order as EToolType enum!
	const Bool ShapeBrushEnabledSliders[][ 10 ] =
	{
		//	radius		intensity	height		slopeAngle	slope		slopeOffset		stampScaleOrigin	stampAngle		filterSize		heightmapButton
		{	false,		false,		false,		false,		false,		false,			false,				false,			false,			false	},		//TT_None,
		{	true,		true,		true,		false,		false,		false,			false,				false,			false,			false	},		//TT_Flatten,
		{	true,		true,		false,		true,		true,		true,			false,				false,			false,			false	},		//TT_Slope,
		{	true,		true,		false,		false,		false,		false,			false,				false,			true,			false	},		//TT_Smooth,
		{	true,		true,		false,		false,		false,		false,			false,				false,			false,			false	},		//TT_RiseLower,
		{																																			},		//TT_PaintTexture,
		{																																			},		//TT_PaintColor,
		{	true,		false,		false,		false,		false,		false,			false,				false,			false,			false	},		//TT_PaintTerrainHoles,
		{	true,		true,		true,		false,		false,		false,			true,				true,			false,			true	},		//TT_Stamp,
		{	true,		true,		false,		false,		false,		false,			false,				false,			true,			false	},		//TT_Melt,
		{																																			},		//TT_CollForceOn,
		{																																			},		//TT_CollForceOff,
		{																																			},		//TT_CollReset,
	};
	static_assert( ARRAY_COUNT( ShapeBrushEnabledSliders ) == TT_Max, "ShapeBrushEnabledSliders does not have the correct number of elements. Maybe a new tool was added?" );

	m_outsideRadiusControl.SetEnabled(	ShapeBrushEnabledSliders[ toolType ][ 0 ] );
	m_intensityControl.SetEnabled(		ShapeBrushEnabledSliders[ toolType ][ 1 ] );
	m_heightControl.SetEnabled(			ShapeBrushEnabledSliders[ toolType ][ 2 ] );
	m_slopeAngleControl.SetEnabled(		ShapeBrushEnabledSliders[ toolType ][ 3 ] );
	m_slopeControl.SetEnabled(			ShapeBrushEnabledSliders[ toolType ][ 4 ] );
	m_slopeOffsetControl.SetEnabled(	ShapeBrushEnabledSliders[ toolType ][ 5 ] );
	m_stampScaleOriginControl.SetEnabled(ShapeBrushEnabledSliders[ toolType ][ 6 ] );
	m_stampAngleControl.SetEnabled(		ShapeBrushEnabledSliders[ toolType ][ 7 ] );
	m_filterSizeControl.SetEnabled(		ShapeBrushEnabledSliders[ toolType ][ 8 ] );
	m_heightmapButton->Enable(			ShapeBrushEnabledSliders[ toolType ][ 9 ] );

	m_materialRadiusControl.SetEnabled( toolType == TT_PaintTexture );

	m_colorRadiusControl.SetEnabled( toolType == TT_PaintColor );
	m_colorIntensityControl.SetEnabled( toolType == TT_PaintColor );
}


void CEdTerrainEditTool::UpdateTerrainParamsControls()
{
	// Collect control feeds
	String sizeString = String::Printf( TXT("%1.0f"), m_terrainParameters.terrainSize );
	String minHeightString = String::Printf( TXT("%1.0f"), m_terrainParameters.lowestElevation );
	String maxHeightString = String::Printf( TXT("%1.0f"), m_terrainParameters.highestElevation );
	Uint32 lodChoice = GetClipmapConfigIndex( m_terrainParameters.clipSize, m_terrainParameters.tileRes );
	ASSERT( lodChoice >=0 );
	Int32 numTilesPerEdge = m_terrainParameters.clipmapSize / m_terrainParameters.tileRes;

	// Apply feeds
	m_numTilesPerEdgeChoice->SetSelection( numTilesPerEdge - 1 );
	m_lodConfigChoice->SetSelection(lodChoice);
	m_sizeText->SetValue( sizeString.AsChar() );
	m_minHeightText->SetValue( minHeightString.AsChar() );
	m_maxHeightText->SetValue( maxHeightString.AsChar() );
}


void CEdTerrainEditTool::InitializeSelectionArray()
{
	m_tilesSelection.ClearFast();

	CClipMap* terrain = m_world->GetTerrain();
	if ( terrain )
	{
		// Rebuild tiles selection flags array	
		for ( Uint32 i = 0; i < terrain->GetNumTilesPerEdge() * terrain->GetNumTilesPerEdge(); ++i )
		{
			m_tilesSelection.PushBack( false );
		}
	}
}

void CEdTerrainEditTool::GenerateTerrainParametersSummary( const SClipmapParameters& params, String& summary )
{
	const Int32 numTilesPerEdge = params.clipmapSize / params.tileRes;
	const Int32 numTiles = numTilesPerEdge * numTilesPerEdge;
	const Float terrainSize = params.terrainSize;
	const Int32 clipmapSize = params.clipmapSize;
	summary += String::Printf( TXT("Number of tiles: %i (%i x %i)\n"), numTiles, numTilesPerEdge, numTilesPerEdge );
	summary += String::Printf( TXT("Resolution of the whole terrain: (%i x %i)\n"), params.clipmapSize, params.clipmapSize );
	summary += String::Printf( TXT("Number of clipmap stack levels: %i\n"), CClipMap::ComputeNumberOfClipmapStackLevels( params.clipmapSize, params.clipSize ) );
	summary += String::Printf( TXT("One tile will cover %f meters\n"), terrainSize / numTilesPerEdge );
	summary += String::Printf( TXT("Spacing between vertices will be %f meters\n"), terrainSize / clipmapSize );
}

void CEdTerrainEditTool::UpdateMaterialParameter( Uint32 index, Float normalizedValue )
{
	ETextureBlendParam param = static_cast<ETextureBlendParam>( index );
	CClipMap* terrain = m_world->GetTerrain();
	if ( terrain )
	{
		terrain->MarkModified();
		terrain->SetTextureParam( m_lastSelectedTexture, (Int32)param, normalizedValue );
		m_world->GetFoliageEditionController().RefreshGrassMask();
	}
}

void CEdTerrainEditTool::SaveAdditionalInfo( Uint32 minX, Uint32 minY, Uint32 sizeX, Uint32 sizeY, const TDynArray< String >& exportedHMFiles, const TDynArray< String > exportedCMFiles, const String& dirPath, const String& filename )
{
	String fullCSVPath = String::Printf( TXT( "%s\\%s.csv" ), dirPath.AsChar(), filename.AsChar() );

	FILE* fp = fopen( UNICODE_TO_ANSI( fullCSVPath.AsChar() ), "w" );
	if ( !fp )
	{
		return;
	}

	fwprintf( fp, TXT( "%d %d %d %d\n" ), minX, minY, sizeX, sizeY );

	for ( Uint32 i = 0; i < exportedHMFiles.Size(); ++i )
	{
		String correctHMPath = String::Printf( TXT( "%s_input\\%s" ), filename.AsChar(), exportedHMFiles[ i ].AsChar() );
		fwprintf( fp, TXT( "%s\n" ), correctHMPath.AsChar() );
	}

	for ( Uint32 i = 0; i < exportedCMFiles.Size(); ++i )
	{
		String correctCMPath = String::Printf( TXT( "%s_input\\%s" ), filename.AsChar(), exportedCMFiles[ i ].AsChar() );
		fwprintf( fp, TXT( "%s\n" ), correctCMPath.AsChar() );
	}

	fclose( fp );
}

void CEdTerrainEditTool::ReadAdditionalInfo( const String& csvPath, Uint32& offsetX, Uint32& offsetY, Uint32& sizeX, Uint32& sizeY, TDynArray< String >& tilesToImport )
{
	FILE* fp = fopen( UNICODE_TO_ANSI( csvPath.AsChar() ), "r" );
	if ( !fp )
	{
		return;
	}

	Char buff[1024];
	fwscanf( fp, TXT( "%u" ), &offsetX );
	fwscanf( fp, TXT( "%u" ), &offsetY );
	fgetws( buff, 1024, fp );
	// buffer must contain something more than NULL that ends the buffer
	if ( Red::System::StringLength( buff ) > 1 )
	{
		swscanf( buff, TXT(" %u %u"), &sizeX, &sizeY );
	}
	else
	{
		while ( fgetws( buff, 1024, fp ) )
		{
			size_t newbuflen = Red::System::StringLength( buff );
			if ( buff[newbuflen - 1] == '\n' )
			{
				buff[newbuflen - 1] = '\0';
			}

			tilesToImport.PushBack( String( buff ) );

			Uint32 localX = 0;
			Uint32 localY = 0;
			CClipMap::GetCoordinatesFromWorldMachineFilename( tilesToImport.Back(), localX, localY );
			sizeX = Max< Uint32 >( sizeX, localX );
			sizeY = Max< Uint32 >( sizeY, localY );
		}

		// filenames are indexed in the range [0..sizeX-1] [0..sizeY-1]
		// determine the actual sizes
		++sizeX;
		++sizeY;
	}
	
	fclose( fp );
}


Vector CEdTerrainEditTool::GetSlopeReferencePoint()
{
	
	// referencePos is in texel coordinates (nearest texel)
	Vector refPos	= m_world->GetTerrain()->GetTexelSpaceNormalizedPosition( m_cursor.m_position );
	refPos.X = ( Int32 )MRound( refPos.X * m_terrainParameters.clipmapSize );
	refPos.Y = ( Int32 )MRound( refPos.Y * m_terrainParameters.clipmapSize );
	// referencePos.Z is meters
	m_world->GetTerrain()->GetHeightForTexelPosition( refPos, refPos.Z );

	return refPos;
}


void CEdTerrainEditTool::StartPainting( IViewport* view )
{
	// set it even if there is no tool selected to enable selection painting
	m_paintingRequest = true;

	// early-out if tool type not valid for painting
	if ( m_cursor.m_toolType == TT_None )
	{
		m_cursor.StoreScreenSpacePosition(); // for calculating paint selection sensitivity threshold
		return;
	}

	m_paintingRequestFrames = 0;
	view->SetMouseMode( MM_Clip );

	m_cursor.m_slopeParams.referencePos = GetSlopeReferencePoint();

	Uint32 bufferFlags = m_cursor.GetPaintingBufferFlags();
	CUndoTerrain::CreateStep( m_viewport->GetUndoManager(), m_cursor.GetToolName(), m_world->GetTerrain(),
		( bufferFlags & PPBF_WriteHM ) != 0,
		( bufferFlags & PPBF_WriteCM ) != 0,
		( bufferFlags & PPBF_WriteColor ) != 0,
		( bufferFlags & PPBF_CollisionType ) != 0 );

	m_cursor.m_lastPosition = m_cursor.m_position;

	m_world->GetTerrain()->SetIsEditing( true );
}

void CEdTerrainEditTool::StopPainting( IViewport* view )
{
	if ( !m_paintingRequest ) return;

	m_world->GetTerrain()->SetIsEditing( false );

	// Painting done
	m_paintingRequest = false;
	m_paintingRequestFrames = 0;
	m_paintSelectionActivated = false;
	
	// Uncapture mouse
	view->SetMouseMode( MM_Normal, true );

	CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );


	// If we were modifying HM, and weren't updating the shadows while drawing, update them now.
	if ( ( m_cursor.GetPaintingBufferFlags() & PPBF_WriteHM ) != 0 && !m_updateShadowsCheckBox->GetValue() )
	{
		( new CRenderCommand_UpdateTerrainShadows( m_world->GetRenderSceneEx() ) )->Commit();
	}

	if ( m_saveStrokesCheckBox->GetValue() )
	{
		m_world->GetTerrain()->SaveAllTiles();
	}

	m_world->GetFoliageEditionController().RefreshGrassMask();
}

void CEdTerrainEditTool::SetStampVisible( Bool visible )
{
	m_world->GetTerrain()->SetStampGizmoVisible( visible );
	m_viewport->GetViewport()->SetTerrainToolStampVisible( visible );
}


Float CEdTerrainEditTool::HeightOffsetToTexels( Float height )
{
	Float range = m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation;

	Float proportion = height;
	if ( range >= 1.f )
	{
		proportion /= range;
	}
	return proportion * 65536.f;

}

Uint16 CEdTerrainEditTool::HeightToTexels( Float height )
{
	Float diff = height - m_terrainParameters.lowestElevation;
	Float range = m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation;
	
	Float proportion = diff;
	if ( range >= 1.f )
	{
		proportion /= range;
	}
	Float texels = proportion * 65536.f;

	return (Uint16)Clamp( texels + 0.5f, 0.f, 65535.f );
}

Float CEdTerrainEditTool::TexelsToHeight( Uint16 texels )
{
	Float proportion = (Float)texels / 65536.f;
	Float range = m_terrainParameters.highestElevation - m_terrainParameters.lowestElevation;

	Float heightAboveLowest = proportion * range;

	Float height = m_terrainParameters.lowestElevation + heightAboveLowest;
	return height;
}

String CEdTerrainEditTool::GetBareFilename( const String& filenameWithExtension )
{
	size_t lastDotIndex;
	return filenameWithExtension.FindCharacter( TXT('.'),lastDotIndex, true ) ? filenameWithExtension.LeftString( lastDotIndex ) : filenameWithExtension;
}

void CEdTerrainEditTool::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

void CEdTerrainEditTool::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CEdTerrainEditTool::SaveSession( CConfigurationManager &config )
{
	if ( m_isStarted )
	{
		// Grab current settings
		m_sessionToolParams[ m_cursor.m_toolType ] = GetCurrControlParams();

		config.Write( TXT("tools/TerrainEdit/ActiveTab"), m_editorTabs->GetSelection() );

		config.Write( TXT("tools/TerrainEdit/PaintTextureActive"), m_paintTextureActive ? 1 : 0 );
		config.Write( TXT("tools/TerrainEdit/PaintColorActive"), m_paintColorActive ? 1 : 0 );
		config.Write( TXT("tools/TerrainEdit/ActiveShapeBrush"), ( Int32 )m_activeToolShapePage );
		config.Write( TXT("tools/TerrainEdit/ActiveCollisionBrush"), ( Int32 )m_activeToolCollisionPage );

		config.Write( TXT("Tools/TerrainEdit/ToolType"), m_cursor.m_toolType );

		config.Write( TXT("Tools/TerrainEdit/SubstepPainting"), m_substepPainting->GetValue() ? 1 : 0 );
		config.Write( TXT("Tools/TerrainEdit/DrawBrushAsOverlay"), m_drawBrushAsOverlay->GetValue() ? 1 : 0 );

		config.Write( TXT("Tools/TerrainEdit/UpdateShadows"), m_updateShadowsCheckBox->GetValue() ? 1 : 0 );
		config.Write( TXT("Tools/TerrainEdit/SaveStrokes"), m_saveStrokesCheckBox->GetValue() ? 1 : 0 );
		LoadSaveToolControlParams( config, m_sessionToolParams, true );
		Int32 x, y;
		m_dialog->GetPosition( &x, &y );
		config.Write( TXT("Tools/TerrainEdit/PosX"), x );
		config.Write( TXT("Tools/TerrainEdit/PosY"), y );

		config.Write( TXT("Tools/TerrainEdit/UseFalloff"), m_useFalloffCheckBox->GetValue() ? 1 : 0 );
		config.Write( TXT("Tools/TerrainEdit/UseColorFalloff"), m_colorUseFalloffCheckBox->GetValue() ? 1 : 0 );
		
		for ( Uint32 i = 0; i < m_presetsRadioBtns.Size(); ++i )
		{
			String confNameBeginning = String::Printf( TXT("Tools/TerrainEdit/MaterialPairSlot%d/"), i + 1 );
			config.Write( confNameBeginning + TXT("SelectedHorizontalTexture"), m_presetsData[i].selectedHorizontalTexture );
			config.Write( confNameBeginning + TXT("SelectedVerticalTexture"), m_presetsData[i].selectedVerticalTexture );
			config.Write( confNameBeginning + TXT("SlopeThresholdAction"), ( Int32 )m_presetsData[i].slopeThresholdAction );
			config.Write( confNameBeginning + TXT("SlopeThresholdIndex"), ( Int32 )m_presetsData[i].slopeThresholdIndex );
			config.Write( confNameBeginning + TXT("VerticalUVMult"), ( Int32 )m_presetsData[i].verticalUVMult );
			config.Write( confNameBeginning + TXT("Probability"), m_presetsData[i].probability );
			config.Write( confNameBeginning + TXT("VerticalUVScaleMask"), m_presetsData[i].verticalUVScaleMask ? 1 : 0 );
			config.Write( confNameBeginning + TXT("SlopeThresholdMask"), m_presetsData[i].slopeThresholdMask ? 1 : 0 );
			config.Write( confNameBeginning + TXT("HorizontalMask"), m_presetsData[i].horizontalMask ? 1 : 0 );
			config.Write( confNameBeginning + TXT("VerticalMask"), m_presetsData[i].verticalMask ? 1 : 0 );

			config.Write( confNameBeginning + TXT("HeightLowLimit"), (Float)m_presetsData[i].lowLimit );
			config.Write( confNameBeginning + TXT("HeightHighLimit"), (Float)m_presetsData[i].highLimit );
			config.Write( confNameBeginning + TXT("LowLimitMask"), m_presetsData[i].lowLimitMask ? 1 : 0 );
			config.Write( confNameBeginning + TXT("HighLimitMask"), m_presetsData[i].highLimitMask ? 1 : 0 );

			config.Write( confNameBeginning + TXT("PresetEnabled"), m_presetsRadioBtns[i]->IsEnabled() ? 1 : 0 );
		}
		config.Write( TXT("Tools/TerrainEdit/PresetIndex"), (Int32)m_presetIndex );

		config.Write( TXT("Tools/TerrainEdit/UseMaterialTextureMasking"), m_useMaterialTextureMasking->GetValue() ? 1 : 0 );
		config.Write( TXT("Tools/TerrainEdit/BrushProbability"), m_brushProbability->GetValue() );
		config.Write( TXT("Tools/TerrainEdit/TextureParametersSplitterPosition"), XRCCTRL( *m_dialog, "m_textureParametersSplitter", wxSplitterWindow )->GetSashPosition() );

		// Write out the color presets.
		for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
		{
			String path = String::Printf( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/Color%d" ), i );
			config.Write( path + TXT( "/R" ), m_colorPresets[ i ].Red() );
			config.Write( path + TXT( "/G" ), m_colorPresets[ i ].Green() );
			config.Write( path + TXT( "/B" ), m_colorPresets[ i ].Blue() );
		}

		config.Write( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/R"), m_currentColor.Red() );
		config.Write( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/G"), m_currentColor.Green() );
		config.Write( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/B"), m_currentColor.Blue() );

		config.Write( TXT("Tools/TerrainEdit/CursorLowLimitSet"),  m_cursor.m_lowLimit.IsInitialized() ? 1 : 0 );
		config.Write( TXT("Tools/TerrainEdit/CursorHighLimitSet"), m_cursor.m_highLimit.IsInitialized() ? 1 : 0 );

		{
			wxTextCtrl* depthLimitCtrl = XRCCTRL( *m_dialog, "m_collAutoDepthLimit", wxTextCtrl );
			Float depthLimit = DEFAULT_AUTO_COLLISION_DEPTH_LIMIT;
			FromString( depthLimitCtrl->GetValue().wc_str(), depthLimit );
			config.Write( TXT("Tools/TerrainEdit/Collision/DepthLimit"), depthLimit );
		}
	}
}

void CEdTerrainEditTool::RestoreSession( CConfigurationManager &config )
{
	LoadSaveToolControlParams( config, m_sessionToolParams, false );

	Int32 page = config.Read( TXT("tools/TerrainEdit/ActiveTab"), m_editorTabs->GetSelection() );
	if ( page >= 0 )
	{
		m_editorTabs->SetSelection( ( size_t )page );
	}

	m_substepPainting->SetValue( config.Read( TXT("Tools/TerrainEdit/SubstepPainting"), 1 ) != 0 );
	m_drawBrushAsOverlay->SetValue( config.Read( TXT("Tools/TerrainEdit/DrawBrushAsOverlay"), 1 ) != 0 );

	m_useFalloffCheckBox->SetValue( config.Read( TXT("Tools/TerrainEdit/UseFalloff"), 1 ) != 0 );
	m_colorUseFalloffCheckBox->SetValue( config.Read( TXT("Tools/TerrainEdit/UseColorFalloff"), 1 ) != 0 );


	m_paintTextureActive = config.Read( TXT("tools/TerrainEdit/PaintTextureActive"), 0 ) != 0;
	m_paintColorActive = config.Read( TXT("tools/TerrainEdit/PaintColorActive"), 0 ) != 0;
	m_activeToolShapePage = ( EToolType )config.Read( TXT("tools/TerrainEdit/ActiveShapeBrush"), TT_None );
	m_activeToolCollisionPage = ( EToolType )config.Read( TXT("tools/TerrainEdit/ActiveCollisionBrush"), TT_None );


	EToolType toolType = ( EToolType )config.Read( TXT("Tools/TerrainEdit/ToolType"), TT_None );
	SelectTool( toolType );

	Int32 x, y;
	m_dialog->GetPosition( &x, &y );
	x = config.Read( TXT("Tools/TerrainEdit/PosX"), x );
	y = config.Read( TXT("Tools/TerrainEdit/PosY"), y );
	m_dialog->Move( x, y );
	
	Int32 checked;

	checked = config.Read( TXT("Tools/TerrainEdit/UpdateShadows"), 0 );
	m_updateShadowsCheckBox->SetValue( checked != 0 );

	checked = config.Read( TXT("Tools/TerrainEdit/SaveStrokes"), 1 );
	m_saveStrokesCheckBox->SetValue( checked != 0 );

	XRCCTRL( *m_dialog, "m_textureParametersSplitter", wxSplitterWindow )->SetSashPosition( config.Read( TXT("Tools/TerrainEdit/TextureParametersSplitterPosition"), 1 ) );

	for ( Uint32 i = 0; i < m_presetsRadioBtns.Size(); ++i )
	{
		String confNameBeginning = String::Printf( TXT("Tools/TerrainEdit/MaterialPairSlot%d/"), i + 1 );

		m_presetsData[i].selectedHorizontalTexture = config.Read( confNameBeginning + TXT("SelectedHorizontalTexture"), 0 );
		m_presetsData[i].selectedVerticalTexture = config.Read( confNameBeginning + TXT("SelectedVerticalTexture"), 0 );
		m_presetsData[i].slopeThresholdAction = ( ETerrainPaintSlopeThresholdAction )config.Read( confNameBeginning + TXT("SlopeThresholdAction"), 0 );
		m_presetsData[i].slopeThresholdIndex = config.Read( confNameBeginning + TXT("SlopeThresholdIndex"), 0 );
		m_presetsData[i].verticalUVMult = config.Read( confNameBeginning + TXT("VerticalUVMult"), 0 );
		m_presetsData[i].probability = config.Read( confNameBeginning + TXT("Probability"), 100 );
		m_presetsData[i].verticalUVScaleMask = config.Read( confNameBeginning + TXT("VerticalUVScaleMask"), 1 ) != 0;
		m_presetsData[i].slopeThresholdMask = config.Read( confNameBeginning + TXT("SlopeThresholdMask"), 1 ) != 0;
		m_presetsData[i].horizontalMask = config.Read( confNameBeginning + TXT("HorizontalMask"), 1 ) != 0;
		m_presetsData[i].verticalMask = config.Read( confNameBeginning + TXT("VerticalMask"), 1 ) != 0;

		m_presetsData[i].lowLimit = config.Read( confNameBeginning + TXT("HeightLowLimit" ), 0.f );
		m_presetsData[i].highLimit = config.Read( confNameBeginning + TXT("HeightHighLimit" ), 0.f );
		m_presetsData[i].lowLimitMask = config.Read( confNameBeginning + TXT("LowLimitMask"), 0 ) != 0;
		m_presetsData[i].highLimitMask = config.Read( confNameBeginning + TXT("HighLimitMask"), 0 ) != 0;

		if ( i < m_presetCheckBoxes.Size() )
		{
			m_presetCheckBoxes[i]->SetValue( config.Read( confNameBeginning + TXT("PresetEnabled"), i < 2 ? 1 : 0 ) != 0 );
		}
	}
	Uint32 presetInd = Min< Uint32 > ( 5, config.Read( TXT("Tools/TerrainEdit/PresetIndex"), 0 ) );
	UpdatePresetsActivity();

	m_useMaterialTextureMasking->SetValue( config.Read( TXT("Tools/TerrainEdit/UseMaterialTextureMasking"), 0 ) != 0 );
	m_brushProbability->SetValue( config.Read( TXT("Tools/TerrainEdit/BrushProbability"), 100 ) );
	SetPreset( presetInd );

	Uint8 currR = config.Read( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/R"), 128 );
	Uint8 currG = config.Read( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/G"), 128 );
	Uint8 currB = config.Read( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/CurrentColor/B"), 128 );
	m_currentColor.Set( currR, currG, currB );

	for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
	{
		String path = String::Printf( TXT("Tools/TerrainEdit/ToolTypes/PaintColor/Color%d" ), i );

		Uint8 r = config.Read( path + TXT( "/R" ), DefaultColorPresets[ i ].Red() );
		Uint8 g = config.Read( path + TXT( "/G" ), DefaultColorPresets[ i ].Green() );
		Uint8 b = config.Read( path + TXT( "/B" ), DefaultColorPresets[ i ].Blue() );

		SetColorPreset( i, wxColour( r, g, b ) );
	}

	{
		Float depthLimit = config.Read( TXT("Tools/TerrainEdit/Collision/DepthLimit"), DEFAULT_AUTO_COLLISION_DEPTH_LIMIT );
		XRCCTRL( *m_dialog, "m_collAutoDepthLimit", wxTextCtrl )->SetValue( ToString( depthLimit ).AsChar() );
	}
}

STerrainToolControlsParams CEdTerrainEditTool::GetCurrControlParams() const
{
	STerrainToolControlsParams params;

	params.radius			= m_outsideRadiusControl.GetValue();
	params.intensity		= m_intensityControl.GetValue();
	params.height			= m_heightControl.GetValue();
	params.slopeAngle		= m_slopeAngleControl.GetValue();
	params.stampAngle		= m_stampAngleControl.GetValue();
	params.stampScaleOrigin	= m_stampScaleOriginControl.GetValue();
	params.slope			= m_slopeControl.GetValue();
	params.offset			= m_slopeOffsetControl.GetValue();
	params.filterSize		= m_filterSizeControl.GetValue();
	params.heightmapPath	= m_cursor.m_heightmapPath;

	// Some tools need to use different sliders.
	switch ( m_cursor.m_toolType )
	{
	case TT_PaintTexture:
		params.radius		= m_materialRadiusControl.GetValue();
		break;

	case TT_PaintColor:
		params.radius		= m_colorRadiusControl.GetValue();
		params.intensity	= m_colorIntensityControl.GetValue();
		break;
	}

	return params;
}

void CEdTerrainEditTool::SetControlParams( STerrainToolControlsParams &params )
{
	m_outsideRadiusControl.UpdateValue( params.radius );
	m_intensityControl.UpdateValue( params.intensity );
	m_heightControl.UpdateValue( params.height );
	m_slopeAngleControl.UpdateValue( params.slopeAngle );
	m_stampScaleOriginControl.UpdateValue( params.stampScaleOrigin );
	m_stampAngleControl.UpdateValue( params.stampAngle );
	m_slopeControl.UpdateValue( params.slope );
	m_slopeOffsetControl.UpdateValue( params.offset );
	m_filterSizeControl.UpdateValue( params.filterSize );

	switch ( m_cursor.m_toolType )
	{
	case TT_PaintTexture:
		m_materialRadiusControl.UpdateValue( params.radius );
		break;

	case TT_PaintColor:
		m_colorRadiusControl.UpdateValue( params.radius );
		m_colorIntensityControl.UpdateValue( params.intensity );
		break;
	}

	// GetCurrControlParams will overwrite the heightmapPath parameter with whatever's currently on the cursor,
	// so we'll make sure it's set correctly first. The other params come just from the sliders, set above, so
	// no worries there.
	m_cursor.m_heightmapPath = params.heightmapPath;

	// Read back values from sliders. If valid range has decreased since the last time we used this tool, the values
	// may be truncated.
	params = GetCurrControlParams();


	m_cursor.SetControlParams( params );
}

Bool CEdTerrainEditTool::HandleSlopeBrushInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( key == IK_LControl )
	{
		if ( action == IACT_Press )
		{
			if ( RIM_IS_KEY_DOWN( IK_Alt ) )
			{
				m_cursor.SetCursorMode( CM_SlopeOffsetChange );
				m_cursor.StoreScreenSpacePosition();
			}
			else
			{
				view->SetMouseMode( MM_ClipAndCapture );
				m_cursor.SetCursorMode( CM_SlopeRotationChange );
				m_cursor.StoreScreenSpacePosition();
			}
		}
		else if ( action == IACT_Release )
		{
			view->SetMouseMode( MM_Normal, true );
			m_cursor.SetCursorMode( CM_Paint );
			m_cursor.RetrieveScreenSpacePosition();
		}
		return true;
	}
	else if ( key == IK_Alt )
	{
		if ( action == IACT_Press )
		{
			if ( RIM_IS_KEY_DOWN( IK_LControl ) )
			{
				m_cursor.SetCursorMode( CM_SlopeOffsetChange );
				m_cursor.StoreScreenSpacePosition();
			}
			else
			{
				view->SetMouseMode( MM_ClipAndCapture );
				m_cursor.SetCursorMode( CM_SlopeAngleChange );
				m_cursor.StoreScreenSpacePosition();
			}
		}
		else if ( action == IACT_Release )
		{
			view->SetMouseMode( MM_Normal, true );
			m_cursor.SetCursorMode( CM_Paint );			
			m_cursor.RetrieveScreenSpacePosition();
		}
		return true;
	}

	// handle Undo on our own because of manually setting mouse mode on viewport
	if ( key == IK_Z && action == IACT_Release && GetKeyState( VK_CONTROL ) < 0  )
	{
		CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );
		m_viewport->GetUndoManager()->Undo();
		return true;
	}
	if ( key == IK_Y && action == IACT_Release && GetKeyState( VK_CONTROL ) < 0 )
	{
		CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );
		m_viewport->GetUndoManager()->Redo();
		return true;
	}

	Bool  handled = false;
	Float slopeDelta  = 0.f;
	Float angleDelta  = 0.f;
	Float offsetDelta = 0.f;

	if ( IK_T == key )	{ handled = true; slopeDelta  += 5.f;  }
	if ( IK_G == key )	{ handled = true; slopeDelta  -= 5.f;  }
	if ( IK_F == key )	{ handled = true; angleDelta  -= 5.f;  }
	if ( IK_H == key )	{ handled = true; angleDelta  += 5.f;  }
	if ( IK_Y == key )	{ handled = true; offsetDelta += 10.f; }
	if ( IK_R == key )	{ handled = true; offsetDelta -= 10.f; }

	if ( 0.f != slopeDelta )
	{
		Float prevValue = m_slopeControl.GetValue();
		Float newValue  = Clamp( prevValue + slopeDelta, m_slopeControl.GetMin(), m_slopeControl.GetMax() );		
		m_slopeControl.UpdateValue( newValue );
	}

	if ( 0.f != angleDelta )
	{
		Float range = (m_slopeAngleControl.GetMax() - m_slopeAngleControl.GetMin());
		Float prevValue = m_slopeAngleControl.GetValue();
		Float newValue  = fmod( Max( 0.f, (prevValue + angleDelta - m_slopeAngleControl.GetMin() + 10.f * range)), range ) + m_slopeAngleControl.GetMin();		
		m_slopeAngleControl.UpdateValue( newValue );
	}

	if ( 0.f != offsetDelta )
	{
		Float prevValue = m_slopeOffsetControl.GetValue();
		Float newValue  = Clamp( prevValue + offsetDelta, m_slopeOffsetControl.GetMin(), m_slopeOffsetControl.GetMax() );		
		m_slopeOffsetControl.UpdateValue( newValue );
	}

	return handled;
}

Bool CEdTerrainEditTool::HandleStampBrushInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( key == IK_LControl )
	{
		if ( action == IACT_Press )
		{
			view->SetMouseMode( MM_ClipAndCapture );
			m_cursor.SetCursorMode( CM_StampRotationChange );
			m_cursor.StoreScreenSpacePosition();
		}
		else if ( action == IACT_Release )
		{
			view->SetMouseMode( MM_Normal, true );
			m_cursor.SetCursorMode( CM_Paint );
			m_cursor.RetrieveScreenSpacePosition();
		}
		return true;
	}

	// Hide existing stamp data while holding the "capture" button.
	if ( key == IK_Alt )
	{
		if ( action == IACT_Press )
		{
			view->SetMouseMode( MM_ClipAndCapture );
			SetStampVisible( false );
		}
		else if ( action == IACT_Release )
		{
			view->SetMouseMode( MM_Normal, true );
			SetStampVisible( true );
		}
		return true;
	}

	if ( key == IK_LShift )
	{
		if ( action == IACT_Press )
		{
			SetStampVisible( false );
		}
		else if ( action == IACT_Release )
		{
			SetStampVisible( true );
		}
		return true;
	}

	// handle Undo on our own because of manually setting mouse mode on viewport
	if ( key == IK_Z && action == IACT_Release && GetKeyState( VK_CONTROL ) < 0  )
	{
		CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );
		m_viewport->GetUndoManager()->Undo();
		return true;
	}
	if ( key == IK_Y && action == IACT_Release && GetKeyState( VK_CONTROL ) < 0 )
	{
		CUndoTerrain::FinishStep( m_viewport->GetUndoManager() );
		m_viewport->GetUndoManager()->Redo();
		return true;
	}

	if ( key == IK_Tilde && action == IACT_Release )
	{
		m_cursor.m_stampParams.multiplier *= -1.f;
		return true;
	}


	// Press '/' to clear current stamp.
	if ( key == IK_Slash && action == IACT_Release )
	{
		SetStampOffsetControlsEnabled( false );

		m_cursor.ClearStamp();
		m_world->GetTerrain()->ClearStampData();
		SetStampVisible( false );
		SetBrushThumbnail();
		return true;
	}

	return false;
}

void CEdTerrainEditTool::HandleMouseZBrushGeneral( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;
	if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_intensity ) ) / MAbs( m_intensityControl.GetMax() );
		m_cursor.m_intensity += data * ( m_intensityControl.GetMax() - m_intensityControl.GetMin() ) * mod;
		m_cursor.m_intensity  = Min( m_cursor.m_intensity, m_intensityControl.GetMax() );
		m_intensityControl.UpdateValue( m_cursor.m_intensity );
	}
	else if ( RIM_IS_KEY_DOWN( IK_LControl ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_desiredElevation ) ) / MAbs( m_heightControl.GetMax() );
		m_cursor.m_desiredElevation += data * ( m_heightControl.GetMax() - m_heightControl.GetMin() ) * mod;
		m_cursor.m_desiredElevation  = Min( m_cursor.m_desiredElevation, m_heightControl.GetMax() );
		m_heightControl.UpdateValue( m_cursor.m_desiredElevation );
	}
	else
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_outsideRadiusControl.GetMax() );
		m_cursor.m_brushSize += data * ( m_outsideRadiusControl.GetMax() - m_outsideRadiusControl.GetMin() ) * mod;
		m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_outsideRadiusControl.GetMin(), m_outsideRadiusControl.GetMax() );
		m_outsideRadiusControl.UpdateValue( m_cursor.m_brushSize );
	}
}

void CEdTerrainEditTool::HandleMouseZBrushSlope( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;

	if ( RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_slopeParams.offset ) ) / MAbs( m_slopeOffsetControl.GetMax() );
		m_cursor.m_slopeParams.offset += data * ( m_slopeOffsetControl.GetMax() - m_slopeOffsetControl.GetMin() ) * mod;
		m_cursor.m_slopeParams.offset  = Min( m_cursor.m_slopeParams.offset, m_slopeOffsetControl.GetMax() );
		m_slopeOffsetControl.UpdateValue( m_cursor.m_slopeParams.offset );
	}
	else if ( RIM_IS_KEY_DOWN( IK_LControl ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_slopeParams.yawDegrees ) ) / MAbs( m_slopeAngleControl.GetMax() );
		m_cursor.m_slopeParams.yawDegrees += data * ( m_slopeAngleControl.GetMax() - m_slopeAngleControl.GetMin() ) * mod;
		m_cursor.m_slopeParams.yawDegrees  = Min( m_cursor.m_slopeParams.yawDegrees, m_slopeAngleControl.GetMax() );
		m_slopeAngleControl.UpdateValue( m_cursor.m_slopeParams.yawDegrees );

		m_cursor.m_slopeParams.referencePos = GetSlopeReferencePoint();
	}
	else if ( RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_slopeParams.slopeDegrees ) ) / MAbs( m_slopeControl.GetMax() );
		m_cursor.m_slopeParams.slopeDegrees += data * ( m_slopeControl.GetMax() - m_slopeControl.GetMin() ) * mod;
		m_cursor.m_slopeParams.slopeDegrees  = Min( m_cursor.m_slopeParams.slopeDegrees, m_slopeControl.GetMax() );
		m_slopeControl.UpdateValue( m_cursor.m_slopeParams.slopeDegrees );

		m_cursor.m_slopeParams.referencePos = GetSlopeReferencePoint();
	}
	else if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_intensity ) ) / MAbs( m_intensityControl.GetMax() );
		m_cursor.m_intensity += data * ( m_intensityControl.GetMax() - m_intensityControl.GetMin() ) * mod;
		m_cursor.m_intensity  = Min( m_cursor.m_intensity, m_intensityControl.GetMax() );
		m_intensityControl.UpdateValue( m_cursor.m_intensity );
	}
	else
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_outsideRadiusControl.GetMax() );
		m_cursor.m_brushSize += data * ( m_outsideRadiusControl.GetMax() - m_outsideRadiusControl.GetMin() ) * mod;
		m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_outsideRadiusControl.GetMin(), m_outsideRadiusControl.GetMax() );
		m_outsideRadiusControl.UpdateValue( m_cursor.m_brushSize );
	}
}

void CEdTerrainEditTool::HandleMouseZBrushStamp( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;
	if ( RIM_IS_KEY_DOWN( IK_LShift ) && RIM_IS_KEY_DOWN( IK_LControl ) )
	{
		m_cursor.m_stampScaleOrigin += data * ( m_stampScaleOriginControl.GetMax() - m_stampScaleOriginControl.GetMin() ) * baseMod;
		m_cursor.m_stampScaleOrigin  = Min( m_cursor.m_stampScaleOrigin, m_stampScaleOriginControl.GetMax() );
		m_stampScaleOriginControl.UpdateValue( m_cursor.m_stampScaleOrigin );
	}
	else if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_intensity ) ) / MAbs( m_intensityControl.GetMax() );
		m_cursor.m_intensity += data * ( m_intensityControl.GetMax() - m_intensityControl.GetMin() ) * mod;
		m_cursor.m_intensity  = Min( m_cursor.m_intensity, m_intensityControl.GetMax() );
		m_intensityControl.UpdateValue( m_cursor.m_intensity );

		// Show the stamp, since Shift hides it to allow picking height values.
		SetStampVisible( true );
	}
	else if ( RIM_IS_KEY_DOWN( IK_LControl ) )
	{
		m_cursor.m_desiredElevation += data * ( m_heightControl.GetMax() - m_heightControl.GetMin() ) * baseMod;
		m_cursor.m_desiredElevation  = Min( m_cursor.m_desiredElevation, m_heightControl.GetMax() );
		m_heightControl.UpdateValue( m_cursor.m_desiredElevation );
	}
	else
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_outsideRadiusControl.GetMax() );
		m_cursor.m_brushSize += data * ( m_outsideRadiusControl.GetMax() - m_outsideRadiusControl.GetMin() ) * mod;
		m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_outsideRadiusControl.GetMin(), m_outsideRadiusControl.GetMax() );
		m_outsideRadiusControl.UpdateValue( m_cursor.m_brushSize );
	}
}

void CEdTerrainEditTool::HandleMouseZBrushPaint( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;
	if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_colorIntensityControl.GetValue() ) ) / MAbs( m_colorIntensityControl.GetMax() );
		Float intensity = m_colorIntensityControl.GetValue();
		intensity += data * ( m_colorIntensityControl.GetMax() - m_colorIntensityControl.GetMin() ) * mod;
		intensity = Min( intensity, m_colorIntensityControl.GetMax() );
		m_colorIntensityControl.UpdateValue( intensity );
	}
	else
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_colorRadiusControl.GetMax() );
		m_cursor.m_brushSize += data * ( m_colorRadiusControl.GetMax() - m_colorRadiusControl.GetMin() ) * mod;
		m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_colorRadiusControl.GetMin(), m_colorRadiusControl.GetMax() );
		m_colorRadiusControl.UpdateValue( m_cursor.m_brushSize );
	}
}

void CEdTerrainEditTool::HandleMouseZBrushPaintTex( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;

	Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_materialRadiusControl.GetMax() );
	m_cursor.m_brushSize += data * ( m_materialRadiusControl.GetMax() - m_materialRadiusControl.GetMin() ) * mod;
	m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_materialRadiusControl.GetMin(), m_materialRadiusControl.GetMax() );
	m_materialRadiusControl.UpdateValue( m_cursor.m_brushSize );
}

void CEdTerrainEditTool::HandleMouseZBrushSmoothMelt( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	Float baseMod = 0x0001 & GetKeyState( VK_CAPITAL ) ? 0.005f : 0.05f;
	if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_intensity ) ) / MAbs( m_intensityControl.GetMax() );
		m_cursor.m_intensity += data * ( m_intensityControl.GetMax() - m_intensityControl.GetMin() ) * mod;
		m_cursor.m_intensity  = Min( m_cursor.m_intensity, m_intensityControl.GetMax() );
		m_intensityControl.UpdateValue( m_cursor.m_intensity );
	}
	else if ( RIM_IS_KEY_DOWN( IK_LControl ) )
	{
		m_cursor.m_filterSize += MSign( data );
		m_cursor.m_filterSize  = ( Uint32 )Min( m_cursor.m_filterSize, m_filterSizeControl.GetMax() );
		m_filterSizeControl.UpdateValue( m_cursor.m_filterSize );
	}
	else
	{
		Float mod = baseMod * Max( 0.001f, MAbs( m_cursor.m_brushSize ) ) / MAbs( m_outsideRadiusControl.GetMax() );
		m_cursor.m_brushSize += data * ( m_outsideRadiusControl.GetMax() - m_outsideRadiusControl.GetMin() ) * mod;
		m_cursor.m_brushSize = Clamp( m_cursor.m_brushSize, m_outsideRadiusControl.GetMin(), m_outsideRadiusControl.GetMax() );
		m_outsideRadiusControl.UpdateValue( m_cursor.m_brushSize );
	}
}

void CEdTerrainEditTool::SetPreset( Int32 slot )
{
	m_presetIndex = slot;
	m_textureArrayGrid->SetHook( NULL );

	m_selectedHorizontalTexture = m_presetsData[m_presetIndex].selectedHorizontalTexture;
	m_selectedVerticalTexture = m_presetsData[m_presetIndex].selectedVerticalTexture;
	m_verticalUVMult = m_presetsData[m_presetIndex].verticalUVMult;
	m_slopeThresholdAction = m_presetsData[m_presetIndex].slopeThresholdAction;
	m_slopeThresholdIndex = m_presetsData[m_presetIndex].slopeThresholdIndex;
	m_textureArrayGrid->SetSelected( m_presetsData[m_presetIndex].selectedHorizontalTexture );
	m_textureArrayGrid->SetSecondary( m_presetsData[m_presetIndex].selectedVerticalTexture );
	m_slopeThresholdActionChoice->SetSelection( m_presetsData[m_presetIndex].slopeThresholdAction );
	m_paintSlopeThresholdSpin->SetValue( m_presetsData[m_presetIndex].slopeThresholdIndex + 1 );
	m_uvMultVerticalSpin->SetValue( m_presetsData[m_presetIndex].verticalUVMult + 1 );
	m_paintProbabilitySlider->SetValue( m_presetsData[m_presetIndex].probability );
	m_verticalUVScaleMask->SetValue( m_presetsData[m_presetIndex].verticalUVScaleMask );
	m_slopeThresholdMask->SetValue( m_presetsData[m_presetIndex].slopeThresholdMask );
	m_horizontalMask->SetValue( m_presetsData[m_presetIndex].horizontalMask );
	m_verticalMask->SetValue( m_presetsData[m_presetIndex].verticalMask );

	( (wxCheckBox*) XRCCTRL( *m_dialog, "m_lowLimitCheck", wxCheckBox ) )->SetValue( m_presetsData[ m_presetIndex ].lowLimitMask );
	( (wxCheckBox*) XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox ) )->SetValue( m_presetsData[ m_presetIndex ].highLimitMask );

	( (wxTextCtrl*) XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl ) )->ChangeValue( ToString( m_presetsData[ m_presetIndex ].lowLimit ).AsChar() );
	( (wxTextCtrl*) XRCCTRL( *m_dialog, "m_highLimitEdit",  wxTextCtrl ) )->ChangeValue( ToString( m_presetsData[ m_presetIndex ].highLimit ).AsChar() );
	m_cursor.m_lowLimit = m_presetsData[ m_presetIndex ].lowLimitMask ? HeightToTexels( m_presetsData[ m_presetIndex ].lowLimit ) : TOptional< Float >() ;
	m_cursor.m_highLimit = m_presetsData[ m_presetIndex ].highLimitMask ? HeightToTexels( m_presetsData[ m_presetIndex ].highLimit ) : TOptional< Float >();

	for ( Uint32 i = 0; i < m_presetsRadioBtns.Size(); ++i )
	{
		m_presetsRadioBtns[i]->SetValue( m_presetIndex == i );
	}
	m_textureArrayGrid->SetHook( this );
}

void CEdTerrainEditTool::UpdatePresetData()
{
	m_presetsData[m_presetIndex].selectedHorizontalTexture = m_selectedHorizontalTexture;
	m_presetsData[m_presetIndex].selectedVerticalTexture = m_selectedVerticalTexture;
	m_presetsData[m_presetIndex].verticalUVMult = m_verticalUVMult;
	m_presetsData[m_presetIndex].slopeThresholdAction = m_slopeThresholdAction;
	m_presetsData[m_presetIndex].slopeThresholdIndex = m_slopeThresholdIndex;
	m_presetsData[m_presetIndex].probability = m_paintProbabilitySlider->GetValue();
	m_presetsData[m_presetIndex].verticalUVScaleMask = m_verticalUVScaleMask->GetValue();
	m_presetsData[m_presetIndex].slopeThresholdMask = m_slopeThresholdMask->GetValue();
	m_presetsData[m_presetIndex].horizontalMask = m_horizontalMask->GetValue();
	m_presetsData[m_presetIndex].verticalMask = m_verticalMask->GetValue();
	
	Float limit;
	if ( FromString( String( XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_presetsData[ m_presetIndex ].lowLimit = limit;
	}
	if ( FromString( String( XRCCTRL( *m_dialog, "m_highLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_presetsData[ m_presetIndex ].highLimit = limit;
	}
	
	m_presetsData[ m_presetIndex ].lowLimitMask = XRCCTRL( *m_dialog, "m_lowLimitCheck" , wxCheckBox )->GetValue();
	m_presetsData[ m_presetIndex ].highLimitMask = XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->GetValue();
}

void CEdTerrainEditTool::UpdateMaterialTexturesGrid()
{
	CTextureArray* textureArray = NULL;
	CClipMap* terrain = m_world->GetTerrain();
	if ( terrain )
	{
		IMaterial* material = terrain->GetMaterial();
		if ( material )
		{
			THandle< CTextureArray > texture;
			if ( material->ReadParameter( CNAME(diffuse), texture ) )
			{
				textureArray = Cast< CTextureArray >( texture.Get() );
			}
		}
	}

	m_textureArrayGrid->SetTextureArray( textureArray );
}

void CEdTerrainEditTool::UpdateTextureParameters()
{
	CClipMap* terrain = m_world->GetTerrain();

	m_textureParametersPanel->DestroyChildren();
	m_textureParametersPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	CTextureArray* textureArray = m_textureArrayGrid->GetTextureArray();
	String textureName( TXT("None") );
	if ( textureArray && m_lastSelectedTexture < (Int32)textureArray->GetTextureCount() )
	{
		TDynArray< CBitmapTexture* > textures;
		textureArray->GetTextures( textures );
		ASSERT( textures.Size() == textureArray->GetTextureCount() );
		textureName = GetBareFilename( textures[ m_lastSelectedTexture ]->GetFile()->GetFileName() );
	}
	

	wxStaticText* label1 = new wxStaticText( m_textureParametersPanel, wxID_ANY, wxT("Parameters for ") );
	wxStaticText* label2 = new wxStaticText( m_textureParametersPanel, wxID_ANY, wxString( textureName.AsChar() ) );
	static_cast<wxBoxSizer*>( m_textureParametersPanel->GetSizer() )->Add( label1, 0, wxUP|wxLEFT, 5 );
	static_cast<wxBoxSizer*>( m_textureParametersPanel->GetSizer() )->Add( label2, 0, wxDOWN|wxLEFT, 5 );

	for ( Uint32 i=0; i<TBP_EnumSize; ++i )
	{
		if ( i > 0 )
		{
			static_cast<wxBoxSizer*>( m_textureParametersPanel->GetSizer() )->Add( 5, 5, 0 );
		}
		wxStaticText* label = new wxStaticText( m_textureParametersPanel, wxID_ANY, TextureBlendParamNames[ i ] );
		static_cast<wxBoxSizer*>( m_textureParametersPanel->GetSizer() )->Add( label, 0, wxALL, 5 );
		wxToolTip* tooltip = new wxToolTip( TextureBlendParamTooltips[ i ] );
		tooltip->SetDelay( 0 );
		tooltip->SetAutoPop( 999999 );
		label->SetToolTip( tooltip );

		Int32 value = 0;
		if ( terrain )
		{
			Float paramValueF = terrain->GetTextureParam( m_lastSelectedTexture, i );
			value = paramValueF * m_textureMaterialParamsSliderRange;
		}

		wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
		static_cast<wxBoxSizer*>( m_textureParametersPanel->GetSizer() )->Add( sizer, 0, wxEXPAND );

		wxSlider* slider = new wxSlider( m_textureParametersPanel, wxID_MATERIAL_PARAMETER_START + i * 2, value, 0, m_textureMaterialParamsSliderRange, wxDefaultPosition, wxDefaultSize, wxSL_BOTH );
		sizer->Add( slider, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5 );
		slider->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnTextureParameterSliderUpdate ), NULL, this );
		slider->SetClientData( reinterpret_cast<void*>( i ) );

		wxTextCtrl* textCtrl = new wxTextCtrl( m_textureParametersPanel, wxID_MATERIAL_PARAMETER_START + i * 2 + 1, wxString::Format( wxT( "%i" ), value / m_textureMaterialParamsSliderToTBoxRange ), 
			wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RIGHT, wxTextValidator( wxFILTER_DIGITS ) );
		sizer->Add( textCtrl, 0, wxRIGHT|wxBOTTOM, 5 );
		textCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdTerrainEditTool::OnTextureParameterTextBoxUpdate ), NULL, this );
		textCtrl->SetClientData( reinterpret_cast<void*>( i ) );
	}


	m_textureParametersPanel->Layout();

	if ( terrain )
	{
		// Update grass brush name
		CVegetationBrush* grassBrush = terrain->GetGrassBrush( m_lastSelectedTexture );
		if ( grassBrush )
		{
			m_grassBrushLink->SetLabel( grassBrush->GetFile()->GetFileName().AsChar() );
		}
		else
		{
			m_grassBrushLink->SetLabel( TXT("no grass") );
		}
	}
}


void CEdTerrainEditTool::OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary )
{
	// Set mask
	if ( wxGetKeyState( WXK_ALT ) )
	{
		TDynArray<CBitmapTexture*> textures;
		m_textureArrayGrid->GetTextureArray()->GetTextures( textures );

		grid->SetHook( NULL );
		// Horizontal mask
		if ( primary )
		{
			m_horizontalTextureMaskIndex = grid->GetSelected();
			m_useHorizontalTextureMask->SetValue( true );
			m_horizontalTextureMask->SetValue( textures[m_horizontalTextureMaskIndex]->GetFile()->GetFileName().AsChar() );
			grid->SetSelected( m_selectedHorizontalTexture );
		}
		else // Vertical mask
		{
			m_verticalTextureMaskIndex = grid->GetSecondary();
			m_useVerticalTextureMask->SetValue( true );
			m_verticalTextureMask->SetValue( textures[m_verticalTextureMaskIndex]->GetFile()->GetFileName().AsChar() );
			grid->SetSecondary( m_selectedVerticalTexture );
		}
		grid->SetHook( this );

		return;
	}

	m_selectedHorizontalTexture = grid->GetSelected();
	m_selectedVerticalTexture = grid->GetSecondary();
	
	m_lastSelectedTexture = grid->GetLastSelected();

	UpdatePresetData();
	UpdateTextureParameters();
}

void CEdTerrainEditTool::OnTextureParameterSliderUpdate( wxCommandEvent& event )
{
	intptr_t index = reinterpret_cast<intptr_t>( static_cast<wxSlider*>( event.GetEventObject() )->GetClientData() );
	Int32 sliderValue = static_cast<wxSlider*>( event.GetEventObject() )->GetValue();

	wxTextCtrl* tBox = static_cast<wxTextCtrl*>( wxWindow::FindWindowById( wxID_MATERIAL_PARAMETER_START + 1 + index * 2 ) );
	if ( tBox )
	{
		tBox->SetValue( wxString::Format( wxT( "%i" ), sliderValue / m_textureMaterialParamsSliderToTBoxRange ) );
	}

	UpdateMaterialParameter( index, (Float)sliderValue / m_textureMaterialParamsSliderRange );
}

void CEdTerrainEditTool::OnTextureParameterTextBoxUpdate( wxCommandEvent& event )
{
	wxTextCtrl* tbox = static_cast<wxTextCtrl*>( event.GetEventObject() );
	if ( tbox )
	{
		intptr_t index = reinterpret_cast<intptr_t>( tbox->GetClientData() );
		Int32 tBoxValue = wxAtoi( tbox->GetValue() );

		Int32 sliderValue = Min( m_textureMaterialParamsSliderRange, tBoxValue * m_textureMaterialParamsSliderToTBoxRange );
		sliderValue = Max( 0, sliderValue );
		wxSlider* slider = static_cast<wxSlider*>( wxWindow::FindWindowById( wxID_MATERIAL_PARAMETER_START + index * 2 ) );
		if ( slider )
		{
			slider->SetValue( sliderValue );
			tbox->SetValue( wxString::Format( wxT( "%i" ), sliderValue / m_textureMaterialParamsSliderToTBoxRange ) );
			UpdateMaterialParameter( index, (Float)sliderValue / m_textureMaterialParamsSliderRange );
		}
	}
}

void CEdTerrainEditTool::OnGrassBrushHyperlinkClicked( wxHyperlinkEvent& event )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}

	CVegetationBrush* brush = m_world->GetTerrain()->GetGrassBrush( m_lastSelectedTexture );
	if ( brush )
	{
		wxTheFrame->GetAssetBrowser()->SelectFile( brush->GetDepotPath() );
	}
}

void CEdTerrainEditTool::OnSetGrassBrushClicked( wxCommandEvent& event )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}
	String activeBrushName;
	if ( GetActiveResource( activeBrushName ) )
	{
		CVegetationBrush* brush = Cast< CVegetationBrush >( GDepot->LoadResource( activeBrushName ) );
		if ( brush )
		{
			if ( m_world->GetTerrain()->MarkModified() )
			{
				m_world->GetTerrain()->SetGrassBrush( m_lastSelectedTexture, brush );
				m_grassBrushLink->SetLabel( brush->GetFile()->GetFileName().AsChar() );
			}
		}
	}
}

void CEdTerrainEditTool::OnRemoveGrassBrushClicked( wxCommandEvent& event )
{
	if ( !m_world || !m_world->GetTerrain() )
	{
		return;
	}

	if ( m_world->GetTerrain()->MarkModified() )
	{
		m_world->GetTerrain()->SetGrassBrush( m_lastSelectedTexture, NULL );
		m_grassBrushLink->SetLabel( TXT("no grass") );
	}
}

Uint32 CEdTerrainEditTool::GetColorPresetIndex( wxWindowBase* colorButton ) const
{
	for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
	{
		if ( m_colorPresetButtons[ i ] == colorButton )
		{
			return i;
		}
	}
	return NUM_COLOR_PRESETS;
}

void CEdTerrainEditTool::OnPaintColorPanel( wxPaintEvent& event )
{
	wxWindow* button = wxDynamicCast( event.GetEventObject(), wxWindow );
	wxBufferedPaintDC dc( button );

	dc.SetPen( *wxTRANSPARENT_PEN );

	Bool isEditing = ( m_colorPickerWindow && m_colorPickerWindow->IsShown() && m_colorPickerWindow->GetClientData() == button );
	Int32 w = isEditing ? 2 : 1;

	dc.SetBrush( wxBrush( wxColour( 64, 64, 64 ) ) );
	dc.DrawRectangle( 0, 0, button->GetSize().GetWidth(), button->GetSize().GetHeight() );
	dc.SetBrush( wxBrush( wxColour( 192, 192, 192 ) ) );
	dc.DrawRectangle( w, w, button->GetSize().GetWidth()-w*2, button->GetSize().GetHeight()-w*2 );

	Int32 fillWidth = button->GetSize().GetWidth()-w*4;
	Int32 fillHeight = button->GetSize().GetHeight()-w*4;

	Uint32 whichPreset = GetColorPresetIndex( button );

	wxColour fillColor;

	if ( whichPreset < NUM_COLOR_PRESETS )
	{
		fillColor = m_colorPresets[ whichPreset ];
	}
	else
	{
		fillColor = m_currentColor;
	}

	dc.SetBrush( wxBrush( fillColor ) );
	dc.DrawRectangle( w*2, w*2, fillWidth, fillHeight );
}

void CEdTerrainEditTool::SetColorPreset( Uint32 whichPreset, const wxColour& color )
{
	ASSERT( whichPreset < NUM_COLOR_PRESETS );
	if ( whichPreset >= NUM_COLOR_PRESETS ) return;

	m_colorPresets[ whichPreset ] = color;

	wxWindowBase* presetButton = m_colorPresetButtons[ whichPreset ];
	if ( presetButton )
	{
		presetButton->Refresh();

		if ( m_colorPickerWindow && m_colorPickerWindow->GetClientData() == presetButton )
		{
			m_colorPickerWindow->SetColor( Color( color.GetPixel() ) );
		}
	}
}

void CEdTerrainEditTool::SetCurrentColor( const wxColour& color )
{
	m_currentColor = color;
	if ( m_currentColorButton )
	{
		m_currentColorButton->Refresh();

		if ( m_colorPickerWindow && m_colorPickerWindow->GetClientData() == m_currentColorButton )
		{
			m_colorPickerWindow->SetColor( Color( color.GetPixel() ) );
		}
	}
}


void CEdTerrainEditTool::OnColorPresetClicked( wxMouseEvent& event )
{
	wxWindowBase* presetButton = wxDynamicCast( event.GetEventObject(), wxWindowBase );

	Uint32 whichPreset = GetColorPresetIndex( presetButton );
	ASSERT( whichPreset < NUM_COLOR_PRESETS );
	if ( whichPreset < NUM_COLOR_PRESETS )
	{
		SetCurrentColor( m_colorPresets[ whichPreset ] );
	}
}

void CEdTerrainEditTool::OnCurrentColorClicked( wxMouseEvent& event )
{
	OpenColorPicker( m_currentColorButton );
}


void CEdTerrainEditTool::OnColorPresetRightClicked( wxMouseEvent& event )
{
	wxWindowBase* button = wxDynamicCast( event.GetEventObject(), wxWindowBase );

	// context menu
	wxMenu* menu = new wxMenu();

	menu->Append( wxID_EDIT_PRESET,				wxT("Edit Color") );
	menu->Append( wxID_SET_PRESET_FROM_CURRENT,	wxT("From Current") );
	menu->Append( wxID_RESET_PRESET,			wxT("Reset") );
	menu->Append( wxID_RESET_ALL_PRESETS,		wxT("Reset All") );

	menu->Connect( wxID_EDIT_PRESET,				wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnEditColorPreset ),			event.GetEventObject(), this );
	menu->Connect( wxID_SET_PRESET_FROM_CURRENT,	wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnSetColorPresetFromCurrent ),	event.GetEventObject(), this );
	menu->Connect( wxID_RESET_PRESET,				wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnClearColorPreset ),			event.GetEventObject(), this );
	menu->Connect( wxID_RESET_ALL_PRESETS,			wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdTerrainEditTool::OnClearAllColorPresets ),		event.GetEventObject(), this );

	button->PopupMenu( menu );
}

void CEdTerrainEditTool::OnEditColorPreset( wxCommandEvent& event )
{
	// There's supposed to be wxEvent::GetEventUserData(), but it looks like our version doesn't have it.
	wxWindowBase* presetButton = wxDynamicCast( event.m_callbackUserData, wxWindowBase );
	OpenColorPicker( presetButton );
}

void CEdTerrainEditTool::OnSetColorPresetFromCurrent( wxCommandEvent& event )
{
	// There's supposed to be wxEvent::GetEventUserData(), but it looks like our version doesn't have it.
	wxWindowBase* presetButton = wxDynamicCast( event.m_callbackUserData, wxWindowBase );

	Uint32 whichPreset = GetColorPresetIndex( presetButton );
	ASSERT( whichPreset < NUM_COLOR_PRESETS );
	if ( whichPreset < NUM_COLOR_PRESETS )
	{
		SetColorPreset( whichPreset, m_currentColor );
	}
}

void CEdTerrainEditTool::OnClearColorPreset( wxCommandEvent& event )
{
	// There's supposed to be wxEvent::GetEventUserData(), but it looks like our version doesn't have it.
	wxWindowBase* presetButton = wxDynamicCast( event.m_callbackUserData, wxWindowBase );

	Uint32 whichPreset = GetColorPresetIndex( presetButton );
	ASSERT( whichPreset < NUM_COLOR_PRESETS );
	if ( whichPreset < NUM_COLOR_PRESETS )
	{
		SetColorPreset( whichPreset, DefaultColorPresets[ whichPreset ] );
	}
}

void CEdTerrainEditTool::OnClearAllColorPresets( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
	{
		SetColorPreset( i, DefaultColorPresets[ i ] );
	}
}


void CEdTerrainEditTool::OpenColorPicker( wxWindowBase* colorButton )
{
	Uint32 whichPreset = GetColorPresetIndex( colorButton );

	wxColour currentColorWX = ( whichPreset < NUM_COLOR_PRESETS ) ? m_colorPresets[ whichPreset ] : m_currentColor;
	Color currentColor( currentColorWX.GetPixel() );

	if ( !m_colorPickerWindow )
	{
		m_colorPickerWindow = new CEdAdvancedColorPicker( m_dialog, currentColor, true );
		m_colorPickerWindow->Connect( wxEVT_COMMAND_SCROLLBAR_UPDATED, wxCommandEventHandler( CEdTerrainEditTool::OnColorPickerChanged ), NULL, this );
		m_colorPickerWindow->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdTerrainEditTool::OnColorPickerClosed ), 0, this );
	}
	else
	{
		// If it's already opened, just bring it to the front and update the reference color.
		m_colorPickerWindow->SetReferenceColor( currentColor );
		m_colorPickerWindow->Show( true );
		m_colorPickerWindow->Raise();
		m_colorPickerWindow->Maximize( false );
	}

	StringAnsi activePreset;
	if ( colorButton == m_currentColorButton )
	{
		activePreset = "Active Color";
	}
	else
	{
		activePreset = StringAnsi::Printf( "Preset %d", GetColorPresetIndex( colorButton ) );
	}


	// Redraw previously-edited panel, so it won't have a thick border anymore.
	wxWindowBase* oldButton = wxDynamicCast( m_colorPickerWindow->GetClientData(), wxWindowBase );
	if ( oldButton )
	{
		oldButton->Refresh();
	}

	m_colorPickerWindow->SetTitle( activePreset.AsChar() );
	m_colorPickerWindow->SetClientData( colorButton );
	m_colorPickerWindow->SetColor( currentColor );

	// Redraw newly-editing panel, so it'll have a thick border!
	colorButton->Refresh();

	m_dialog->Refresh();
}


void CEdTerrainEditTool::OnColorPickerChanged( wxCommandEvent& event )
{
	// Get the panel for the color we're modifying.
	wxWindowBase* colorPanel = wxDynamicCast( m_colorPickerWindow->GetClientData(), wxWindowBase );

	// If it's the "current color" panel...
	if ( colorPanel == m_currentColorButton )
	{
		SetCurrentColor( wxColour( m_colorPickerWindow->GetColor().ToUint32() ) );
	}
	else
	{
		// Otherwise, it's one of the presets.
		Uint32 whichPreset = GetColorPresetIndex( colorPanel );
		ASSERT( whichPreset < NUM_COLOR_PRESETS, TXT("Invalid color preset panel") );
		if ( whichPreset < NUM_COLOR_PRESETS )
		{
			SetColorPreset( whichPreset, wxColour( m_colorPickerWindow->GetColor().ToUint32() ) );
		}
	}
}

void CEdTerrainEditTool::OnColorPickerClosed( wxCloseEvent& event )
{
	wxWindowBase* oldButton = wxDynamicCast( m_colorPickerWindow->GetClientData(), wxWindowBase );
	m_colorPickerWindow->SetClientData( NULL );

	// Redraw edited panel, so it won't have a thick border anymore.
	if ( oldButton )
	{
		oldButton->Refresh();
	}

	m_colorPickerWindow = NULL;

	event.Skip();
}

void CEdTerrainEditTool::OnSaveColorPresets( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString;
	String wildCard = TXT("Terrain Color Presets (*.xml)|*.xml");

	wxFileDialog saveFileDialog( m_dialog, TXT("Save color presets"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		String savePath = saveFileDialog.GetPath().wc_str();

		IFile* file = GFileManager->CreateFileWriter( savePath, FOF_Buffered | FOF_AbsolutePath );
		if ( !file )
		{
			GFeedback->ShowError( TXT("Unable to open Color Preset file for writing. %s"), savePath.AsChar() );
			return;
		}

		CXMLFileWriter xmlWriter( *file );

		xmlWriter.BeginNode( TXT("ColorPresets") );

		for ( Uint32 i = 0; i < NUM_COLOR_PRESETS; ++i )
		{
			xmlWriter.BeginNode( TXT("preset") );
			xmlWriter.AttributeT( TXT("r"), m_colorPresets[ i ].Red() );
			xmlWriter.AttributeT( TXT("g"), m_colorPresets[ i ].Green() );
			xmlWriter.AttributeT( TXT("b"), m_colorPresets[ i ].Blue() );
			xmlWriter.EndNode();
		}

		xmlWriter.EndNode();
		xmlWriter.Flush();

		delete file;
	}
}

void CEdTerrainEditTool::OnLoadColorPresets( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString;
	String wildCard = TXT("Terrain Color Presets (*.xml)|*.xml|All files (*.*)|*.*");
	wxFileDialog loadFileDialog( m_dialog, wxT("Load palette"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = loadFileDialog.GetPath().wc_str();

		ASSERT( !loadPath.Empty() );
		if ( loadPath.Empty() ) return;

		IFile* file = GFileManager->CreateFileReader( loadPath, FOF_Buffered | FOF_AbsolutePath );
		if ( !file )
		{
			GFeedback->ShowError( TXT("Unable to open Color Preset file. %s"), loadPath.AsChar() );
			return;
		}

		// When xmlReader goes out of scope, it'll delete file for us.
		CXMLFileReader xmlReader( file );

		if ( !xmlReader.BeginNode( TXT("ColorPresets") ) )
		{
			GFeedback->ShowError( TXT("Invalid Color Preset file. Check log for errors. Aborting. %s"), loadPath.AsChar() );
			return;
		}

		Uint32 currPreset = 0;
		while ( currPreset < NUM_COLOR_PRESETS && xmlReader.BeginNode( TXT("preset") ) )
		{
			Uint8 r, g, b;
			if ( xmlReader.AttributeT( TXT("r"), r ) &&
				 xmlReader.AttributeT( TXT("g"), g ) &&
				 xmlReader.AttributeT( TXT("b"), b ) )
			{
				SetColorPreset( currPreset, wxColour( r, g, b ) );
				++currPreset;
			}
			else
			{
				GFeedback->ShowWarn( TXT("Invalid color preset entry. Check log for errors. Skipping.") );
				WARN_EDITOR( TXT("Invalid terrain color preset entry. Skipping.") );
			}
			xmlReader.EndNode();
		}
		if ( currPreset < NUM_COLOR_PRESETS )
		{
			GFeedback->ShowMsg( TXT(""), TXT("Color Preset file had fewer entries than available slots. Filling the rest with defaults.") );

			// Fill any remaining with the defaults.
			for ( ; currPreset < NUM_COLOR_PRESETS; ++currPreset )
			{
				SetColorPreset( currPreset, DefaultColorPresets[ currPreset ] );
			}
		}

		xmlReader.EndNode();
	}
}

void CEdTerrainEditTool::OnLowLimitChoice( wxCommandEvent& event )
{
	if ( event.GetSelection() )
	{
		OnLowLimitText( event );
	}
	else
	{
		m_cursor.m_lowLimit = TOptional< Float >();
		UpdatePresetData();
	}

	event.Skip();
}

void CEdTerrainEditTool::OnHighLimitChoice( wxCommandEvent& event )
{
	if ( event.GetSelection() )
	{
		OnHighLimitText( event );
	}
	else
	{
		m_cursor.m_highLimit = TOptional< Float >();
		UpdatePresetData();
	}

	event.Skip();
}

void CEdTerrainEditTool::OnLowLimitText( wxCommandEvent& event )
{
	Float limit;
	if ( FromString( String( XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_cursor.m_lowLimit = HeightToTexels( limit );
		UpdatePresetData();
	}
}

void CEdTerrainEditTool::OnHighLimitText( wxCommandEvent& event )
{
	Float limit;
	if ( FromString( String( XRCCTRL( *m_dialog, "m_highLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_cursor.m_highLimit = HeightToTexels( limit );
		UpdatePresetData();
	}

	event.Skip();
}

void CEdTerrainEditTool::OnLimitPresetSelected( wxCommandEvent& event )
{
	Uint32 preset = 0;
	for ( Uint32 i = 0; i < m_presetsRadioBtns.Size(); ++i )
	{
		if ( event.GetEventObject() == m_presetsRadioBtns[i] )
		{
			preset = i;
		}
	}

	UpdatePresetData();
	SetPreset( preset );
}

void CEdTerrainEditTool::OnConfigurePresetsBtnClicked( wxCommandEvent& event )
{
	if ( m_presetsConfigurationDlg->ShowModal() == wxID_OK )
	{
		UpdatePresetsActivity();
	}
}

void CEdTerrainEditTool::UpdatePresetsActivity()
{
	Uint32 stop = Min( m_presetsRadioBtns.Size(), m_presetCheckBoxes.Size() );
	for ( Uint32 i = 0; i < stop; ++i )
	{
		m_presetsRadioBtns[i]->Enable( m_presetCheckBoxes[i]->GetValue() );
	}
}

void CEdTerrainEditTool::OnTextureUsageTool( wxCommandEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		CEdTerrainTextureUsageTool dialog( m_dialog, m_world->GetTerrain() );
		if ( dialog.Execute() )
		{
			m_textureArrayGrid->UpdateEntriesFromTextureArray(); // in case unused were removed
		}
	}
}

void CEdTerrainEditTool::OnUpdateUI( wxUpdateUIEvent& event )
{
	if ( !m_isStarted )
	{
		return;
	}

	if ( m_cursor.m_limitsAvailable )
	{
		XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->Enable();
		XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->Enable();
		Bool lowLimit  = XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->GetValue();
		Bool highLimit = XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->GetValue();
		XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->Enable( lowLimit );
		XRCCTRL( *m_dialog, "m_highLimitEdit", wxTextCtrl )->Enable( highLimit );
	}
	else
	{
		XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->Disable();
		XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->Disable();
		XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->Disable();
		XRCCTRL( *m_dialog, "m_highLimitEdit", wxTextCtrl )->Disable();
	}
}

void CEdTerrainEditTool::GetImportConfiguration( Bool& importHeightmap, Bool& importColormap )
{
	Bool both = XRCCTRL(*m_dialog, "m_radioImportHMandCM", wxRadioButton)->GetValue();
	Bool hm = XRCCTRL(*m_dialog, "m_radioImportHM", wxRadioButton)->GetValue();
	Bool cm = XRCCTRL(*m_dialog, "m_radioImportCM", wxRadioButton)->GetValue();

	importHeightmap = both || hm;
	importColormap = both || cm;
}

void CEdTerrainEditTool::GetExportConfiguration( Bool& exportHeightmap, Bool& exportColormap )
{
	Bool both = XRCCTRL(*m_dialog, "m_radioExportHMandCM", wxRadioButton)->GetValue();
	Bool hm = XRCCTRL(*m_dialog, "m_radioExportHM", wxRadioButton)->GetValue();
	Bool cm = XRCCTRL(*m_dialog, "m_radioExportCM", wxRadioButton)->GetValue();

	exportHeightmap = both || hm;
	exportColormap = both || cm;
}

void CEdTerrainEditTool::OnSlopeThresholdSpin( wxCommandEvent& event )
{
	m_slopeThresholdIndex = m_paintSlopeThresholdSpin->GetValue() - 1;
	UpdatePresetData();
}

void CEdTerrainEditTool::OnSlopeThresholdAction( wxCommandEvent& event )
{
	m_slopeThresholdAction = ( ETerrainPaintSlopeThresholdAction )m_slopeThresholdActionChoice->GetSelection();
	UpdatePresetData();
}

void CEdTerrainEditTool::OnUVMultVerticalSpin( wxCommandEvent& event )
{
	m_verticalUVMult = m_uvMultVerticalSpin->GetValue() - 1;
	UpdatePresetData();
}

void CEdTerrainEditTool::OnRowSpinUp( wxSpinEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		m_world->GetTerrain()->OffsetTiles( 1, 0 );
	}
}

void CEdTerrainEditTool::OnRowSpinDown( wxSpinEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		m_world->GetTerrain()->OffsetTiles( -1, 0 );
	}
}

void CEdTerrainEditTool::OnColSpinLeft( wxSpinEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		m_world->GetTerrain()->OffsetTiles( 0, -1 );
	}
}

void CEdTerrainEditTool::OnColSpinRight( wxSpinEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		m_world->GetTerrain()->OffsetTiles( 0, 1 );
	}
}


void CEdTerrainEditTool::OnPageChanging( wxCommandEvent& event )
{
	if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "shape", wxPanel ) )
	{
		m_activeToolShapePage = m_cursor.m_toolType;
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_materialPanel", wxPanel ) )
	{
		m_paintTextureActive = m_toolButtons[ TT_PaintTexture ] && m_toolButtons[ TT_PaintTexture ]->GetValue();
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_colorsPanel", wxPanel ) )
	{
		m_paintColorActive = m_toolButtons[ TT_PaintColor ] && m_toolButtons[ TT_PaintColor ]->GetValue();
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_collisionPanel", wxPanel ) )
	{
		m_activeToolCollisionPage = m_cursor.m_toolType;

		// Clear collision state overlay
		if ( m_world && m_world->GetTerrain() && m_world->GetTerrain()->GetTerrainProxy() )
		{
			( new CRenderCommand_ClearTerrainCustomOverlay( m_world->GetTerrain()->GetTerrainProxy() ) )->Commit();
		}
	}
}

void CEdTerrainEditTool::OnPageChanged( wxCommandEvent& event )
{
	if ( !m_isStarted ) return;

	EToolType activeTool = TT_None;

	if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "shape", wxPanel ) )
	{
		activeTool = m_activeToolShapePage;
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_materialPanel", wxPanel ) )
	{
		if ( m_paintTextureActive )
		{
			activeTool = TT_PaintTexture;
		}
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_colorsPanel", wxPanel ) )
	{
		if ( m_paintColorActive )
		{
			activeTool = TT_PaintColor;
		}
	}
	else if ( m_editorTabs->GetCurrentPage() == XRCCTRL( *m_dialog, "m_collisionPanel", wxPanel ) )
	{
		activeTool = m_activeToolCollisionPage;
		UpdateTerrainCollisionMask();
	}

	SelectTool( activeTool );
}


void CEdTerrainEditTool::OnFalloffCurveChanged( wxCommandEvent& event )
{
	if ( m_cursor.m_toolType != TT_None && m_cursor.GetUseFalloffCurve() )
	{
		m_cursor.m_falloffDataNeedsRebuild = true;
	}
}
