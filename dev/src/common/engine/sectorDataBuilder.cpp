/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorData.h"
#include "sectorDataBuilder.h"

#ifndef NO_EDITOR

#include "mesh.h"
#include "bitmapTexture.h"
#include "meshComponent.h"
#include "staticMeshComponent.h"
#include "rigidMeshComponent.h"
#include "particleComponent.h"
#include "particleSystem.h"
#include "dimmerComponent.h"
#include "decalComponent.h"
#include "pointLightComponent.h"
#include "spotLightComponent.h"

CSectorDataBuilder::CSectorDataBuilder( CSectorData* data )
	: m_data( data )
{
	// allocate empty resource (should not be used)
	CSectorData::PackedResource emptyRes;
	Red::MemoryZero( &emptyRes, sizeof(emptyRes) );
	m_data->m_resources.PushBack(emptyRes);

	// reserve some space
	m_data->m_dataStream.Reserve( 1024*128 );
}

CSectorDataBuilder::~CSectorDataBuilder()
{
}

Bool CSectorDataBuilder::ExtractComponent( const class CComponent* component )
{
	// component that is attached to something cannot be extracted
	if ( !component->GetParentAttachments().Empty() || !component->GetChildAttachments().Empty() )
		return false;

	// component extraction chain
	if ( component->IsExactlyA< CMeshComponent >() || component->IsExactlyA< CStaticMeshComponent >() )
	{
		return ExtractMeshComponent( static_cast< const CMeshComponent* >( component ) );
	}
	else if ( component->IsExactlyA< CDimmerComponent >() )
	{
		return ExtractDimmerComponent( static_cast< const CDimmerComponent* >( component ) );
	}
	else if ( component->IsExactlyA< CDecalComponent >() )
	{
		return ExtractDecalComponent( static_cast< const CDecalComponent* >( component ) );
	}
	else if ( component->IsExactlyA< CPointLightComponent >() )
	{
		return ExtractPointLightComponent( static_cast< const CPointLightComponent * >( component ) );
	}
	else if ( component->IsExactlyA< CSpotLightComponent >() )
	{
		return ExtractSpotLightComponent( static_cast< const CSpotLightComponent * >( component ) );
	}
	else if ( component->IsExactlyA< CRigidMeshComponent >() )
	{
		return ExtractRigidMeshComponent( static_cast< const CRigidMeshComponent* >( component ) );
	}
	else if ( component->IsExactlyA< CParticleComponent >() )
	{
		return ExtractParticleComponent( static_cast< const CParticleComponent* >(component ) );
	}

	// not extracted
	return false;
}

void CSectorDataBuilder::GetUsedResources( TDynArray< String >& outResources ) const
{
	outResources = m_resources;
}

Bool CSectorDataBuilder::ExtractMeshComponent( const class CMeshComponent* mc )
{
	// no mesh, we can still extract it any way - this way we delete components with no meshes
	THandle< CMesh > mesh  = mc->GetMeshNow();
	if ( !mesh )
	{
		return true; // report as extracted
	}

	// do we have a component in the entity that is a mesh proxy ?
	Float rootEntityProxyComponentVisibilityDistance = 75.0f;
	CMeshComponent* rootEntityProxyComponent = nullptr;
	{
		for ( CComponent* otherComp : mc->GetEntity()->GetComponents() )
		{
			CMeshComponent* otherMC = Cast< CMeshComponent >( otherComp );
			if ( otherMC )
			{
				THandle< CMesh > otherMesh = otherMC->GetMeshNow();
				if ( otherMesh && otherMesh->IsEntityProxy() )
				{
					// get the distance of LOD1 from the proxy mesh
					if ( otherMesh->GetNumLODLevels() != 2 )
					{
						ERR_ENGINE( TXT("Entity proxy mesh '%ls' does not have 2 LOD levels"), otherMesh->GetDepotPath().AsChar() );
					}
					else
					{
						rootEntityProxyComponentVisibilityDistance = otherMesh->GetLODLevel(1).GetDistance();
					}

					rootEntityProxyComponent = otherMC;
					break;
				}
			}
		}
	}

	// Basic data
	SectorData::PackedMesh info;
	Red::System::MemoryZero( &info, sizeof( info ) );
	info.m_mesh = MapResource( mesh );
	info.m_occlusionId = GetHash( mesh->GetDepotPath() );
	info.m_localToWorld.Pack( mc->GetLocalToWorld() );
	info.m_lightChannels = mc->GetLightChannels();
	info.m_forcedLODLevel = (Int8)mc->GetForcedLODLevel();
	info.m_shadowBias = 0;
	info.m_flags = 0;

	if ( mc->HasForcedAutoHideDistance() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_AllowAutoHideOverride;
		info.m_forcedAutoHide = (Uint16) Red::Math::MRound( mc->GetAutoHideDistance() );
	}

	// Mesh is casting shadows
	if ( mc->IsCastingShadows() )
	{
		info.m_flags |=  SectorData::ePackedFlag_Mesh_CastingShadows;
		info.m_shadowBias = (Int8) mc->GetShadowImportanceBias();
	}

	// Mesh is visible
	if ( mc->IsVisible() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_Visible;
	}

	// Mesh is casting shadows when not visible
	if ( mc->IsCastingShadowsEvenIfNotVisible() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_CastingShadowsWhenNotVisible;
		info.m_shadowBias = (Int8) mc->GetShadowImportanceBias();
	}

	// Mesh is casting shadows when not visible
	if ( mc->IsCastingShadowsFromLocalLightsOnly() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_CastingShadowsFromLocalLightsOnly;		
		info.m_shadowBias = (Int8) mc->GetShadowImportanceBias();
	}	

	// Mesh is transformed by camera
	if ( mc->IsCameraTransformComponentWithoutRotation() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_CameraTransformTranslate;

		// restore position
		Matrix localToWorld;
		mc->CalcLocalTransformMatrix( localToWorld );
		info.m_localToWorld.Pack( localToWorld );
	}

	// Mesh is transformed by camera
	if ( mc->IsCameraTransformComponentWithRotation() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_CameraTransformRotate;

		// hack
		Matrix localToWorld;
		mc->CalcLocalTransformMatrix( localToWorld );
		info.m_localToWorld.Pack( localToWorld );
	}

	// Force no autohide
	if ( mc->IsForceNoAutohide() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_ForceNoAutohide;
	}

	if ( mc->GetDrawableFlags() & DF_NoDissolves )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_NoDissolves;
	}

	// Rendering plane data
	info.m_renderingPlane = static_cast< Uint8 >( mc->GetRenderingPlane() );

	// Extra extraction for mesh
	if ( mc->IsExactlyA< CStaticMeshComponent >() )
	{
		const CStaticMeshComponent* smc = static_cast< const CStaticMeshComponent* >( mc );
		if ( smc->IsFadingOnCameraCollision() )
		{
			info.m_flags |= SectorData::ePackedFlag_Mesh_FadeOnCameraCollision;
		}
	}

	// Insert mesh
	Vector streamingPos = mc->GetLocalToWorld().TransformBox( mesh->GetBoundingBox() ).CalcCenter(); // use center of visual representation as streaming reference
	Float streamingRadius = mc->GetAutoHideDistance();

	// Entity proxy hacks&features
	// Conform the autohide distances of entity proxy and other meshes in the entity
	if ( rootEntityProxyComponent != nullptr )
	{
		// always stream relative to the LOCATION of the proxy mesh
		// this is REQUIRED in order to sync the proxy hide distance (which is given as LOD distance) and the high detail geometry appear distance (which is it's streaming distance).
		streamingPos = rootEntityProxyComponent->GetLocalToWorld().GetTranslation();

		// is this the proxy or other component ?
		if ( mc == rootEntityProxyComponent )
		{
			LOG_ENGINE( TXT("Proxy root: '%ls' proxy mesh: '%ls'"),
				mc->GetFriendlyName().AsChar(), rootEntityProxyComponent->GetMeshNow()->GetDepotPath().AsChar() );

			info.m_flags |= SectorData::ePackedFlag_Mesh_RootEntityProxy;
		}
		else
		{
			LOG_ENGINE( TXT("Proxy elem: '%ls' proxy mesh: '%ls'"),
				mc->GetFriendlyName().AsChar(), rootEntityProxyComponent->GetMeshNow()->GetDepotPath().AsChar() );

			// mark this object as part of 
			info.m_flags |= SectorData::ePackedFlag_Mesh_PartOfEntityProxy;

			// override the streaming to match the entity proxy LOD switch distance
			// NOTE: there's no additional shift in the distance here, it should not be needed
			streamingRadius = rootEntityProxyComponentVisibilityDistance;
		}
	}

	// Add mesh to sector data
	AddMesh( info, streamingPos, streamingRadius );

	// Collision
	if ( mc->IsExactlyA< CStaticMeshComponent >() )
	{
		const CStaticMeshComponent* smc = static_cast< const CStaticMeshComponent* >( mc );
		if ( mesh->GetCollisionMesh() )
		{
			// Basic data
			SectorData::PackedCollision colInfo;
			Red::System::MemoryZero( &colInfo, sizeof( colInfo ) );
			colInfo.m_mesh = MapResource( mesh );
			colInfo.m_occlusionId = 0;
			colInfo.m_localToWorld.Pack( mc->GetLocalToWorld() );

			// Extract collision info, we assume they will no change between cook time and runtime
			RED_FATAL_ASSERT( GPhysicEngine != nullptr, "Physical engine is required during cooking to extract collision flags" );
			smc->GetCollisionWithClimbFlags().RetrieveCollisionMasks( colInfo.m_collisionMask, colInfo.m_collisionGroup );

			// Add collision object
			const Vector collisionStreamingPos = smc->GetLocalToWorld().TransformBox( mesh->GetBoundingBox() ).CalcCenter(); // use center of visual representation as streaming reference
			const Float collisionStreamingRadius = mesh->GetAutoHideDistance();
			AddCollision( colInfo, collisionStreamingPos, collisionStreamingRadius );
		}
	}

	// Data extracted
	return true;
}

Bool CSectorDataBuilder::ExtractRigidMeshComponent( const class CRigidMeshComponent* mc )
{
	// no mesh, we can still extract it any way - this way we delete components with no meshes
	THandle< CMesh > mesh  = mc->GetMeshNow();
	if ( !mesh )
		return true; // report as extracted

	// no mesh collision
#ifndef NO_RESOURCE_IMPORT
	if ( !mesh->GetCollisionMesh() )
		return true;
#endif

	// Basic data
	SectorData::PackedRigidBody info;
	Red::System::MemoryZero( &info, sizeof( info ) );
	info.m_mesh = MapResource( mesh );
	info.m_occlusionId = GetHash( mesh->GetDepotPath() );
	info.m_localToWorld.Pack( mc->GetLocalToWorld() );
	info.m_lightChannels = mc->GetLightChannels();
	info.m_forcedLODLevel = (Int8)mc->GetForcedLODLevel();
	info.m_shadowBias = 0;
	info.m_flags = 0;

	// Extract collision info, we assume they will no change between cook time and runtime
	mc->GetPhysicalCollision().RetrieveCollisionMasks( info.m_collisionMask, info.m_collisionGroup );

	// Mesh is casting shadows
	if ( mc->IsCastingShadows() )
	{
		info.m_flags |=  SectorData::ePackedFlag_Mesh_CastingShadows;
		info.m_shadowBias = (Int8) mc->GetShadowImportanceBias();
	}

	if ( mc->IsCastingShadowsFromLocalLightsOnly() )
	{
		info.m_flags |=  SectorData::ePackedFlag_Mesh_CastingShadowsFromLocalLightsOnly;
		info.m_shadowBias = (Int8) mc->GetShadowImportanceBias();
	}

	// Mesh is visible
	if ( mc->IsVisible() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_Visible;
	}

	// Force no autohide
	if ( mc->IsForceNoAutohide() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_ForceNoAutohide;
	}

	// Fade on camera collision
	if ( mc->IsFadingOnCameraCollision() )
	{
		info.m_flags |= SectorData::ePackedFlag_Mesh_FadeOnCameraCollision;
	}

	// Rendering plane data
	info.m_renderingPlane = static_cast< Uint8 >( mc->GetRenderingPlane() );

	// Physical attributes
	info.m_angularDamping = mc->GetAngularDamping();
	info.m_linearDamping = mc->GetLinearDamping();
	info.m_linearVelocityClamp = mc->GetLinearVelocityClamp();
	info.m_motionType = (Uint8) mc->GetMotionType();

	// Insert mesh
	const Vector streamingPos = mc->GetLocalToWorld().TransformBox( mesh->GetBoundingBox() ).CalcCenter(); // use center of visual representation as streaming reference
	const Float streamingRadius = mesh->GetAutoHideDistance();
	AddRigidBody( info, streamingPos, streamingRadius );

	// Data extracted
	return true;
}

Bool CSectorDataBuilder::ExtractParticleComponent( const class CParticleComponent* pc )
{
	THandle< CParticleSystem > ps  = pc->GetParticleSystem();
	if ( !ps )
		return true; // report as extracted

	// Basic data
	SectorData::PackedParticles info;
	Red::System::MemoryZero( &info, sizeof( info ) );
	info.m_particleSystem = MapResource( ps );
	info.m_occlusionId = GetHash( ps->GetDepotPath() );
	info.m_localToWorld.Pack( pc->GetLocalToWorld() );
	info.m_lightChannels = pc->GetLightChannels();
	info.m_flags = 0;

	// PS is visible
	if ( pc->IsVisible() )
	{
		info.m_flags |= SectorData::ePackedFlag_Particles_Visible;
	}

	// Force no autohide
	if ( pc->IsForceNoAutohide() )
	{
		info.m_flags |= SectorData::ePackedFlag_Particles_ForceNoAutohide;
	}

	// Rendering plane data
	info.m_renderingPlane = static_cast< Uint8 >( pc->GetRenderingPlane() );

	// Physical attributes
	info.m_envAutoHideGroup = (Uint8) pc->GetEnvAutoHideGroup();
	info.m_transparencySortGroup = (Uint8) pc->GetTransparencySortGroup();
	info.m_globalEmissionScale = (Uint8) pc->GetGlobalEmissionScale();

	// Insert particle comp
	const Vector streamingPos = pc->GetLocalToWorld().GetTranslationRef();
	const Float streamingRadius = ps->GetAutoHideDistance() + 10; // just in case we need component to be streamed on time before starting the effect
	AddParticles( info, streamingPos, streamingRadius );

	// Data extracted
	return true;
}

Bool CSectorDataBuilder::ExtractDimmerComponent( const class CDimmerComponent* component )
{
	SectorData::PackedDimmer info;
	Red::System::MemoryZero( &info, sizeof( info ) );

	info.m_localToWorld.Pack( component->GetLocalToWorld() );
	info.m_occlusionId = component->GetOcclusionId();

	info.m_flags = 0;
	if ( component->IsAreaMarker() )
		info.m_flags |= SectorData::ePackedFlag_Dimmer_AreaMarker;


	info.m_dimmerType = (Uint8) component->GetDimmerType();
	info.m_ambientLevel = component->GetAmbientLevel();
	info.m_marginFactor = component->GetMarginFactor();

	// insert object
	const Vector streamingPos = component->GetLocalToWorld().GetTranslationRef();
	const Float streamingRadius = component->GetAutoHideDistance();
	AddDimmer( info, streamingPos, streamingRadius );
	return true;
}

Bool CSectorDataBuilder::ExtractDecalComponent( const class CDecalComponent* component )
{
	SectorData::PackedDecal info;
	Red::System::MemoryZero( &info, sizeof( info ) );

	if ( !component->GetDiffuseTexture() ) 
		return true;

	info.m_localToWorld.Pack( component->GetLocalToWorld() );
	info.m_occlusionId = GetHash( component->GetDiffuseTexture()->GetDepotPath() );

	info.m_flags = 0;
	if ( component->GetVerticalFlip() )
		info.m_flags |= SectorData::ePackedFlag_Decal_VerticalFlip;
	if ( component->GetHorizontalFlip() )
		info.m_flags |= SectorData::ePackedFlag_Decal_HorizontalFlip;

	info.m_diffuseTexture = MapResource( component->GetDiffuseTexture() );
	info.m_normalThreshold = component->GetNormalThreshold();
	info.m_specularColor = component->GetSpecularColor();
	info.m_specularity = component->GetSpecularity();
	info.m_fadeTime = component->GetFadeTime();

	// insert object
	const Vector streamingPos = component->GetLocalToWorld().GetTranslationRef();
	const Float streamingRadius = component->GetAutoHideDistance() + CDecalComponent::AUTOHIDE_MARGIN;
	AddDecal( info, streamingPos, streamingRadius );
	return true;
}

Bool CSectorDataBuilder::ExtractPointLightComponent( const class CPointLightComponent* component )
{
	SectorData::PackedLight info;
	Red::System::MemoryZero( &info, sizeof( info ) );

	info.m_localToWorld.Pack( component->GetLocalToWorld() );
	info.m_occlusionId = component->GetOcclusionId();

	info.m_flags = 0;

	if ( component->GetAllowDistantLight() )
		info.m_flags |= SectorData::ePackedFlag_Light_AllowDistanceFade;

	if ( component->IsCachingStaticShadows() )
		info.m_flags |= SectorData::ePackedFlag_Light_CacheStaticShadows;

	info.m_color = component->GetColor();
	info.m_radius = component->GetRadius();
	info.m_brightness = component->GetBrightness();
	info.m_attenuation = component->GetAttenuation();
	info.m_autoHideRange = component->GetAutoHideRange();
	info.m_shadowFadeDistance = component->GetShadowFadeDistance();
	info.m_shadowFadeRange = component->GetShadowFadeRange();
	info.m_lightFlickering[0] = component->GetLightFlickering()->m_positionOffset;
	info.m_lightFlickering[1] = component->GetLightFlickering()->m_flickerStrength;
	info.m_lightFlickering[2] = component->GetLightFlickering()->m_flickerPeriod;	
	info.m_shadowCastingMode = (Uint8) component->GetShadowCastingMode();
	info.m_dynamicShadowsFaceMask = component->GetDynamicShadowsFaceMask();
	info.m_envColorGroup = component->GetEnvColorGroup();
	info.m_lightUsageMask = component->GetLightUsageMask();
	info.m_shadowBlendFactor = component->GetShadowBlendFactor();

	// insert object
	const Vector streamingPos = component->GetLocalToWorld().GetTranslationRef();
	// when creating the IRenderProxyLight (in sectorDataRuntime.cpp) the proper autohide is extracted from the radius, keep it in sync
	const Float streamingRadius = component->GetAutoHideDistance() + component->GetRadius() + component->GetAutoHideRange();
	AddLight( info, streamingPos, streamingRadius );
	return true;
}

Bool CSectorDataBuilder::ExtractSpotLightComponent( const class CSpotLightComponent* component )
{
	SectorData::PackedSpotLight info;
	Red::System::MemoryZero( &info, sizeof( info ) );

	info.m_localToWorld.Pack( component->GetLocalToWorld() );
	info.m_occlusionId = component->GetOcclusionId();

	info.m_flags = 0;

	// synchronize with renderPoroxyLight.cpp #124 :(
	if ( component->GetAllowDistantLight() && component->GetOuterAngle() > 120.0f )
		info.m_flags |= SectorData::ePackedFlag_Light_AllowDistanceFade;

	info.m_color = component->GetColor();
	info.m_radius = component->GetRadius();
	info.m_brightness = component->GetBrightness();
	info.m_attenuation = component->GetAttenuation();
	info.m_autoHideRange = component->GetAutoHideRange();
	info.m_shadowFadeDistance = component->GetShadowFadeDistance();
	info.m_shadowFadeRange = component->GetShadowFadeRange();
	info.m_lightFlickering[0] = component->GetLightFlickering()->m_positionOffset;
	info.m_lightFlickering[1] = component->GetLightFlickering()->m_flickerStrength;
	info.m_lightFlickering[2] = component->GetLightFlickering()->m_flickerPeriod;	
	info.m_shadowCastingMode = (Uint8) component->GetShadowCastingMode();
	info.m_dynamicShadowsFaceMask = 0;
	info.m_envColorGroup = component->GetEnvColorGroup();
	info.m_lightUsageMask = component->GetLightUsageMask();
	info.m_shadowBlendFactor = component->GetShadowBlendFactor();

	info.m_innerAngle = component->GetInnerAngle();
	info.m_outerAngle = component->GetOuterAngle();
	info.m_softness = component->GetSoftness();
	info.m_projectionTextureAngle = component->GetProjectionTextureAngle();
	info.m_projectionTexureUBias = component->GetProjectionTextureUBias();
	info.m_projectionTexureVBias = component->GetProjectionTextureVBias();

	if ( component->GetProjectionTexture() )
	{
		info.m_flags |= SectorData::ePackedFlag_Light_HasProjectionTexture;
		info.m_projectionTexture = MapResource( component->GetProjectionTexture() );
	}
	else
	{
		info.m_projectionTexture = 0;
	}

	// insert object
	const Vector streamingPos = component->GetLocalToWorld().GetTranslationRef();
	// when creating the IRenderProxyLight (in sectorDataRuntime.cpp) the proper autohide is extracted from the radius, keep it in sync
	const Float streamingRadius = component->GetAutoHideDistance() + component->GetRadius() + component->GetAutoHideRange();
	AddSpotLight( info, streamingPos, streamingRadius );
	return true;
}

void CSectorDataBuilder::AddMesh( const SectorData::PackedMesh& mesh, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &mesh, sizeof(mesh) );
	AddDataObject( SectorData::eObject_Mesh, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddCollision( const SectorData::PackedCollision& mesh, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &mesh, sizeof(mesh) );
	AddDataObject( SectorData::eObject_Collision, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddDecal( const SectorData::PackedDecal& decal, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &decal, sizeof(decal) );
	AddDataObject( SectorData::eObject_Decal, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddDimmer( const SectorData::PackedDimmer& dimmer, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &dimmer, sizeof(dimmer) );
	AddDataObject( SectorData::eObject_Dimmer, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddLight( const SectorData::PackedLight& light, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &light, sizeof(light) );
	AddDataObject( SectorData::eObject_PointLight, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddSpotLight( const SectorData::PackedSpotLight& light, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &light, sizeof(light) );
	AddDataObject( SectorData::eObject_SpotLight, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddRigidBody( const SectorData::PackedRigidBody& body, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &body, sizeof(body) );
	AddDataObject( SectorData::eObject_RigidBody, internalObjectFlags, dataOffset, streamingPos, radius );
}

void CSectorDataBuilder::AddParticles( const SectorData::PackedParticles& system, const Vector& streamingPos, const Float radius, const Uint8 internalObjectFlags /*= 0*/ )
{
	const Uint32 dataOffset = AddDataBlock( &system, sizeof(system) );
	AddDataObject( SectorData::eObject_Particles, internalObjectFlags, dataOffset, streamingPos, radius );
}

Uint32 CSectorDataBuilder::AddDataBlock( const void* data, const Uint32 dataSize )
{
	const Uint32 prevSize = m_data->m_dataStream.Size( );
	const Uint32 offset = (Uint32) AlignOffset( prevSize, 4 );
	m_data->m_dataStream.Grow( (offset + dataSize) - m_data->m_dataStream.Size() );
	Red::System::MemoryZero( m_data->m_dataStream.TypedData() + prevSize, offset - prevSize );
	Red::MemoryCopy( m_data->m_dataStream.TypedData() + offset, data, dataSize );
	return offset;
}

void CSectorDataBuilder::AddDataObject( const SectorData::EObjectType type, const Uint8 internalObjectFlags, const Uint32 internalDataOffset, const Vector& streamingPos, const Float radius )
{
	SectorData::PackedObject obj;
	Red::System::MemoryZero( &obj, sizeof( obj ) );
	obj.m_type = type;
	obj.m_flags = internalObjectFlags;
	obj.m_offset = internalDataOffset;
	obj.m_pos = streamingPos;
	obj.m_radius = (Uint16) Clamp< Float >( radius, 1.0f, 8192.0f );
	m_data->m_objects.PushBack(obj);
}

SectorData::PackedResourceRef CSectorDataBuilder::MapResource( const String& depotPath, const Box& box )
{
	// empty resource cache
	if ( depotPath.Empty() )
		return SectorData::PackedResourceRef(0);

	// conform the path
	String temp;
	const String& safePath = CFilePath::ConformPath( depotPath, temp );

	// compute path hash
	const Uint64 pathHash = Red::System::CalculatePathHash64( safePath.AsChar() );

	// use existing
	Uint32 resIndex = 0;
	if ( m_resourceMap.Find( pathHash, resIndex ) )
		return SectorData::PackedResourceRef(resIndex);

	// format info
	CSectorData::PackedResource resInfo;
	Red::System::MemoryZero( &resInfo, sizeof( resInfo ) );
	resInfo.m_pathHash = pathHash;
	resInfo.m_box[ SectorData::eBox_MinX ] = box.Min.X;
	resInfo.m_box[ SectorData::eBox_MinY ] = box.Min.Y;
	resInfo.m_box[ SectorData::eBox_MinZ ] = box.Min.Z;
	resInfo.m_box[ SectorData::eBox_MaxX ] = box.Max.X;
	resInfo.m_box[ SectorData::eBox_MaxY ] = box.Max.Y;
	resInfo.m_box[ SectorData::eBox_MaxZ ] = box.Max.Z;

	// store
	resIndex = (Uint32) m_data->m_resources.Size();
	m_data->m_resources.PushBack( resInfo );

	// map
	m_resourceMap.Insert( pathHash, resIndex );
	m_resources.PushBack( safePath );
	return resIndex;
}

SectorData::PackedResourceRef CSectorDataBuilder::MapResource( const class CResource* resource )
{
	// null resource
	if ( !resource )
		return 0;

	// resources without file
	if ( !resource->GetFile() )
		return 0;

	// get the box (only meshes)
	Box box( Vector::ZEROS, 0.5f );
	if ( resource->IsA< CMesh >() )
	{
		box = static_cast< const CMesh* >( resource )->GetBoundingBox();
	}

	// map by depot path
	return MapResource( resource->GetFile()->GetDepotPath(), box );
}

#endif
