/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecal.h"
#include "renderDynamicDecalTarget.h"
#include "renderDynamicDecalChunk.h"
#include "renderProxyDrawable.h"
#include "renderProxyDecal.h"
#include "renderTexture.h"
#include "renderElementMap.h"
#include "renderScene.h"
#include "../engine/component.h"
#include "../engine/renderSettings.h"


//////////////////////////////////////////////////////////////////////////
// TODO : Use proper texture distance for prefetch and rendering.
// For rendering, might need to split up the texture binds from the constant updates, so we can still avoid rewriting
// the constant buffer when batching by decal, but also set appropriate texture distances.
// Leaving it for now, because we need to get this texture stuff in!
//////////////////////////////////////////////////////////////////////////

Red::Threads::CMutex CRenderDynamicDecal::m_internalMutex;


CRenderDynamicDecal::CRenderDynamicDecal( const SDynamicDecalInitInfo& initInfo )
	: m_initInfo( initInfo )
	, m_staticDecal( nullptr )
{
	// SDynamicDecalInitInfo doesn't hold references to these, so we'll do it ourselves
	if ( m_initInfo.m_diffuseTexture != nullptr )
	{
		m_initInfo.m_diffuseTexture->AddRef();
	}
	if ( m_initInfo.m_normalTexture != nullptr )
	{
		m_initInfo.m_normalTexture->AddRef();
	}

	m_parentToDecal = m_initInfo.GetWorldToDecalMatrix();
	m_lifetime = m_initInfo.m_timeToLive;

	// Resolve auto-hide distance the same as a normal decal. Applies a default value based on bounding box for -1.
	m_initInfo.m_autoHideDistance = CComponent::CalculateAutoHideDistance( m_initInfo.m_autoHideDistance, m_initInfo.GetWorldBounds(), Config::cvDynDecalsHideDistance.Get(), 80.0f );
	// Clamp fade values so we dont divide by 0
	m_initInfo.m_fadeTime	= Max(m_initInfo.m_fadeTime , 0.001f);
}

CRenderDynamicDecal::~CRenderDynamicDecal()
{
	RED_ASSERT( m_chunks.Empty(), TXT("Dynamic decal destroyed, but still has chunks!") );
	RED_ASSERT( m_staticDecal == nullptr, TXT("Dynamic decal destroyed, but it still has its static decal") );

	SAFE_RELEASE( m_initInfo.m_diffuseTexture );
	SAFE_RELEASE( m_initInfo.m_normalTexture );
}


String CRenderDynamicDecal::GetDisplayableName() const
{
	return TXT("Dynamic Decal");
}

CName CRenderDynamicDecal::GetCategory() const
{
	return RED_NAME( RenderDynamicDecal );
}

Uint32 CRenderDynamicDecal::GetUsedVideoMemory() const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_internalMutex );

	Uint32 totalSize = 0;

	for ( Uint32 i = 0; i < m_chunks.Size(); ++i )
	{
		totalSize += m_chunks[i]->GetUsedVideoMemory();
	}

	return totalSize;
}

void CRenderDynamicDecal::OnDeviceLost()
{
}

void CRenderDynamicDecal::OnDeviceReset()
{
}


Box CRenderDynamicDecal::GetInitialBounds() const
{
	return m_initInfo.GetWorldBounds();
}


void CRenderDynamicDecal::Prefetch( CRenderFramePrefetch* prefetch ) const
{
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_initInfo.m_diffuseTexture ), 0.0f );
	prefetch->AddTextureBind( static_cast< CRenderTextureBase* >( m_initInfo.m_normalTexture ), 0.0f );
}


Bool CRenderDynamicDecal::DestroyChunks()
{
	DestroyDecal();
	return m_staticDecal == nullptr;
}


void CRenderDynamicDecal::AddChunk( CRenderDynamicDecalChunk* chunk )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_internalMutex );

	RED_ASSERT( chunk != nullptr, TXT("Trying to add null chunk to decal") );
	RED_ASSERT( !m_chunks.Exist( chunk ), TXT("Decal Chunk already found in decal") );
	RED_ASSERT( chunk->m_ownerDecal == this, TXT("Adding chunk to decal, but chunk is owned by a different decal") );

	chunk->AddRef();
	m_chunks.PushBack( chunk );
}

void CRenderDynamicDecal::DestroyDecal()
{
	RED_ASSERT( !m_scene->IsRendering(), TXT("Cannot destroy a dynamic decal mid-render!") );

	// Keep destroying chunks until they're all gone! DestroyDecalChunk will call OnDynamicDecalChunkDestroyed, and so
	// will remove the chunk in question.
	while ( m_chunks.Size() > 0 )
	{
		m_chunks[0]->DestroyDecalChunk();
	}
}


void CRenderDynamicDecal::OnDynamicDecalChunkDestroyed( CRenderDynamicDecalChunk* chunk )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_internalMutex );

	if ( m_chunks.RemoveFast( chunk ) )
	{
		// Unregister with the elements map.
		m_scene->GetRenderElementMap()->UnregisterDynamicDecalChunk( chunk );

		chunk->m_ownerDecal = nullptr;
		chunk->Release();
	}
}


void CRenderDynamicDecal::CollectChunks( CRenderCollector& collector )
{
	if ( m_initInfo.m_timeToLive <= 0.0f )
	{
		return;
	}

	const Vector& cameraPos = collector.m_frame->GetFrameInfo().m_camera.GetPosition();
	const Float cameraFovMultiplier = collector.GetRenderCamera().GetFOVMultiplier();

	// Divide autohide by fov multiplier instead of the usual multiply camera distance, to avoid multiplying on every chunk.
	const Float autoHideSq = m_initInfo.m_autoHideDistance * m_initInfo.m_autoHideDistance / cameraFovMultiplier;

	for ( CRenderDynamicDecalChunk* chunk : m_chunks )
	{
		const Vector pos = chunk->GetBoundingBox().CalcCenter();
		const Float distSq = cameraPos.DistanceSquaredTo( pos );
		if ( distSq < autoHideSq )
		{
			collector.PushDynamicDecalChunk( chunk );
		}
	}
}


void CRenderDynamicDecal::RegisterChunks()
{
	CRenderElementMap* elementsMap = m_scene->GetRenderElementMap();
	for ( Uint32 i = 0; i < m_chunks.Size(); ++i )
	{
		elementsMap->RegisterDynamicDecalChunk( m_chunks[i] );
	}
}


void CRenderDynamicDecal::AttachToScene( CRenderSceneEx* scene, Bool createStaticDecal )
{
	// spawn static decal
	if ( createStaticDecal )
	{
		m_staticDecal = new CRenderProxy_Decal( m_initInfo );
		m_staticDecal->SetScale( GetScale() );
		m_staticDecal->SetAlpha( GetAlpha() );
		scene->AddProxy( m_staticDecal );
	}

	m_scene = scene;
	RegisterChunks();
}

void CRenderDynamicDecal::DetachFromScene( CRenderSceneEx* scene )
{
	// Don't have to unregister chunks from ElementsMap, since that happens when the chunks are destroyed.

	// remove static decal
	if ( m_staticDecal != nullptr )
	{
		scene->RemoveProxy( m_staticDecal );

		SAFE_RELEASE( m_staticDecal );
	}
}


Bool CRenderDynamicDecal::Tick( CRenderSceneEx* scene, Float timeDelta )
{
	// If we have no chunks, we can be destroyed.
	if ( m_chunks.Size() == 0 && m_staticDecal == nullptr )
	{
		m_initInfo.m_timeToLive = 0.0f;
		return false;
	}

	if ( m_initInfo.m_timeToLive > 0.0f )
	{
		m_initInfo.m_timeToLive -= timeDelta;
	
		if ( m_staticDecal != nullptr )
		{
			m_staticDecal->SetScale( GetScale() );
			m_staticDecal->SetAlpha( GetAlpha() );
		}

		// When TTL reaches 0, we don't need to do anything. Scene will clear out any dead decals itself.
		return true;
	}

	return false;
}


void CRenderDynamicDecal::BindGenerateParameters( const Matrix& worldToDecal ) const
{
	//#define WorldToDecal		VSC_Custom_Matrix
	//#define NormalFadeBias	(VSC_Custom_0.x)
	//#define NormalFadeScale	(VSC_Custom_0.y)
	//#define DepthFadePower	(VSC_Custom_0.z)
	//#define DepthFadeOrigin	(VSC_Custom_0.w)
	//#define TwoSided			(VSC_Custom_1.x != 0.0f)
	//#define Ellipsoidal		(VSC_Custom_1.y != 0.0f)
	//#define DecalDirection	(VSC_Custom_2.xyz)

	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_Matrix, worldToDecal );

	// Get the decal's direction from the world-to-decal matrix. Shader expects it to be normalized.
	Matrix decalToWorld = worldToDecal;
	decalToWorld.SetTranslation(0, 0, 0);
	decalToWorld.FullInvert();
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_2, decalToWorld.GetAxisY().Normalized3() );


	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector(
		m_initInfo.m_normalFadeBias,
		m_initInfo.m_normalFadeScale,
		m_initInfo.m_depthFadePower,
		-m_initInfo.m_nearZ / ( m_initInfo.m_farZ - m_initInfo.m_nearZ )	// Z value in decal space corresponding to the decal's origin
		) );
	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_1, Vector(
		IsDoubleSided() ? 1.0f : 0.0f,
		IsEllipsoid() ? 1.0f : 0.0f,
		0.0f,
		0.0f
		) );
}

void CRenderDynamicDecal::BindRenderParameters() const
{
	// Set up some common state used by all types of decals.

	if ( m_initInfo.m_diffuseTexture != nullptr )
	{
		static_cast< CRenderTexture* >( m_initInfo.m_diffuseTexture )->Bind( 0, GpuApi::SAMPSTATEPRESET_ClampLinearMip );
	}
	else
	{
		GpuApi::BindTextures( 0, 1, nullptr, GpuApi::PixelShader );
	}

	if ( m_initInfo.m_normalTexture != nullptr )
	{
		static_cast< CRenderTexture* >( m_initInfo.m_normalTexture )->Bind( 1, GpuApi::SAMPSTATEPRESET_ClampLinearMip );
	}
	else
	{
		GpuApi::BindTextures( 1, 1, nullptr, GpuApi::PixelShader );
	}


	// TODO : Instead of rewriting the *_Custom_* constants for every decal, the decal could have its own cbuffer. Would still need to
	// update with alpha, but that would be just once per frame.


	//#define ClippingEllipseMatrix		VSC_Custom_Matrix
	//#define DecalFade					VSC_Custom_0.x

	//#define SpecularColor				PSC_Custom_0.xyz
	//#define SpecularScale				PSC_Custom_1.x
	//#define SpecularBase				PSC_Custom_1.y
	//#define AdditiveNormals			(PSC_Custom_1.z != 0.0f)

	const Float alpha = GetAlpha();


	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector( alpha, 0, 0, 0 ) );
		
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, m_initInfo.m_specularColor.ToVector() );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector(
		m_initInfo.m_specularScale,
		m_initInfo.m_specularBase,
		m_initInfo.m_additiveNormals ? 1.0f : 0.0f,
		0.0f
		) );

	Vector diffuseColor = m_initInfo.m_diffuseColor.ToVector();

	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, diffuseColor );
}

Float CRenderDynamicDecal::GetScale() const
{
	return Lerp( 1.0f - m_initInfo.m_startScale , 1.0f , Clamp(  (m_lifetime - m_initInfo.m_timeToLive) / m_initInfo.m_scaleTime , 0.0f , 1.0f ));
}

Float CRenderDynamicDecal::GetAlpha() const
{
	Float fadeInAlpha = m_initInfo.m_fadeInTime == 0.0f ? 1.0f : Clamp<Float>( (m_lifetime - m_initInfo.m_timeToLive) / m_initInfo.m_fadeInTime, 0.0f, 1.0f );
	return fadeInAlpha * Clamp( m_initInfo.m_timeToLive / m_initInfo.m_fadeTime, 0.0f, 1.0f );
}



IRenderResource* CRenderInterface::CreateDynamicDecal( const SDynamicDecalInitInfo& info )
{
	return new CRenderDynamicDecal( info );
}
