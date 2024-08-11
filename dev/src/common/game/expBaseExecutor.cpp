
#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/behaviorGraphInstance.h"

#include "expBaseExecutor.h"
#include "expIntarface.h"

#include "../engine/skeletalAnimationContainer.h"

ExpBaseExecutor::ExpBaseExecutor( const ExecutorSetup& setup, Float blendIn, Float blendOut, Float earlyEndOffset )
	: m_entity( setup.m_entity )
	, m_duration( 1.f )
	, m_timeMul( 1.f )
	, m_blendIn( blendIn )
	, m_blendOut( blendOut )
	, m_earlyEndOffset( earlyEndOffset )
	, m_firstUpdate( true )
	, m_raisedBehaviorEventAtEnd( false )
	, m_blendOutOnEnd( false )
	, m_stayInSlotAfterEnd( false )
{
	FindAnimSlot( setup.m_entity, CNAME( EXP_SLOT ), m_slot );
}

ExpBaseExecutor::~ExpBaseExecutor()
{
	if ( ! m_stayInSlotAfterEnd )
	{
		m_slot.BlendOut( m_blendOut, true ); // TODO different weight? or just blend out
	}
}

void ExpBaseExecutor::SetTimeMul( Float p )
{
	m_timeMul = p;
}

void ExpBaseExecutor::SyncAnimToStart()
{
	SyncAnim( 0.f );
}

void ExpBaseExecutor::SyncAnimToEnd()
{
	SyncAnim( m_duration );
}

Float ExpBaseExecutor::CalcWeight( Float time ) const
{
	if ( m_blendIn > 0.f && time < m_blendIn )
	{
		const Float w = time / m_blendIn;
		ASSERT( w >= 0.f && w <= 1.f );
		return w;
	}
	else 
	{
		Float endsAt = Min( m_duration - m_earlyEndOffset, m_duration - m_blendOut );
		if ( m_blendOut > 0.f && time > endsAt )
		{
			const Float w = Clamp( 1.0f - (time - endsAt) / m_blendOut, 0.f, 1.f );
			ASSERT( w >= 0.f && w <= 1.f ); // this is an assert just in case clamp would not work, right? :D
			return w;
		}
	}

	return 1.f;
}

Bool ExpBaseExecutor::IsBlendingOut( Float time ) const
{
	if ( m_blendIn > 0.f && time < m_blendIn )
	{
		return false;
	}
	else 
	{
		Float endsAt = Min( m_duration - m_earlyEndOffset, m_duration - m_blendOut );
		if ( time > endsAt )
		{
			return true;
		}
	}

	return false;
}

void ExpBaseExecutor::WrapTime( Float& time, Float& marker ) const
{
	if ( time < 0.f )
	{
		marker = time;
		time = Max( 0.f, m_duration + time );
	}
	else
	{
		marker = 0.f;
	}
}

void ExpBaseExecutor::UnwrapTime( Float& time, Float& marker ) const
{
	if ( marker < 0.f )
	{
		time = marker;
	}
}

void ExpBaseExecutor::AlignPreTimeToCurr( Float& prev, const Float curr ) const
{
	if ( m_timeMul < 0.f && prev < curr )
	{
		prev = m_duration - prev;
	}
}

void ExpBaseExecutor::RaiseBehaviorEventAtEnd()
{
	if ( ! m_raisedBehaviorEventAtEnd && ! m_raiseBehaviorEventAtEnd.Empty() )
	{
		m_slot.GetInstance()->GenerateForceEvent( m_raiseBehaviorEventAtEnd );
	}
	m_raisedBehaviorEventAtEnd = true;
}

void ExpBaseExecutor::CloseSlot()
{
	if ( m_slot.IsValid() )
	{
		RaiseBehaviorEventAtEnd();
		if ( ! m_callScriptEventAtEnd.Empty() )
		{
			m_slot.GetInstance()->GetAnimatedComponentUnsafe()->GetEntity()->CallEvent( m_callScriptEventAtEnd );
		}
		if ( ! m_stayInSlotAfterEnd )
		{
			if ( m_earlyEndOffset > 0.0f || m_blendOutOnEnd )
			{
				// if it is already blended out then we're fine
				m_slot.BlendOut( m_blendOut, true );
			}
			else
			{
				m_slot.ResetMotion();
				m_slot.Stop();
			}
		}
	}
}

void ExpBaseExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	result.m_finished = UpdateAnimation( context.m_dt, result.m_timeRest, result );
	if ( result.m_finished )
	{
		CloseSlot();
	}
}

void ExpBaseExecutor::GenerateDebugFragments( CRenderFrame* frame )
{

}

Bool ExpBaseExecutor::FindAnimSlot( const CEntity* entity, const CName& slotName, CBehaviorManualSlotInterface& slot )
{
	const CAnimatedComponent* root = entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		return root->GetBehaviorStack()->GetSlot( slotName, slot );
	}
	return false;
}

const CSkeletalAnimationSetEntry* ExpBaseExecutor::FindAnimation( const CEntity* entity, const CName& animation )
{
	const CAnimatedComponent* root = entity->GetRootAnimatedComponent();
	if ( root && root->GetAnimationContainer() )
	{
		return root->GetAnimationContainer()->FindAnimation( animation );
	}
	return NULL;
}

void ExpBaseExecutor::OnActionStoped()
{
	m_stayInSlotAfterEnd = false;
}
