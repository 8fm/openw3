/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorDataParticlesRuntime.h"
#include "sectorDataResourceLoader.h"
#include "../core/configVar.h"
#include "../core/objectGC.h"

#include "entity.h"
#include "particleSystem.h"
#include "dynamicLayer.h"

namespace Config
{
	TConfigVar< Bool >  cvSectorAllowParticles( "Streaming/Sectors/Filter", "AllowParticles", true );
}

IMPLEMENT_ENGINE_CLASS( CParticleComponentCooked );

CParticleComponentCooked::CParticleComponentCooked()
{
}

void CParticleComponentCooked::SetupFromCookedData( CParticleSystem* particleSystem, const SectorData::PackedParticles& data )
{
	RED_FATAL_ASSERT( particleSystem != nullptr, "Runtime representation should not be created when there's no asset" );

	// mesh
	SetResource( (CResource*) particleSystem );

	// rendering params
	m_lightChannels = data.m_lightChannels;
	m_renderingPlane = (ERenderingPlane) data.m_renderingPlane;

	// rendering flags
	if ( data.HasFlag( SectorData::ePackedFlag_Particles_Visible ) )
		m_drawableFlags |= DF_IsVisible;
	if ( data.HasFlag( SectorData::ePackedFlag_Particles_ForceNoAutohide ) )
		m_drawableFlags |= DF_ForceNoAutohide;

	// physical params
	m_globalEmissionScale = data.m_globalEmissionScale;
	m_envAutoHideGroup = (EEnvAutoHideGroup) data.m_envAutoHideGroup;
	m_transparencySortGroup = (ETransparencySortGroup) data.m_transparencySortGroup;
}

CSectorDataObjectParticles::CSectorDataObjectParticles()
{
}

CSectorDataObjectParticles::~CSectorDataObjectParticles()
{
	RED_FATAL_ASSERT( !GObjectGC->IsDoingGC(), "Streaming performed during GC" );

	// delete entity that was not picked up
	if ( m_entity )
	{
		RED_FATAL_ASSERT( !m_entity->IsAttached(), "Trying to release an entity that is already attached. Wrong state of the streaming object." );

		m_entity->Discard();
		m_entity = nullptr;
	}
}

CSectorDataObjectParticles::EResult CSectorDataObjectParticles::Stream( const Context& context, const Bool asyncPipeline )
{
	RED_FATAL_ASSERT( !GObjectGC->IsDoingGC(), "Streaming performed during GC" );

	// particles are not allowed
	if ( !Config::cvSectorAllowParticles.Get() )
		return eResult_NotReady;

	// load the particle system
	const auto& data = GetData();
	const auto ret = context.m_resourceLoader->PrefetchResource( data.m_particleSystem );
	if ( ret != CSectorDataResourceLoader::eResult_Loaded )
		return eResult_NotReady;

	// Create the entity
	if ( !m_entity )
	{

		// We have entity already, link it with layer
		// add ref count to the resource but only after it's ready to be used
		const THandle< CParticleSystem > particleSystem = Cast< CParticleSystem >( context.m_resourceLoader->GetResourceAddRef( data.m_particleSystem ) );
		RED_FATAL_ASSERT( particleSystem != nullptr, "Resource lost altough it's reported as loaded" );

		// bind resource
		m_resourceToUse.Bind( context.m_resourceLoader, data.m_particleSystem );

		// create entity
		m_entity = CreateObject< CEntity >( context.m_dynamicLayer );
		EngineTransform entityTransform( data.m_localToWorld.Unpack() );
		m_entity->SetRawPlacement( &entityTransform.GetPosition(), &entityTransform.GetRotation(), nullptr );

		// create fake component
		CParticleComponentCooked* comp = CreateObject< CParticleComponentCooked >( m_entity );
		comp->SetupFromCookedData( particleSystem, data );
#ifndef RED_FINAL_BUILD
		comp->SetName( TXT("CookedParticleSystem") );
#endif
		m_entity->AddComponent( comp );

		// magic hack to "stabilize" the LocalToWorld matrices
		m_entity->ForceUpdateTransformNodeAndCommitChanges();
	}

	// we need sync part to link the entity with layer
	if ( asyncPipeline )
		return eResult_RequiresSync;

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

CSectorDataObjectParticles::EResult CSectorDataObjectParticles::Unstream( const Context& context, const Bool asyncPipeline )
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
