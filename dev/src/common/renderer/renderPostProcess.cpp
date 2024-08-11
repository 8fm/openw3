/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderPostProcess.h"
using namespace PostProcessUtilities;
#include "renderPostFx.h"
#include "gameplayFx.h"
#include "renderCollector.h"
#include "renderShaderPair.h"

#include "renderScene.h"
#include "renderProxyWater.h"
#include "renderInterface.h"
#include "renderPostFxStateGrabber.h"
#include "../engine/renderFragment.h"
#include "../engine/viewport.h"
#include "../engine/bitmapTexture.h"

namespace Config
{
	TConfigVar<Int32> cvPaintEffectForcedSize			( "PostProcess", "PaintEffectForcedSize",			-1 );
}

#ifdef USE_ANSEL
extern Bool isAnselCaptureActive;
#endif // USE_ANSEL

SPostFxEffects::SPostFxEffects ()
{
	NullEffects();
}

void SPostFxEffects::CreateEffects( const CRenderSurfaces* surfaces )
{
	fxPresent			= new CPostFxPresent;
	fxCopy				= new CPostFxCopy;
	fxDOF				= new CPostFxDepthOfField;
	fxBlurGauss			= new CPostFxBlurGauss;
	fxToneMapping		= new CPostFxToneMapping;
	fxBlurFilter		= new CPostFxBlur;
	fxFlare				= new CPostFxFlare;
	fxDebug				= new CPostFxDebug;
	fxHBSSAO			= new CPostFXHorizonBasedSSAO;
	fxMSSSAO			= new CPostFXMicrosoftSSAO;
	fxMotionBlur		= new CPostFxMotionBlur;
	fxUnderwater		= new CPostFxUnderwater;
	fxBokehDof			= new CPostFxBokehDof;
	fxSurfaceFlow		= new CPostFxSurfaceFlow;	
	fxStateGrabber		= new CPostFxStateGrabber;
}

void SPostFxEffects::DestroyEffects()
{
	delete fxPresent;
	delete fxCopy;
	delete fxDOF;
	delete fxBlurGauss;
	delete fxToneMapping;
	delete fxBlurFilter;
	delete fxDebug;
	delete fxFlare;
	delete fxHBSSAO;
	delete fxMSSSAO;
	delete fxMotionBlur;
	delete fxBokehDof;
	delete fxStateGrabber;

	NullEffects();
}

void SPostFxEffects::NullEffects()
{
	fxPresent			= nullptr;
	fxCopy				= nullptr;
	fxDOF				= nullptr;
	fxBlurGauss			= nullptr;
	fxToneMapping		= nullptr;
	fxBlurFilter	    = nullptr;
	fxDebug				= nullptr;
	fxFlare				= nullptr;
	fxHBSSAO			= nullptr;
	fxMSSSAO			= nullptr;
	fxMotionBlur		= nullptr;
	fxBokehDof			= nullptr;
	fxStateGrabber		= nullptr;
}


void CPostProcessDrawer::OnLostDevice()
{
	if ( m_pEffects && m_pEffects->fxHBSSAO )
	{
		m_pEffects->fxHBSSAO->OnLostDevice();
	}
}

/************************************************************************/
/* Post-process utilities namespace                                     */
/************************************************************************/

namespace PostProcessUtilities
{
	TexelArea::TexelArea ()
	{
		m_width = 0;
		m_height = 0;
		m_offsetX = 0;
		m_offsetY = 0;
	}

	TexelArea::TexelArea ( Int32 width, Int32 height )
	{
		m_width = width;
		m_height = height;
		m_offsetX = 0;
		m_offsetY = 0;
	}

	TexelArea::TexelArea ( Int32 width, Int32 height, Int32 offsetX, Int32 offsetY )
	{
		m_width = width;
		m_height = height;
		m_offsetX = offsetX;
		m_offsetY = offsetY;
	}

	bool TexelArea::operator==( const TexelArea &other ) const
	{
		return 
			m_width  == other.m_width	 &&
			m_height == other.m_height	 &&
			m_offsetX == other.m_offsetX &&
			m_offsetY == other.m_offsetY;
	}

	bool TexelArea::operator!=( const TexelArea &other ) const
	{
		return !operator==(other);
	}

	bool TexelArea::IsEmpty() const
	{
		return m_width <= 0 || m_height <= 0;
	}

	bool TexelArea::IsBothDimsEqual( const TexelArea &other ) const
	{
		return m_width == other.m_width && m_height == other.m_height;
	}

	bool TexelArea::IsBothDimsGreater( const TexelArea &other ) const
	{
		return m_width > other.m_width && m_height > other.m_height;
	}

	bool TexelArea::IsBothDimsGreaterOrEqual( const TexelArea &other ) const
	{
		return m_width >= other.m_width && m_height >= other.m_height;
	}

	bool TexelArea::IsValidForResolution( Int32 fullWidth, Int32 fullHeight ) const
	{
		return 
			fullWidth >= 0 && fullHeight >= 0 &&
			m_offsetX >= 0 && m_offsetY  >= 0 &&
			m_width   >= 0 && m_height   >= 0 &&
			m_offsetX + m_width  <= fullWidth && 
			m_offsetY + m_height <= fullHeight;
	}

	TexelArea TexelArea::GetWithNoOffset() const
	{
		return TexelArea ( m_width, m_height, 0, 0 );
	}

	Int32 TexelArea::GetSize() const
	{
		return IsEmpty() ? 0 : m_width * m_height;
	}

	void CalculateTriangleStripVertices( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Vector outVerts[4], Float z /*= 0.5f*/ )
	{
		ASSERT( !renderArea.IsEmpty() && renderArea.IsValidForResolution( renderFullWidth, renderFullHeight ) );

		const Float x = renderArea.m_offsetX / (Float) renderFullWidth;
		const Float y = renderArea.m_offsetY / (Float) renderFullHeight;
		const Float w = renderArea.m_width   / (Float) renderFullWidth;
		const Float h = renderArea.m_height  / (Float) renderFullHeight;

		outVerts[0].Set3( x+w, 1-y,     z );
		outVerts[1].Set3( x+w, 1-(y+h), z );
		outVerts[2].Set3( x,   1-y,     z );
		outVerts[3].Set3( x,   1-(y+h), z );

		for ( Uint32 i=0; i<4; ++i )
		{
			outVerts[i].X = 2.f * outVerts[i].X - 1.f;
			outVerts[i].Y = 2.f * outVerts[i].Y - 1.f;
		}
	}

	Vector CalculateTexCoordTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( !renderArea.IsEmpty() && renderArea.IsValidForResolution( renderFullWidth, renderFullHeight ) );
		ASSERT( !sampleArea.IsEmpty() && sampleArea.IsValidForResolution( sampleFullWidth, sampleFullHeight ) );

		Float rw = 2.f * renderArea.m_width   / (Float) renderFullWidth;
		Float rh = 2.f * renderArea.m_height  / (Float) renderFullHeight;
		Float rx = 2.f * renderArea.m_offsetX / (Float) renderFullWidth - 1.f;
		Float ry = 2.f * renderArea.m_offsetY / (Float) renderFullHeight - 1.f;		

		Float sw = sampleArea.m_width / (Float) sampleFullWidth;
		Float sh = sampleArea.m_height / (Float) sampleFullHeight;
		Float sx = sampleArea.m_offsetX / (Float) sampleFullWidth;
		Float sy = sampleArea.m_offsetY / (Float) sampleFullHeight;				

		Float scale_x = sw / rw;
		Float scale_y = sh / rh;
		Float off_x   = sx - rx * sw / rw;
		Float off_y   = ry * sh / rh + sh + (1-sh-sy);

		return Vector ( scale_x, -scale_y, off_x, 1 - off_y );
	}

	Vector CalculateTexCoordTransform( const TexelArea &renderArea, const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( 0==renderArea.m_offsetX && 0==renderArea.m_offsetY && "Full size can't be the same as area size otherwise" );
		return CalculateTexCoordTransform( renderArea, renderArea.m_width, renderArea.m_height, sampleArea, sampleFullWidth, sampleFullHeight );
	}

	Vector CalculateTexCoordTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, const TexelArea &sampleArea )
	{
		ASSERT( 0==sampleArea.m_offsetX && 0==sampleArea.m_offsetY && "Full size can't be the same as area size otherwise" );
		return CalculateTexCoordTransform( renderArea, renderFullWidth, renderFullHeight, sampleArea, sampleArea.m_width, sampleArea.m_height );
	}

	Vector CalculateTexCoordTransform( const TexelArea &renderArea, const TexelArea &sampleArea )
	{
		ASSERT( 0==renderArea.m_offsetX && 0==renderArea.m_offsetY && "Full size can't be the same as area size otherwise" );
		ASSERT( 0==sampleArea.m_offsetX && 0==sampleArea.m_offsetY && "Full size can't be the same as area size otherwise" );		
		return CalculateTexCoordTransform( renderArea, renderArea.m_width, renderArea.m_height, sampleArea, sampleArea.m_width, sampleArea.m_height );
	}

	Vector CalculateTexCoordTransformOneToOne( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( sampleFullWidth > 0 && sampleFullHeight > 0 );
		ASSERT( !renderArea.IsEmpty() && renderArea.IsValidForResolution( renderFullWidth, renderFullHeight ) );

		// renderArea as source and dest is not a mistake here!
		Vector transform = CalculateTexCoordTransform( renderArea, renderFullWidth, renderFullHeight, renderArea, renderFullWidth, renderFullHeight );
		Float  sx = renderFullWidth / (Float) Max( 1, sampleFullWidth );
		Float  sy = renderFullHeight / (Float) Max( 1, sampleFullHeight );
		return transform * Vector ( sx, sy, sx, sy );		
	}

	Vector CalculateTexCoordTransformOneToOne( const TexelArea &renderArea, Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( 0==renderArea.m_offsetX && 0==renderArea.m_offsetY && "Full size can't be the same as area size otherwise" );
		return CalculateTexCoordTransformOneToOne( renderArea, renderArea.m_width, renderArea.m_height, sampleFullWidth, sampleFullHeight );
	}

	Vector CalculateViewSpaceVecTransform( const TexelArea &renderArea, Int32 renderFullWidth, Int32 renderFullHeight, Float cameraFovAngleDegrees, Float cameraAspectRatio, Float cameraZoom )
	{
		ASSERT( !renderArea.IsEmpty() && renderArea.IsValidForResolution( renderFullWidth, renderFullHeight ) );

		Float sx = renderFullWidth / (Float) renderArea.m_width;
		Float sy = renderFullHeight / (Float) renderArea.m_height;
		Float tx = - sx * (2.f * renderArea.m_offsetX / (Float) renderFullWidth - 1 ) - 1;
		Float ty = - sy * (2.f * (renderFullHeight - renderArea.m_height - renderArea.m_offsetY) / (Float) renderFullHeight - 1 ) - 1;

		Float _tanY = tanf( 0.5f * DEG2RAD( cameraFovAngleDegrees ) );
		Float _tanX = _tanY * cameraAspectRatio;

		Float zoomScale = 1.f / Max(0.001f, cameraZoom);
		return Vector ( sx, sy, tx, ty ) * Vector ( _tanX, _tanY, _tanX, _tanY ) * zoomScale;
	}

	Vector CalculateViewSpaceVecTransform( const TexelArea &renderArea, Float cameraFovAngleDegrees, Float cameraAspectRatio, Float cameraZoom )
	{
		ASSERT( 0==renderArea.m_offsetX && 0==renderArea.m_offsetY && "Full size can't be the same as area size otherwise" );
		return CalculateViewSpaceVecTransform( renderArea, renderArea.m_width, renderArea.m_height, cameraFovAngleDegrees, cameraAspectRatio, cameraZoom );
	}

	Vector CalculateTexCoordTexelOffset( Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( sampleFullWidth > 0 && sampleFullHeight > 0 );
		return Vector ( 1.f / sampleFullWidth, 1.f / sampleFullHeight, 0, 0 );
	}

	Vector CalculateTexCoordClamp( const TexelArea &sampleArea, Int32 sampleFullWidth, Int32 sampleFullHeight )
	{
		ASSERT( !sampleArea.IsEmpty() && sampleArea.IsValidForResolution( sampleFullWidth, sampleFullHeight ) );

		const Float x  = (Float) sampleArea.m_offsetX;
		const Float y  = (Float) sampleArea.m_offsetY;
		const Float x2 = (Float) sampleArea.m_offsetX + sampleArea.m_width;
		const Float y2 = (Float) sampleArea.m_offsetY + sampleArea.m_height;
		const Float w  = (Float) sampleFullWidth;
		const Float h  = (Float) sampleFullHeight;

		return Vector ( x/w, y/h, (x2-1.0f)/w, (y2-1.0f)/h );
	}
	
	CRenderTexture* ExtractRenderTexture( const CBitmapTexture *bitmapTexture )
	{
		if ( NULL == bitmapTexture )
			return NULL;
		IRenderResource *res = bitmapTexture->GetRenderResource();	
		return res && res->IsRenderTexture() ? static_cast< CRenderTexture* >( res ) : NULL;
	}

	CRenderTexture* ExtractRenderTexture( const TSoftHandle< CBitmapTexture > &bitmapTexture )
	{
		return ExtractRenderTexture( bitmapTexture.Get() );
	}
}

/************************************************************************/
/* Post-process quad array												*/
/************************************************************************/

CPostProcessQuadArray::CPostProcessQuadArray( Uint32 width, Uint32 height, const Float z /*=0.5f*/ )
	: m_width( width )
	, m_height( height )
	, m_numQuads( width*height )
	, m_isValid( false )
{
	TDynArray< DebugVertexUV >		vertices;
	TDynArray< Uint32 >				indices; // TODO moradin why is this Uint32?

	// Allocate data buffers
	vertices.Resize( m_numQuads * 4 );
	Red::System::MemorySet( vertices.TypedData(), 0, vertices.DataSize() );
	indices.Resize( m_numQuads * 6 );
	Red::System::MemorySet( indices.TypedData(), 0, indices.DataSize() );

	// Determine vertex positions
	Vector positions[4];
	positions[0].Set4( 0.0f, +1.0f, z, 0.0f );
	positions[1].Set4( +1.0f, +1.0f, z, 0.0f );
	positions[2].Set4( +1.0f, 0.0f, z, 0.0f );
	positions[3].Set4( 0.0f, 0.0f, z, 0.0f );

	// Generate quads
	Uint32 baseIndex = 0;
	Uint32* writeIndex = indices.TypedData();
	DebugVertexUV* writeVertex = vertices.TypedData();
	for ( Uint32 y=0; y<height; ++y )
	{
		for ( Uint32 x=0; x<width; ++x )
		{
			// Calculate UV
			const Float u = (Float)x;
			const Float v = (Float)y;

			// Assemble vertices	
			writeVertex[0].Set( positions[0], Color::WHITE, u, v );
			writeVertex[1].Set( positions[1], Color::WHITE, u, v );
			writeVertex[2].Set( positions[2], Color::WHITE, u, v );
			writeVertex[3].Set( positions[3], Color::WHITE, u, v );

			// Assemble indices
			writeIndex[0] = baseIndex + 0;
			writeIndex[1] = baseIndex + 1;
			writeIndex[2] = baseIndex + 2;
			writeIndex[3] = baseIndex + 0;
			writeIndex[4] = baseIndex + 2;
			writeIndex[5] = baseIndex + 3;

			// Advance to next quad
			baseIndex += 4;
			writeIndex += 6;
			writeVertex += 4;
		}
	}

	{
		// Create buffers
		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = vertices.TypedData();
			m_vertices = GpuApi::CreateBuffer( vertices.Size() * sizeof(DebugVertexUV), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
			if ( !m_vertices )
			{
				ASSERT( m_vertices );
				return;
			}
			GpuApi::SetBufferDebugPath( m_vertices, "postprocessquadVB" );
		}

		// Create index buffer
		{
			GpuApi::BufferInitData bufInitData;
			bufInitData.m_buffer = indices.TypedData();
			m_indices = GpuApi::CreateBuffer( indices.Size() * 4, GpuApi::BCC_Index32Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
			if ( !m_indices )
			{
				ASSERT( m_indices );
				GpuApi::SafeRelease( m_vertices );
				return;
			}
			GpuApi::SetBufferDebugPath( m_indices, "postprocessquadIB" );
		}

		// Valid
		m_isValid = true;
	}
}

CPostProcessQuadArray::~CPostProcessQuadArray()
{
	GpuApi::SafeRelease( m_vertices );
	GpuApi::SafeRelease( m_indices );
}

void CPostProcessQuadArray::Draw() const
{
	if ( m_isValid )
	{
		ASSERT( m_vertices && m_indices );
		// Bind buffers
		Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPosColorUV );
		Uint32 vbOffset = 0;
		GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPosColorUV );
		GpuApi::BindVertexBuffers(0, 1, &m_vertices, &vbSstride, &vbOffset);

		GpuApi::BindIndexBuffer( m_indices );

		// Draw chunk
		GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_numQuads * 4, 0, m_numQuads * 2 );
	}
}


/************************************************************************/
/* Post-process drawer                                                  */
/************************************************************************/

CPostProcessDrawer::CPostProcessDrawer( const CRenderSurfaces* surfaces )
{
	// Create effects
	m_pEffects = new SPostFxEffects;
	m_pEffects->CreateEffects( surfaces );
}

CPostProcessDrawer::~CPostProcessDrawer ()
{
	// Destroy effects
	m_pEffects->DestroyEffects();
	delete m_pEffects;
}

void CPostProcessDrawer::DrawQuad( const PostProcessUtilities::TexelArea &renderArea, CRenderShaderPair *shader, Float z /*= 0.5f*/ ) const
{
	// Bind shader
	if ( shader )
	{
		shader->Bind();
	}

	// Get viewport size
	Int32 viewportWidth  = GpuApi::GetViewport().width;
	Int32 viewportHeight = GpuApi::GetViewport().height;
	ASSERT( renderArea.IsValidForResolution( viewportWidth, viewportHeight ) );

	// Calculate positions
	Vector positions[4];
	PostProcessUtilities::CalculateTriangleStripVertices( renderArea, viewportWidth, viewportHeight, positions, z );

	// Assemble vertices	
	DebugVertexUV points[4];
	points[0].Set( positions[0], Color::WHITE, 0, 0 );
	points[1].Set( positions[1], Color::WHITE, 0, 0 );
	points[2].Set( positions[2], Color::WHITE, 0, 0 );
	points[3].Set( positions[3], Color::WHITE, 0, 0 );

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 2, points );
}

void CPostProcessDrawer::DrawQuadArray( CRenderShaderPair *shader, const CPostProcessQuadArray& quads ) const
{
	// Bind shader
	if ( shader )
	{
		shader->Bind();
	}

	// Draw quads
	quads.Draw();
}

void CPostProcessDrawer::DrawQuad( CRenderShaderPair *shader, Float z /*= 0.5f*/, const Color& color /*= Color::WHITE*/ ) const
{
	// Bind shader
	if ( shader )
	{
		shader->Bind();
	}

	// Assemble vertices	
	DebugVertexUV points[3];
	points[0].Set( Vector(  -1.f, 3.f, z ), color, 0, 0 );
	points[1].Set( Vector(  3.f, -1.f, z ), color, 0, 0 );
	points[2].Set( Vector(  -1.f, -1.f, z ), color, 0, 0 );

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 1, points );
}

/************************************************************************/
/* Screen fade															*/
/************************************************************************/

bool SRenderFadeParams::IsEnabled() const
{
	return m_currentAlpha > 0.0f;
}

void SRenderFadeParams::Update( Float timeDeltaR )
{
	// Update alpha
	if ( m_velocityAlpha != 0.0f )
	{
		const Float delta = timeDeltaR * m_velocityAlpha;
		if ( delta > 0.0f )
		{
			// Fade out
			m_currentAlpha += delta;
			if ( m_currentAlpha >= 1.0f )
			{
				// End fade out
				m_currentAlpha = 1.0f;
				m_velocityAlpha = 0.0f;
			}
		}
		else if ( delta < 0.0f )
		{
			// Fade in
			m_currentAlpha += delta;
			if ( m_currentAlpha <= 0.0f )
			{
				// End fade in
				m_currentAlpha = 0.0f;
				m_velocityAlpha = 0.0f;
			}
		}
	}

	// Interpolate color
	if ( m_velocityColor != 0.0f )
	{
		const Float deltaC = timeDeltaR * m_velocityColor;
		if ( deltaC > 0.0f )
		{
			// Process fade
			m_currentColorLerp += deltaC;
			if ( m_currentColorLerp >= 1.0f )
			{
				m_currentColor = m_targetColor;
				m_currentColorLerp = 1.0f;
				m_velocityColor = 0.0f;
			}
			else
			{
				// Interpolate
				m_currentColor = ::Lerp< Vector >( m_currentColorLerp, m_sourceColor, m_targetColor );
			}
		}
	}
}

void SRenderFadeParams::FadeOut( const Color& target, Float fadeTime )
{
	// Instant fade
	if ( fadeTime <= 0.0f )
	{
		// Instant alpha fade
		m_currentAlpha = 1.0f;
		m_velocityAlpha = 0.0f;

		// Instant color fade
		m_currentColor = target.ToVector();
		m_sourceColor = target.ToVector();
		m_targetColor = target.ToVector();
		m_velocityColor = 0.0f;
		m_currentColorLerp = 0.0f;
		return;
	}

	// Process alpha
	if ( m_currentAlpha < 1.0f )
	{
		//const Float alphaDelta = 1.0f - m_currentAlpha;
		//m_velocityAlpha = alphaDelta / fadeTime;
		m_velocityAlpha = 1.0f / fadeTime;
	}

	// Process color
	m_sourceColor = m_currentColor;
	m_targetColor = target.ToVector();
	m_velocityColor = 1.0f / fadeTime;
	m_currentColorLerp = 0.0f;
}

void SRenderFadeParams::FadeIn( Float fadeTime )
{
	// Instant fade
	if ( fadeTime <= 0.0f )
	{
		// Instant alpha fade
		m_currentAlpha = 0.0f;
		m_velocityAlpha = 0.0f;

		// Reset color
		m_currentColor = m_targetColor;
		return;
	}

	// Process alpha	
	//const Float alphaDelta = 0.0f - m_currentAlpha;
	//m_velocityAlpha = alphaDelta / fadeTime;

	m_velocityAlpha = -1.0f / fadeTime;	
}

void SRenderFadeParams::SetFade( Bool isIn, const Color& color, Float progress )
{
	m_velocityAlpha = 0.0f;

	m_sourceColor = m_targetColor = m_currentColor = color.ToVector();
	m_velocityColor = 0.0f;
	m_currentColorLerp = 1.0f;

	if ( isIn )
	{
		m_currentAlpha = 1.0f - progress;
	}
	else
	{
		m_currentAlpha = progress;
	}
}

/************************************************************************/
/* Post-process framework                                               */
/************************************************************************/

CRenderPostProcess::CRenderPostProcess( const CRenderSurfaces* surfaces )
	: m_lumSurfaceSimple( RTN_LuminanceSimpleAPing )
{	
	// Create drawer
	m_drawer			= new CPostProcessDrawer ( surfaces );

	m_quadArray = NULL;
	m_quadArrayWidth = 0;
	m_quadArrayHeight = 0;

	// Create histogram buffer
	{
		Uint32 numElements = 64 * HISTOGRAM_CAPACITY_MULTIPLIER;
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = nullptr;
		bufInitData.m_elementCount = numElements;
		m_histogramBuffer = GpuApi::CreateBuffer( 4 * numElements, GpuApi::BCC_StructuredUAV, GpuApi::BUT_Default, 0, &bufInitData );
	}
}

CRenderPostProcess::~CRenderPostProcess()
{	
	// Destroy drawer
	delete m_drawer;

	if ( m_quadArray )
	{
		delete m_quadArray;
	}

	// Destroy histogram buffer
	GpuApi::SafeRelease( m_histogramBuffer );

	// Destroy temporal AA buffer
	GpuApi::SafeRelease( m_temporalAABuffer );
}

#if 0
#define SHOW_DEBUG_SURFACE(index, surface) if (debugOutput == index) { debugToShow = surface; goto debugLabel;}
#else
#define SHOW_DEBUG_SURFACE(index, surface)
#endif

void CRenderPostProcess::Update( Float timeDeltaR )
{
	m_fade.Update( timeDeltaR );
}

void CRenderPostProcess::FadeOut( const Color& color, Float fadeTime )
{
	m_fade.FadeOut( color, fadeTime );
}

void CRenderPostProcess::FadeIn( Float fadeTime )
{
	m_fade.FadeIn( fadeTime );
}

void CRenderPostProcess::SetFade( Bool isIn, const Color& color, Float progress )
{
	m_fade.SetFade( isIn, color, progress );
}

Bool CRenderPostProcess::IsFadeInProgress() const
{
	return m_fade.m_velocityAlpha != 0.0f;
}

Bool CRenderPostProcess::IsBlackscreen() const
{
	return m_fade.m_currentAlpha == 1.0f;
}


Bool CRenderPostProcess::DrawSSAO( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnTexture, EPostProcessCategoryFlags flags )
{
	//decide which solution to use
	// force hbao+ when in Ansel mode
	Int32 choice = 
#ifdef USE_ANSEL
		isAnselCaptureActive ? 2 :
#endif // USE_ANSEL
		Config::cvSSAOVersion.Get(); 
	
	if ( choice == 2 )
	{
#ifndef USE_NVSSAO
		choice = 0;
#endif
	}

	if ( choice == 1 )
	{
#ifndef USE_MSSSAO
		choice = 0;
#endif
	}

#ifdef USE_NVSSAO
	if ( choice == 2 )
	{
		// Early exit if there would be no visible effect
		if ( !info.m_isWorldScene || !m_drawer->GetEffects().fxHBSSAO->IsSSAOEnabled( info.m_envParametersGame.m_displaySettings, info.m_envParametersArea ) )
		{
			return false;
		}

		const CGpuApiScopedDrawContext baseDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );
		return m_drawer->GetEffects().fxHBSSAO->Apply( *m_drawer, surfaces, rtnTexture, RTN_Color, info );
	}
#endif

#if defined( USE_MSSSAO )
	if( choice == 1 )
	{
		// Early exit if there would be no visible effect
		if ( !info.m_isWorldScene || !m_drawer->GetEffects().fxMSSSAO->IsSSAOEnabled( info.m_envParametersGame.m_displaySettings, info.m_envParametersArea ) )
		{
			return false;
		}

		const CGpuApiScopedDrawContext baseDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );
		m_drawer->GetEffects().fxMSSSAO->Apply( *m_drawer, surfaces, rtnTexture, RTN_Color, info );
	}
#endif

	return true;
}

static RED_INLINE ERenderTargetName AdvanceLuminanceRenderTargetSimple( ERenderTargetName current )
{
	if ( current == RTN_LuminanceSimpleAPing )
	{
		return RTN_LuminanceSimpleBPing;
	}
	else if ( current == RTN_LuminanceSimpleBPing )
	{
		return RTN_LuminanceSimpleAPong;
	}
	else if ( current == RTN_LuminanceSimpleAPong )
	{
		return RTN_LuminanceSimpleBPong;
	}
	else
	{
		return RTN_LuminanceSimpleAPing;
	}
}

static RED_INLINE ERenderTargetName GetCorrespondingRenderTargetForRead( ERenderTargetName current )
{
	if ( current == RTN_LuminanceSimpleAPing )
	{
		return RTN_LuminanceSimpleBPong;
	}
	else if ( current == RTN_LuminanceSimpleBPing )
	{
		return RTN_LuminanceSimpleAPing;
	}
	else if ( current == RTN_LuminanceSimpleAPong )
	{
		return RTN_LuminanceSimpleBPing;
	}
	else 
	{
		return RTN_LuminanceSimpleAPong;
	}
}

void CRenderPostProcess::PresentCopy( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName from, const GpuApi::Rect& sourceRect, const GpuApi::Rect& targetRect )
{
	Rect sourceArea;
	sourceArea.m_left = sourceRect.left;
	sourceArea.m_top = sourceRect.top;
	sourceArea.m_right = sourceRect.right;
	sourceArea.m_bottom = sourceRect.bottom;

	Rect targetArea;
	targetArea.m_left = targetRect.left;
	targetArea.m_top = targetRect.top;
	targetArea.m_right = targetRect.right;
	targetArea.m_bottom = targetRect.bottom;

	GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( from ), sourceArea, GpuApi::GetBackBufferTexture(), targetArea );
}

void CRenderPostProcess::ProcessTonemappingHelpers( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Uint32 colorTargetWidth, Uint32 colorTargetHeight, ERenderTargetName rtnColor, ERenderTargetName rtnColorTemp, ERenderTargetName rtnMainColor )
{
	ASSERT( rtnColor != rtnColorTemp );
	ASSERT( rtnMainColor != rtnColorTemp );
	ASSERT( rtnMainColor != rtnColor );

	const Bool adaptation_disabled    = GIsRendererTakingUberScreenshot || info.m_baseLightingParameters.m_hdrAdaptationDisabled;

	ERenderTargetName rtAverageRead = RTN_LuminanceSimpleAPing;

	if ( !info.m_envParametersGame.m_displaySettings.m_disableTonemapping )
	{
		m_lumSurfaceSimple = AdvanceLuminanceRenderTargetSimple( m_lumSurfaceSimple );
		rtAverageRead = GetCorrespondingRenderTargetForRead( m_lumSurfaceSimple );

		const Float prev_frame_time		= GetRenderer()->GetLastTickDelta();
		const Float clamped_frame_time	= Clamp( prev_frame_time, 1.f / 250.f, 1.f / 5.f );

		// Time based adaptation
		const Float adaptationValueUp   = 1.f - exp( -( clamped_frame_time * info.m_envParametersDayPoint.m_toneMappingAdaptationSpeedUp * 0.5f ) );
		const Float adaptationValueDown = 1.f - exp( -( clamped_frame_time * info.m_envParametersDayPoint.m_toneMappingAdaptationSpeedDown * 0.5f ) );

		const Bool instant_adaptation    = info.m_instantAdaptation || prev_frame_time < 0 || Config::cvForceInstantAdaptation.Get() || surfaces->IsPersistentSurfaceDirty( CRenderSurfaces::PS_Luminance );
		
		ERenderTargetName tempLum;

		{
			tempLum = GetCorrespondingRenderTargetForRead( rtAverageRead );
			CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

			// Bind target, 8bit
			{
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor) ); // decoy
				rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
				GpuApi::SetupRenderTargets( rtSetup );
			}

			// Clear histogram - before it's even bound
			{
				Uint32 values[4] = { 0, 0, 0, 0 };
				GetRenderer()->ClearBufferUAV_Uint(m_histogramBuffer, values);
			}

			// Bind some global constants
			GpuApi::BindConstantBuffer( 13, GetRenderer()->GetTileDeferredConstantsBuffer(), GpuApi::ComputeShader );
			GetRenderer()->BindSharedConstants( GpuApi::ComputeShader );

			// Setup custom constants
			{
				struct SConstantBuffer
				{
					Vector histogramParams;		
					Vector fullScreenSize;
					Vector skyLumParams;
				};

				const Float rejectThreshold = info.m_envParametersArea.m_toneMapping.m_rejectThreshold.GetScalarClamp( 0.f, 1.f );
				const Float rejectExtent    = info.m_envParametersArea.m_toneMapping.m_rejectSmoothExtent.GetScalarClampMin( 0.f );
				const Float rejectThresholdMin = Max( 0.f, rejectThreshold - rejectExtent );
				const Float rejectThresholdMax = Min( 1.f, rejectThreshold + rejectExtent );

				SConstantBuffer constBuffer;
				constBuffer.histogramParams = Vector ( (Float)colorTargetWidth, (Float)colorTargetHeight, rejectThresholdMin, rejectThresholdMax );
				constBuffer.fullScreenSize = Vector ( (Float)info.m_width, (Float)info.m_height, 0, 0 );
				{
					const Uint32 depthSizeMul = colorTargetWidth > 0 ? info.m_width / colorTargetWidth : 1;
					ASSERT( 0 == colorTargetHeight || depthSizeMul == info.m_height / colorTargetHeight );
					ASSERT( info.m_width - info.m_width % depthSizeMul == depthSizeMul * colorTargetWidth );
					ASSERT( info.m_height - info.m_height % depthSizeMul == depthSizeMul * colorTargetHeight );
					constBuffer.skyLumParams = Vector ( info.m_envParametersArea.m_toneMapping.m_skyLuminanceCustomValue.GetScalarClampMin(0.f), info.m_envParametersArea.m_toneMapping.m_skyLuminanceCustomAmount.GetScalarClamp(0.f, 1.f), (Float)depthSizeMul, (info.m_camera.IsReversedProjection() ? 0.f : 1.f) );
				}
				GpuApi::SetComputeShaderConsts( constBuffer );
			}

			const Bool showDebugHistogram = info.IsShowFlagOn( SHOW_Histogram );

			// Prepare histogram
			{
				GpuApi::BindBufferUAV( m_histogramBuffer, 0 );
				GpuApi::TextureRef textures[] = { surfaces->GetRenderTargetTex( rtnColor ), surfaces->GetDepthBufferTex() };
				GpuApi::BindTextures( 0, ARRAY_COUNT(textures), textures, GpuApi::ComputeShader );

				GetRenderer()->m_shaderHistogramPrepare->Dispatch( colorTargetHeight, 1, 1 );

				GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );
				GpuApi::BindTextures( 0, ARRAY_COUNT(textures), nullptr, GpuApi::ComputeShader );
			}			

			// Obtain key luminance from the histogram
			{
				if ( showDebugHistogram )
				{
					const GpuApi::TextureRef debugOutput = surfaces->GetRenderTargetTex( rtnMainColor );
					GpuApi::BindTextureUAVs( 2, 1, &debugOutput );
				}

				GpuApi::BindBufferUAV( m_histogramBuffer, 0 );
				GpuApi::TextureRef uavRef = surfaces->GetRenderTargetTex( tempLum );
				GpuApi::BindTextureUAVs( 1, 1, &uavRef );

				(showDebugHistogram ? GetRenderer()->m_shaderHistogramDebug : GetRenderer()->m_shaderHistogramGather)->Dispatch( 1, 1, 1 );

				GpuApi::BindBufferUAV( GpuApi::BufferRef::Null(), 0 );
				GpuApi::BindTextureUAVs( 1, 1, nullptr );

				if ( showDebugHistogram )
				{
					GpuApi::BindTextureUAVs( 2, 1, nullptr );
				}
			}

			// Unbind stuff
			GetRenderer()->UnbindSharedConstants( GpuApi::ComputeShader );
			GpuApi::BindConstantBuffer( 13, GpuApi::BufferRef::Null(), GpuApi::ComputeShader );
		}

		if ( !adaptation_disabled )
		{
			// Bind target
			{
				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( m_lumSurfaceSimple ) );
				rtSetup.SetViewport( 1, 1, 0, 0 );
				GpuApi::SetupRenderTargets( rtSetup );
			}

			PC_SCOPE_RENDER_LVL1( AdaptLuminance ); 

			// Process
			m_drawer->GetEffects().fxToneMapping->AdaptLuminancesSimple( *m_drawer, surfaces, tempLum, instant_adaptation ? tempLum : rtAverageRead, adaptationValueUp, adaptationValueDown );
		}

	}

	if ( info.m_envParametersGame.m_displaySettings.m_disableTonemapping )
	{
		// Clear whole target surface
		const Vector clearValue( 0.5f, 0.5f, 0.5f, 0.5f );
		GetRenderer()->ClearColorTarget( surfaces->GetRenderTargetTex( RTN_LuminanceSimpleFinal ), clearValue );
	}
	else if ( adaptation_disabled )
	{
		Rect area;
		area.m_left = 0;
		area.m_top = 0;
		area.m_right = 1;
		area.m_bottom = 1;
		GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( rtAverageRead ), area, surfaces->GetRenderTargetTex( RTN_LuminanceSimpleFinal ), area );
		surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_Luminance, false );
	}
	else
	{
		Rect area;
		area.m_left = 0;
		area.m_top = 0;
		area.m_right = 1;
		area.m_bottom = 1;
		GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( m_lumSurfaceSimple ), area, surfaces->GetRenderTargetTex( RTN_LuminanceSimpleFinal ), area );
		surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_Luminance, false );
	}
}

Bool CRenderPostProcess::IsDebugOverlayActivated( const CRenderFrameInfo& info, Bool allowZoom ) const
{
	return CPostFxDebug::IsActivated( info, allowZoom );
}

void CRenderPostProcess::DrawDebugOverlay( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Bool allowZoom )
{
	if ( !IsDebugOverlayActivated( info, allowZoom ) )
	{
		return;
	}
	
	// Build RenderTarget setup
	GpuApi::RenderTargetSetup rtSetup;
	const ERenderTargetName rtnOutputColor = RTN_FinalColor;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnOutputColor ) );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );

	// Build feedback rtSetup
	const ERenderTargetName rtnFeedback = RTN_Color;
	COMPILE_ASSERT( rtnFeedback != rtnOutputColor );
	GpuApi::RenderTargetSetup rtSetupFeedback;
	rtSetupFeedback.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnFeedback ) );
	rtSetupFeedback.SetViewport( info.m_width, info.m_height, 0, 0 );

	// Build feedback2 rtSetup
	const ERenderTargetName rtnFeedback2 = RTN_Color2;
	COMPILE_ASSERT( rtnFeedback2 != rtnOutputColor );
	COMPILE_ASSERT( rtnFeedback2 != rtnFeedback );
	GpuApi::RenderTargetSetup rtSetupFeedback2;
	rtSetupFeedback2.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnFeedback2 ) );
	rtSetupFeedback2.SetViewport( info.m_width, info.m_height, 0, 0 );

	// Draw
	const Float gammaToLinear = 2.2f;
	const Float linearToGamma = 1.f / gammaToLinear;
	const EEnvManagerModifier displayMode = (EEnvManagerModifier)info.m_envParametersGame.m_displaySettings.m_displayMode;
	switch ( displayMode )
	{
	case EMM_None:				//< original image zooming
	case EMM_LinearAll:			//< actual tonemapping reparametrization handled elsewhere (handle zoom here)
	case EMM_Bloom:				//< actual bloom display handled elsewhere (handle zoom here)
	case EMM_DecomposedAmbient:
	case EMM_DecomposedDiffuse:
	case EMM_DecomposedSpecular:
	case EMM_DecomposedReflection:
		{
			ERenderTargetName rtnTemp = RTN_GBuffer0;
			
			// Copy finalbuffer
			ASSERT ( rtnTemp != rtnOutputColor );
			ASSERT( surfaces->GetRenderTarget(rtnTemp)->GetFormat() == surfaces->GetRenderTarget(rtnOutputColor)->GetFormat() );

			if ( !info.IsShowFlagOn( SHOW_Wireframe ) )
			{
				GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( rtnOutputColor ), surfaces->GetRenderTargetTex( rtnTemp ) );
			}

			// Apply debug preview (this gives us zoom)
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( rtnTemp ), 0, 1, 2, 1.f, allowZoom ); //< already in gamma space
		}
		break;

	case EMM_LocalReflections:
		{
			const Bool isMainRender = collector.IsWorldScene();
			if ( isMainRender )
			{
				if ( info.IsShowFlagOn( SHOW_LocalReflections ) )
				{
					GpuApi::SetupRenderTargets( rtSetup );
					m_drawer->GetEffects().fxDebug->ApplyDecodeLocalReflection( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_RLRResultHistory ), linearToGamma, allowZoom );
				}
				else
				{
					CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );
					for ( Uint32 i=0; i<rtSetup.numColorTargets; ++i )
					{
						ASSERT( -1 == rtSetup.colorTargetsSlices[i] );
						GetRenderer()->ClearColorTarget( rtSetup.colorTargets[i], Vector::ZEROS );
					}
				}
			}
		}
		break;

	case EMM_DecomposedLightingAmbient:
		{
			GpuApi::SetupRenderTargets( rtSetupFeedback );
			m_drawer->GetEffects().fxDebug->ApplyLightingAmbient( info, *m_drawer, false, 1.f );

			GpuApi::SetupRenderTargets( rtSetupFeedback2 );
			m_drawer->GetEffects().fxToneMapping->ApplySimple( *m_drawer, surfaces, false, rtnFeedback, RTN_LuminanceSimpleFinal, info );

			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( rtnFeedback2 ), 0, 1, 2, linearToGamma, allowZoom );
		}
		break;

	case EMM_DecomposedLightingReflection:
		{
			GpuApi::SetupRenderTargets( rtSetupFeedback );
			m_drawer->GetEffects().fxDebug->ApplyLightingReflection( info, *m_drawer, false, 1.f );

			GpuApi::SetupRenderTargets( rtSetupFeedback2 );
			m_drawer->GetEffects().fxToneMapping->ApplySimple( *m_drawer, surfaces, false, rtnFeedback, RTN_LuminanceSimpleFinal, info );

			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( rtnFeedback2 ), 0, 1, 2, linearToGamma, allowZoom );
		}
		break;

	case EMM_GBuffAlbedo:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), 0, 1, 2, 1.f, allowZoom ); //< already in gamma space
		break;

	case EMM_GBuffAlbedoNormalized:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyNormalizedAlbedo( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), surfaces->GetRenderTargetTex( RTN_GBuffer2 ), 1.f, allowZoom ); //< already in gamma space
		break;

	case EMM_Depth:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyShowDepth( info, *m_drawer, allowZoom );
		break;

	case EMM_GBuffNormalsWorldSpace:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeNormals( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer1 ), linearToGamma, allowZoom, false );
		break;

	case EMM_GBuffNormalsViewSpace:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeNormals( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer1 ), linearToGamma, allowZoom, true );
		break;

	case EMM_GBuffSpecularity:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer2 ), 0, 1, 2, 1.f, allowZoom ); //< already in gamma space
		break;

	case EMM_GBuffRoughness:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeChannel( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer1 ), 3, 1.f, allowZoom );	//< display linear values (input roughness is also linear), so this is for the artists to be able to readback values easily
		break;

	case EMM_GBuffTranslucency:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyDecodeChannel( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), 3, linearToGamma, allowZoom );
		break;

	case EMM_DimmersSurface:	// falldown
	case EMM_DimmersVolume:
		{
			const Bool isVolumeDisplay = (EMM_DimmersVolume == displayMode);
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDimmers( info, *m_drawer, linearToGamma, allowZoom, isVolumeDisplay );
		}
		break;

	case EMM_ComplexityEnvProbes:
		{
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyComplexityEnvProbes( info, *m_drawer, allowZoom );
		}
		break;

	case EMM_MaskShadow:
		{
			const Int32 channel = GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW;
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ), channel, channel, channel, linearToGamma, allowZoom );
		}
		break;

	case EMM_MaskSSAO:
		{
			const Int32 channel = GLOBAL_SHADOW_BUFFER_CHANNEL_SSAO;
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ), channel, channel, channel, linearToGamma, allowZoom );
		}
		break;

	case EMM_MaskInterior:
		{
			const Int32 channel = GLOBAL_SHADOW_BUFFER_CHANNEL_INTERIOR_FACTOR;
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ), channel, channel, channel, linearToGamma, allowZoom );
		}
		break;

	case EMM_MaskDimmers:
		{
			const Int32 channel = GLOBAL_SHADOW_BUFFER_CHANNEL_DIMMERS;
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyDecodeColor( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ), channel, channel, channel, linearToGamma, allowZoom );
		}
		break;

	case EMM_InteriorsVolume: // falldown
	case EMM_InteriorsFactor:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyShowVolumeRendering( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), collector, EMM_InteriorsFactor==displayMode );
		break;

	case EMM_StencilMix:
		{
			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyShowStencilBits( info, *m_drawer, surfaces->GetDepthBufferTex(), 0xff, allowZoom );
		}
		break;

	case EMM_Stencil0:
	case EMM_Stencil1:
	case EMM_Stencil2:
	case EMM_Stencil3:
	case EMM_Stencil4:
	case EMM_Stencil5:
	case EMM_Stencil6:
	case EMM_Stencil7:
		{
			const Uint32 stencilBitIndex = (Uint32)displayMode - EMM_Stencil0;

			GpuApi::SetupRenderTargets( rtSetup );
			m_drawer->GetEffects().fxDebug->ApplyShowStencilBits( info, *m_drawer, surfaces->GetDepthBufferTex(), (1 << stencilBitIndex), allowZoom );
		}
		break;
	case EMM_LightsOverlay:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyLightsOverlay( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), 1.f, allowZoom );
		break;
	case EMM_LightsOverlayDensity:
		GpuApi::SetupRenderTargets( rtSetup );
		m_drawer->GetEffects().fxDebug->ApplyLightsOverlayDensity( info, *m_drawer, surfaces->GetRenderTargetTex( RTN_GBuffer0 ), 1.f, allowZoom );
		break;
	
	default:
		ASSERT ( !"Invalid display mode" );
	}
}

Bool CRenderPostProcess::ResolveMSAAStencil( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, Uint8 stencilBits )
{
	if ( !GetRenderer()->IsMSAAEnabled( info ) )
	{
		return false;
	}

	if ( !stencilBits )
	{
		return false;
	}

	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	GetRenderer()->ClearStencilTarget( 0 );

	for ( Uint32 bitIndex=0; bitIndex<8; ++bitIndex )
	{
		const Uint8 currStencilMask = 1 << bitIndex;
		if ( !(stencilBits & currStencilMask) )
		{
			continue;
		}

		// Render stencil values to color buffer
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_MSAAColor ) );
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTexMSAA(), -1, true );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			GetRenderer()->ClearColorTarget( Vector::ZEROS );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet_StencilMatchAny, currStencilMask );
			GetRenderer()->GetDebugDrawer().DrawQuad2DEx( 0, 0, info.m_width, info.m_height, Color::WHITE );
		}

		// Update non-msaa depth buffer stencil bit
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcNoColor_SetStencilBits, currStencilMask );

			GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( RTN_MSAAColor );
			GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );
			m_drawer->DrawQuad( GetRenderer()->m_shaderResolveStencil_MSAA );
			GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
		}
	}

	return true;
}

Bool CRenderPostProcess::ResolveMSAADepth( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces )
{
	if ( !GetRenderer()->IsMSAAEnabled( info ) )
	{
		return false;
	}

	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	GpuApi::RenderTargetSetup rtSetupResolve;
	rtSetupResolve.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
	rtSetupResolve.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupResolve );

	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet_DepthWrite );

	GpuApi::TextureRef colorRef = surfaces->GetDepthBufferTexMSAA();
	GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );
	m_drawer->DrawQuad( GetRenderer()->m_shaderResolveDepth_MSAA );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	return true;
}

Bool CRenderPostProcess::ResolveMSAABufferSimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnSampler, ERenderTargetName rtnTarget )
{
	if ( rtnSampler == rtnTarget || !(!surfaces->GetRenderTarget(rtnSampler)->IsMultisampled() && !surfaces->GetRenderTarget(rtnTarget)->IsMultisampled()) )
	{
		return false;
	}

	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	GpuApi::RenderTargetSetup rtSetupResolve;
	rtSetupResolve.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnTarget ) );
	rtSetupResolve.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupResolve );

	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );		

	GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( rtnSampler );
	GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );
	m_drawer->DrawQuad( GetRenderer()->m_shaderResolveColor_SingleMSAABuffered );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	return true;
}

Bool CRenderPostProcess::ResolveMSAAColorSimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnTarget )
{
	return ResolveMSAASimple( info, surfaces, RTN_MSAAColor, rtnTarget );
}

Bool CRenderPostProcess::ResolveMSAASimple( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName rtnSampler, ERenderTargetName rtnTarget )
{
	if ( rtnSampler == rtnTarget || !(surfaces->GetRenderTarget(rtnSampler)->IsMultisampled() && !surfaces->GetRenderTarget(rtnTarget)->IsMultisampled()) )
	{
		return false;
	}
	
	CGpuApiDeviceStateGrabber targetStateGrabber ( GpuApi::DEVSTATECAT_RenderTargetSetup );

	GpuApi::RenderTargetSetup rtSetupResolve;
	rtSetupResolve.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnTarget ) );
	rtSetupResolve.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetupResolve );

	CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );		


	GpuApi::TextureRef colorRef = surfaces->GetRenderTargetTex( rtnSampler );
	GpuApi::BindTextures( 0, 1, &colorRef, GpuApi::PixelShader );
	m_drawer->DrawQuad( GetRenderer()->m_shaderResolveColor_SingleMSAA );
	GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );

	return true;
}

static void RenderEnvProbeHelpers( const CRenderFrameInfo &info, ERenderTargetName rtnColor, bool displayBig, bool displayCompact )
{
	if ( !displayBig && !displayCompact )
	{
		return;
	}

	CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();

	// Setup targets
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColor ) );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	// Setup camera
	GetRenderer()->GetStateManager().SetCamera2D();

	// Bind constants
	GetRenderer()->BindForwardConsts( info, GetRenderer()->GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::PixelShader );
	GetRenderer()->BindForwardConsts( info, GetRenderer()->GetGlobalCascadesShadowResources(), surfaces, true, GpuApi::VertexShader );

	// 
	CGpuApiScopedDrawContext scopedDrawContext ( GpuApi::DRAWCONTEXT_PostProcSet );
	GetRenderer()->m_shaderEnvProbe->Bind();

	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, info.m_camera.GetPosition() );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, info.m_camera.GetWorldToView().GetAxisX() );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, info.m_camera.GetWorldToView().GetAxisY() );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, info.m_camera.GetWorldToView().GetAxisZ() );

	// Draw compact probes
	if ( displayCompact )
	{
		const Float sphereRadius = 50.f;
		const Float microSphereRadius = 35.f;
		const Float marginRight = 8.f;
		const Float marginBottom = 14.f;
		const Float marginMicro = 10.f;
		const Float microGap = 5.f;
		const Float gap = 10.f;

		const Float col_gap = sphereRadius * 2 + gap;
		const Float col_x = info.m_width - marginRight - sphereRadius;
		const Float row_y = info.m_height - marginBottom - sphereRadius;
		const Float micro_col_gap = microSphereRadius * 2 + microGap;
		const Float micro_col_x = info.m_width - marginRight - microSphereRadius;
		const Float micro_row_y = row_y - microSphereRadius - marginMicro - sphereRadius;

		Matrix localToWorld;
		localToWorld.SetIdentity();
		localToWorld.SetScale33( Vector ( sphereRadius, sphereRadius, 0.f, 1.f ) );

		Matrix microLocalToWorld;
		microLocalToWorld.SetIdentity();
		microLocalToWorld.SetScale33( Vector ( microSphereRadius, microSphereRadius, 0.f, 1.f ) );

		// Ambient - base mip
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( col_x - col_gap, row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 0, 1 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		// Ambient - second mip
		COMPILE_ASSERT( 2 == CRenderEnvProbeManager::AMBIENT_CUBE_NUM_MIPS );
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( col_x - col_gap * 2, row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 1, 1 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		// Reflection - base mip
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( col_x, row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 0, 2 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		// Micro reflectio probes
		for ( Uint32 i=0; i<CRenderEnvProbeManager::REFLECTION_CUBE_NUM_MIPS; ++i )
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &microLocalToWorld.SetTranslation( micro_col_x - i * micro_col_gap, micro_row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, (Float)i, 2 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}
	}

	// Draw big probes
	if ( displayBig )
	{
		const Float sphereRadius = 110.f;
		const Float marginLeft = 8.f;
		const Float marginBottom = 14.f;
		const Float gap = 20.f;

		const Float col_gap = sphereRadius * 2 + gap;
		const Float col_x = marginLeft + sphereRadius;
		const Float row_y = info.m_height - marginBottom - sphereRadius;
		
		Matrix localToWorld;
		localToWorld.SetIdentity();
		localToWorld.SetScale33( Vector ( sphereRadius, sphereRadius, 0.f, 1.f ) );

		// Ambient - base mip
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( col_x, row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 0, 1 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}		

		// Reflection - base mip
		{
			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( col_x + col_gap, row_y, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( 0, 0, 0, 2 ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( 1, 1, 1, 1 ) );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}
	}

	// Unbind constants
	GetRenderer()->UnbindForwardConsts( info, GpuApi::PixelShader );
	GetRenderer()->UnbindForwardConsts( info, GpuApi::VertexShader );

}

static void RenderDebugCameraInteriorFactor( const CRenderCollector &collector, ERenderTargetName rtnColor )
{
	if ( !collector.m_scene )
	{
		return;
	}

	const CRenderFrameInfo &info = collector.GetRenderFrameInfo();

	const Vector gameplayScaleColorBack = Vector::ONES * Clamp( 1.f - info.m_gameplayCameraLightsFactor, 0.f, 1.f );
	const Vector gameplayScaleColorFront = Vector::ONES * Clamp( info.m_gameplayCameraLightsFactor, 0.f, 1.f );
	const Vector interiorColorBack  = Vector::ONES * Clamp( collector.m_scene->GetDelayedCameraInteriorFactor(), 0.f, 1.f );
	const Vector interiorColorFront = Vector::ONES * Clamp( 1.f - collector.m_scene->GetDelayedCameraInteriorFactor(), 0.f, 1.f );
	const Vector scenesColorBack  = Vector::ONES * Clamp( 1.f - collector.GetRenderFrameInfo().m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor, 0.f, 1.f );
	const Vector scenesColorFront = Vector::ONES * Clamp( collector.GetRenderFrameInfo().m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor, 0.f, 1.f );

	CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();
	
	// Setup targets
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColor ) );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	// Setup camera
	GetRenderer()->GetStateManager().SetCamera2D();

	// 
	CGpuApiScopedDrawContext scopedDrawContext ( GpuApi::DRAWCONTEXT_PostProcSet );
	GetRenderer()->m_shaderSingleColor->Bind();
	
	// Render interior visualiser
	{
		const Float sphereRadiusBack  = 28.f;
		const Float sphereRadiusFront = 25.f;
		const Float marginBottom = 10.f;
		const Float centerX = info.m_width / 2.f;
		const Float centerY = info.m_height - marginBottom - sphereRadiusBack;

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusBack, sphereRadiusBack, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, interiorColorBack );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusFront, sphereRadiusFront, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, interiorColorFront );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}
	}

	// Render scenes visualiser
	{
		const Float sphereRadiusBack  = 18.f;
		const Float sphereRadiusFront = 15.f;
		const Float marginBottom = 10.f;
		const Float centerX = info.m_width / 2.f + 50.f;
		const Float centerY = info.m_height - marginBottom - sphereRadiusBack;

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusBack, sphereRadiusBack, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, scenesColorBack );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusFront, sphereRadiusFront, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, scenesColorFront );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}
	}

	// Render gameplay camera lights factor visualiser
	{
		const Float sphereRadiusBack  = 18.f;
		const Float sphereRadiusFront = 15.f;
		const Float marginBottom = 10.f;
		const Float centerX = info.m_width / 2.f - 50.f;
		const Float centerY = info.m_height - marginBottom - sphereRadiusBack;

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusBack, sphereRadiusBack, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, gameplayScaleColorBack );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}

		{
			Matrix localToWorld;
			localToWorld.SetIdentity();
			localToWorld.SetScale33( Vector ( sphereRadiusFront, sphereRadiusFront, 0.f, 1.f ) );

			GetRenderer()->GetStateManager().SetLocalToWorld( &localToWorld.SetTranslation( centerX, centerY, 0.5f ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_ConstColor, gameplayScaleColorFront );
			GetRenderer()->GetDebugDrawer().DrawUnitSphere();
		}
	}
}

static void RenderTemporalAALuminance( const CRenderFrameInfo& info, const GpuApi::TextureRef &texColor, const GpuApi::TextureRef &texLuminance )
{
	CRenderSurfaces *surfaces = GetRenderer()->GetSurfaces();

	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, texLuminance );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	const GpuApi::TextureRef tex[] = { texColor };
	GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

	CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
	GetRenderer()->m_postFXTemporalAALum->Bind();

	GpuApi::MultiGPU_BeginEarlyPushTexture( texLuminance );
	GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
	GpuApi::MultiGPU_EndEarlyPushTexture( texLuminance );

	GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
}

struct SBloomConfig
{
	SBloomConfig ()
	{
		// empty - trash !
	}

	SBloomConfig ( const CRenderFrameInfo &info )
	{
		Init( info );
	}

	void Init( const CRenderFrameInfo &info )
	{
		const Int32 fullWidth	= info.m_width;
		const Int32 fullHeight	= info.m_height;

		requestedSkipHiLevels = info.m_worldRenderSettings.m_bloomLevelsOffset;
		requestedNumLevels = Min<Uint32>( 7, info.m_worldRenderSettings.m_bloomLevelsRange ) + 1; // don't go too high in here because we may run out of texture space. also there is some overhead because bloom area gets bigger with each level.

		bloomMaxLevels = MLog2( Min( fullWidth, fullHeight ) );
		bloomLevels = Min( requestedNumLevels, bloomMaxLevels );
		bloomSizeSnap = (1 << (bloomLevels - 1));
		bloomFullWidth = (fullWidth + bloomSizeSnap - 1) & ~(bloomSizeSnap-1);
		bloomFullHeight = (fullHeight + bloomSizeSnap - 1) & ~(bloomSizeSnap-1);
		bloomOffsetX = ((Int32)fullWidth - (Int32)bloomFullWidth) / 2;
		bloomOffsetY = ((Int32)fullHeight - (Int32)bloomFullHeight) / 2;
		precisionScale = Max( 0.001f, info.m_worldRenderSettings.m_bloomPrecision );
		ASSERT( bloomFullWidth > 0 );
		ASSERT( bloomFullHeight > 0 );
		shaftsBrightIndex = info.m_worldRenderSettings.m_shaftsLevelIndex;

		sizeTestFullWidth = fullWidth;
		sizeTestFullHeight = fullHeight;
		sizeTestHalfWidth = sizeTestFullWidth / 2;
		sizeTestHalfHeight = sizeTestFullHeight / 2;
	}

	Uint32 requestedSkipHiLevels;
	Uint32 requestedNumLevels;
	Uint32 bloomMaxLevels;
	Uint32 bloomLevels;
	Uint32 bloomSizeSnap;
	Uint32 bloomFullWidth;
	Uint32 bloomFullHeight;
	Int32 bloomOffsetX;
	Int32 bloomOffsetY;
	Float precisionScale;
	Uint32 sizeTestFullWidth;
	Uint32 sizeTestFullHeight;
	Uint32 sizeTestHalfWidth;
	Uint32 sizeTestHalfHeight;
	Uint32 shaftsBrightIndex;
};

struct SBrightInfo
{
	SBrightInfo ()
	{}

	SBrightInfo ( ERenderTargetName newTarget, Int32 newOffsetX, Uint32 newWidth, Uint32 newHeight )
		: rtnBrightTarget ( newTarget )
		, offsetX ( newOffsetX )
		, width ( newWidth )
		, height ( newHeight )
	{}

	ERenderTargetName rtnBrightTarget;
	Int32 offsetX;
	Uint32 width;
	Uint32 height;
};

Uint32 BloomDownsample( const CRenderFrameInfo &info, const SBloomConfig &bloomConfig, Uint32 brightInfoCapacity, SBrightInfo *brightInfo, Float *outShaftsLevelInvScale, ERenderTargetName rtnColorSource, ERenderTargetName rtnColorTarget, ERenderTargetName rtnColorTempHalf1, Bool performRender )
{
	const Int32 fullWidth	= info.m_width;
	const Int32 fullHeight	= info.m_height;

	CRenderSurfaces * const surfaces = GetRenderer()->GetSurfaces();
	const CEnvBloomNewParametersAtPoint &bloomParams = info.m_envParametersArea.m_bloomNew;

	if ( outShaftsLevelInvScale )
	{
		*outShaftsLevelInvScale = -1.f;
	}

	Uint32 brightCount = 0;

	//
	if ( brightInfoCapacity <= 0 )
	{
		return 0;
	}

	// brightpass
	{	
		Uint32 levelWidth = bloomConfig.bloomFullWidth / 2;
		Uint32 levelHeight = bloomConfig.bloomFullHeight / 2;
		if ( levelWidth <= bloomConfig.sizeTestFullWidth && levelHeight <= bloomConfig.sizeTestFullHeight )
		{
			if ( performRender )
			{
				PC_SCOPE_RENDER_LVL1( BloomBrightPass );

				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
				rtSetup.SetViewport( levelWidth, levelHeight, 0, 0 );
				GpuApi::SetupRenderTargets( rtSetup );

				const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetDepthBufferTex() };
				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

				const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnColorSource ), 0 );
				const GpuApi::TextureLevelDesc targetDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnColorTarget ), 0 );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sourceDesc.width, (Float)sourceDesc.height, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)levelWidth * 2, (Float)levelHeight * 2, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)targetDesc.width, (Float)targetDesc.height, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)levelWidth, (Float)levelHeight, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( (Float)bloomConfig.bloomOffsetX, (Float)bloomConfig.bloomOffsetY, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( 0, 0, (Float)(fullWidth - 1), (Float)(fullHeight - 1) ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector ( bloomConfig.precisionScale, 0.f, 0.f, 0.f ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, Vector ( bloomParams.m_brightnessMax.GetScalarClampMin(0), 0, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_8, Vector ( bloomParams.m_threshold.GetScalarClampMin(0), 1.f / bloomParams.m_thresholdRange.GetScalarClampMin(0.001f), 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_9, bloomParams.m_brightPassWeights.GetColorGammaToLinear( true ) );

				CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
				GetRenderer()->m_postfxBloomBrightpass->Bind();
				GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			}

			brightInfo[brightCount++] = SBrightInfo( rtnColorTarget, 0, levelWidth, levelHeight );
		}
	}

	// downsample brightness
	Float shaftsLevelInvScale = 1.f;
	if ( 1 == brightCount )
	{
		for ( Uint32 level_i=2; level_i<bloomConfig.bloomLevels && brightCount < brightInfoCapacity; ++level_i )
		{
			const Uint32 levelWidth = bloomConfig.bloomFullWidth / (1 << level_i);
			const Uint32 levelHeight = bloomConfig.bloomFullHeight / (1 << level_i);

			ERenderTargetName rtnBrightness = brightInfo[brightCount-1].rtnBrightTarget;
			ERenderTargetName rtnBrightnessFeedback = rtnBrightness == rtnColorTarget ? rtnColorTempHalf1 : rtnColorTarget;
			Int32 sourceOffX = brightInfo[brightCount-1].offsetX;
			Int32 targetOffX = level_i == 2 ? 0 : ( brightInfo[brightCount-2].offsetX + brightInfo[brightCount-2].width );

			if ( rtnColorTarget == rtnBrightnessFeedback )
			{
				if ( targetOffX + levelWidth > bloomConfig.sizeTestFullWidth || 0 + levelHeight > bloomConfig.sizeTestFullHeight )
				{
					break;
				}
			}
			else
			{
				RED_ASSERT( rtnColorTempHalf1 == rtnBrightnessFeedback );
				if ( targetOffX + levelWidth > bloomConfig.sizeTestHalfWidth || 0 + levelHeight > bloomConfig.sizeTestHalfHeight )
				{
					break;
				}
			}

			const Float resultScale = 1.f / ( Max( 0.0001f, info.m_worldRenderSettings.m_bloomDownscaleDivBase ) * powf( Max( 0.0001f, info.m_worldRenderSettings.m_bloomDownscaleDivExp ), (Float)level_i ) );
			if ( performRender )
			{
				PC_SCOPE_RENDER_LVL1( BloomDownsample );

				GpuApi::RenderTargetSetup rtSetup;
				rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnBrightnessFeedback ) );
				rtSetup.SetViewport( levelWidth, levelHeight, targetOffX, 0 );
				GpuApi::SetupRenderTargets( rtSetup );

				const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( rtnBrightness ) };
				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
				GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

				const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnBrightness ), 0 );
				const GpuApi::TextureLevelDesc targetDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( rtnBrightnessFeedback ), 0 );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sourceDesc.width, (Float)sourceDesc.height, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)levelWidth * 2, (Float)levelHeight * 2, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)targetDesc.width, (Float)targetDesc.height, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)levelWidth, (Float)levelHeight, 0, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( (Float)sourceOffX, 0, (Float)targetOffX, 0 ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( (Float)sourceOffX, 0, (Float)(sourceOffX + levelWidth * 2 - 1), (Float)(levelHeight * 2 - 1) ) );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector ( resultScale, 0, 0, 0 ) );

				CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
				GetRenderer()->m_postfxBloomDownscale->Bind();
				GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

				GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
			}

			if ( brightCount == bloomConfig.shaftsBrightIndex )
			{
				shaftsLevelInvScale = 1.f / Max( 0.0001f, resultScale );
			}

			brightInfo[brightCount++] = SBrightInfo( rtnBrightnessFeedback, targetOffX, levelWidth, levelHeight );
		}
	}

	//
	if ( outShaftsLevelInvScale && bloomConfig.shaftsBrightIndex < brightCount )
	{
		*outShaftsLevelInvScale = shaftsLevelInvScale;
	}

	//
	return brightCount;
}

ERenderTargetName CRenderPostProcess::DrawFinal( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, EPostProcessCategoryFlags categoriesMask, bool isUbersampleFirst, bool isUbersampleLast )
{
	PC_SCOPE_RENDER_LVL0( PostProcess );

	// in case of thumbnails we won't have a viewport
	if ( !info.IsViewportPresent() && !collector.m_frame->GetRenderTarget() )
	{
		const ERenderTargetName resultTarget = RTN_Color;
		if ( GetRenderer()->IsMSAAEnabled( info ) )
		{
			ResolveMSAAColorSimple( info, surfaces, resultTarget );
		}

		return resultTarget;
	}

	GetRenderer()->BindSharedConstants( GpuApi::PixelShader );


	// TODO : Should we copy GBuffer0/2 out before we get to this point? They're only used for the debug overlay, so maybe don't need to...
	// Copy from RTN_Color to PPTarget1. PPTarget is in ESRAM, so we want to use that.
	// Alternatively, could just read from RTN_Color for the first one, and then switch to the PPTarget's.
	// For now, this copy is safe. RTN_Color is in ESRAM as well, but PPTarget1 doesn't overlap it.
	GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( RTN_Color ), surfaces->GetRenderTargetTex( RTN_PostProcessTarget1 ) );


	ERenderTargetName rtnColorSource		= RTN_PostProcessTarget1;
	ERenderTargetName rtnColorTarget		= RTN_PostProcessTarget2;
	ERenderTargetName rtnColorTemp			= RTN_PostProcessTempFull;
	ERenderTargetName rtnColorTempHalf1		= RTN_PostProcessTempHalf1;
	ERenderTargetName rtnColorTempHalf2		= RTN_PostProcessTempHalf2;
	ERenderTargetName rtnColorTempHalf3		= RTN_PostProcessTempHalf3;


	const Int32 fullWidth				= info.m_width;
	const Int32 fullHeight				= info.m_height;
	const Int32 fullWidthScalable		= (Int32) ( info.m_width  - info.m_width  % 4 );
	const Int32 fullHeightScalable		= (Int32) ( info.m_height - info.m_height % 4 );
	const Int32 halfWidthScalable		= fullWidthScalable  / 2;
	const Int32 halfHeightScalable		= fullHeightScalable / 2;	
	const Int32 quarterWidthScalable	= fullWidthScalable  / 4;
	const Int32 quarterHeightScalable	= fullHeightScalable  / 4;
	const Int32 octWidthScalable		= fullWidthScalable  / 8;
	const Int32 octHeightScalable		= fullHeightScalable / 8;

	GetRenderer()->GetStateManager().SetLocalToWorld( NULL );
	GetRenderer()->GetStateManager().SetCamera2D();

	// Set most commonly used draw context, to limit unnecessary context switches by draw context automatic objects
	// This is not needed, so consider this as optimization.

	const CGpuApiScopedDrawContext baseDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

	// Underwater effects
	if ( PPCF_Underwater & categoriesMask )
	{
		PC_SCOPE_RENDER_LVL1( RenderUnderwater );

		// TODO move to env params
		Float underwaterBlurIntensity = 1.2f;

		if( collector.m_scene != nullptr )
		{
			CRenderProxy_Water* waterProxy = collector.m_scene->GetWaterProxy();

			if( waterProxy != nullptr && waterProxy->ShouldRenderUnderwater() )
			{
				m_drawer->GetEffects().fxUnderwater->ApplyUnderwaterFx(
					*m_drawer,			surfaces,
					rtnColorSource,		TexelArea ( info.m_width, info.m_height ),
					rtnColorTarget,		TexelArea ( info.m_width, info.m_height ),
					waterProxy->GetUnderwaterIntersectionTexture(),			underwaterBlurIntensity,
					waterProxy->GetFurierTexture()
					);

				// Update curr color target info
				Swap( rtnColorSource, rtnColorTarget );
			}
		}
	}

	// Motion blur (in case non-postTonemapping option chosen)
	if ( !info.m_worldRenderSettings.m_motionBlurSettings.m_isPostTonemapping && (PPCF_MotionBlur & categoriesMask) 
#ifdef USE_ANSEL		
		&& !isAnselCaptureActive 
#endif // USE_ANSEL
		)
	{
		PC_SCOPE_RENDER_LVL1( MotionBlur );
		const Bool applied = m_drawer->GetEffects().fxMotionBlur->Apply(  *m_drawer, surfaces, rtnColorSource, rtnColorTarget, rtnColorTempHalf1, rtnColorTempHalf2, rtnColorTempHalf3, info );
#ifndef RED_FINAL_BUILD
		// Uncomment this if You want to see when motion blur is applied
		// collector.m_frame->AddDebugScreenFormatedText( 50, 200, applied ? Color( 0,255,0,255 ) : Color( 255,0,0,255 ) , TXT("MotionBlur: %d") , (Uint32)applied );
#endif
	}

	// Process tonemapping helpers
	if ( (PPCF_Tonemapping & categoriesMask) && !(info.m_tonemapFixedLumiance > 0) )
	{
		PC_SCOPE_RENDER_LVL1( ToneMappingHelpers );

		// Process helpers
		if ( isUbersampleFirst && 0 != (info.m_renderFeaturesFlags & DMFF_ToneMappingLuminanceUpdate) )
		{
			ERenderTargetName histogramTargetName = rtnColorSource;
			Int32 histogramTargetWidth = fullWidth;
			Int32 histogramTargetHeight = fullHeight;

			{
				// ace_optimize: do this with one pass

				const Uint32 downscaledWidth   = histogramTargetWidth / 2;
				const Uint32 downscaledHeight  = histogramTargetHeight / 2;
				const Uint32 downscaledWidth2  = downscaledWidth / 2;
				const Uint32 downscaledHeight2 = downscaledHeight / 2;

				GetRenderer()->StretchRect( surfaces->GetRenderTargetTex(histogramTargetName), Rect(0, histogramTargetWidth, 0, histogramTargetHeight),
					surfaces->GetRenderTargetTex(rtnColorTempHalf1), Rect( 0, downscaledWidth, 0, downscaledHeight ) );
				GetRenderer()->StretchRect( surfaces->GetRenderTargetTex(rtnColorTempHalf1), Rect(0, downscaledWidth, 0, downscaledHeight),
					surfaces->GetRenderTargetTex(rtnColorTempHalf2), Rect( 0, downscaledWidth2, 0, downscaledHeight2 ) );

				histogramTargetName = rtnColorTempHalf2;
				histogramTargetWidth = downscaledWidth2;
				histogramTargetHeight = downscaledHeight2;
			}

			ProcessTonemappingHelpers( info, surfaces, histogramTargetWidth, histogramTargetHeight, histogramTargetName, rtnColorTemp, rtnColorSource );
		}
	}

	// Render debug envprobe values for current camera position
	RenderEnvProbeHelpers( info, rtnColorSource, info.IsShowFlagOn( SHOW_EnvProbesBigOverlay ), info.IsShowFlagOn( SHOW_EnvProbesOverlay ) );

	// -----------------------------------------------------------
	// EPO_PRE_TONEMAPPING pass of post processes
	// -----------------------------------------------------------
	{
		GetRenderer()->GetGameplayFX()->Apply( EPO_PRE_TONEMAPPING , collector , info, rtnColorSource , rtnColorTarget );
		/*
		Here should be following effetcs:
			-	nothing
		*/
	}

	// Perform tonemapping
	if ( PPCF_Tonemapping & categoriesMask )
	{
		PC_SCOPE_RENDER_LVL1( ToneMappingApply ); 

		const Bool displayDebugCurve = info.IsShowFlagOn( SHOW_TonemappingCurve );

		// Setup targets
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		// Draw
		m_drawer->GetEffects().fxToneMapping->ApplySimple( *m_drawer, surfaces, displayDebugCurve, rtnColorSource, RTN_LuminanceSimpleFinal, info );
		Swap( rtnColorSource, rtnColorTarget );
	}

	Bool sunShaftActive = false;
	SFXFlareParameters shaftParamsSun;

	if ( categoriesMask & ( PPCF_DepthOfField | PPCF_CutsceneDOF ) )
	{
		PC_SCOPE_RENDER_LVL1( DOF ) 

		// Dof filter

		const CGameEnvironmentParams &gameParams = info.m_envParametersGame;
		
		if ( info.m_envParametersGame.m_displaySettings.m_allowDOF && info.m_envParametersArea.m_depthOfField.IsDofExposed() )
		{

			// Choose dof type
			bool cutsceneDof = gameParams.m_cutsceneDofMode && (PPCF_CutsceneDOF & categoriesMask);

			Float cutsceneDofBlendFactor = info.m_cameraLightsModifiersSetup.m_scenesSystemActiveFactor && !info.m_cameraLightsModifiersSetup.m_disableDof;

			// Bind target
			// Gameplay Dof draw final results into currently bound target.
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			// Process
			if ( cutsceneDof || cutsceneDofBlendFactor > 0.01f )
			{
				// Check if we need use cached para,meters instead actual ( they might be arleady reseted or overwriten, so blending does not work )
				Bool useCoherentParams	= ( cutsceneDofBlendFactor < 1.0 || !cutsceneDof );
				Bool useCoherentDof		= useCoherentParams && m_drawer->GetEffects().fxStateGrabber->HasDirtyMask( EPDM_DofParams );
				Bool useCoherentBokeh	= useCoherentParams && m_drawer->GetEffects().fxStateGrabber->HasDirtyMask( EPDM_BokehDofParams );
				
				const CEnvDepthOfFieldParametersAtPoint*	dofParams		= &info.m_envParametersArea.m_depthOfField;
				const SBokehDofParams*						bokehDofParams	= &info.m_envParametersDayPoint.m_bokehDofParams;

				if( useCoherentDof )
				{
					dofParams		= &m_drawer->GetEffects().fxStateGrabber->GetLastDofParams();
				}
				if( useCoherentBokeh )
				{
					bokehDofParams	= &m_drawer->GetEffects().fxStateGrabber->GetLastBokehDofParams();
				}

				if ( bokehDofParams->m_enabled )
				{
					PC_SCOPE_RENDER_LVL1( RenderBokehDof );
					
					m_drawer->GetEffects().fxBokehDof->ApplyBokehDoffx(
						*m_drawer,			
						surfaces,
						info.m_camera,		
						*bokehDofParams, *dofParams, cutsceneDofBlendFactor , 
						rtnColorSource,		TexelArea ( info.m_width,   info.m_height ),
						rtnColorTarget
						);
				}
				else
				{
					PC_SCOPE_RENDER_LVL1( DOF );

					// Can give rtnColorTarget as a helper here, because helper3 is not read from when writing the final output.
					m_drawer->GetEffects().fxDOF->ApplyCutsceneDof(
						*m_drawer,			surfaces,   *dofParams, cutsceneDofBlendFactor,
						rtnColorSource,		TexelArea ( info.m_width,   info.m_height   ),
						rtnColorTempHalf1,	TexelArea ( info.m_width/2, info.m_height/2 ),
						rtnColorTempHalf2,	TexelArea ( info.m_width/2, info.m_height/2 ),
						rtnColorTarget,		TexelArea ( info.m_width,	info.m_height   ),
						TexelArea ( info.m_width,   info.m_height   ) );

				}

				m_drawer->GetEffects().fxStateGrabber->SetLastDofParams( *dofParams );
				m_drawer->GetEffects().fxStateGrabber->SetLastBokehDofParams( *bokehDofParams );

				// Update curr color target info
				Swap( rtnColorSource, rtnColorTarget );

			}

			if ( ( PPCF_DepthOfField & categoriesMask ) && ( cutsceneDofBlendFactor < 1.0f ) )
			{
				ERenderTargetName rtnHelper = RTN_Color2;
			#ifdef RED_PLATFORM_DURANGO
				switch ( rtnColorSource )
				{
				case RTN_PostProcessTarget1:		rtnHelper = RTN_DURANGO_PostProcessTarget2_R10G10B10_6E4_A2_FLOAT; break;
				case RTN_PostProcessTarget2:		rtnHelper = RTN_DURANGO_PostProcessTarget1_R10G10B10_6E4_A2_FLOAT; break;
				default:							RED_ASSERT( !"Invalid" );
				}					
			#endif

				m_drawer->GetEffects().fxDOF->ApplyGameplayDof( *m_drawer, surfaces, info, 1.0f - cutsceneDofBlendFactor, rtnColorSource, rtnHelper );
			}
		}
	}

	// Radial blur
	const CEnvRadialBlurParameters &blurParams = info.m_envParametersGame.m_radialBlur;
	if ( ( PPCF_Blur & categoriesMask ) && blurParams.m_radialBlurAmount != 0.0f )
	{
		PC_SCOPE_RENDER_LVL1( RadialBlur ); 

		// Bind target
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );

		// Process
		m_drawer->GetEffects().fxBlurFilter->Apply(
			*m_drawer, surfaces, blurParams, info.m_camera,
			rtnColorSource,	TexelArea ( info.m_width, info.m_height, 0, 0 ) );

		// Update curr color target info
		Swap( rtnColorSource, rtnColorTarget );
	}
	
	// -----------------------------------------------------------
	// EPO_POST_TONEMAPPING pass of post processes
	// -----------------------------------------------------------
	{
		GetRenderer()->GetGameplayFX()->Apply( EPO_POST_TONEMAPPING , collector , info, rtnColorSource , rtnColorTarget );
		/*
		Here should be following effetcs:
			-	Cat View
			-	Drunk
		*/
	}

	// Fullscreen blur

	const Float fullscreenBlurIntensity = Clamp( info.m_envParametersGame.m_fullscreenBlurIntensity, 0.f, 1.f );
	if ( ( PPCF_Blur & categoriesMask ) && fullscreenBlurIntensity > 0.005f )
	{
		PC_SCOPE_RENDER_LVL1( BlurGauss );

		m_drawer->GetEffects().fxBlurGauss->ApplyNoStretch(
			*m_drawer, surfaces,
			rtnColorSource,		TexelArea ( info.m_width, info.m_height, 0, 0 ),
			rtnColorSource,		TexelArea ( info.m_width, info.m_height, 0, 0 ),
			rtnColorTarget,		TexelArea ( info.m_width, info.m_height, 0, 0 ),
			GpuApi::DRAWCONTEXT_PostProcSet, fullscreenBlurIntensity );
	}
	
	// Motion blur (in case postTonemapping option chosen)
	if ( info.m_worldRenderSettings.m_motionBlurSettings.m_isPostTonemapping && (PPCF_MotionBlur & categoriesMask) 
#ifdef USE_ANSEL
		&& !isAnselCaptureActive
#endif // USE_ANSEL
		)
	{
		PC_SCOPE_RENDER_LVL1( MotionBlur );
		const Bool applied = m_drawer->GetEffects().fxMotionBlur->Apply(  *m_drawer, surfaces, rtnColorSource, rtnColorTarget, rtnColorTempHalf1, rtnColorTempHalf2, rtnColorTempHalf3, info );
		// Apply() swapped source/target appropriately, so we don't need to.
#ifndef RED_FINAL_BUILD
		// Uncomment this if You want to see when motion blur is applied
		// collector.m_frame->AddDebugScreenFormatedText( 50, 200, applied ? Color( 0,255,0,255 ) : Color( 255,0,0,255 ) , TXT("MotionBlur: %d") , (Uint32)applied );
#endif

	}

	// -----------------------------------------------------------
	// Render new bloom
	// -----------------------------------------------------------
	if ( (PPCF_Tonemapping & categoriesMask) &&  
		 info.m_envParametersGame.m_displaySettings.m_allowBloom && 
		 info.m_envParametersArea.m_bloomNew.IsBloomExposed( info.m_envParametersDayPoint ) &&
		 (PPCF_Bloom & categoriesMask) )
	{
		PC_SCOPE_RENDER_LVL1( NewBloom ); 

		const CEnvBloomNewParametersAtPoint &bloomParams = info.m_envParametersArea.m_bloomNew;

		const SBloomConfig bloomConfig ( info );				

		// Calculate number of level which we're able to generate
		Uint32 brightCount = 0;
		SBrightInfo brightInfo[16];		
		Float shaftsLevelInvScale = 1.f;
		brightCount = BloomDownsample( info, bloomConfig, ARRAY_COUNT(brightInfo), brightInfo, &shaftsLevelInvScale, rtnColorTemp, rtnColorTarget, rtnColorTempHalf1, false );


		CRenderShaderPair* shaftsShader;
		if ( info.m_height <= (Uint32)Config::cvShaftsHeightLimitQ0.Get() )
		{
			shaftsShader = GetRenderer()->m_postfxBloomShaftsQ0;
		}
		else if ( info.m_height <= (Uint32)Config::cvShaftsHeightLimitQ1.Get() )
		{
			shaftsShader = GetRenderer()->m_postfxBloomShaftsQ1;
		}
		else
		{
			shaftsShader = GetRenderer()->m_postfxBloomShaftsQ2;
		}

		// Render shafts
		Bool isBloomDownsampled = false;
		if ( (PPCF_Shafts & categoriesMask) && collector.IsRenderingSunLightingEnabled() && bloomParams.IsShaftsExposed( info.m_envParametersDayPoint ) && shaftsLevelInvScale > 0 && bloomConfig.shaftsBrightIndex < brightCount && info.m_worldRenderSettings.m_shaftsIntensity > 0 )
		{	
			PC_SCOPE_RENDER_LVL1( BloomShafts );

			// Use moon or sun?
			Vector direction = info.m_envParametersDayPoint.m_useMoonForShafts ? info.m_envParametersDayPoint.m_moonDirection : info.m_envParametersDayPoint.m_sunDirection;

			Vector sunPosWorldSpace = info.m_camera.GetPosition() + (direction * 500.0f);
			sunPosWorldSpace.W = 1.0f;
			Vector vSunPos( info.m_camera.GetWorldToScreen().TransformVectorWithW( sunPosWorldSpace ) );
			vSunPos /= vSunPos.W;

			const float shaftsIntensity = Clamp( 1.f - ( Max( fabsf( vSunPos.X ), fabsf( vSunPos.Y ) ) - 1.f ) / 0.5f, 0.f, 1.f );

			sunShaftActive = true;
			sunShaftActive &= shaftsIntensity > 0;
			sunShaftActive &= ( Vector::Dot3( direction, info.m_camera.GetCameraForward() ) >= -0.4f );

			// Brightpass + downsample
			
			if ( sunShaftActive )
			{
				// Prepare skyonly texture
				{
					PC_SCOPE_RENDER_LVL1( BloomSkyColor );

					// Setup rendertarget
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTemp ) );
					rtSetup.SetViewport( fullWidth, fullHeight, 0, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					// Set textures
					GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetDepthBufferTex() };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

					// Set draw context
					CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_PostProcSet );

					//
					GetRenderer()->m_postfxBloomSkyFilter->Bind();
					GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

					// Unbind texture
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				}

				// Prepare surfaces
				brightCount = BloomDownsample( info, bloomConfig, Min<Uint32>( bloomConfig.shaftsBrightIndex + 1, ARRAY_COUNT(brightInfo) ), brightInfo, &shaftsLevelInvScale, rtnColorTemp, rtnColorTarget, rtnColorTempHalf1, true );
				
				// Get shafts brightInfo
				SBrightInfo shaftsBright = brightInfo[bloomConfig.shaftsBrightIndex]; //< copy because it will get overwritten!

				// Calc sun position
				Vector shaderSunPos = vSunPos * Vector( 0.5f, -0.5f, 0.f, 0.f ) + Vector( 0.5f, 0.5f, 0.f, 0.f );
				shaderSunPos.X = (shaderSunPos.X * fullWidth  - (Float)bloomConfig.bloomOffsetX) / bloomConfig.bloomFullWidth;
				shaderSunPos.Y = (shaderSunPos.Y * fullHeight - (Float)bloomConfig.bloomOffsetY) / bloomConfig.bloomFullHeight;

				// Choose temp rendertarget (we prefer half, because it's in esram on durango)
				ERenderTargetName tempTarget = rtnColorTempHalf2;
				if ( shaftsBright.width  > surfaces->GetRenderTarget( tempTarget )->GetWidth() || shaftsBright.height > surfaces->GetRenderTarget( tempTarget )->GetHeight() )
				{
					tempTarget = RTN_Color2;
				}

				Int32 targetAreaOffX = 0;
				for ( Uint32 shafts_pass_i=0; shafts_pass_i<2; ++shafts_pass_i )
				{
					PC_SCOPE_RENDER_LVL1( BloomShaftsPass );

					if ( 1 == shafts_pass_i )
					{
						RED_ASSERT( !isBloomDownsampled );
						brightCount = BloomDownsample( info, bloomConfig, ARRAY_COUNT(brightInfo), brightInfo, nullptr, rtnColorSource, rtnColorTarget, rtnColorTempHalf1, true );
						isBloomDownsampled = true;

						tempTarget = brightInfo[ bloomConfig.shaftsBrightIndex ].rtnBrightTarget;
						targetAreaOffX = brightInfo[ bloomConfig.shaftsBrightIndex ].offsetX;
					}

					// Render
					const Int32 areaWidth			= shaftsBright.width;
					const Int32 areaHeight			= shaftsBright.height;
					const Int32 sampleFullWidth		= surfaces->GetRenderTarget( shaftsBright.rtnBrightTarget )->GetWidth();
					const Int32 sampleFullHeight	= surfaces->GetRenderTarget( shaftsBright.rtnBrightTarget )->GetHeight();			
					const Int32 sampleAreaOffX		= shaftsBright.offsetX;
					const Int32 targetFullWidth		= surfaces->GetRenderTarget( tempTarget )->GetWidth();
					const Int32 targetFullHeight	= surfaces->GetRenderTarget( tempTarget )->GetHeight();			
					
					// Setup rendertarget
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( tempTarget ) );
					rtSetup.SetViewport( areaWidth, areaHeight, targetAreaOffX, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					// Set textures
					GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( shaftsBright.rtnBrightTarget ) };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
					GpuApi::SetSamplerStateCommon( 0, 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

					// Set draw context
					CGpuApiScopedDrawContext scopedDrawContext( shafts_pass_i ? GpuApi::DRAWCONTEXT_PostProcMax : GpuApi::DRAWCONTEXT_PostProcSet );

					Vector shaderSunPosFinal = shaderSunPos;

					const Float thresholdsScale = shaftsLevelInvScale * Max( 0.f, info.m_worldRenderSettings.m_shaftsThresholdsScale );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sampleFullWidth, (Float)sampleFullHeight, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)targetFullWidth, (Float)targetFullHeight, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)areaWidth, (Float)areaHeight, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)sampleAreaOffX, 0, (Float)targetAreaOffX, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( shaderSunPosFinal.X, shaderSunPosFinal.Y, (Float)shafts_pass_i, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( bloomParams.m_shaftsRadius.GetScalarClampMin(0), bloomParams.m_shaftsShapeExp.GetScalarClampMin(0), powf( bloomParams.m_shaftsShapeInvSquare.GetScalarClampMin(0), 2.f ), 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, shafts_pass_i ? bloomParams.m_shaftsColor.GetColorScaledGammaToLinear(true) * shaftsLevelInvScale * shaftsIntensity * info.m_worldRenderSettings.m_shaftsIntensity : Vector::ONES );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, shafts_pass_i ? Vector::ZEROS : Vector ( thresholdsScale * bloomConfig.precisionScale * bloomParams.m_shaftsThreshold.GetScalarClampMin(0), thresholdsScale * bloomConfig.precisionScale / bloomParams.m_shaftsThresholdRange.GetScalarClampMin(0.0001f), 0, 0 ) );
				
					shaftsShader->Bind();
					GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

					// Unbind texture
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );

					// Update shafts bright info
					Swap( shaftsBright.rtnBrightTarget, tempTarget );
					Swap( shaftsBright.offsetX, targetAreaOffX );
				}
			}
		}

		// downsample bloom temp surfaces if needed
		if ( !isBloomDownsampled )
		{
			brightCount = BloomDownsample( info, bloomConfig, ARRAY_COUNT(brightInfo), brightInfo, nullptr, rtnColorSource, rtnColorTarget, rtnColorTempHalf1, true );
			isBloomDownsampled = true;
		}

		// upsample with gauss blur
		{
			Float bloomLevelsScales[] = { 
				info.m_worldRenderSettings.m_bloomLevelScale0,
				info.m_worldRenderSettings.m_bloomLevelScale1,
				info.m_worldRenderSettings.m_bloomLevelScale2,
				info.m_worldRenderSettings.m_bloomLevelScale3,
				info.m_worldRenderSettings.m_bloomLevelScale4,
				info.m_worldRenderSettings.m_bloomLevelScale5,
				info.m_worldRenderSettings.m_bloomLevelScale6,
				info.m_worldRenderSettings.m_bloomLevelScale7,
			};

			for ( Uint32 i=0; i<ARRAY_COUNT(bloomLevelsScales); ++i )
			{
				bloomLevelsScales[i] = Max( 0.01f, bloomLevelsScales[i] );
			}

			for ( Uint32 i=ARRAY_COUNT(bloomLevelsScales)-1; i>0; --i )
			{
				bloomLevelsScales[i] = bloomLevelsScales[i] / bloomLevelsScales[i-1];
			}

			for ( Int32 level_i=(Int32)brightCount - 1; level_i>=(Int32)bloomConfig.requestedSkipHiLevels; --level_i )
			{
				PC_SCOPE_RENDER_LVL1( BloomUpsample );

				const SBrightInfo &currBright = brightInfo[level_i];
				const Bool isFinalCombine = level_i == (Int32)bloomConfig.requestedSkipHiLevels;
				const Bool isFirstIteration = level_i == (Int32)brightCount - 1;

				// Choose temp rendertarget (we prefer half, because it's in esram on durango)
				ERenderTargetName tempTarget = rtnColorTempHalf2;
				if ( currBright.width  > surfaces->GetRenderTarget( tempTarget )->GetWidth() || currBright.height > surfaces->GetRenderTarget( tempTarget )->GetHeight() )
				{
					tempTarget = RTN_Color2;
				}

				RED_ASSERT( tempTarget != currBright.rtnBrightTarget, TXT("Render targets got messed up!") );

				// horizontal
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( tempTarget ) );
					rtSetup.SetViewport( currBright.width, currBright.height, 0, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					const GpuApi::TextureRef tex[] = { GpuApi::TextureRef::Null(), surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ) };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
				
					const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ), 0 );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_10, Vector ( (Float)currBright.offsetX, 0, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_11, Vector ( 1, 0, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_12, Vector ( (Float)currBright.offsetX, 0, (Float)(currBright.offsetX + currBright.width - 1), (Float)(currBright.height - 1) ) );

					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
					GetRenderer()->m_postfxBloomGauss->Bind();
					GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				}

				// vertical
				if ( isFirstIteration )
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ) );
					rtSetup.SetViewport( currBright.width, currBright.height, currBright.offsetX, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					const GpuApi::TextureRef tex[] = { GpuApi::TextureRef::Null(), surfaces->GetRenderTargetTex( tempTarget ) };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
				
					const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( tempTarget ), 0 );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_10, Vector ( 0, 0, (Float)currBright.offsetX, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_11, Vector ( 0, 1, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_12, Vector ( 0, 0, (Float)(currBright.width - 1), (Float)(currBright.height - 1) ) );

					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
					GetRenderer()->m_postfxBloomGauss->Bind();
					GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				}
				else
				{
					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ) );
					rtSetup.SetViewport( currBright.width, currBright.height, currBright.offsetX, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					const Uint32 prevLevelIndex = level_i + 1;
					const SBrightInfo &prevBright = brightInfo[prevLevelIndex];

					const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( prevBright.rtnBrightTarget ), surfaces->GetRenderTargetTex( tempTarget ) };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

					// gauss params
					{
						const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( tempTarget ), 0 );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_10, Vector ( 0, 0, (Float)currBright.offsetX, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_11, Vector ( 0, 1, 0, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_12, Vector ( 0, 0, (Float)(currBright.width - 1), (Float)(currBright.height - 1) ) );
					}

					// combine params
					{
						ERenderTargetName targetName = currBright.rtnBrightTarget;
						Uint32 targetWidth = currBright.width;
						Uint32 targetHeight = currBright.height;
						Int32 targetOffX = currBright.offsetX;

						GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

						const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( prevBright.rtnBrightTarget ), 0 );
						const GpuApi::TextureLevelDesc targetDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( targetName ), 0 );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sourceDesc.width, (Float)sourceDesc.height, 0, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)prevBright.width, (Float)prevBright.height, 0, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)targetDesc.width, (Float)targetDesc.height, 0, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)targetWidth, (Float)targetHeight, 0, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( (Float)prevBright.offsetX, 0, (Float)targetOffX, 0 ) );
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( (Float)prevBright.offsetX, 0, (Float)(prevBright.offsetX + prevBright.width - 1), (Float)(prevBright.height - 1) ) );

						const Float combineScale = prevLevelIndex < ARRAY_COUNT(bloomLevelsScales) ? bloomLevelsScales[prevLevelIndex] : 1.f;
						GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector ( combineScale, combineScale, combineScale, (Float)level_i ) );
					}

					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
					GetRenderer()->m_postfxBloomGaussCombine->Bind();
					GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				}

				// combine
				if ( isFinalCombine )
				{
					ERenderTargetName targetName = rtnColorSource;
					Uint32 targetWidth = bloomConfig.bloomFullWidth;
					Uint32 targetHeight = bloomConfig.bloomFullHeight;
					Int32 targetOffX = bloomConfig.bloomOffsetX;
					Int32 targetOffY = bloomConfig.bloomOffsetY;
					
					if ( info.m_envParametersGame.m_displaySettings.m_displayMode == (Uint8)EMM_Bloom )
					{
						GetRenderer()->ClearColorTarget( surfaces->GetRenderTargetTex( targetName ), Vector( 0, 0, 0, 1 ) );
					}

					GpuApi::RenderTargetSetup rtSetup;
					rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( targetName ) );
					rtSetup.SetViewport( fullWidth, fullHeight, 0, 0 );
					GpuApi::SetupRenderTargets( rtSetup );

					const GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ) };
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
					GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

					const GpuApi::TextureLevelDesc sourceDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( currBright.rtnBrightTarget ), 0 );
					const GpuApi::TextureLevelDesc targetDesc = GpuApi::GetTextureLevelDesc( surfaces->GetRenderTargetTex( targetName ), 0 );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)sourceDesc.width, (Float)sourceDesc.height, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( (Float)currBright.width, (Float)currBright.height, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( (Float)targetDesc.width, (Float)targetDesc.height, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, Vector ( (Float)targetWidth, (Float)targetHeight, 0, 0 ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_4, Vector ( (Float)currBright.offsetX, 0, (Float)targetOffX, (Float)targetOffY ) );
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_5, Vector ( (Float)currBright.offsetX, 0, (Float)(currBright.offsetX + currBright.width - 1), (Float)(currBright.height - 1) ) );
					
					const Float combineScale = level_i < ARRAY_COUNT(bloomLevelsScales) ? bloomLevelsScales[level_i] : 1.f;
					const Float bloomColorScale = info.m_worldRenderSettings.m_bloomScaleConst  * combineScale / bloomConfig.precisionScale;
					const Vector bloomColor = bloomParams.m_color.GetColorScaledGammaToLinear( true ) * bloomColorScale;
					GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_6, Vector ( bloomColor.X, bloomColor.Y, bloomColor.Z, (Float)level_i ) );
					
					CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcAdd );
					{
						Bool combineWithoutDirt = true;

						if ( bloomParams.IsCameraDirtExposed( info.m_envParametersDayPoint ) )
						{
							CRenderTexture *cameraDirtRenderTexture = info.m_envParametersDayPoint.m_cameraDirtTexture.Get< CRenderTexture >();
							if ( cameraDirtRenderTexture )
							{
								combineWithoutDirt = false;
								cameraDirtRenderTexture->BindNoSampler( 1, RST_PixelShader );								
								GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_WrapLinearNoMip, GpuApi::PixelShader );
								
								GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_7, bloomParams.m_dirtColor.GetColorScaledGammaToLinear( true ) * bloomColorScale );
								const Float tileScaleY = info.m_envParametersDayPoint.m_cameraDirtNumVerticalTiles / (Float)info.m_height;
							#if MICROSOFT_ATG_DYNAMIC_SCALING
								const Float tileScaleX = tileScaleY * ( (surfaces->GetWidth(true)/(Float)Max<Uint32>(1,surfaces->GetHeight(true))) / (surfaces->GetWidth(false)/(Float)Max<Uint32>(1,surfaces->GetHeight(false))) );								
							#else
								const Float tileScaleX = tileScaleY;
							#endif
								GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_8, Vector ( tileScaleX, tileScaleY, 0.f, 1.f ) );

								GetRenderer()->m_postfxBloomCombineDirtOn->Bind();
								GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
							}
						}

						if ( combineWithoutDirt )
						{
							GetRenderer()->m_postfxBloomCombineDirtOff->Bind();
							GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
						}
					}
					
					GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );
				}
			}
		}
	}

	// -----------------------------------------------------------
	// Render sharpen
	// ----------------------------------------------------------
	{
		PC_SCOPE_RENDER_LVL1( RenderSharpen );

		const CEnvSharpenParametersAtPoint &sharpenParams = info.m_envParametersArea.m_sharpen;
		
		// sharpen can be artificially bumped up by user settings
		const Float sharpenFar = sharpenParams.m_sharpenFar.GetScalarClampMin(0) + ( Config::cvAllowSharpen.Get() > 1 ? 1.6f : 0.0f );
		const Float sharpenNear = sharpenParams.m_sharpenNear.GetScalarClampMin(0) + ( Config::cvAllowSharpen.Get() > 1 ? 1.6f : 0.0f );
		
		if ( ( PPCF_Sharpen & categoriesMask ) && (sharpenNear > 0.01f || sharpenFar > 0.01f) && !GIsRendererTakingUberScreenshot )
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
			GetRenderer()->m_postFXSharpen->Bind();

			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
						
			GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			const Float sharpenDistScale = 1.f / Max( 0.001f, sharpenParams.m_distanceFar.GetScalar() - sharpenParams.m_distanceNear.GetScalar() );
			const Float lumFilterOffset = sharpenParams.m_lumFilterOffset.GetScalar();
			const Float lumFilterRange = sharpenParams.m_lumFilterRange.GetScalarClampMin( 0.001f );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( (Float)info.m_width, (Float)info.m_height, (Float)surfaces->GetWidth(), (Float)surfaces->GetHeight() ) );
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector ( sharpenNear, sharpenFar, sharpenDistScale, -sharpenParams.m_distanceNear.GetScalar() * sharpenDistScale ) );			
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector ( -1.f / lumFilterRange, 1 + lumFilterOffset / lumFilterRange, 0 ) );			

			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );

			Swap( rtnColorTarget, rtnColorSource );
		}
	}

	// -----------------------------------------------------------
	// EPO_POST_BLOOM gameplay effects pass
	// -----------------------------------------------------------
	{
		GetRenderer()->GetGameplayFX()->Apply( EPO_POST_BLOOM , collector, info, rtnColorSource , rtnColorTarget );
		/*
		Here should be following effects:
			-	Focus Mode
		*/
	}

	// -----------------------------------------------------------
	// Render paint effect
	// ---------------------------------------------------------
	{
		PC_SCOPE_RENDER_LVL1( RenderPaintEffect );

		const CEnvPaintEffectParametersAtPoint &paintEffectParams = info.m_envParametersArea.m_paintEffect;
		const Float paintEffectAmount = paintEffectParams.m_amount.GetScalarClamp( 0, 1 );
		if ( paintEffectAmount > 0 )
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );

			Uint32 effectSizeIndex = 0;
			if ( Config::cvPaintEffectForcedSize.Get() >= 0 )
			{
				effectSizeIndex = (Uint32)Config::cvPaintEffectForcedSize.Get();
			}
			else
			{
 				if ( info.m_height > 2440 )			effectSizeIndex = 2;
 				else if ( info.m_height > 1200 )	effectSizeIndex = 1;
 				else								effectSizeIndex = 0;
			}

			switch ( effectSizeIndex )
			{
			case 2:		GetRenderer()->m_postFXPaintEffect2->Bind(); break;
			case 1:		GetRenderer()->m_postFXPaintEffect1->Bind(); break;
			default:	GetRenderer()->m_postFXPaintEffect0->Bind(); break;
			}
			
			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );

			GpuApi::TextureRef tex[] = { surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetDepthBufferTex() };
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );
			GpuApi::BindTextureStencil( ARRAY_COUNT(tex), surfaces->GetDepthBufferTex(), GpuApi::PixelShader );

			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( paintEffectAmount, 0, 0, 0 ) );

			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );

			GpuApi::BindTextures( 0, ARRAY_COUNT(tex) + 1, nullptr, GpuApi::PixelShader );

			Swap( rtnColorTarget, rtnColorSource );
		}
	}

	// -----------------------------------------------------------
	// Render lens flares
	// -----------------------------------------------------------
	if ( info.IsShowFlagOn( SHOW_Flares ) )
	{
		PC_SCOPE_RENDER_LVL1( RenderLensFlares );

		ERenderTargetName rtnFlaresAccum = rtnColorTemp;
		ASSERT( rtnFlaresAccum != rtnColorSource );

		// Build flares texture
		{
			GetRenderer()->GetStateManager().SetCamera( collector.GetRenderFrameInfo().m_camera );

			// Bind flares target
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnFlaresAccum ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			// Clear flares target
			GetRenderer()->ClearColorTarget( Vector::ZEROS );

			// Render flares
			RenderingContext rc( collector.m_frame->GetFrameInfo().m_camera );
			rc.m_terrainToolStampVisible = collector.GetRenderFrameInfo().IsTerrainToolStampVisible();
			rc.m_materialDebugMode = collector.GetRenderFrameInfo().m_materialDebugMode;
			rc.m_pass = RP_NoLighting;
			collector.RenderFlares( rc, false, true );

			GetRenderer()->GetStateManager().SetCamera2D();
		}		

		// Merge flares with scene color
		{
			// Bind color target
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );

			//
			m_drawer->GetEffects().fxFlare->ApplyToScene( *m_drawer, surfaces, Vector::ONES, 1, 0, rtnFlaresAccum, TexelArea ( fullWidth, fullHeight ), rtnColorSource, TexelArea ( fullWidth, fullHeight ) );
		}

		//
		Swap( rtnColorSource, rtnColorTarget );
	}

	// apply balancemap
	if ( info.m_envParametersGame.m_displaySettings.m_displayMode != (Uint8)EMM_Bloom )
	{
		PC_SCOPE_RENDER_LVL1( BalanceMap ); 

		const CEnvFinalColorBalanceParametersAtPoint &params = info.m_envParametersArea.m_finalColorBalance;

		// get the balance map
		Float balanceMapLerpFactor = 0;
		Float balanceMapAmountFactor = 0;
		Float balancePostBrightness = 1.f;
		CRenderTexture *balanceMap0 = NULL;
		CRenderTexture *balanceMap1 = NULL;
		Float balanceMapLerpFactorB = 0;
		Float balanceMapAmountFactorB = 0;
		Float balancePostBrightnessB = 1.f;
		CRenderTexture *balanceMap0B = NULL;
		CRenderTexture *balanceMap1B = NULL;
		if ( info.m_envParametersGame.m_displaySettings.m_allowColorMod && params.m_activatedBalanceMap )
		{
			if ( info.m_envBlendingFactor > 0.f )
			{
				const CEnvFinalColorBalanceParametersAtPoint &paramsB1 = info.m_envParametersAreaBlend1.m_finalColorBalance;
				const CEnvFinalColorBalanceParametersAtPoint &paramsB2 = info.m_envParametersAreaBlend2.m_finalColorBalance;

				if ( paramsB1.m_activatedBalanceMap )
				{
					balanceMapLerpFactor = paramsB1.m_balanceMapLerp.GetScalar();
					balanceMapAmountFactor = paramsB1.m_balanceMapAmount.GetScalar();
					balanceMap0 = ExtractRenderTexture( paramsB1.m_balanceMap0 );
					balanceMap1 = ExtractRenderTexture( paramsB1.m_balanceMap1 );
					balancePostBrightness = paramsB1.m_balancePostBrightness.GetScalar();
				}

				if ( paramsB2.m_activatedBalanceMap )
				{

					balanceMapLerpFactorB = paramsB2.m_balanceMapLerp.GetScalar();
					balanceMapAmountFactorB = paramsB2.m_balanceMapAmount.GetScalar();
					balanceMap0B = ExtractRenderTexture( paramsB2.m_balanceMap0 );
					balanceMap1B = ExtractRenderTexture( paramsB2.m_balanceMap1 );
					balancePostBrightnessB = paramsB2.m_balancePostBrightness.GetScalar();
				}				
			}
			else
			{
				balanceMapLerpFactor = params.m_balanceMapLerp.GetScalar();
				balanceMapAmountFactor = params.m_balanceMapAmount.GetScalar();
				balanceMap0 = ExtractRenderTexture( params.m_balanceMap0 );
				balanceMap1 = ExtractRenderTexture( params.m_balanceMap1 );
				balancePostBrightness = params.m_balancePostBrightness.GetScalar();
			}
		}

		// perform balance
		if ( m_drawer->GetEffects().fxCopy->ApplyWithBalanceMap( *m_drawer, surfaces, rtnColorTarget, TexelArea ( info.m_width, info.m_height, 0, 0 ), rtnColorSource, TexelArea ( info.m_width, info.m_height, 0, 0 ), balanceMapLerpFactor, balanceMapAmountFactor, balancePostBrightness, balanceMap0, balanceMap1, info.m_envBlendingFactor, balanceMapLerpFactorB, balanceMapAmountFactorB, balancePostBrightnessB, balanceMap0B, balanceMap1B ) )
		{
			Swap( rtnColorSource, rtnColorTarget );
		}
	}

	// Perform temporal antialiasing
	if ( !IsPostFxTemporalAAEnabled( collector ) )
	{
		if ( collector.IsWorldScene() )
		{
			surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_TemporalAntialias, true );
		}
	}
	else 
#ifdef USE_ANSEL
		if ( !isAnselCaptureActive ) // ANSEL integration
#endif // USE_ANSEL
	{
		PC_SCOPE_RENDER_LVL1( TemporalAA );

		struct history_info_t
		{
			Vector subPixelOffset;
			Matrix worldToScreenWithOffset;
			Matrix worldToScreenNoOffset;			
			Matrix worldToScreenWithOffsetInv;
			Matrix worldToScreenNoOffsetInv;			
			Matrix viewToScreenWithOffset;
			Matrix viewToScreenNoOffset;			
			Matrix viewToScreenWithOffsetInv;
			Matrix viewToScreenNoOffsetInv;			
			Matrix reprojectNoOffset;			
			Matrix reprojectWithOffset;			
			Matrix reprojectNoOffsetInv;		
		};

		const Uint32 numHistory = 4;
		static history_info_t historyInfo[numHistory] =
		{
			{ Vector::ZEROS, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY },
			{ Vector::ZEROS, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY },
			{ Vector::ZEROS, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY },
			{ Vector::ZEROS, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY, Matrix::IDENTITY },
		};

		const Bool wasDirty = surfaces->IsPersistentSurfaceDirty( CRenderSurfaces::PS_TemporalAntialias );
		if ( wasDirty )
		{
			surfaces->SetPersistentSurfaceDirty( CRenderSurfaces::PS_TemporalAntialias, false );
		}
		
		{
			{
				history_info_t lastHistoryBackup = historyInfo[numHistory-1];
				for ( Uint32 i=numHistory-1; i>0; --i )
				{
					historyInfo[i] = historyInfo[i-1];
				}
				historyInfo[0] = lastHistoryBackup;
			}

			historyInfo[0].subPixelOffset = Vector( info.m_camera.GetSubpixelOffsetX() * info.m_width / 2.f, info.m_camera.GetSubpixelOffsetY() * info.m_height / 2.f, 0, 0 );
			{
				historyInfo[0].worldToScreenWithOffset = info.m_camera.GetWorldToScreen();
				historyInfo[0].viewToScreenWithOffset = info.m_camera.GetViewToScreen();

				CRenderCamera cam = info.m_camera;
				cam.SetSubpixelOffset( 0, 0, info.m_width, info.m_height );
				historyInfo[0].worldToScreenNoOffset = cam.GetWorldToScreen();
				historyInfo[0].viewToScreenNoOffset = cam.GetViewToScreen();

				historyInfo[0].worldToScreenWithOffsetInv = historyInfo[0].worldToScreenWithOffset.FullInverted();
				historyInfo[0].worldToScreenNoOffsetInv = historyInfo[0].worldToScreenNoOffset.FullInverted();
				historyInfo[0].viewToScreenWithOffsetInv = historyInfo[0].viewToScreenWithOffset.FullInverted();
				historyInfo[0].viewToScreenNoOffsetInv = historyInfo[0].viewToScreenNoOffset.FullInverted();
				historyInfo[0].reprojectNoOffset = Matrix::IDENTITY;
				historyInfo[0].reprojectWithOffset = Matrix::IDENTITY;
				historyInfo[0].reprojectNoOffsetInv = Matrix::IDENTITY;

				for ( Uint32 ii=1; ii<4; ++ii )
				{
					historyInfo[ii].reprojectNoOffset = historyInfo[0].worldToScreenNoOffsetInv * historyInfo[ii].worldToScreenNoOffset;
					historyInfo[ii].reprojectWithOffset = historyInfo[0].worldToScreenWithOffsetInv * historyInfo[ii].worldToScreenWithOffset;
					historyInfo[ii].reprojectNoOffsetInv = historyInfo[ii].reprojectNoOffset.FullInverted();

					MatrixDouble m0, m1;
					m0.Import( historyInfo[0].worldToScreenNoOffset );
					m1.Import( historyInfo[ii].worldToScreenNoOffset );
					(m0.FullInverted() * m1).Export( historyInfo[ii].reprojectNoOffset );
				}
			}
		}

		if ( wasDirty )
		{
			// make all history equal current
			for ( Uint32 i=1; i<numHistory; ++i )
			{
				historyInfo[i] = historyInfo[0];
			}

			GpuApi::MultiGPU_BeginEarlyPushTexture( surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );
			GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );
			GpuApi::MultiGPU_EndEarlyPushTexture( surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );

			// currColor -> lum0
			RenderTemporalAALuminance( info, surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetRenderTargetTex( RTN_TemporalAALum0 ) );

			// currColor -> lum1
			RenderTemporalAALuminance( info, surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetRenderTargetTex( RTN_TemporalAALum1 ) );			
		}
		else
		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtnColorTarget ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
		
			const Uint32 constBufferSize = numHistory * sizeof(history_info_t);
			if ( !m_temporalAABuffer )
			{
				m_temporalAABuffer = GpuApi::CreateBuffer( constBufferSize, GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
				ASSERT( m_temporalAABuffer );
			}
		
			{
				void* pConstantData = GpuApi::LockBuffer( m_temporalAABuffer, GpuApi::BLF_Discard, 0, constBufferSize );
				ASSERT( pConstantData );
				if ( pConstantData )
				{
					memcpy( pConstantData, historyInfo, constBufferSize );
					GpuApi::UnlockBuffer( m_temporalAABuffer );
				}
			}

			const Uint32 constBufferIndex = 10;
			GpuApi::BindConstantBuffer( constBufferIndex, m_temporalAABuffer, GpuApi::PixelShader );

			GpuApi::TextureRef tex[] = { 
				surfaces->GetRenderTargetTex( rtnColorSource ), 
				surfaces->GetRenderTargetTex( RTN_TemporalAAColor ),
				surfaces->GetRenderTargetTex( RTN_TemporalAALum0 ),
				surfaces->GetRenderTargetTex( RTN_TemporalAALum1 ),
				surfaces->GetDepthBufferTex()
			};
			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), tex, GpuApi::PixelShader );

			GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 1, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );

			{
				Vector dim ( (Float)info.m_width, (Float)info.m_height, 0, 0 );
				Vector scaleValue = dim * 0.5f * Vector( 1.f, -1.f, 0.f, 0.f );
				Vector biasValue = dim * 0.5f - Vector( 0.5f, 0.5f, 0.f, 0.f );
				GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector ( scaleValue.X, scaleValue.Y, biasValue.X, biasValue.Y ) );
			}

			CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_PostProcSet );
			GetRenderer()->m_postFXTemporalAA->Bind();

			GetRenderer()->BindSharedConstants( GpuApi::PixelShader );
			GetRenderer()->GetDebugDrawer().DrawQuad( Vector (-1, -1, 0, 1), Vector (1, 1, 0, 1), 0.5f );
			GetRenderer()->UnbindSharedConstants( GpuApi::PixelShader );

			GpuApi::BindTextures( 0, ARRAY_COUNT(tex), nullptr, GpuApi::PixelShader );

			GpuApi::BindConstantBuffer( constBufferIndex, GpuApi::BufferRef::Null(), GpuApi::PixelShader );

			// lum0 -> lum1
			surfaces->SwapRenderTargetPointers( RTN_TemporalAALum0, RTN_TemporalAALum1 );
			// prevColor -> lum0
			RenderTemporalAALuminance( info, surfaces->GetRenderTargetTex( RTN_TemporalAAColor ), surfaces->GetRenderTargetTex( RTN_TemporalAALum0 ) );		

			// currColor -> prevColor
			if ( surfaces->IsRenderTargetsSwappable( rtnColorSource, RTN_TemporalAAColor ) && !GpuApi::MultiGPU_IsActive() )
			{
				surfaces->SwapRenderTargetPointers( rtnColorSource, RTN_TemporalAAColor );
			}
			else
			{
				GpuApi::MultiGPU_BeginEarlyPushTexture( surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );
				GetRenderer()->StretchRect( surfaces->GetRenderTargetTex( rtnColorSource ), surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );
				GpuApi::MultiGPU_EndEarlyPushTexture( surfaces->GetRenderTargetTex( RTN_TemporalAAColor ) );
			}

			//
			Swap( rtnColorSource, rtnColorTarget );
		}
	}

	//
	GetRenderer()->UnbindSharedConstants( GpuApi::PixelShader );

	//
	return rtnColorSource;
}

void CRenderPostProcess::DrawFinalizationAndAA( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, EPostProcessCategoryFlags categoriesMask, ERenderTargetName rtSample )
{
	const Bool enableAA = 
#ifdef USE_ANSEL
		isAnselCaptureActive ? false : 
#endif // USE_ANSEL
		IsPostFxAntialiasingEnabled( categoriesMask, info ) && !IsPostFxTemporalAAEnabled( collector );

	// Apply AA
	//
	// Note! nVidia readme suggests to perform it in gamma space instead of linear space, 
	//       but WE'RE OPERATING IN LINEAR BECAUSE luminance preservation is a lot better this way,
	//       and dithering artifacts mentioned by nVidia are not visible because we're using 
	//		 highest fxaa quality anyway.
	//
	if ( enableAA )
	{
		PC_SCOPE_RENDER_LVL1( RenderAntialiasing );

		ERenderTargetName rtTemp = RTN_PostProcessTarget1 == rtSample ? RTN_PostProcessTarget2 : RTN_PostProcessTarget1;

		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( rtTemp ) );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
		}	

		{
			PC_SCOPE_RENDER_LVL1( PostFxAA );

			m_drawer->GetEffects().fxPresent->ApplyAA(
				*m_drawer, surfaces->GetRenderTargetTex( rtSample ),
				TexelArea ( info.m_width, info.m_height ) );
		}

		GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 0, 4, nullptr, GpuApi::VertexShader );

		rtSample = rtTemp;
	}

	// Perform finalize
	{
		PC_SCOPE_RENDER_LVL1( RenderFinalization );

		ASSERT( RTN_FinalColor != rtSample );

		{
			GpuApi::RenderTargetSetup rtSetup;
			rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
			//rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
			rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
			GpuApi::SetupRenderTargets( rtSetup );
		}

		{
			PC_SCOPE_RENDER_LVL1( RenderFinalize ); 

			CPostFxPresent::SVignetteParams	vignetteParams;

			vignetteParams.enableVignette = info.m_envParametersGame.m_displaySettings.m_allowVignette 
#ifdef USE_ANSEL
				&& !isAnselCaptureActive
#endif // USE_ANSEL
				;

			if ( vignetteParams.enableVignette )
			{
				vignetteParams.vignetteTexture = info.m_envParametersDayPoint.m_vignetteTexture.Get<CRenderTexture>();
				vignetteParams.vignetteWeights = Vector (1.f, 1.f, 1.f, 0.f) * info.m_envParametersArea.m_finalColorBalance.m_vignetteWeights.GetColor().ToVector();
				vignetteParams.vignetteColor   = info.m_envParametersArea.m_finalColorBalance.m_vignetteColor.GetColor().ToVector();
				vignetteParams.vignetteOpacity = info.m_envParametersArea.m_finalColorBalance.m_vignetteOpacity.GetScalarClamp( 0.f, 1.f );
			}

			m_drawer->GetEffects().fxPresent->ApplyFinalize( *m_drawer, surfaces, rtSample,	TexelArea ( info.m_width, info.m_height, 0, 0 ), info, vignetteParams, categoriesMask );
		}
	}

	// Draw debug CameraInteriorFactor
	if ( info.IsShowFlagOn( SHOW_CameraInteriorFactor ) )
	{
		RenderDebugCameraInteriorFactor( collector, RTN_FinalColor );
	}
}

void CRenderPostProcess::DrawFrozenFrame( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, class CRenderViewport* viewport, EPostProcessCategoryFlags categoriesMask )
{

	{
		GpuApi::RenderTargetSetup rtSetup;
		rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_FinalColor ) );
		rtSetup.SetDepthStencilTarget( surfaces->GetDepthBufferTex() );
		rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
		GpuApi::SetupRenderTargets( rtSetup );
	}

	GpuApi::BindTextures( 0, 8, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 0, 4, nullptr, GpuApi::VertexShader );

}

void CRenderPostProcess::DrawFade( const CRenderFrameInfo& info )
{
	if ( m_fade.m_currentAlpha > 0.0f && (!info.m_isNonInteractiveRendering || info.m_forceFade) )
	{
// 		GpuApi::RenderTargetSetup rtSetup = GpuApi::GetRenderTargetSetup();
// 		//rtSetup.SetColorTarget( 0, GpuApi::GetBackBufferTexture() );
// 		rtSetup.SetViewport( info.GetWidth(), info.GetHeight() );
// 		GpuApi::SetupRenderTargets(rtSetup);
		CGpuApiScopedDrawContext drawContext( GpuApi::DRAWCONTEXT_ScreenFade );

		// Calculate fade color
		Color fadeColor;
		fadeColor.R = (Uint8)Clamp< Int32 >( (Int32)(m_fade.m_currentColor.X * 255.0f), 0, 255 );
		fadeColor.G = (Uint8)Clamp< Int32 >( (Int32)(m_fade.m_currentColor.Y * 255.0f), 0, 255 );
		fadeColor.B = (Uint8)Clamp< Int32 >( (Int32)(m_fade.m_currentColor.Z * 255.0f), 0, 255 );
		fadeColor.A = (Uint8)Clamp< Int32 >( (Int32)(m_fade.m_currentAlpha   * 255.0f), 0, 255 );

		// Draw fullscreen quad

		const GpuApi::RenderTargetSetup& rtSetup = GpuApi::GetRenderTargetSetup();

		GetRenderer()->GetDebugDrawer().DrawQuad2DEx( rtSetup.viewport.x, rtSetup.viewport.y, rtSetup.viewport.width, rtSetup.viewport.height, fadeColor );
	}
}


void CRenderPostProcess::DrawSurfaceFlow( CRenderCollector& collector, const CRenderFrameInfo& info, class CRenderSurfaces* surfaces,
										  ERenderTargetName rtnTarget, ERenderTargetName rtnHelper )
{
	PC_SCOPE_RENDER_LVL1( RenderSurfaceFlow );

	if ( collector.m_scene != nullptr )
	{
		Float immediateWetSurfaceEffectStrength = info.m_envParametersDayPoint.m_immediateWetSurfaceEffectStrength;
		CRenderTextureArray *flowTexArr = info.m_envParametersDayPoint.m_wetSurfaceTexture.Get< CRenderTextureArray >();

		if ( immediateWetSurfaceEffectStrength > 0.01f && flowTexArr != nullptr )
		{
			GpuApi::TextureRef waterIntersectionTex = GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D );

			CRenderProxy_Water* waterProxy = collector.m_scene->GetWaterProxy();
			if( waterProxy != nullptr && waterProxy->ShouldRenderUnderwater() ) 
			{
				waterIntersectionTex = waterProxy->GetUnderwaterIntersectionTexture();
			}	

			m_drawer->GetEffects().fxSurfaceFlow->ApplySurfaceFlowFx(
				*m_drawer,		surfaces,
				rtnTarget,		TexelArea( info.m_width, info.m_height ),
				rtnHelper,		TexelArea( info.m_width, info.m_height ),
				flowTexArr, immediateWetSurfaceEffectStrength, waterIntersectionTex
				);
		}
	}
}

void CRenderPostProcess::DrawFeedbackDepthBuffer( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces )
{
	PC_SCOPE_RENDER_LVL1( DrawFeedbackDepthBuffer );

	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( RTN_GBuffer3Depth ) );
	rtSetup.SetViewport( (Int32)GpuApi::GetViewport().width, (Int32)GpuApi::GetViewport().height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	// Downscale
	GpuApi::TextureRef sampleTexture = GetRenderer()->IsMSAAEnabled( info ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex();
	Rect rect( 0, info.m_width, 0, info.m_height );
	GetRenderer()->StretchRect( surfaces->GetDepthBufferTex(), rect, surfaces->GetRenderTargetTex( RTN_GBuffer3Depth ), rect );
}

void CRenderPostProcess::CopyWithScale( const GpuApi::TextureRef &fromTex, const PostProcessUtilities::TexelArea &fromArea, const GpuApi::TextureRef &toTex, const PostProcessUtilities::TexelArea &toArea, Float scalar, GpuApi::eDrawContext context )
{
	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, toTex );
	rtSetup.SetViewport( toArea.m_width, toArea.m_height, toArea.m_offsetX, toArea.m_offsetY );
	GpuApi::SetupRenderTargets( rtSetup );

	// Downscale
	m_drawer->GetEffects().fxCopy->ApplyWithScale( *m_drawer, fromTex, fromArea, scalar, context );
}

void CRenderPostProcess::CopyWithScale( const CRenderFrameInfo& info, class CRenderSurfaces* surfaces, ERenderTargetName from, ERenderTargetName to, Float scalar, GpuApi::eDrawContext context )
{
	// Bind target
	GpuApi::RenderTargetSetup rtSetup;
	rtSetup.SetColorTarget( 0, surfaces->GetRenderTargetTex( to ) );
	rtSetup.SetViewport( info.m_width, info.m_height, 0, 0 );
	GpuApi::SetupRenderTargets( rtSetup );

	// Downscale
	m_drawer->GetEffects().fxCopy->ApplyWithScale( *m_drawer, surfaces, from, TexelArea (info.m_width, info.m_height ), scalar, context );
}

void CRenderPostProcess::CopyDepthTarget( const GpuApi::TextureRef &from, const PostProcessUtilities::TexelArea &fromArea )
{
	m_drawer->GetEffects().fxCopy->ApplyDepthTarget( *m_drawer, from, fromArea );
}

void CRenderPostProcess::OnLostDevice()
{
	m_drawer->OnLostDevice();
}

Bool CRenderPostProcess::IsPostFxAntialiasingEnabled( EPostProcessCategoryFlags flags, const CRenderFrameInfo& info )
{
	return m_drawer->GetEffects().fxPresent->IsPostFxAntialiasingEnabled( flags, info );
}

Bool CRenderPostProcess::IsPostFxTemporalAAEnabled( const CRenderCollector& collector, Bool testDirtyFlag )
{
	if( !Config::cvEnableTemporalAA.Get() || !collector.IsWorldScene() || GIsRendererTakingUberScreenshot || !collector.m_info->IsShowFlagOn( SHOW_PreferTemporalAA ) )
		return false;

	if( !IsPostFxAntialiasingEnabled( collector.m_postProcess, *collector.m_info ) )
		return false;

	if ( !GetRenderer()->GetSurfaces()->IsTemporalAASupported() )
		return false;

#ifndef RED_FINAL_BUILD
	const Uint32 debugDPreview = collector.m_info->m_envParametersGame.m_displaySettings.m_displayMode;

	if( collector.m_info->IsShowFlagOn( SHOW_Wireframe ) // turn off AA for wireframe mode - otherwise the view is shaky
		|| !( EMM_None == debugDPreview 
			|| ( collector.m_info->IsShowFlagOn( SHOW_AllowDebugPreviewTemporalAA ) 
				&& !(EMM_DecomposedAmbient == debugDPreview 
					|| EMM_DecomposedDiffuse == debugDPreview 
					|| EMM_DecomposedSpecular == debugDPreview 
					|| EMM_DecomposedReflection == debugDPreview 
					|| EMM_DecomposedLightingAmbient == debugDPreview 
					|| EMM_DecomposedLightingReflection == debugDPreview) ) ) )
	{
		return false;
	}
#endif // RED_FINAL_BUILD
	
	if( testDirtyFlag && GetRenderer()->GetSurfaces()->IsPersistentSurfaceDirty( CRenderSurfaces::PS_TemporalAntialias ) )
		return false;

	return true;
}
