
#include "build.h"
#include "animationTrajectorySync.h"
#include "../core/scriptStackFrame.h"


IMPLEMENT_RTTI_ENUM( EActionMoveAnimationSyncType );
IMPLEMENT_ENGINE_CLASS( CActionMoveAnimationProxy );

CActionMoveAnimationProxy::CActionMoveAnimationProxy()
	: m_isValid( false )
	, m_duration( 1.f )
	, m_prevTime( 0.f )
	, m_currTime( 0.f )
	, m_finished( false )
	, m_isInitialized( false )
{

}

Bool CActionMoveAnimationProxy::IsValid() const
{
	return m_isValid;
}

Bool CActionMoveAnimationProxy::IsInitialized() const
{
	return m_isInitialized;
}

Bool CActionMoveAnimationProxy::WillBeFinished( Float time ) const
{
	ASSERT( IsValid() );
	return m_currTime + time >= m_duration;
}

Bool CActionMoveAnimationProxy::IsFinished() const
{
	ASSERT( IsValid() );
	return m_currTime >= m_duration;
}

void CActionMoveAnimationProxy::funcIsInitialized( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool ret = IsInitialized();
	RETURN_BOOL( ret );
}

void CActionMoveAnimationProxy::funcIsValid( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool ret = IsValid();
	RETURN_BOOL( ret );
}

void CActionMoveAnimationProxy::funcIsFinished( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Bool ret = IsFinished();
	RETURN_BOOL( ret );
}

void CActionMoveAnimationProxy::funcWillBeFinished( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, time, 0.f );
	FINISH_PARAMETERS;

	const Bool ret = WillBeFinished( time );
	RETURN_BOOL( ret );
}
