#include "build.h"
#include "boidInstance.h"
#include "boidLairEntity.h"
#include "boidSpecies.h"
#include "animBoidNode.h"
#include "../engine/swarmRenderComponent.h"
#include "../engine/animatedSkeleton.h"
#include "../engine/skeletalAnimationEntry.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/dynamicLayer.h"

#include "../engine/fxState.h"

#include "../engine/fxDefinition.h"
#include "../engine/world.h"
#include "../engine/tickManager.h"

CBoidInstance::CBoidInstance( IBoidLairEntity* lair, const Vector& position, const EulerAngles& orientation, Float scale )
	: m_boidState( 0 )
	, m_lair( lair )
	, m_position( position )
	, m_orientation( orientation )
	, m_scale( scale )
	, m_currentAtomicBoidNode( NULL )
	, m_pendingAtomicBoidNode( NULL )
	, m_pendingBoidState( NULL )
	, m_currentStateCounter( 0 )
	, m_animSpeedMultiplier( 0.0f )
	, m_animation( CName::NONE )
	, m_fxHolder( nullptr )
{
	m_animSpeedMultiplier	= GEngine->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f ); // Getting a rand var in the range of [-1.0f, 1.0f]
	ASSERT( -1.0f <= m_animSpeedMultiplier && m_animSpeedMultiplier <= 1.0f );
	m_swarmRenderingComponent = m_lair->FindComponent< CSwarmRenderComponent >();
}

CBoidInstance::~CBoidInstance()
{
}

void CBoidInstance::OnTick( CLoopedAnimPriorityQueue &loopedAnimPriorityQueue, Float time, const Vector& position, const EulerAngles& orientation )
{
	m_position		= position;
	m_orientation	= orientation;

	// update effects holder
	if( m_fxHolder )
	{
		m_fxHolder->SetPosition( m_position );
		m_fxHolder->SetRotation( m_orientation );
	}
}

void CBoidInstance::SetBoidState( Boids::BoidState state, CLoopedAnimPriorityQueue &loopAnimPriorityQueue, Float time )
{
	const CBoidLairParams *const params = m_lair->GetBoidLairParams();
	if ( state != m_boidState )
	{
		if ( m_boidState == BOID_STATE_NOT_SPAWNED )
		{
			// set everything visible
			m_lair->AddActiveBoid();
		}
		else
		{
			// Making sure the current effect stops properly
			if ( m_currentAtomicBoidNode && params->m_allowAnimationInteruption )
			{
				m_currentAtomicBoidNode->Deactivate( this );
			}
		}

		m_boidState = state;
		++m_currentStateCounter;

		// TODO - set animation
		if ( state == BOID_STATE_NOT_SPAWNED )
		{
			// set everything non visible
			m_lair->RemoveActiveBoid();
		}
		else
		{
			const CBoidState & boidState				= m_lair->GetBoidLairParams()->m_boidStateArray[ state ];
			const CAtomicBoidNode *const nextBoidNode	= m_lair->GetBoidLairParams()->GetFirstAtomicNodeFromState( state );
			if (nextBoidNode)
			{
				if ( m_currentAtomicBoidNode == NULL || params->m_allowAnimationInteruption )
				{
					m_currentAtomicBoidNode = nextBoidNode;
					m_pendingAtomicBoidNode	= NULL;
					m_currentAtomicBoidNode->Activate(this, loopAnimPriorityQueue, time, boidState.m_startRandom );
				}
				else
				{
					m_pendingAtomicBoidNode = nextBoidNode;
					m_pendingBoidState		= &boidState;
				}
			}
		}
	}
}

Float CBoidInstance::PlayAnimation( CName animName, Bool looped, Float allowedVariationOnSpeed, Bool startRandom )
{
	if (m_swarmRenderingComponent.IsValid())
	{
		const Float animationDuration				= m_swarmRenderingComponent->GetAnimationDuration( animName );
		m_animation									= animName;
		return animationDuration;
	}

	return 0.f;
}

void CBoidInstance::PlayEffect( CName effectName )
{ 
	if ( effectName.Empty() == true )
	{
		return;
	}

	// Find effect by name
	CEntityTemplate* boidTemplate	= m_swarmRenderingComponent->GetBoidTemplate();
	CFXDefinition* effectToPlay		= ( boidTemplate ? boidTemplate->FindEffect( effectName ) : nullptr );
	if( !effectToPlay )
	{
		return;
	}

	// distance check
	CWorld* world = m_lair->GetLayer()->GetWorld();
	const Vector& camPos = world->GetCameraPosition();
	const Float showDistanceSqr = effectToPlay->GetShowDistanceSqr();
	if ( m_position.DistanceSquaredTo( camPos ) > showDistanceSqr )
	{
		// Too far away
		return;
	}

	// spawn effect holder entity
	EntitySpawnInfo info;
	info.m_entityClass = ClassID<CEntity>();
	CLayer* dynamicLayer = world->GetDynamicLayer();
	m_fxHolder = dynamicLayer->CreateEntitySync( info );

	// Create FX state
	CFXState* fxState = new CFXState( effectToPlay, showDistanceSqr, effectToPlay->GetName(), m_fxHolder, 0.0f, false, CName::NONE, nullptr, CName::NONE, false );
	if ( !fxState )
	{
		return;
	}

	// Attach to world
	world->GetEffectManager().AddEffect( fxState );
	m_activeEffects.PushBack( fxState );
}

void CBoidInstance::StopEffect( CName effectName )
{
	if ( effectName.Empty() )
	{
		return;
	}

	// remove effect
	for ( Uint32 i=0; i<m_activeEffects.Size(); i++ )
	{
		CFXState* state = m_activeEffects[i];
		if ( state && state->GetTag() == effectName )
		{
			CWorld* world = m_lair->GetLayer()->GetWorld();
			world->GetEffectManager().RemoveEffect( state );
			state->Destroy();
			m_activeEffects.RemoveAt( i );
			break;
		}
	}

	// no fx? ... no holder!
	if( m_activeEffects.Size() == 0 && m_fxHolder )
	{
		m_fxHolder->Destroy();
		m_fxHolder = nullptr;
	}
}

void CBoidInstance::ActivateNextAnimNode( CLoopedAnimPriorityQueue &loopAnimPriorityQueue, Float time )
{
	if ( m_currentAtomicBoidNode )
	{
		m_currentAtomicBoidNode->Deactivate(this);

		m_currentAtomicBoidNode = m_currentAtomicBoidNode->GetNextAtomicAnimNode();
		if ( m_currentAtomicBoidNode )
		{
			m_currentAtomicBoidNode->Activate( this, loopAnimPriorityQueue, time, false );
		}
	}
}