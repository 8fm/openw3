
#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "expBlendChainExecutor.h"
#include "expEvents.h"
#include "expIntarface.h"

ExpBlendChainExecutor::ExpBlendChainExecutor( const ExecutorSetup& setup, IExpExecutor* destExe, const CName& animEnding, const CName& animStarting, Float blendTime )
	: ExpBaseExecutor( setup, 0.0f, 0.0f, 0.0f )
	, m_destExe( destExe )
	, m_currentTime( 0.0f )
{
	m_animationEntry[0] = FindAnimation( setup.m_entity, animEnding );
	if ( m_animationEntry[0] )
	{
		m_animationStates[0].m_animation = m_animationEntry[0]->GetName();
		m_animationStates[0].m_currTime = m_animationEntry[0]->GetDuration() * 0.99f;
		m_animationStates[0].m_prevTime = m_animationStates[0].m_currTime;
	}

	m_animationEntry[1] = FindAnimation( setup.m_entity, animStarting );
	if ( m_animationEntry[1] )
	{
		m_animationStates[1].m_animation = m_animationEntry[1]->GetName();
		m_animationStates[1].m_currTime = 0.f;
		m_animationStates[1].m_prevTime = 0.f;
	}

	m_duration = blendTime;
}

ExpBlendChainExecutor::~ExpBlendChainExecutor()
{

}

void ExpBlendChainExecutor::SyncAnim( Float time )
{
	if ( m_slot.IsValid() )
	{
		m_slot.PlayAnimations( m_animationStates[0], m_animationStates[1], m_duration != 0.0f? 0.0f : 1.0f, 1.0f );
	}
}

void ExpBlendChainExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	ExpBaseExecutor::Update( context, result );

	if ( result.m_finished )
	{
		result.m_nextExe = m_destExe;
	}
}

Bool ExpBlendChainExecutor::UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result )
{
	m_currentTime += dt;
	if ( m_slot.IsValid() )
	{
		m_slot.PlayAnimations( m_animationStates[0], m_animationStates[1], m_duration != 0.0f? Clamp(m_currentTime / m_duration, 0.0f, 1.0f) : 1.0f, 1.0f );
	}

	timeRest = Max( 0.001f, m_currentTime - m_duration ); // time rest should be non zero, so next exe will be updated this frame
	return m_currentTime >= m_duration;
}

