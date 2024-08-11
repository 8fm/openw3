/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorDataRigidMeshRuntime.h"
#include "sectorDataResourceLoader.h"

#include "entity.h"
#include "dynamicLayer.h"
#include "mesh.h"

#include "../core/configVar.h"
#include "../core/objectGC.h"

namespace Config
{
	TConfigVar< Bool >	cvSectorAllowRigidBodies( "Streaming/Sectors/Filter", "AllowRigidBodies", true );
}

IMPLEMENT_ENGINE_CLASS( CRigidMeshComponentCooked );

CRigidMeshComponentCooked::CRigidMeshComponentCooked()
{
}

void CRigidMeshComponentCooked::SetupFromCookedData( CMesh* mesh, const SectorData::PackedRigidBody& data )
{
	RED_FATAL_ASSERT( mesh != nullptr, "Runtime representation should not be created when there's no asset" );

	// mesh
	SetResource( (CResource*) mesh );

	// rendering params
	m_lightChannels = data.m_lightChannels;
	m_forceLODLevel = data.m_forcedLODLevel;
	m_shadowImportanceBias = (EMeshShadowImportanceBias) data.m_shadowBias;
	m_renderingPlane = (ERenderingPlane) data.m_renderingPlane;
	m_fadeOnCameraCollision = data.HasFlag( SectorData::ePackedFlag_Mesh_FadeOnCameraCollision );

	// rendering flags
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_CastingShadows ) )
		m_drawableFlags |= DF_CastShadows;
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_CastingShadowsFromLocalLightsOnly ) )
		m_drawableFlags |= DF_CastShadowsFromLocalLightsOnly;
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_Visible ) )
		m_drawableFlags |= DF_IsVisible;
	if ( data.HasFlag( SectorData::ePackedFlag_Mesh_ForceNoAutohide ) )
		m_drawableFlags |= DF_ForceNoAutohide;

	// physical params
	m_angularDamping = data.m_angularDamping;
	m_linearDamping = data.m_linearDamping;
	m_linearVelocityClamp = data.m_linearVelocityClamp;
	m_physicalCollisionType = CPhysicalCollision( data.m_collisionMask, data.m_collisionGroup );
	m_motionType = (EMotionType)data.m_motionType;
}

CSectorDataObjectRigidBody::CSectorDataObjectRigidBody()
{
}

CSectorDataObjectRigidBody::~CSectorDataObjectRigidBody()
{
	RED_FATAL_ASSERT( !GObjectGC->IsDoingGC(), "Streaming performed during GC" );

	// delete entity that was not picked up
	if ( m_entity )
	{
		RED_FATAL_ASSERT( !m_entity->IsAttached(), "Entity should not be attached here" );
		m_entity->Discard();
		m_entity = nullptr;
	}
}

CSectorDataObjectRigidBody::EResult CSectorDataObjectRigidBody::Stream( const Context& context, const Bool asyncPipeline )
{
	RED_FATAL_ASSERT( !GObjectGC->IsDoingGC(), "Streaming performed during GC" );

	// particles are not allowed
	if ( !Config::cvSectorAllowRigidBodies.Get() )
		return eResult_NotReady;

	// load the particle system
	const auto& data = GetData();
	const auto ret = context.m_resourceLoader->PrefetchResource( data.m_mesh );
	if ( ret != CSectorDataResourceLoader::eResult_Loaded )
		return eResult_NotReady;

	// Create entity if not there yet
	if ( !m_entity )
	{
		// add ref count to the resource but only after it's ready to be used
		const THandle< CMesh > mesh = Cast< CMesh >( context.m_resourceLoader->GetResourceAddRef( data.m_mesh ) );
		RED_FATAL_ASSERT( mesh != nullptr, "Resource lost altough it's reported as loaded" );

		// keep reference to used resource
		m_resourceToUse.Bind( context.m_resourceLoader, data.m_mesh );

		// create entity
		m_entity = CreateObject< CEntity >( context.m_dynamicLayer );
		EngineTransform entityTransform( data.m_localToWorld.Unpack() );
		m_entity->SetRawPlacement( &entityTransform.GetPosition(), &entityTransform.GetRotation(), &entityTransform.GetScale() );

		// create fake component
		CRigidMeshComponentCooked* comp = CreateObject< CRigidMeshComponentCooked >( m_entity );
		comp->SetupFromCookedData( mesh, data );
#ifndef RED_FINAL_BUILD
		comp->SetName( TXT("CookedRigidBody") );
#endif
		m_entity->AddComponent( comp );

		// magic hack to "stabilize" the LocalToWorld matrices
		m_entity->ForceUpdateTransformNodeAndCommitChanges();
	}

	// Particle system is to complicated to be created asynchronously because it's using component system... :(
	if ( asyncPipeline )
		return eResult_RequiresSync;

	// We have entity already, link it with layer
	RED_FATAL_ASSERT( m_entity != nullptr, "Trying to finish streaming job with no entity" );
	RED_FATAL_ASSERT( !m_entity->IsAttached(), "Entity should not be attached in here" );

	// add entity to the world
	EntitySpawnInfo spawnInfo;
	spawnInfo.m_entityClass = ClassID< CEntity >();
	spawnInfo.m_spawnPosition = m_entity->GetPosition();
	spawnInfo.m_spawnRotation = m_entity->GetRotation();
	spawnInfo.m_spawnScale = m_entity->GetScale();
	context.m_dynamicLayer->LinkEntityWithLayer( m_entity, spawnInfo );

	// done
	return eResult_Finished;
}

CSectorDataObjectRigidBody::EResult CSectorDataObjectRigidBody::Unstream( const Context& context, const Bool asyncPipeline )
{
	RED_FATAL_ASSERT( !GObjectGC->IsDoingGC(), "Streaming performed during GC" );

	// we cannot do anything in the async part
	if ( asyncPipeline )
		return eResult_RequiresSync;

	// destroy entity
	if ( m_entity )
	{
		m_entity->Destroy();
		m_entity = nullptr;
	}

	// done
	return eResult_Finished;
}
