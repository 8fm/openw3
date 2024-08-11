/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataResourceLoader.h"
#include "sectorDataGenericRuntime.h"

#include "bitmapTexture.h"

#include "mesh.h"
#include "particleSystem.h"
#include "collisionMesh.h"
#include "collisionCache.h"
#include "decalComponent.h"

#include "renderer.h"
#include "renderProxy.h"
#include "renderProxyInterface.h"
#include "renderProxyIterator.h"
#include "renderCommands.h"

#include "../physics/physicsWorld.h"
#include "../physics/physicsSimpleBodyWrapper.h"
#include "physicsTileWrapper.h"

#include "../core/configVar.h"

#include "physicsDataProviders.h"

#include "terrainTile.h"
#include "clipMap.h"

namespace Config
{
	TConfigVar< Bool >	cvSectorAllowMeshes( "Streaming/Sectors/Filter", "AllowMeshes", true );
	TConfigVar< Bool >	cvSectorAllowCollision( "Streaming/Sectors/Filter", "AllowCollision", true );
	TConfigVar< Bool >	cvSectorAllowDimmers( "Streaming/Sectors/Filter", "AllowDimmers", true );
	TConfigVar< Bool >	cvSectorAllowDecals( "Streaming/Sectors/Filter", "AllowDecals", true );
	TConfigVar< Bool >	cvSectorAllowLights( "Streaming/Sectors/Filter", "AllowLights", true );
}

//----------------------------------------------------------------------------

CSectorDataObjectMesh::CSectorDataObjectMesh()
	: m_proxy( nullptr )
{
}

CSectorDataObjectMesh::EResult CSectorDataObjectMesh::Stream( const Context& context, const Bool asyncPipeline )
{
	// meshes are not allowed
	if ( !Config::cvSectorAllowMeshes.Get() )
		return eResult_NotReady;

	// important object ?
	const Bool isImportant = 0 != (GetObject().m_flags & SectorData::ePackedObjectFlag_Important);

	// load the mesh
	const auto& data = GetData();
	const auto ret = context.m_resourceLoader->PrefetchResource( data.m_mesh, isImportant ? +10 : 0 );
	if ( ret != CSectorDataResourceLoader::eResult_Loaded )
		return eResult_NotReady;

	// add ref count to the resource but only after it's ready to be used
	const auto mesh = context.m_resourceLoader->GetResourceAddRef( data.m_mesh );
	RED_FATAL_ASSERT( mesh != nullptr, "Resource lost altough it's reported as loaded" );

	// bind resource reference
	m_resourceToUse.Bind( context.m_resourceLoader, data.m_mesh );

	// prepare mesh info
	RenderProxyMeshInitData meshInfo;
	meshInfo.m_localToWorld = data.m_localToWorld.Unpack();
	meshInfo.m_occlusionId = data.m_occlusionId;
	meshInfo.m_mesh = static_cast< CMesh* >( mesh.Get() );
	meshInfo.m_cameraTransformRotate = data.HasFlag( SectorData::ePackedFlag_Mesh_CameraTransformRotate );	
	meshInfo.m_cameraTransformTranslate = data.HasFlag( SectorData::ePackedFlag_Mesh_CameraTransformTranslate );
	meshInfo.m_castingShadows = data.HasFlag( SectorData::ePackedFlag_Mesh_CastingShadows );
	meshInfo.m_castingShadowsFromLocalLightsOnly = data.HasFlag( SectorData::ePackedFlag_Mesh_CastingShadowsFromLocalLightsOnly );			
	meshInfo.m_noAutoHide = data.HasFlag( SectorData::ePackedFlag_Mesh_ForceNoAutohide );
	meshInfo.m_autoHideDistance = meshInfo.m_mesh->GetAutoHideDistance();
	meshInfo.m_fadeOnCameraCollision = data.HasFlag( SectorData::ePackedFlag_Mesh_FadeOnCameraCollision);
	meshInfo.m_castingShadowsAlways = data.HasFlag( SectorData::ePackedFlag_Mesh_CastingShadowsWhenNotVisible);
	meshInfo.m_visible = data.HasFlag( SectorData::ePackedFlag_Mesh_Visible );
	meshInfo.m_noFOVAdjustedLOD = data.HasFlag( SectorData::ePackedFlag_Mesh_PartOfEntityProxy ) || data.HasFlag( SectorData::ePackedFlag_Mesh_RootEntityProxy ); 
	meshInfo.m_forcedLODLevel = data.m_forcedLODLevel;
	meshInfo.m_lightChannels = data.m_lightChannels;
	meshInfo.m_shadowBias = data.m_shadowBias;
	meshInfo.m_useCustomReferencePoint = false;
	meshInfo.m_forceNoUmbraCulling = false;
	meshInfo.m_renderingPlane = data.m_renderingPlane;

	// Forced auto hide
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_AllowAutoHideOverride ) )
	{
		meshInfo.m_autoHideDistance = data.m_forcedAutoHide;
	}

	// Proxies and objects that turn into proxies should have fixed visibility reference distance
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_PartOfEntityProxy ) || data.HasFlag( SectorData::ePackedFlag_Mesh_RootEntityProxy ) )
	{
		meshInfo.m_useCustomReferencePoint = true;
		meshInfo.m_customReferencePoint = GetObject().m_pos;

		// HACK: until he Umbra issue with mesh proxies is resolved force no static culling on it
		if ( data.HasFlag( SectorData::ePackedFlag_Mesh_RootEntityProxy ) )
		{
			meshInfo.m_forceNoUmbraCulling = true;
		}
	}	

	// create rendering proxy
	RenderProxyInitInfo info;
	info.m_packedData = &meshInfo;

	// Create proxy
	RED_FATAL_ASSERT( m_proxy == nullptr, "Invalid state of object" );
	m_proxy = GRender->CreateProxy( info );

	// add proxy to scene - note that it may fail creation
	if ( m_proxy )
	{
		// Attach to rendering scene
		( new CRenderCommand_AddProxyToScene( context.m_renderScene, m_proxy ) )->Commit();

		// Fade in when first streamed in
		if ( !data.HasFlag( SectorData::ePackedFlag_Mesh_NoDissolves ) )
		{
			( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeInStart ) )->Commit();
		}
	}

	// proxy got created
	return eResult_Finished;
}

CSectorDataObjectMesh::EResult CSectorDataObjectMesh::Unstream( const Context& context, const Bool asyncPipeline )
{
	// remove the proxy
	if ( m_proxy != nullptr )
	{
		// Remove from rendering scene
		if ( GetData().HasFlag( SectorData::ePackedFlag_Mesh_NoDissolves ) || context.m_instantUnload )
		{
			( new CRenderCommand_RemoveProxyFromScene( context.m_renderScene, m_proxy ) )->Commit();
		}
		else
		{
			( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeOutAndDestroy ) )->Commit();
		}

		// Delete local proxy reference
		m_proxy->Release();
		m_proxy= nullptr;
	}

	// proxy got destroyed
	return eResult_Finished;
}

//----------------------------------------------------------------------------

CSectorDataObjectCollision::CSectorDataObjectCollision()
	: m_physicsBodyIndex( - 1 )
	, m_invalidAreaID( 0 )
	, m_invalidAreaPhysicsWorld( nullptr )
{
}

CSectorDataObjectCollision::~CSectorDataObjectCollision()
{
	ReleaseInvalidArea();
}

CPhysicsTileWrapper* CSectorDataObjectCollision::CreatePhysicsTileForPos( const Context& context, const Vector2& pos ) const
{
	CPhysicsTileWrapper* tileWrapper = nullptr;
	const CClipMap* terrainClipMap = context.m_terrainClipMap;

	if( terrainClipMap )
	{
		Int32 x = 0;
		Int32 y = 0;
		CTerrainTile* terrainTile = terrainClipMap->GetTileFromPosition( pos, x, y );
		if( terrainTile && terrainTile->IsCollisionEnabled() )
		{
			Box area = terrainClipMap->GetBoxForTile( x, y, 0.0f );
			Box2 box = Box2( ( area.Min ).AsVector2(), area.Max.AsVector2() );
			tileWrapper = context.m_physicsScene->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( nullptr ), context.m_physicsScene, box );
		}
	}

	return tileWrapper;
}

void CSectorDataObjectCollision::CreateInvalidAreaIfNotThere( const Context& context )
{
	// already created
	if ( m_invalidAreaID != 0 )
		return;

	// mark region as not yet ready
	const auto& data = GetData();
	const Box localBox = context.m_resourceLoader->GetResourceBoxNoRef( data.m_mesh );
	m_invalidAreaID = context.m_physicsScene->GetInvalidAreaCache().AddBox( localBox, data.m_localToWorld.Unpack() );
	m_invalidAreaPhysicsWorld = context.m_physicsScene;
}

void CSectorDataObjectCollision::ReleaseInvalidArea()
{
	if ( m_invalidAreaID != 0 )
	{
		m_invalidAreaPhysicsWorld->GetInvalidAreaCache().RemoveBox( m_invalidAreaID );

		m_invalidAreaPhysicsWorld = nullptr;
		m_invalidAreaID = 0;
	}
}

CSectorDataObjectCollision::EResult CSectorDataObjectCollision::Stream( const Context& context, const Bool asyncPipeline )
{
	// collision is not allowed
	if ( !Config::cvSectorAllowCollision.Get() )
		return eResult_NotReady;

	// load the collision
	const auto& data = GetData();
	if ( !m_collisionToUse )
	{
		// load the mesh
		const auto ret = context.m_resourceLoader->PrefetchCollision( data.m_mesh );
		if ( ret == CSectorDataResourceLoader::eResult_Failed )
		{
			// we don't have the collision in the collision cache, assume the object is streamed
			ReleaseInvalidArea(); // this is needed because we may initially though that we will have collision but for example the deseralization has failed
			return eResult_Finished;
		}
		else if ( ret != CSectorDataResourceLoader::eResult_Loaded )
		{
			CreateInvalidAreaIfNotThere( context );
			return eResult_NotReady;
		}

		// extract collision to use
		m_collisionToUse = context.m_resourceLoader->GetCollisionAddRef( data.m_mesh );

		// bind resource reference
		if ( m_collisionToUse )
		{
			m_resourceToUse.Bind( context.m_resourceLoader, data.m_mesh );
		}
	}

	// TODO FOR W3 (for LukaszZ): making this work asynchronously would save a lot of time on the main thread
	// Right now we cannot add new collision object to scene asynchronously, we need to deffer it and call shit from main thread
	if ( asyncPipeline )
		return eResult_RequiresSync;

	Vector2 pos = data.m_localToWorld.GetTranslation();

	// if we need collision for this object make sure the physical tile exists
	CPhysicsTileWrapper* tileWrapper = context.m_physicsScene->GetTerrainTileWrapper( pos );
	if ( !tileWrapper )
	{
		if( context.m_forceStream )
		{			
			tileWrapper = CreatePhysicsTileForPos( context, pos );
		}
		
		if ( !tileWrapper )
		{
			// there's no terrain there yet, do not create geometry
			CreateInvalidAreaIfNotThere( context );
			return eResult_NotReady;
		}
	}

	// we already have collision loaded
	if ( m_collisionToUse )
	{
		// create physical body
		m_physicsBodyIndex = tileWrapper->AddStaticBody( CPhysicsWrapperParentResourceProvider( GGame->GetActiveWorld().Get() ), data.m_localToWorld.Unpack(), m_collisionToUse, data.m_collisionMask, data.m_collisionGroup );
	}

	// body created
	ReleaseInvalidArea();
	return eResult_Finished;
}

CSectorDataObjectCollision::EResult CSectorDataObjectCollision::Unstream( const Context& context, const Bool asyncPipeline )
{
	// always release invalid area
	ReleaseInvalidArea();

	// collision cannot be release from asynchronous task
	if ( asyncPipeline )
		return eResult_RequiresSync;

	// remove from physical scene
	if ( m_physicsBodyIndex != -1 )
	{
		CPhysicsTileWrapper* tileWrapper = context.m_physicsScene->GetTerrainTileWrapper( GetData().m_localToWorld.GetTranslation() );
		if ( tileWrapper != nullptr )
		{
			tileWrapper->Release( m_physicsBodyIndex );
		}

		// cleanup
		m_physicsBodyIndex = -1;
	}

	// object removed
	return eResult_Finished;
}

//----------------------------------------------------------------------------

CSectorDataObjectDecal::CSectorDataObjectDecal()
	: m_proxy( nullptr )
{
}

CSectorDataObjectDecal::EResult CSectorDataObjectDecal::Stream( const Context& context, const Bool asyncPipeline )
{
	// decals are not allowed
	if ( !Config::cvSectorAllowDecals.Get() )
		return eResult_NotReady;

	// load the diffuse texture
	const auto& data = GetData();
	const auto ret = context.m_resourceLoader->PrefetchResource( data.m_diffuseTexture );
	if ( ret != CSectorDataResourceLoader::eResult_Loaded )
		return eResult_NotReady;

	// add ref count to the resource but only after it's ready to be used
	const auto texture = context.m_resourceLoader->GetResourceAddRef( data.m_diffuseTexture );
	RED_FATAL_ASSERT( texture != nullptr, "Resource lost altough it's reported as loaded" );

	// bind resource reference
	m_resourceToUse.Bind( context.m_resourceLoader, data.m_diffuseTexture );

	// prepare decal info
	RenderProxyDecalInitData proxyInfo;
	proxyInfo.m_localToWorld = data.m_localToWorld.Unpack();
	proxyInfo.m_occlusionId = data.m_occlusionId;
	proxyInfo.m_autoHideDistance = GetObject().m_radius < 0.0f ? GetObject().m_radius : ( GetObject().m_radius - CDecalComponent::AUTOHIDE_MARGIN );
	proxyInfo.m_texture = static_cast< CBitmapTexture* >( texture.Get() );
	proxyInfo.m_specularColor = data.m_specularColor;
	proxyInfo.m_normalThreshold = data.m_normalThreshold;
	proxyInfo.m_specularity = data.m_specularity;
	proxyInfo.m_verticalFlip = 0 != (data.m_flags & SectorData::ePackedFlag_Decal_VerticalFlip);
	proxyInfo.m_horizontalFlip = 0 != (data.m_flags & SectorData::ePackedFlag_Decal_HorizontalFlip);
	proxyInfo.m_fadeTime = data.m_fadeTime;

	// create rendering proxy
	RenderProxyInitInfo info;
	info.m_packedData = &proxyInfo;

	// Create proxy
	m_proxy = GRender->CreateProxy( info );
	if ( m_proxy )
	{
		// Attach to rendering scene
		( new CRenderCommand_AddProxyToScene( context.m_renderScene, m_proxy ) )->Commit();
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeInStart ) )->Commit();
	}

	// proxy got created
	return eResult_Finished;
}

CSectorDataObjectDecal::EResult CSectorDataObjectDecal::Unstream( const Context& context, const Bool asyncPipeline )
{
	// remove the proxy
	if ( m_proxy != nullptr )
	{
		// Remove from rendering scene
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeOutAndDestroy ) )->Commit();

		// Delete local proxy reference
		m_proxy->Release();
		m_proxy= nullptr;
	}

	// proxy got destroyed
	return eResult_Finished;
}

//----------------------------------------------------------------------------

CSectorDataObjectDimmer::CSectorDataObjectDimmer()
	: m_proxy( nullptr )
{
}

CSectorDataObjectDimmer::EResult CSectorDataObjectDimmer::Stream( const Context& context, const Bool asyncPipeline )
{
	// dimmers are not allowed
	if ( !Config::cvSectorAllowDimmers.Get() )
		return eResult_NotReady;

	// get source data
	const auto& data = GetData();

	// prepare dimmer info
	RenderProxyDimmerInitData proxyInfo;
	proxyInfo.m_localToWorld = data.m_localToWorld.Unpack();
	proxyInfo.m_occlusionId = data.m_occlusionId;
	proxyInfo.m_ambientLevel = data.m_ambientLevel;
	proxyInfo.m_marginFactor = data.m_marginFactor;
	proxyInfo.m_dimmerType = data.m_dimmerType;
	proxyInfo.m_autoHideDistance = GetObject().m_radius + 5.0f; // hack
	proxyInfo.m_areaMarker = 0 != (data.m_flags & SectorData::ePackedFlag_Dimmer_AreaMarker);

	// create rendering proxy
	RenderProxyInitInfo info;
	info.m_packedData = &proxyInfo;

	// Create proxy
	m_proxy = GRender->CreateProxy( info );

	// add proxy to scene
	if ( m_proxy )
	{
		// Attach to rendering scene
		( new CRenderCommand_AddProxyToScene( context.m_renderScene, m_proxy ) )->Commit();
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeInStart ) )->Commit();
	}

	// proxy got created
	return eResult_Finished;
}

CSectorDataObjectDimmer::EResult CSectorDataObjectDimmer::Unstream( const Context& context, const Bool asyncPipeline )
{
	// remove the proxy
	if ( m_proxy != nullptr )
	{
		// Remove from rendering scene
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeOutAndDestroy ) )->Commit();

		// Delete local proxy reference
		m_proxy->Release();
		m_proxy= nullptr;
	}

	// proxy got destroyed
	return eResult_Finished;
}

//----------------------------------------------------------------------------

CSectorDataObjectPointLight::CSectorDataObjectPointLight()
	: m_proxy( nullptr )
{
}

CSectorDataObjectPointLight::EResult CSectorDataObjectPointLight::Stream( const Context& context, const Bool asyncPipeline )
{
	// dimmers are not allowed
	if ( !Config::cvSectorAllowLights.Get() )
		return eResult_NotReady;

	// source data
	const auto& data = GetData();

	// prepare light info
	RenderProxyPointLightInitData proxyInfo;
	proxyInfo.m_localToWorld = data.m_localToWorld.Unpack();
	proxyInfo.m_occlusionId = data.m_occlusionId;
	// streaming radius is a sum of autohide, radius and hiderange of light, keep this in sync with sectorDataBuilder.cpp
	proxyInfo.m_autoHideDistance = GetObject().m_radius - data.m_autoHideRange - data.m_radius;
	proxyInfo.m_color = data.m_color;
	proxyInfo.m_radius = data.m_radius;
	proxyInfo.m_brightness = data.m_brightness;
	proxyInfo.m_attenuation = data.m_attenuation;
	proxyInfo.m_autoHideRange = data.m_autoHideRange;
	proxyInfo.m_shadowFadeDistance = data.m_shadowFadeDistance;
	proxyInfo.m_shadowFadeRange = data.m_shadowFadeRange;
	proxyInfo.m_shadowBlendFactor = data.m_shadowBlendFactor;
	proxyInfo.m_lightFlickering = data.m_lightFlickering;;
	proxyInfo.m_shadowCastingMode = data.m_shadowCastingMode;
	proxyInfo.m_dynamicShadowsFaceMask = 0x3F; 
	proxyInfo.m_envColorGroup = data.m_envColorGroup;
	proxyInfo.m_lightUsageMask = data.m_lightUsageMask;
	proxyInfo.m_allowDistanceFade = 0 != (data.m_flags & SectorData::ePackedFlag_Light_AllowDistanceFade);

	// create rendering proxy
	RenderProxyInitInfo info;
	info.m_packedData = &proxyInfo;

	// Create proxy
	m_proxy = GRender->CreateProxy( info );

	// add proxy to scene
	if ( m_proxy )
	{
		// Attach to rendering scene
		( new CRenderCommand_AddProxyToScene( context.m_renderScene, m_proxy ) )->Commit();
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeInStart ) )->Commit();
	}

	// proxy got created
	return eResult_Finished;
}

CSectorDataObjectPointLight::EResult CSectorDataObjectPointLight::Unstream( const Context& context, const Bool asyncPipeline )
{
	// remove the proxy
	if ( m_proxy != nullptr )
	{
		// Remove from rendering scene
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeOutAndDestroy ) )->Commit();

		// Delete local proxy reference
		m_proxy->Release();
		m_proxy= nullptr;
	}

	// proxy got destroyed
	return eResult_Finished;
}

//----------------------------------------------------------------------------

CSectorDataObjectSpotLight::CSectorDataObjectSpotLight()
	: m_proxy( nullptr )
{
}

CSectorDataObjectSpotLight::EResult CSectorDataObjectSpotLight::Stream( const Context& context, const Bool asyncPipeline )
{
	// dimmers are not allowed
	if ( !Config::cvSectorAllowLights.Get() )
		return eResult_NotReady;

	// source data
	const auto& data = GetData();

	// precache projection texture
	THandle< CResource > projectionTexture;
	if ( data.m_flags & SectorData::ePackedFlag_Light_HasProjectionTexture )
	{
		// load the diffuse texture
		const auto& data = GetData();
		const auto ret = context.m_resourceLoader->PrefetchResource( data.m_projectionTexture );
		if ( ret != CSectorDataResourceLoader::eResult_Loaded )
			return eResult_NotReady;

		// add ref count to the resource but only after it's ready to be used
		projectionTexture = context.m_resourceLoader->GetResourceAddRef( data.m_projectionTexture );
		RED_FATAL_ASSERT( projectionTexture != nullptr, "Resource lost altough it's reported as loaded" );

		// keep reference to resource
		m_resourceToUse.Bind( context.m_resourceLoader, data.m_projectionTexture );
	}

	// prepare light info
	RenderProxySpotLightInitData proxyInfo;
	proxyInfo.m_localToWorld = data.m_localToWorld.Unpack();
	proxyInfo.m_occlusionId = data.m_occlusionId;
	// streaming radius is a sum of autohide, radius and hiderange of light, keep this in sync with sectorDataBuilder.cpp
	proxyInfo.m_autoHideDistance = GetObject().m_radius - data.m_radius - data.m_autoHideRange;
	proxyInfo.m_color = data.m_color;
	proxyInfo.m_radius = data.m_radius;
	proxyInfo.m_brightness = data.m_brightness;
	proxyInfo.m_attenuation = data.m_attenuation;
	proxyInfo.m_autoHideRange = data.m_autoHideRange;
	proxyInfo.m_shadowFadeDistance = data.m_shadowFadeDistance;
	proxyInfo.m_shadowFadeRange = data.m_shadowFadeRange;
	proxyInfo.m_shadowBlendFactor = data.m_shadowBlendFactor;
	proxyInfo.m_lightFlickering = data.m_lightFlickering;;
	proxyInfo.m_shadowCastingMode = data.m_shadowCastingMode;
	proxyInfo.m_dynamicShadowsFaceMask = 0x3F; 
	proxyInfo.m_envColorGroup = data.m_envColorGroup;
	proxyInfo.m_lightUsageMask = data.m_lightUsageMask;
	proxyInfo.m_allowDistanceFade = 0 != (data.m_flags & SectorData::ePackedFlag_Light_AllowDistanceFade);

	// spot
	proxyInfo.m_innerAngle = data.m_innerAngle;
	proxyInfo.m_outerAngle = data.m_outerAngle;;
	proxyInfo.m_softness = data.m_softness;
	proxyInfo.m_projectionTextureAngle = data.m_projectionTextureAngle;
	proxyInfo.m_projectionTexureUBias = data.m_projectionTexureUBias;
	proxyInfo.m_projectionTexureVBias = data.m_projectionTexureVBias;
	proxyInfo.m_projectionTexture = static_cast< CBitmapTexture* >( projectionTexture.Get() );

	// create rendering proxy
	RenderProxyInitInfo info;
	info.m_packedData = &proxyInfo;

	// Create proxy
	m_proxy = GRender->CreateProxy( info );

	// add proxy to scene
	if ( m_proxy )
	{
		// Attach to rendering scene
		( new CRenderCommand_AddProxyToScene( context.m_renderScene, m_proxy ) )->Commit();
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeInStart ) )->Commit();
	}

	// proxy got created
	return eResult_Finished;
}

CSectorDataObjectSpotLight::EResult CSectorDataObjectSpotLight::Unstream( const Context& context, const Bool asyncPipeline )
{
	// remove the proxy
	if ( m_proxy != nullptr )
	{
		// Remove from rendering scene
		( new CRenderCommand_SetAutoFade( context.m_renderScene, m_proxy, FT_FadeOutAndDestroy ) )->Commit();

		// Delete local proxy reference
		m_proxy->Release();
		m_proxy= nullptr;
	}

	// proxy got destroyed
	return eResult_Finished;
}

//----------------------------------------------------------------------------
