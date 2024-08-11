/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCollector.h"
#include "renderElementMap.h"
#include "renderMesh.h"
#include "renderProxyDecal.h"
#include "renderShaderPair.h"
#include "renderTexture.h"
#include "renderScene.h"
#include "renderVisibilityQueryManager.h"
#include "../engine/dynamicDecal.h"
#include "../engine/renderFragment.h"
#include "../engine/decalComponent.h"
#include "../engine/renderSettings.h"

#define MIN_DEPTH_FADE_MUL 1.0f
#define MAX_DEPTH_FADE_MUL 8.0f
#define STATIC_DEPTH_FADE_MUL 1024.0f		// Big enough that it shouldn't make static decals fade out when that wasn't intended.

GpuApi::BufferRef CRenderProxy_Decal::g_vertices = GpuApi::BufferRef::Null();
GpuApi::BufferRef CRenderProxy_Decal::g_indices = GpuApi::BufferRef::Null();
GpuApi::BufferRef CRenderProxy_Decal::g_constants = GpuApi::BufferRef::Null();

static Bool CreateDecalBuffers( GpuApi::BufferRef& vertexBuffer, GpuApi::BufferRef& indexBuffer, GpuApi::BufferRef& constantBuffer )
{
	if ( vertexBuffer )
	{
		GpuApi::AddRef( vertexBuffer );
	}
	else
	{
		const Float extent = 0.5f;

		Float verts[24];
		verts[0]	= -extent;	verts[1] = -extent;		verts[2] = -extent;
		verts[3]	= -extent;	verts[4] = extent;		verts[5] = -extent;
		verts[6]	= extent;	verts[7] = extent;		verts[8] = -extent;
		verts[9]	= extent;	verts[10] = -extent;	verts[11] = -extent;
		verts[12]	= -extent;	verts[13] = -extent;	verts[14] = extent;
		verts[15]	= -extent;	verts[16] = extent;		verts[17] = extent;
		verts[18]	= extent;	verts[19] = extent;		verts[20] = extent;
		verts[21]	= extent;	verts[22] = -extent;	verts[23] = extent;

		Uint32 bufferSize = 8 * 3 * sizeof( Float ); // 8 vertices with position only, so 24 Floats (X,Y,Z)
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = verts;
		vertexBuffer = GpuApi::CreateBuffer( bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
		ASSERT( vertexBuffer );
		if ( !vertexBuffer )
		{
			return false;
		}
	}

	if ( indexBuffer )
	{
		GpuApi::AddRef( indexBuffer );
	}
	else
	{
		Uint16 indices[36];
		/*Front side*/
		indices[0]  = 1; indices[1]  = 0; indices[2]  = 2;
		indices[3]  = 2; indices[4]  = 0; indices[5]  = 3;
		/*Back side*/
		indices[6]  = 6; indices[7]  = 4; indices[8]  = 5;
		indices[9]  = 7; indices[10] = 4; indices[11] = 6;
		/*Left side*/
		indices[12] = 5; indices[13] = 4; indices[14] = 1;
		indices[15] = 1; indices[16] = 4; indices[17] = 0;
		/*Right side*/
		indices[18] = 2; indices[19] = 3; indices[20] = 6;
		indices[21] = 6; indices[22] = 3; indices[23] = 7;
		/*Top*/
		indices[24] = 5; indices[25] = 1; indices[26] = 6;
		indices[27] = 6; indices[28] = 1; indices[29] = 2;
		/*Bottom*/
		indices[30] = 0; indices[31] = 4; indices[32] = 3;
		indices[33] = 3; indices[34] = 4; indices[35] = 7;

		Uint32 bufferSize = 12 * 3 * sizeof( Uint16 ); // 12 triangles with 3 indices per triangle, so 36 Uint16-s
		GpuApi::BufferInitData bufInitData;
		bufInitData.m_buffer = indices;
		indexBuffer = GpuApi::CreateBuffer( bufferSize, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
		ASSERT( indexBuffer );
		if ( !indexBuffer )
		{
			GpuApi::SafeRelease( vertexBuffer );
			return false;
		}
	}

	if ( constantBuffer )
	{
		GpuApi::AddRef( constantBuffer );
	}
	else
	{
		constantBuffer = GpuApi::CreateBuffer( sizeof( CRenderProxy_Decal::SDecalConstants ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		RED_ASSERT( constantBuffer );
		if ( !constantBuffer )
		{
			GpuApi::SafeRelease( vertexBuffer );
			GpuApi::SafeRelease( indexBuffer );
			return false;
		}
	}

	return true;
}

CRenderProxy_Decal::CRenderProxy_Decal( const RenderProxyInitInfo& info ) 
	: IRenderProxyDrawable( RPT_SSDecal, info )
	, m_diffuseTexture( nullptr )
	, m_normalTexture( nullptr )
	, m_alpha( 1.0f )
	, m_scale( 1.0f )
	, m_useVerticalProjection( false )
	, m_horizontalFlip( false )
	, m_verticalFlip( false )
	, m_diffuseColor( Color::WHITE )
	, m_atlasVector( .5f ,.5f , .5f , .5f )
	, m_specularity( -1.0f )
	, m_specularColor( Color::BLACK )
	, m_lastUpdateFrame( -5 )
	, m_cachedSubUVClip( 0.0f, 0.0f, 1.0f, 1.0f )
	, m_up( 0.0, 0.0,1.0,0.0 )
	, m_tangent( 0.0, 1.0, 0.0,0.0 )
	, m_Zdepthfade( STATIC_DEPTH_FADE_MUL )
{
	if ( info.m_component && info.m_component->IsA< CDecalComponent >() )
	{
		const CDecalComponent* component = static_cast< const CDecalComponent* >( info.m_component );
		
		ASSERT( component );
		if ( component->GetDiffuseTexture() )
		{
			m_diffuseTexture = static_cast< CRenderTexture* >( component->GetDiffuseTexture()->GetRenderResource() );
			if (m_diffuseTexture)
			{
				m_diffuseTexture->AddRef();
			}
		}
		m_autoHideDistance = component->GetAutoHideDistance();
		m_normalThreshold = DEG2RAD( component->GetNormalThreshold() );

		m_horizontalFlip = component->GetHorizontalFlip();
		m_verticalFlip = component->GetVerticalFlip();

		m_specularity = component->GetSpecularity();
		m_specularColor = component->GetSpecularColor();

		// m_fadeSpeed = 1.0f / Max( component->GetFadeTime(), 0.001f );
	}
	else if ( info.m_packedData && info.m_packedData->GetType() == RPT_SSDecal )
	{
		const RenderProxyDecalInitData* data = static_cast< const RenderProxyDecalInitData* >( info.m_packedData );

		if ( data->m_texture )
		{
			m_diffuseTexture = static_cast< CRenderTexture* >( data->m_texture->GetRenderResource() );
			if (m_diffuseTexture)
			{
				m_diffuseTexture->AddRef();
			}
		}

		m_autoHideDistance = data->m_autoHideDistance;
		m_normalThreshold = DEG2RAD( data->m_normalThreshold );

		m_horizontalFlip = data->m_horizontalFlip;
		m_verticalFlip = data->m_verticalFlip;

		m_specularity = data->m_specularity;
		m_specularColor = data->m_specularColor;

		// m_fadeSpeed = 1.0f / Max( data->m_fadeTime, 0.001f );
	}

	m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;

#ifdef USE_UMBRA
	if ( m_diffuseTexture )
	{
		m_umbraProxyId = GlobalVisID( m_diffuseTexture->GetUmbraId(), GetLocalToWorld() );
	}
#endif

	CacheProjectionMatrix();
}

CRenderProxy_Decal::CRenderProxy_Decal( const SDynamicDecalInitInfo& decalInfo )
	: IRenderProxyDrawable( RPT_SSDecal, RenderProxyInitInfo() )
	, m_diffuseTexture( nullptr )
	, m_normalTexture( nullptr )
	, m_alpha( 1.0f )
	, m_scale(1.0f)
	, m_horizontalFlip( false )
	, m_verticalFlip( false )
	, m_diffuseColor( decalInfo.m_diffuseColor )
	, m_atlasVector( decalInfo.m_atlasVector )
	, m_specularity( decalInfo.m_specularity )
	, m_specularColor( decalInfo.m_specularColor )
	, m_cachedSubUVClip( 0.0f, 0.0f, 1.0f, 1.0f )
	, m_up( 0.0, 0.0,1.0,0.0 )
	, m_tangent( 0.0, 1.0, 0.0,0.0 )
	, m_Zdepthfade( Lerp( MAX_DEPTH_FADE_MUL , MIN_DEPTH_FADE_MUL , decalInfo.m_depthFadePower ) )
{
	Matrix templ2w = decalInfo.GetDecalToWorldMatrix();
	// Scale up in this case, because the SS decal mesh has vertices in range [-0.5, 0.5], instead of [-1,1].
	// Swap X/Z because SS decals project along Z, while dyn decals project along Y.
	m_localToWorld.SetRow( 0, templ2w.GetRow( 0 )*2 );
	m_localToWorld.SetRow( 1, templ2w.GetRow( 2 )*2 );
	m_localToWorld.SetRow( 2, -templ2w.GetRow( 1 )*2 );
	m_localToWorld.SetRow( 3, templ2w.GetRow( 3 ) );

	m_boundingBox = decalInfo.GetWorldBounds();

	if ( decalInfo.m_diffuseTexture != nullptr )
	{
		m_diffuseTexture = static_cast< CRenderTexture* >( decalInfo.m_diffuseTexture );
		m_diffuseTexture->AddRef();
	}
	if ( decalInfo.m_normalTexture != nullptr )
	{
		m_normalTexture = static_cast< CRenderTexture* >( decalInfo.m_normalTexture );
		m_normalTexture->AddRef();
	}

	// SS decals use a hard threshold, but dynamic decals fade out. We'll just pick the mid-point of the fading as our threshold.
	m_normalThreshold = /*MAcos*/( Clamp( 0.5f / decalInfo.m_normalFadeScale - decalInfo.m_normalFadeBias, -1.0f, 1.0f ) );

	m_autoHideDistance = CDecalComponent::CalculateAutoHideDistance( decalInfo.m_autoHideDistance, m_boundingBox );
	m_autoHideDistanceSquared = m_autoHideDistance * m_autoHideDistance;

#ifdef USE_UMBRA
	if ( m_diffuseTexture )
	{
		m_umbraProxyId = GlobalVisID( m_diffuseTexture->GetUmbraId(), GetLocalToWorld() );
	}
#endif

	CacheProjectionMatrix();
}


CRenderProxy_Decal::~CRenderProxy_Decal()
{
	SAFE_RELEASE( m_diffuseTexture );
	SAFE_RELEASE( m_normalTexture );
}


void CRenderProxy_Decal::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	const Float distanceSq = CalcCameraDistanceSqForTextures( prefetch->GetCameraPosition(), prefetch->GetCameraFovMultiplierUnclamped() );
	prefetch->AddTextureBind( m_diffuseTexture, distanceSq );
	prefetch->AddTextureBind( m_normalTexture, distanceSq );
}


void CRenderProxy_Decal::AttachToScene( CRenderSceneEx* scene )
{
	IRenderProxyDrawable::AttachToScene( scene );

	VERIFY( CreateDecalBuffers( g_vertices, g_indices, g_constants ) );
}

void CRenderProxy_Decal::DetachFromScene( CRenderSceneEx* scene )
{
	// sort of a "safe release" for the buffers
	if ( g_vertices && 0 == GpuApi::Release( g_vertices ) )
	{
		g_vertices = GpuApi::BufferRef::Null();
	}
	if ( g_indices && 0 == GpuApi::Release( g_indices ) )
	{
		g_indices = GpuApi::BufferRef::Null();
	}
	if ( g_constants && 0 == GpuApi::Release( g_constants ) )
	{
		g_constants = GpuApi::BufferRef::Null();
	}

	IRenderProxyDrawable::DetachFromScene( scene );
}

void CRenderProxy_Decal::Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	if ( !m_diffuseTexture )
	{
		return;
	}

	if ( !context.CheckLightChannels( m_lightChannels ) )
	{
		return;
	}

	if ( !g_indices || !g_vertices )
	{
		return;
	}

	RED_ASSERT( m_alpha > 0.0f, TXT("Decal with alpha == 0. This should have been filtered during collection!") );
	RED_ASSERT( !GetGenericFadeAlpha().IsZero(), TXT("Decal with generic alpha == 0. This should have been filtered during collection!") );
	// If we're totally faded out, no point in drawing.
	if ( m_alpha <= 0.0f || GetGenericFadeAlpha().IsZero() )
	{
		return;
	}

	const Float textureDistance = AdjustCameraDistanceSqForTextures( GetCachedDistanceSquared() );

	m_diffuseTexture->Bind( 0, RST_PixelShader, textureDistance );

	if( m_normalTexture )
	{
		m_normalTexture->Bind( 4, RST_PixelShader, textureDistance );
	}

	UpdateConstants();
	GpuApi::BindConstantBuffer( 6, g_constants, GpuApi::PixelShader );

	// Bind vertex buffer
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPos );
	Uint32 vbOffset = 0;
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos, false );
	GpuApi::BindVertexBuffers( 0, 1, &g_vertices, &vbSstride, &vbOffset );

	// Bind index buffer
	GpuApi::BindIndexBuffer( g_indices );

	// Draw chunk
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, 8, 0, 12 );

#ifndef RED_FINAL_BUILD
	// push to final frame stats
	{
		extern SceneRenderingStats GRenderingStats;
		GRenderingStats.m_numDecalProxies += 1;
		GRenderingStats.m_numDecalTriangles += 12;
		GRenderingStats.m_numDecalChunks += 1;
	}
#endif
}

const EFrameUpdateState CRenderProxy_Decal::UpdateOncePerFrame( const CRenderCollector& collector )
{
	const auto ret = IRenderProxyDrawable::UpdateOncePerFrame( collector );
	if ( ret == FUS_AlreadyUpdated )
		return ret;

	const Bool wasVisibleLastFrame = ( ret == FUS_UpdatedLastFrame );
	UpdateFade( collector, wasVisibleLastFrame );

	return ret;
}

void CRenderProxy_Decal::UpdateFade( const CRenderCollector& collector, const Bool wasVisibleLastFrame )
{
	// Fade in/out when we hit auto-hide distance.
	IRenderProxyFadeable::UpdateDistanceFade( GetCachedDistanceSquared() , m_autoHideDistanceSquared, wasVisibleLastFrame && !collector.WasLastFrameCameraInvalidated() );
}

void CRenderProxy_Decal::OnNotVisibleFromAutoHide( CRenderCollector& collector )
{
	// Update once per frame
	UpdateOncePerFrame( collector );
}


void CRenderProxy_Decal::CollectElements( CRenderCollector& collector )
{
	// Update once per frame
	UpdateOncePerFrame( collector );

	// Check fade, don't draw if we're faded out.
	if ( !IsFadeVisible() || m_alpha <= 0.0f )
	{
		return;
	}

	// Update the scene visibility query
	UpdateVisibilityQueryState( CRenderVisibilityQueryManager::VisibleScene );

	// Add to visibility list
	collector.PushDecal( this );
}

void CRenderProxy_Decal::Relink( const Box& boundingBox, const Matrix& localToWorld )
{
	IRenderProxyBase::Relink( boundingBox, localToWorld );
	CacheProjectionMatrix();
}

void CRenderProxy_Decal::RelinkTransformOnly( const Matrix& localToWorld )
{
	IRenderProxyBase::RelinkTransformOnly( localToWorld );
	CacheProjectionMatrix();
}

void CRenderProxy_Decal::RelinkBBoxOnly( const Box& boundingBox )
{
	IRenderProxyBase::RelinkBBoxOnly( boundingBox );
	CacheProjectionMatrix();
}

void CRenderProxy_Decal::CacheProjectionMatrix()
{
	// calculation of the decalWorldToUV matrix, idea:
	// 1) we have the world space positions of the pixels
	// 2) we have to build view matrix in a way, that camera will be on the "up" side of the box, facing towards the center
	// 3) projection matrix is the ortho matrix with width, height, near and far being the appropriate sizes of the box

	// parameters
	Vector scaleCashe = m_localToWorld.GetScale33();

	Vector position = m_localToWorld.GetTranslation() + m_localToWorld.GetAxisZ().Normalized3() * scaleCashe.Z / 2.0f;

	Matrix orthoCameraRotation = Matrix::IDENTITY;
	orthoCameraRotation.V[0] = m_localToWorld.GetAxisX().Normalized3();		// right vector
	orthoCameraRotation.V[1] = -m_localToWorld.GetAxisZ().Normalized3();	// forward vector
	orthoCameraRotation.V[2] = m_localToWorld.GetAxisY().Normalized3();		// up vector

	Float orthoW = scaleCashe.X;
	Float orthoH = scaleCashe.Y;
	Float nearPlane = 0.0f;
	Float farPlane = scaleCashe.Z;

	// world to screen from render camera
	CRenderCamera cam( position, orthoCameraRotation.ToEulerAngles(), 0.0f, orthoH / orthoW, nearPlane, farPlane, orthoW );
	Matrix wToS_RC = cam.GetWorldToScreen();

	Matrix clipSpaceToTextureSpace = Matrix::IDENTITY;
	clipSpaceToTextureSpace.V[0].X = m_horizontalFlip ? -m_atlasVector.X : m_atlasVector.X;
	clipSpaceToTextureSpace.V[1].Y = m_verticalFlip ? -m_atlasVector.Y : m_atlasVector.Y;
	clipSpaceToTextureSpace.SetTranslation( m_atlasVector.Z, m_atlasVector.W, 0.0f );

	m_cachedProjection = wToS_RC * clipSpaceToTextureSpace;

	// Cache clipping sub uv. This tells the shader the real using atlas's tile part
	// so he can cut off the pixels outside the tile. ZW is the middle if the tile , 
	// XY is the half size of the tile.
	m_cachedSubUVClip.X = m_atlasVector.Z - m_atlasVector.X;
	m_cachedSubUVClip.Y = m_atlasVector.W - m_atlasVector.Y;

	m_cachedSubUVClip.Z = m_atlasVector.Z + m_atlasVector.X;
	m_cachedSubUVClip.W = m_atlasVector.W + m_atlasVector.Y;
	m_tangent = orthoCameraRotation.V[0];
	m_up = -orthoCameraRotation.V[1];

}

void CRenderProxy_Decal::UpdateConstants()
{
	GetRenderer()->GetStateManager().SetLocalToWorld( &m_localToWorld );
	
	void* lockedConstantsData = GpuApi::LockBuffer( g_constants, GpuApi::BLF_Discard, 0, sizeof( SDecalConstants ) );
	RED_ASSERT( lockedConstantsData );
	SDecalConstants* lockedDecalConstants = static_cast< SDecalConstants* >( lockedConstantsData );
	
	// if m_effectParams is present, the m_effectParams->m_customParam0.X holds the normal threshold IN RADIANS, so no further conversions are necessary
	const Float threshold = m_effectParams ? m_effectParams->m_customParam0.X : m_normalThreshold;
	const Float finalAlpha = m_alpha * GetGenericFadeFraction();
	
	lockedDecalConstants->m_cachedProjection	=	m_cachedProjection.Transposed();
	lockedDecalConstants->m_cachedSubUVClip		=	m_cachedSubUVClip;
	lockedDecalConstants->m_specularColor		=	m_specularColor.ToVector();
	lockedDecalConstants->m_normalThreshold		=	threshold;
	lockedDecalConstants->m_decalFade			=	finalAlpha;
	lockedDecalConstants->m_scale				=	(1.0f / m_scale);
	lockedDecalConstants->m_specularity			=	m_specularity;
	lockedDecalConstants->m_diffuseColor		=	m_diffuseColor.ToVector();
	lockedDecalConstants->m_tangent				=	m_tangent;
	lockedDecalConstants->m_up					=	m_up;
	lockedDecalConstants->m_Zdepthfade			=	m_Zdepthfade;


	GpuApi::UnlockBuffer( g_constants );
}
