
#include "build.h"
#include "animationJobs.h"
#include "animatedComponent.h"
#include "../core/taskBatch.h"

CJobImmediateUpdateAnimation::CJobImmediateUpdateAnimation( STaskBatchSyncToken& syncToken, CAnimatedComponent* component )
	: m_syncToken( syncToken )
	, m_component( component )
{
	RED_ASSERT( m_component );
}

void CJobImmediateUpdateAnimation::Run()
{
	PC_SCOPE_PIX( CJobImmediateUpdateAnimation );

	if ( m_context.m_updateAnimations )
	{
		//PC_SCOPE( UpdateAndSampleAnimationJobImmediate );
		m_component->InternalUpdateAndSampleMultiAsyncPart( m_context.m_timeDelta );
	}
	if ( m_context.m_updatePoseConstraints )
	{
		//PC_SCOPE( UpdatePoseConstraintsJobImmediate );
		m_component->UpdatePoseConstraints( m_context.m_timeDelta );
	}
	m_syncToken.Sync();
}

//////////////////////////////////////////////////////////////////////////

CJobAsyncUpdateAnimation::CJobAsyncUpdateAnimation( CAnimatedComponent* component, CJobImmediateUpdateAnimationContext context )
	: m_context( context )
	, m_component( component )
{

}

CJobAsyncUpdateAnimation::~CJobAsyncUpdateAnimation()
{

}

void CJobAsyncUpdateAnimation::Run()
{
	PC_SCOPE_PIX( CJobAsyncUpdateAnimation );

	if ( m_context.m_updateAnimations )
	{
		//PC_SCOPE( UpdateAndSampleAnimationJobAsync );
		m_component->InternalUpdateAndSampleMultiAsyncPart( m_context.m_timeDelta );
	}
}
