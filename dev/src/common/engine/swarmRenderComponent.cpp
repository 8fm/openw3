/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "swarmRenderComponent.h"
#include "mesh.h"
#include "animatedComponent.h"
#include "renderCommands.h"
#include "renderFrame.h"
#include "meshTypeResource.h"
#include "renderSwarmData.h"
#include "evaluatorVector.h"
#include "tickManager.h"
#include "entity.h"
#include "entityTemplate.h"
#include "meshComponent.h"
#include "skeletalAnimationSet.h"

IMPLEMENT_ENGINE_CLASS( CSwarmRenderComponent );

CSwarmRenderComponent::CSwarmRenderComponent()
	: m_mesh( nullptr )
	, m_animationSet( nullptr )
	, m_skeleton( nullptr )
	, m_boidCount( 0 )
	, m_loadingDone( false )
{
	SetStreamed( false );
}

CSwarmRenderComponent::~CSwarmRenderComponent()
{

}

void CSwarmRenderComponent::OnLairActivated()
{
	if ( m_loadingDone == false )
	{
		m_loadingDone = true;
		// Initialise animations
		CEntityTemplate *const entityTemplate	= m_boidTemplateHandle.Get();
		if ( entityTemplate )
		{
			CAnimatedComponent* animatedComponent	= entityTemplate->GetEntityObject()->FindComponent< CAnimatedComponent >();

			if ( animatedComponent )
			{
				CSkeletalAnimationSetEntry* animEntry		= nullptr;
				auto & animationSets						= animatedComponent->GetAnimationSets();
				for ( auto it = animationSets.Begin(), end  = animationSets.End(); it != end; ++it )
				{
					CSkeletalAnimationSet *const animationSet	= *it;

					if (animationSet == nullptr)
					{
						WARN_CORE( TXT("Animation set in animation component of boid entity template is invalid. File: %ls"), entityTemplate->GetDepotPath().AsChar() );
						continue;
					}

					auto & animations = animationSet->GetAnimations( );
					for ( auto it2 = animations.Begin(), end2  = animations.End(); it2 != end2; ++it2 )
					{
						animEntry = *it2;
						m_animationDurationMap.Insert( animEntry->GetName(), animEntry->GetDuration() );
					}
				}
			}
			InitializeFromTemplate( entityTemplate );
		}

	}

	if( !m_swarmData )
	{
		m_swarmData.Reset( GRender->CreateSwarmBuffer( MAX_NUM_BOIDS ) );
	}

	RegisterTicks( true );
	SetVisible( true );
}

void CSwarmRenderComponent::RegisterTicks( Bool registerTick )
{
	CWorld *const world = GetWorld();
	if ( registerTick )
	{
		// tick me
		world->GetTickManager()->AddToGroup( this, TICK_PostPhysics );

		// might be needed later
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Bboxes );
	}
	else
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Bboxes );

		// un-tick me
		world->GetTickManager()->RemoveFromGroup( this, TICK_PostPhysics );
	}
}

void CSwarmRenderComponent::OnLairDeactivated()
{
	
	m_loadingDone = false;
	RegisterTicks( false );
	m_swarmData.Reset();

	m_mesh			= nullptr;
	m_animationSet	= nullptr;
	m_skeleton		= nullptr;

	SetVisible( false );
	RefreshRenderProxies();
}

void CSwarmRenderComponent::SetBoidTemplateHandle( const TSoftHandle<CEntityTemplate> & boidTemplateHandle )
{ 
	// Should be loaded at game init
	m_boidTemplateHandle = boidTemplateHandle.Get(); 
}

void CSwarmRenderComponent::OnTickPostPhysics( Float timeDelta )
{
	// send new swarm data to renderer
	if( m_swarmData && m_boidCount > 0 )
	{
		( new CRenderCommand_UpdateSwarmData( m_renderProxy, m_swarmData.Get(), m_boidCount ) )->Commit();
	}
}

void CSwarmRenderComponent::OnInitializeProxy()
{
	// Pass to base class so we get a proxy created
	TBaseClass::OnInitializeProxy();
}

void CSwarmRenderComponent::InitializeFromTemplate( CEntityTemplate *const entityTemplate )
{
	if( !entityTemplate )
	{
		return;
	}
	if ( m_mesh )
	{
		return;
	}

	// for each component...
	const TDynArray<CComponent*>& components = entityTemplate->GetEntityObject()->GetComponents();
	for( Uint32 c=0; c<components.Size(); ++c )
	{
		// extract mesh
		const CComponent* component = components[ c ];
		if( component->IsA( CMeshComponent::GetStaticClass() ) )
		{
			if( m_mesh )
			{
				RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Warning: boid template: %s contains more than 1 mesh component" ), entityTemplate->GetFriendlyName().AsChar() );
				continue;
			}

			// extract mesh resource
			const CMeshComponent* meshComponent = Cast<const CMeshComponent>( component );
			m_mesh = meshComponent->GetMeshTypeResource();

			/*
			TODO: Check that each mesh has:
				- 3 LODs (High, Medium, Low)... Maybe we even skip animation/skinning in the lowest LOD?
				- Only 1 Mesh Chunk per LOD
				- Skinning
				- As cheap skeleton as possible (no unused bones)
				- All LODs share 1 Material
			*/
		}
		else if( component->IsA( CAnimatedComponent::GetStaticClass() ) )
		{
			const CAnimatedComponent* animatedComponent = Cast<const CAnimatedComponent>( component );
			
			/*
			TODO: Check that each animation component has:
				- Only 1 animation set
				- Only looping animations
			*/

			// should not have at least 1 animation set
			if( animatedComponent->GetAnimationSets().Size() == 0 )
			{
				RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Warning: boid template: %s does not contain an animation set" ), entityTemplate->GetFriendlyName().AsChar() );
				continue;
			}

			// should not have more than 1 animation set!
			if( animatedComponent->GetAnimationSets().Size() > 1 )
			{
				RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Warning: boid template: %s contains more than 1 animation set" ), entityTemplate->GetFriendlyName().AsChar() );
			}

			// get animation set
			m_animationSet	= animatedComponent->GetAnimationSets()[ 0 ].Get();
			m_skeleton		= animatedComponent->GetSkeleton();
		}
	}

	// Set new mesh
	if ( m_mesh )
	{
		// Update bounding
		OnUpdateBounds();
		RefreshRenderProxies();

		/*RegisterTicks( false );
		// Full recreation
		PerformFullRecreation();
		RegisterTicks( true );*/
	}
}

CSwarmBoidData* CSwarmRenderComponent::GetWriteData()
{ 
	return m_swarmData ? m_swarmData->GetWriteData() : nullptr; 
}

void CSwarmRenderComponent::SetBoundingBox( const Box& b ) 
{ 
	m_boundingBox = b; 
	RelinkProxy();
}

void CSwarmRenderComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::RED );
}

void CSwarmRenderComponent::OnUpdateBounds()
{
	if( m_swarmData )
	{
		// this is set explicitly by a boid lair
	}
	else
	{
		TBaseClass::OnUpdateBounds();
	}
}

CEntityTemplate *const CSwarmRenderComponent::GetBoidTemplate()
{
	return m_boidTemplateHandle.Get();
}

Float CSwarmRenderComponent::GetAnimationDuration( CName animationName )const
{ 
	Float speed =  1.0f;
	if ( m_animationDurationMap.Find( animationName, speed ) )
	{
		return speed;
	}
	return 1.0f;
}
void CSwarmRenderComponent::OnAttached( CWorld* world )
{
	SetVisible( false );
	TBaseClass::OnAttached( world );
}

void CSwarmRenderComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
}
