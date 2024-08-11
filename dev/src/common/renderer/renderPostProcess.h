/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderHelpers.h"

/// Post-process utilities
namespace PostProcessUtilities
{
	struct TexelArea
	{
		TexelArea ();
		TexelArea ( Int32 width, Int32 height );
		TexelArea ( Int32 width, Int32 height, Int32 offsetX, Int32 offsetY );

		bool operator==( const TexelArea &other ) const;
		bool operator!=( const TexelArea &other ) const;

		bool IsEmpty() const;
		bool IsBothDimsEqual( const TexelArea &other ) const;
		bool IsBothDimsGreater( const TexelArea &other ) const;
		bool IsBothDimsGreaterOrEqual( const TexelArea &other ) const;
		bool IsValidForResolution( Int32 fullWidth, Int32 fullHeight ) const;
		TexelArea GetWithNoOffset() const;
		Int32  GetSize() const;
		
		Int32 m_width;
		Int32 m_height;
		Int32 m_offsetX;
		Int32 m_offsetY;
	};	

	/// Calculate clipspace vertices based on render area.
	void CalculateTriangleStripVertices( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Vector outVerts[4], Float z = 0.5f );
	
	/// Calculates transformation (scale_x, scale_y, bias_x, bias_y) which maps clipspace vertices generated for given render area, into given sample area.
	Vector CalculateTexCoordTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight );
	Vector CalculateTexCoordTransform( const TexelArea &renderArea, const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight );
	Vector CalculateTexCoordTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, const TexelArea &sampleArea );
	Vector CalculateTexCoordTransform( const TexelArea &renderArea, const TexelArea &sampleArea );

	/// Calculates transformation (scale_x, scale_y, bias_x, bias_y) which maps clipspace vertices generated for given render area, wrapped into given sample area.
	Vector CalculateTexCoordTransformOneToOne( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Int32 sampleFullWidth, Int32 sampleFullHeight );
	Vector CalculateTexCoordTransformOneToOne( const TexelArea &renderArea, Int32 sampleFullWidth, Int32 sampleFullHeight );

	/// Calculates transformation (scale_x, scale_y, bias_x, bias_y) which maps clipspace vertices generated for given render area, into vector which may be multiplied by linear depth value to obtain view space position.
	/** Obtained view vectory in meant to be interpolated across vertices. */
	Vector CalculateViewSpaceVecTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Float cameraFovAngleDegrees, Float cameraAspectRatio, Float cameraZoom );
	Vector CalculateViewSpaceVecTransform( const TexelArea &renderArea, Float cameraFovAngleDegrees, Float cameraAspectRatio, Float cameraZoom );

	/// Calculates texcoord delta for two neighbouring texels of given sample area
	Vector CalculateTexCoordTexelOffset( Int32 sampleFullWidth, Int32 sampleFullHeight );

	/// Calculates clamping borders (clamp_min_x, clamp_min_y, clamp_max_x, clamp_max_y) for given sample area.
	Vector CalculateTexCoordClamp( const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight );
}


struct SRenderLuminanceInfo
{
	SRenderLuminanceInfo ()
		: renderLumMin( 0.f )
		, renderLumMax( 1.f )
		, absoluteLumMin( 0.f )
		, absoluteLumMax( 1.f )
	{}

	Float renderLumMin;		//< Minimum luminance which we should render
	Float renderLumMax;		//< Maximum luminance which we should render
	Float absoluteLumMin;	//< Minimum absolute luminance
	Float absoluteLumMax;	//< Maximum absolute luminance
};

/// Fade params
struct SRenderFadeParams
{
	// Color fade
	Vector		m_sourceColor;			//!< Source color
	Vector		m_targetColor;			//!< Target color
	Vector		m_currentColor;			//!< Current color
	Float		m_velocityColor;		//!< Color blend velocity
	Float		m_currentColorLerp;		//!< Current color lerp

	// Alpha fade
	Float		m_currentAlpha;			//!< Current fade alpha
	Float		m_velocityAlpha;		//!< Current alpha velocity

	SRenderFadeParams()
		: m_currentColor( Vector::ZEROS )
		, m_targetColor( Vector::ZEROS )
		, m_sourceColor( Vector::ZEROS )
		, m_currentColorLerp( 0.0f )
		, m_velocityColor( 0.0f )
		, m_currentAlpha( 0.0f )
		, m_velocityAlpha( 0.0f )
	{};

	//! Is fade enabled ?
	bool IsEnabled() const;

	//! Start fade to color
	void FadeOut( const Color& target, Float fadeTime );

	//! Start fade in
	void FadeIn( Float fadeTime );

	//! Sets fade and keeps it
	void SetFade( Bool isIn, const Color& color, Float progress );

	//! Update
	void Update( Float timeDeltaR );
};

class CPostProcessQuadArray;

/// Post-process renderer
class CRenderPostProcess
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );
public:
	class CPostProcessDrawer*	m_drawer;							//!< Post process drawer	
	SRenderFadeParams			m_fade;								//!< Fade effect
	ERenderTargetName			m_lumSurfaceSimple;					//!< Render target for luminance analysing
	CPostProcessQuadArray*		m_quadArray;						//!< Quad array
	Int32						m_quadArrayWidth;					//!< Quad array size
	Int32						m_quadArrayHeight;					//!< Quad array size

private:
	GpuApi::BufferRef			m_histogramBuffer;
	GpuApi::BufferRef			m_temporalAABuffer;

public:
	CRenderPostProcess( const CRenderSurfaces* surfaces );
	~CRenderPostProcess();

	// Update internals, renderer time base
	void Update( Float timeDeltaR );

	// Draw scene enrichment filters
	void DrawSceneEnrichment( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, const CRenderCollector& collector, Bool blendAccumulativeRefraction = false );

	// Draw final image (with optional tone mapping, bloom etc applied)
	ERenderTargetName DrawFinal( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, EPostProcessCategoryFlags categoriesMask, bool isUbersampleFirst, bool isUbersampleLast );

	// Draw 
	void DrawFinalizationAndAA( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, EPostProcessCategoryFlags categoriesMask, ERenderTargetName rtSample );

	// Frozen frame shit
	void DrawFrozenFrame( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, class CRenderViewport* viewport, EPostProcessCategoryFlags categoriesMask );

	// Draw fade quad
	void DrawFade( const CRenderFrameInfo& info );

	// Draw ssao
	Bool DrawSSAO( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnTexture, EPostProcessCategoryFlags flags );

	// Process tonemapping
	void ProcessTonemappingHelpers( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Uint32 colorTargetWidth, Uint32 colorTargetHeight, ERenderTargetName rtnColor, ERenderTargetName rtnColorTemp, ERenderTargetName rtnMainColor );

	// Resolve MSAA stencil
	Bool ResolveMSAAStencil( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Uint8 stencilBits );

	// Resolve MSAA depth
	Bool ResolveMSAADepth( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces );

	// Resolve MSAA simple with custom targets
	Bool ResolveMSAASimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnSampler, ERenderTargetName rtnTarget );

	// Resolve MSAA color simple
	Bool ResolveMSAAColorSimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnTarget );

	// Resolve MSAA buffer
	Bool ResolveMSAABufferSimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnSampler, ERenderTargetName rtnTarget );

	// Render debug overlay
	Bool IsDebugOverlayActivated( const CRenderFrameInfo& info, Bool allowZoom ) const;
	void DrawDebugOverlay( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Bool allowZoom );

	// Copy
	void CopyWithScale( const GpuApi::TextureRef &fromTex, const PostProcessUtilities::TexelArea &fromArea, const GpuApi::TextureRef &toTex, const PostProcessUtilities::TexelArea &toArea, Float scalar, GpuApi::eDrawContext context );
	void CopyWithScale( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName from, ERenderTargetName to, Float scalar, GpuApi::eDrawContext context );
	void CopyDepthTarget( const GpuApi::TextureRef &from, const PostProcessUtilities::TexelArea &fromArea );

	// Build feedback depth buffer
	void DrawFeedbackDepthBuffer( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces );

	// Render surface flow effect (for rain drops etc)
	void DrawSurfaceFlow( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnTarget, ERenderTargetName rtnHelper );

	// Pre-present copy, aware of cachets
	void PresentCopy( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName from, const GpuApi::Rect& sourceRect, const GpuApi::Rect& targetRect );

	// On lost device
	void OnLostDevice();

public:
	//! Start fade out to color
	void FadeOut( const Color& color, Float fadeTime );

	//! Start fade int
	void FadeIn( Float fadeTime );

	Bool IsFadeInProgress() const;

	void SetFade( Bool isIn, const Color& color, Float progress );

	Bool IsBlackscreen() const;

	Bool IsPostFxAntialiasingEnabled( EPostProcessCategoryFlags flags, const CRenderFrameInfo& info );

	Bool IsPostFxTemporalAAEnabled( const CRenderCollector& collector, Bool testDirtyFlag = false );
};
