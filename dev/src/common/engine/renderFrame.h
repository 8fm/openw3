/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderMemoryPool.h"
#include "renderObject.h"
#include "renderFrameInfo.h"
#include "renderGameplayRenderTargetInterface.h"

#include "../gpuApiUtils/gpuApiInterface.h"

/// Rendering sort groups
enum ERenderingSortGroup : CEnum::TValueType
{
	RSG_DebugUnlit,
	RSG_Unlit,
	RSG_LitOpaque,
	RSG_LitOpaqueWithEmissive,
	RSG_DecalModulativeColor,
	RSG_DecalBlendedColor,
	RSG_DecalBlendedNormalsColor,
	RSG_DecalBlendedNormals,
	RSG_Sprites,
	RSG_TransparentBackground,
	RSG_RefractiveBackground,
	RSG_RefractiveBackgroundDepthWrite,
	RSG_Transparent,
	RSG_TransparentFullres,			//< todo: remove this
	RSG_DebugTransparent,
	RSG_DebugOverlay,
	RSG_2D,
	RSG_Prepare,
	RSG_Skin,
	RSG_Hair,
	RSG_Forward,
	RSG_EyeOverlay,
	RSG_Volumes,
	RSG_WaterBlend,
	RSG_Max,
};

BEGIN_ENUM_RTTI( ERenderingSortGroup );
	ENUM_OPTION( RSG_DebugUnlit );
	ENUM_OPTION( RSG_Unlit );
	ENUM_OPTION( RSG_LitOpaque );
	ENUM_OPTION( RSG_LitOpaqueWithEmissive );
	ENUM_OPTION( RSG_DecalModulativeColor );
	ENUM_OPTION( RSG_DecalBlendedColor );
	ENUM_OPTION( RSG_DecalBlendedNormalsColor );
	ENUM_OPTION( RSG_DecalBlendedNormals );
	ENUM_OPTION( RSG_Sprites );
	ENUM_OPTION( RSG_TransparentBackground );
	ENUM_OPTION( RSG_RefractiveBackground );
	ENUM_OPTION( RSG_RefractiveBackgroundDepthWrite );
	ENUM_OPTION( RSG_Transparent );
	ENUM_OPTION( RSG_TransparentFullres );
	ENUM_OPTION( RSG_DebugTransparent );
	ENUM_OPTION( RSG_DebugOverlay );
	ENUM_OPTION( RSG_2D );
	ENUM_OPTION( RSG_Prepare );
	ENUM_OPTION( RSG_Skin );
	ENUM_OPTION( RSG_Volumes );	
	ENUM_OPTION( RSG_WaterBlend );	
	ENUM_OPTION( RSG_Hair );
	ENUM_OPTION( RSG_Forward );
	ENUM_OPTION( RSG_EyeOverlay );
	ENUM_OPTION( RSG_Max );
END_ENUM_RTTI();

/// Transparency sort groups
enum ETransparencySortGroup : CEnum::TValueType
{
	TSG_Sky,
	TSG_Clouds,
	TSG_Scene,
	TSG_Max
};

BEGIN_ENUM_RTTI( ETransparencySortGroup );
	ENUM_OPTION( TSG_Sky );
	ENUM_OPTION( TSG_Clouds );
	ENUM_OPTION( TSG_Scene );
	ENUM_OPTION( TSG_Max );
END_ENUM_RTTI();

/// Rendering blend modes
enum ERenderingBlendMode : CEnum::TValueType
{
	RBM_None,
	RBM_Addtive,
	RBM_AlphaBlend,
	RBM_Refractive,
};

BEGIN_ENUM_RTTI( ERenderingBlendMode );
	ENUM_OPTION( RBM_None );
	ENUM_OPTION( RBM_Addtive );
	ENUM_OPTION( RBM_AlphaBlend );
	ENUM_OPTION( RBM_Refractive );
END_ENUM_RTTI();

class IRenderFragment;
class IRenderLight;
class CFont;
class CRenderTexture;
class CRenderCubeTexture;
struct SCurve3DData;
class CBitmapTexture;
class CHitProxyID;
struct DebugVertex;
class IRenderProxy;
class IRenderGameplayRenderTarget;

/// Rendering frame
class CRenderFrame : public IRenderObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
	// TODO: rethink this friend list
	friend class CDX9Render;
	friend class CXenonRender;
	friend class CRenderInterface;
	friend class IRenderFragment;

protected:
	CRenderFrameInfo				m_info;						//!< Common settings this frame was created for
	CRenderFrameOverlayInfo			m_overlayInfo;				//!< 2D Settings this frame was created for
	CRenderMemoryPool				m_memoryPool;				//!< Rendering memory pool
	IRenderFragment* 				m_fragments[ RSG_Max ];		//!< List of fragments for each sort group
	IRenderFragment* 				m_fragmentsLast[ RSG_Max ];	//!< Tail of list of fragments for each sort group
	IRenderGameplayRenderTarget*	m_renderTarget;

public:
	//! Get common rendering frame info used to create this frame
	RED_INLINE CRenderFrameInfo& GetFrameInfo() { return m_info; }
	RED_INLINE const CRenderFrameInfo& GetFrameInfo() const { return m_info; }

	//! Get 2D rendering frame info used to create this frame
	RED_INLINE CRenderFrameOverlayInfo& GetFrameOverlayInfo() { return m_overlayInfo; }

	//! Get the internal memory pool
	RED_INLINE CRenderMemoryPool& GetMemoryPool() { return m_memoryPool; }

	//! Get all fragments of given sort group in this frame
	RED_INLINE IRenderFragment* GetFragments( ERenderingSortGroup sortGroup ) { return m_fragments[ sortGroup ]; }

	//! Does this frame have any fragments for given sort group
	RED_INLINE Bool HasFragments( ERenderingSortGroup sortGroup ) const { return m_fragments[ sortGroup ] != NULL; }

	//! Get render target if registered
	RED_INLINE IRenderGameplayRenderTarget* GetRenderTarget() const { return m_renderTarget; }

public:
	CRenderFrame( const CRenderFrameInfo& info, const CRenderFrameOverlayInfo& overlayInfo );

protected:
	virtual ~CRenderFrame();

public:

	// Draw debug plane
	void AddDebugPlane( const Plane& plane, const Matrix& matrix, const Color& color, Int32 gridSize = 10, Bool overlay=false );
	
	// Draw debug line in the screen coordinates
	void AddDebugLineOnScreen( const Vector2& start, const Vector2& end, const Color& color );

	// Draw debug line in the screen coordinates with linear interpolate between two colors
	void AddDebugGradientLineOnScreen( const Vector2& start, const Vector2& end, const Color& color1, const Color& color2 );

	// Draw debug lines in the screen coordinates with linear interpolate between two colors
	// If normalized, positions are in [-1,1] range (-1,-1 is bottom left). Otherwise pixel coordinates (0,0 top left).
	void AddDebugLinesOnScreen( const DebugVertex* points, const Uint32 numPoints, Bool normalized = false );

	// Draw debug line
	void AddDebugLine( const Vector& start, const Vector& end, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug lines
	void AddDebugLines( const DebugVertex* points, const Uint32 numPoints, Bool overlay=false, Bool alwaysVisible=false );
	void AddDebugLines( const Matrix& localToWorld, const DebugVertex* points, const Uint32 numPoints, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug lines
	void AddDebugIndexedLines( const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, Bool overlay=false, Bool alwaysVisible=false );

	// Draw fat line
	void AddDebugFatLine( const Vector& start, const Vector& end, const Color& color, Float width = 4.0f, Bool overlay=false, Bool alwaysVisible=false );

	// Draw fat lines
	void AddDebugFatLines( const DebugVertex* points, const Uint32 numPoints, Float width = 4.0f, Bool overlay=false, Bool isRibbon=true, Bool alwaysVisible=false );

	// Draw debug line with arrow (arrowPostionOnLine [0.0, 1.0] == [begin, end])
	void AddDebugLineWithArrow( const Vector& start, const Vector& end, Float arrowPostionOnLine, Float arrowSizeX, Float arrowSizeY, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug tetrahedron
	void AddDebugTetrahedron( const Tetrahedron& box, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug brackets
	void AddDebugBrackets( const Box& box, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug brackets
	void AddDebugBrackets( const Vector* corners, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// DEPRECATED! backward compatibility for overlay flag
	void AddDebugBox( const Box& box, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug box wireframe
	void AddDebugBox( const Box& box, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible=false );

	// Draw debug box wireframe
	void AddDebugBox( const Vector* corners, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible=false );

	// Draw debug fat box wireframe
	void AddDebugFatBox( const Box& box, const Matrix& matrix, const Color& color, Float width = 4.0f, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug fat box wireframe
	void AddDebugFatBox( const Vector* corners, const Matrix& matrix, const Color& color, Float width = 4.0f, Bool overlay=false, Bool alwaysVisible=false );

	// DEPRECATED! backward compatibility for overlay flag
	void AddDebugSolidBox( const Box& box, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug solid box
	void AddDebugSolidBox( const Box& box, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible=false );

	// Draw debug solid box
	void AddDebugSolidBox( const Vector* corners, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible=false );

	// Draw debug sphere
	void AddDebugSphere( const Vector& center, Float radius, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug circle
	void AddDebugCircle( const Vector& center, Float radius, const Matrix& matrix, const Color& color, Uint32 numPoints = 12, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug curve
	void AddDebug3DCurve( const SCurve3DData& curve, const Color& color, const Matrix& matrix, Uint32 numPointsPerKeyFrame = 12, Bool overlay=false );

	// Draw debug solid tube
	void AddDebugTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, ERenderingSortGroup sortGroup, Uint16 numPoints = 12, Bool alwaysVisible=false );
	void AddDebugTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Uint16 numPoints = 12, Bool alwaysVisible=false );

	// Solid tube, can be either unlit or overlay.
	void AddDebugSolidTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Bool overlay = false, Uint16 numPoints = 12 );

	// Draw debug wireframe tube
	void AddDebugWireframeTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Bool overlay = false, Uint16 numPoints = 12, Bool alwaysVisible=false );

	// Draw debug capsule
	void AddDebugCapsule( const FixedCapsule& capsule, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug box
	void AddDebugOrientedBox( const OrientedBox& box, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug ellipsoid
	void AddDebugEllipsoid( const Vector& center, Float a, Float b, Float c, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug ellipsoid
	void AddDebugEllipsoid( const Vector& center, const Vector& ellipsoid, const Matrix& matrix, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug angled range
	void AddDebugAngledRange( const Vector& position, Float yaw, Float range, Float rangeAngle, Float heading, Color col, Bool drawSides, Bool alwaysVisible=false );

	// Draw debug angled range
	void AddDebugAngledRange( const Matrix& localToWorld, Float height, Float range, Float rangeAngle, Color col, Bool drawSides, Bool alwaysVisible=false );

	// Draw debug frustum
	void AddDebugFrustum( const Matrix& frustumMatrix, const Color& color, Bool backFaces=false, Bool overlay=false, Float farPlaneScale=1.0f, Bool alwaysVisible=false );

	// Draw debug cone
	void AddDebugCone( const Matrix& matrix, Float radius, Float innerAngle, Float outerAngle, Bool alwaysVisible=false );

	// Draw debug triangles list. If indices is null, treats points as a non-indexed triangle list.
	void AddDebugTriangles( const Matrix &matrix, const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, const Color& color, Bool overlay = true, Bool alwaysVisible=false );

	// Draw debug triangles list in screen space. If indices is null, treats points as a non-indexed triangle list.
	// If normalized, positions are in [-1,1] range (-1,-1 is bottom left). Otherwise pixel coordinates (0,0 is top left).
	void AddDebugTrianglesOnScreen( const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, const Color& color, Bool normalized = false );

	// Draw sprite
	void AddSprite( const Vector& center, Float size, const Color& color, const CHitProxyID& hitId, CBitmapTexture* texture, Bool overlay=false, Bool alwaysVisible=false );

	// Draw env probe
	void AddEnvProbe( Float gameTime, IRenderResource *renderResource, const Vector& center, Float radius, const CHitProxyID& hitId, Bool alwaysVisible=false );

	// Draw text
	void AddDebugText( const Vector& position, const String& text, Bool withBackground = false, const Color &colorText = Color( 255, 255, 255 ), const Color &colorBackground = Color( 0, 0, 0 ), CFont *font = nullptr, Bool alwaysVisible=false );

	// Draw text with offsets
	void AddDebugText( const Vector& position, const String& text, Int32 offsetXinPx, Int32 offsetYinLines, Bool withBackground = false, const Color &colorText = Color( 255, 255, 255 ), const Color &colorBackground = Color( 0, 0, 0 ), CFont *font = nullptr, Bool alwaysVisible=false );

	// Draw text
	void AddDebugScreenText( Int32 X, Int32 Y, const Char* text, const Color& colorText = Color( 255, 255, 255 ), CFont *font = NULL , Bool withBackground  = false ,const Color &colorBackground  = Color( 0, 0, 0 ), Bool alwaysVisible=false  );

	// Draw text
	RED_INLINE void AddDebugScreenText( Int32 X, Int32 Y, const String& text, const Color& colorText = Color( 255, 255, 255 ), CFont *font = NULL , Bool withBackground  = false ,const Color &colorBackground  = Color( 0, 0, 0 ), Bool alwaysVisible=false )
	{
		AddDebugScreenText(X,Y,text.AsChar(),colorText,font,withBackground,colorBackground,alwaysVisible);
	}
	
	void AddDebugScreenText( Int32 X, Int32 Y, const String& text, Uint32 offsetYinLines, Bool withBackground = false, const Color &colorText = Color( 255, 255, 255 ), const Color &colorBackground = Color( 0, 0, 0 ), CFont *font = NULL, Bool alwaysVisible=false );

	// Draw formated text, text length limit is 512
	void AddDebugScreenFormatedText( Int32 X, Int32 Y, const Color& colorText, const Char* format, ... );

	// Draw formated text, color is white, text length limit is 512
	void AddDebugScreenFormatedText( Int32 X, Int32 Y, const Char* format, ... );
	
	// Draw debug axis
	void AddDebugAxis( const Vector& pos, const Matrix& rot, const Float scale, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug axis with color
	void AddDebugAxis( const Vector& pos, const Matrix& rot, const Float scale, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug axis, x,y in px
	void AddDebugAxisOnScreen( const Uint32 x, const Uint32 y, const Matrix& rot, const Float scale, Bool alwaysVisible=false );

	// Draw debug axis on screen, x[0 1] y[0 1]
	void AddDebugAxisOnScreen( const Float x, const Float y, const Matrix& rot, const Float scale, Bool alwaysVisible=false );

	// Draw debug arrow
	void AddDebugArrow( const Matrix& localToWorld, const Vector& direction, const Float scale, const Color& color, Bool overlay=false, Bool alwaysVisible=false );

	// Draw debug 3D arrow. Made up of a tube and cone, useful to better represent a 3D direction.
	// position: world-space position of the tail-end.
	// direction: world-space direction
	// scale: applied to overall length of arrow (does not affect radii)
	// radius1: radius of the arrow tail
	// radius2: max radius of the arrow head (where it attaches to the tail)
	// headLength: absolute length of the head (is not scaled according to scale, unless scale is very small)
	// color1: color of the arrow tail
	// color2: color of the arrow head
	void AddDebug3DArrow( const Vector& position, const Vector& direction, const Float scale, Float radius1, Float radius2, Float headLength, const Color& color );
	void AddDebug3DArrow( const Vector& position, const Vector& direction, const Float scale, Float radius1, Float radius2, Float headLength, const Color& color1, const Color& color2 );

	// Draw debug bone
	void AddDebugBone( const Matrix& tm, const Matrix& ptm, const Color& color, Bool overlay=false, Float mul=1.0f, Bool alwaysVisible=false );

	// Draw debug screen frame
	void AddDebugFrame( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color );

	// Draw debug screen rect
	void AddDebugRect( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color );

	// Draw debug gradient rect
	void AddDebugGradientRect( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool vertical = false );

	// Draw a list of gradient rectangles (screen-space). rects, colors1, colors2 must all be same size.
	void AddDebugGradientRects( Uint32 numRects, const Rect* rects, const Color* colors1, const Color* colors2, Bool vertical = false );

	// Draw debug filled rectangle
	void AddDebugFilledRect( const Matrix& mtx, const Color& color );

	// Draw debug screen rect
	void AddDebugTexturedRect( Int32 x, Int32 y, Int32 width, Int32 height, CBitmapTexture* texture, const Color& color );

	// Draw debug dynamic texture from GpuApi::TextureRef
	void AddDebugDynamicTexture( Int32 x, Int32 y, Int32 width, Int32 height, GpuApi::TextureRef textureRef, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector );

	// Set material debug mode
	void SetMaterialDebugMode( EMaterialDebugMode mdm );


	Rect GetDebugTextBounds( const String& text, CFont* font = NULL );
};

/// New operator for memory allocation from rendering memory pool
RED_INLINE void* operator new( size_t size, CRenderFrame* frame, Uint32 alignment = CSystem::DEFAULT_ALIGNMENT )
{
	return frame->GetMemoryPool().Alloc( static_cast< Uint32 >( size ), alignment );
}

/// New operator for memory allocation from rendering memory pool
RED_INLINE void* operator new[]( size_t size, CRenderFrame* frame, Uint32 alignment = CSystem::DEFAULT_ALIGNMENT )
{
	return frame->GetMemoryPool().Alloc( static_cast< Uint32 >( size ), alignment );
}

RED_INLINE void operator delete( void*, CRenderFrame*, Uint32 )
{
	RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); 
}

RED_INLINE void operator delete[]( void*, CRenderFrame*, Uint32 )
{
	RED_FATAL_ASSERT(0, "This should never happen, we don't use exceptions!"); 
}
