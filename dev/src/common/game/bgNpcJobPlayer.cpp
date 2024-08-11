
#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "bgNpcJobPlayer.h"

#include "../../common/game/jobTreeLeaf.h"
#include "../../common/game/jobTreeNode.h"

#include "../../common/game/actorActionWork.h"
#include "../engine/skeletalAnimationEntry.h"
#include "../engine/skeletalAnimationContainer.h"

//#pragma optimize("",off)

#define SAFE_ASSERT( x )
//#define SAFE_ASSERT( x ) ASSERT( x )
//#define SAFE_ASSERT( x ) { if ( ::SIsMainThread() ) { ASSERT( x ); } else { if ( !x ) { String str = TXT(#x); LOG_GAME( TXT("Safe Assert '%ls'"), str.AsChar() );  } } }

//////////////////////////////////////////////////////////////////////////

CJobTreePlayer::CJobTreePlayer( TSoftHandle< CJobTree >& tree, const CName& category, IJobTreeAnimObject* object )
	: m_tree( tree )
	, m_object( object )
	, m_state( JTPS_Loading )
	, m_randStartingAnimTime( true )
	, m_canSkipFirstPreJob( true )
	, m_category( category )
	, m_requestedJobEnd( false )
	, m_hasEnded( false )
{
	
}

CJobTreePlayer::~CJobTreePlayer()
{
	m_tree.Release();
}

void CJobTreePlayer::Update( const Float dt )
{
	Float dtForA = dt;
	Float dtForB = dt;

	Bool running = true;

	Uint32 debugCounter = 0;

	while ( running )
	{
		if ( debugCounter > 10 )
		{
			running = false;
			SAFE_ASSERT( 0 );
		}

		switch ( m_state )
		{
		case JTPS_Loading:
			{
				BaseSoftHandle::EAsyncLoadingResult ret = m_tree.GetAsync();

				if ( ret == BaseSoftHandle::ALR_Loaded )
				{
					m_treeContext.Reset();
					m_treeContext.m_currentCategories.PushBackUnique( m_category );

					SetState( JTPS_PlayNextAnimation );
				}
				else if ( ret == BaseSoftHandle::ALR_Failed )
				{
					SetState( JTPS_Error );
				}
				else if ( ret == BaseSoftHandle::ALR_InProgress )
				{
					running = false;
				}
				else
				{
					running = false;

					SAFE_ASSERT( 0 );
				}

				debugCounter++;

				break;
			}

		case JTPS_PlayNextAnimation:
			{
				if ( SelectNextJobAnimation( m_nodeA ) )
				{
					SetState( JTPS_UpdateAnimation );
				}
				else
				{
					SetState( JTPS_Error );
				}

				debugCounter++;

				break;
			}

		case JTPS_UpdateAnimation:
			{
				SAFE_ASSERT( dtForA >= 0.f );

				m_nodeA.m_prevTime = m_nodeA.m_currTime;
				m_nodeA.m_currTime += m_nodeA.m_speed * dtForA;

				dtForA = 0.f;

				if ( m_nodeA.m_currTime > m_nodeA.m_duration )
				{
					dtForA = m_nodeA.m_currTime - m_nodeA.m_duration;
					SAFE_ASSERT( dtForA >= 0.f );

					SetState( JTPS_PlayNextAnimation );
				}
				else if ( m_nodeA.m_currTime > m_nodeA.m_blendTime )
				{
					SetState( JTPS_StartBlendingAnimations );
				}
				else
				{
					SyncObjectToAnimation( m_nodeA );

					running = false;
				}

				debugCounter++;

				break;
			}

		case JTPS_StartBlendingAnimations:
			{
				SAFE_ASSERT( m_nodeA.m_currTime <= m_nodeA.m_duration );
				SAFE_ASSERT( m_nodeA.m_duration - m_nodeA.m_blendTime > 0.f );

				if ( SelectNextJobAnimation( m_nodeB ) )
				{
					dtForB = m_nodeA.m_blendTime - m_nodeA.m_prevTime;

					SAFE_ASSERT( MAbs( dtForA ) < 0.0001f );
					SAFE_ASSERT( dtForB >= 0.f );
					SAFE_ASSERT( dtForA >= 0.f );

					SetState( JTPS_BlendAnimations );
				}
				else
				{
					SetState( JTPS_Error );
				}

				debugCounter++;

				break;
			}

		case JTPS_BlendAnimations:
			{
				SAFE_ASSERT( dtForA >= 0.f );
				SAFE_ASSERT( dtForB >= 0.f );

				m_nodeA.m_prevTime = m_nodeA.m_currTime;
				m_nodeA.m_currTime += m_nodeA.m_speed * dtForA;

				m_nodeB.m_prevTime = m_nodeB.m_currTime;
				m_nodeB.m_currTime = Min< Float >( m_nodeB.m_currTime + m_nodeB.m_speed * dtForB, m_nodeB.m_duration );

				dtForA = 0.f;
				dtForB = 0.f;

				SAFE_ASSERT( m_nodeA.m_duration - m_nodeA.m_blendTime > 0.f );

				if ( m_nodeA.m_currTime > m_nodeA.m_duration )
				{
					m_nodeA = m_nodeB;

					SetState( JTPS_UpdateAnimation );
				}
				else
				{
					Float weight = ( m_nodeA.m_currTime - m_nodeA.m_blendTime ) / ( m_nodeA.m_duration - m_nodeA.m_blendTime );
					SAFE_ASSERT( weight >= 0.f && weight <= 1.f );

					SyncObjectToAnimations( m_nodeA, m_nodeB, weight );

					running = false;
				}

				debugCounter++;

				break;
			}

		case JTPS_Error:
			{
				//ERR_GAME( TXT("CJobTreePlayer ERROR. Please DEBUG, Job tree '%ls'"), m_tree.GetPath().AsChar() );

				SetState( JTPS_Loading );

				running = false;
			}
		}
	}
}

CName CJobTreePlayer::GetCurrentActionsItemName() const
{
	return m_currActionItemName;
}

void CJobTreePlayer::SetState( EJobTreePlayerState state )
{
	m_state = state;
}

Bool CJobTreePlayer::SelectNextJobAnimation( JobNodeState& state )
{
	SAFE_ASSERT( m_tree.Get() );

	CJobTreeNode* rootNode = m_tree.Get()->GetRootNode();
	if ( !rootNode )
	{
		// Job tree is invalid
		return false;
	}

	const CJobActionBase* action = NULL;
	if ( m_requestedJobEnd )
	{
		action = rootNode->GetNextExitAction( m_treeContext );
		if ( !action )
		{
			m_hasEnded = true;
		}
	}
	else
	{
		action = rootNode->GetNextAction( m_treeContext );
	}	
	
	if ( action )
	{
		const CJobAction* ja = Cast< const CJobAction >( action );

		if ( m_canSkipFirstPreJob )
		{
			m_canSkipFirstPreJob = false;

			if ( m_treeContext.IsCurrentActionApproach() )
			{
				// Skip this job
				SelectNextJobAnimation( state );
			}
		}

		m_currActionItemName = ja->GetItemName();

		CName animation = action->GetAnimName();
		Float duration = m_object->GetJobAnimationDuration( animation );

		if ( duration > 0.f )
		{
			const Float speeds[ 3 ] = { 0.95f, 1.f, 1.05f };
		
			const Uint32 speedNum = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 3 );
			SAFE_ASSERT( speedNum >= 0 && speedNum <= 2 );

			const Float s = speeds[ speedNum ];

			state.Set( action->GetBlendOut(), duration, animation, s );

			if ( m_randStartingAnimTime )
			{
				const Float weight = GEngine->GetRandomNumberGenerator().Get< Float >( 0.9f );
				state.m_currTime = weight * state.m_duration;

				m_randStartingAnimTime = false;
			}

			return true;
		}
	}

	return false;
}

void CJobTreePlayer::SyncObjectToAnimation( JobNodeState& state )
{
	SAnimationState data;
	data.m_animation = state.m_animation;
	data.m_prevTime = state.m_prevTime;
	data.m_currTime = state.m_currTime;

	SAFE_ASSERT( data.m_prevTime <= state.m_duration );
	SAFE_ASSERT( data.m_currTime <= state.m_duration );
	SAFE_ASSERT( data.m_prevTime >= 0.f );
	SAFE_ASSERT( data.m_currTime >= 0.f );

	m_object->PlayJobAnimation( data );
}

void CJobTreePlayer::SyncObjectToAnimations( JobNodeState& stateA, JobNodeState& stateB, Float weight )
{
	SAnimationState dataA;
	dataA.m_animation = stateA.m_animation;
	dataA.m_prevTime = stateA.m_prevTime;
	dataA.m_currTime = stateA.m_currTime;

	SAnimationState dataB;
	dataB.m_animation = stateB.m_animation;
	dataB.m_prevTime = stateB.m_prevTime;
	dataB.m_currTime = stateB.m_currTime;

	SAFE_ASSERT( dataA.m_prevTime <= stateA.m_duration );
	SAFE_ASSERT( dataA.m_currTime <= stateA.m_duration );
	SAFE_ASSERT( dataA.m_prevTime >= 0.f );
	SAFE_ASSERT( dataA.m_currTime >= 0.f );

	SAFE_ASSERT( dataB.m_prevTime <= stateB.m_duration );
	SAFE_ASSERT( dataB.m_currTime <= stateB.m_duration );
	SAFE_ASSERT( dataB.m_prevTime >= 0.f );
	SAFE_ASSERT( dataB.m_currTime >= 0.f );

	m_object->PlayJobAnimations( dataA, dataB, weight );
}

//////////////////////////////////////////////////////////////////////////

CJobTreeBehaviorSlot::CJobTreeBehaviorSlot()
	: m_owner( NULL )
{

}

CJobTreeBehaviorSlot::~CJobTreeBehaviorSlot()
{
	Clear();
}

Bool CJobTreeBehaviorSlot::Initialize( const CAnimatedComponent* ac )
{
	m_owner = ac;

	CBehaviorGraphStack* stack = m_owner->GetBehaviorStack();
	if ( stack )
	{
		return stack->GetSlot( ActorActionWork::WORK_ANIM_SLOT_NAME, m_slot );
	}
	
	return false;
}

void CJobTreeBehaviorSlot::Clear()
{
	m_slot.Clear();
	m_owner = NULL;
}

void CJobTreeBehaviorSlot::PlayJobAnimation( const SAnimationState& animation )
{
	if ( m_slot.IsValid() && m_owner )
	{
		m_slot.PlayAnimation( animation );
	}
	else
	{
		SAFE_ASSERT( m_slot.IsValid() );
		SAFE_ASSERT( m_owner );
	}
}

void CJobTreeBehaviorSlot::PlayJobAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float weight )
{
	if ( m_slot.IsValid() && m_owner )
	{
		m_slot.PlayAnimations( animationA, animationB, weight );
	}
	else
	{
		SAFE_ASSERT( m_slot.IsValid() );
		SAFE_ASSERT( m_owner );
	}
}

Float CJobTreeBehaviorSlot::GetJobAnimationDuration( const CName& animation ) const
{
	if ( m_owner && m_owner->GetAnimationContainer() )
	{
		const CSkeletalAnimationSetEntry* anim = m_owner->GetAnimationContainer()->FindAnimation( animation );
		return anim ? anim->GetDuration() : 0.f;
	}
	else
	{
		if ( m_owner )
		{
			SAFE_ASSERT( m_owner->GetAnimationContainer() );
		}
		else
		{
			SAFE_ASSERT( m_owner );
		}

		return 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////

//...

//////////////////////////////////////////////////////////////////////////

void CJobTreeDebugObject::PlayJobAnimation( const SAnimationState& animation )
{
	LOG_GAME( TXT("Anim %s %f"), animation.m_animation.AsString().AsChar(), animation.m_currTime );
}

void CJobTreeDebugObject::PlayJobAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float weight )
{
	LOG_GAME( TXT("Anim %f %s %f %s %f"), weight,
		animationA.m_animation.AsString().AsChar(), animationA.m_currTime,
		animationB.m_animation.AsString().AsChar(), animationB.m_currTime );
}

Float CJobTreeDebugObject::GetJobAnimationDuration( const CName& animation ) const
{
	return 3.f;
}

//#pragma optimize("",on)
