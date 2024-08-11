
#include "build.h"
#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "../engine/animatedComponent.h"
#include "../engine/mimicComponent.h"
#include "../engine/entity.h"
#include "actorInterface.h"
#include "poseProvider.h"

IMPLEMENT_ENGINE_CLASS( CAllocatedBehaviorGraphOutput );

CAllocatedBehaviorGraphOutput::CAllocatedBehaviorGraphOutput()
	: m_poseAlloc( NULL )
	, m_mimicAlloc( NULL )
{
}

CAllocatedBehaviorGraphOutput::~CAllocatedBehaviorGraphOutput()
{
	if ( m_pose )
	{
		// pose may still exist when snapshot is being destroyed
		Free();
	}
	ASSERT( !m_pose );
}

void CAllocatedBehaviorGraphOutput::Create( CBehaviorGraphInstance& instance, Bool mimic )
{
	Create( instance.GetAnimatedComponent()->GetBehaviorGraphSampleContext()->GetPoseProvider(), instance.GetAnimatedComponent()->GetBehaviorGraphSampleContext()->GetMimicProvider(), mimic );
}

void CAllocatedBehaviorGraphOutput::Create( CPoseProvider* poseAlloc, CPoseProvider* mimicAlloc, Bool mimic )
{
	ASSERT( !m_pose );

	m_mimic = mimic;
	m_poseAlloc = poseAlloc;
	m_mimicAlloc = mimicAlloc;

	if ( m_mimic )
	{
		if ( m_mimicAlloc )
		{
			m_pose = m_mimicAlloc->AcquirePose();
		}
	}
	else
	{
		if ( m_poseAlloc )
		{
			m_pose = m_poseAlloc->AcquirePose();
		}
		else
		{
			ASSERT( m_poseAlloc );
		}

		ASSERT( m_pose );
	}
}

void CAllocatedBehaviorGraphOutput::Cache( CBehaviorGraphInstance& instance )
{
	Cache( instance.GetAnimatedComponent() );
}

void CAllocatedBehaviorGraphOutput::Cache( const CAnimatedComponent* ac )
{
	if ( m_pose )
	{
		if ( m_mimic )
		{
			// TODO
			// PTom: to jest zle - do naprawienia
			const IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
			if ( actor )
			{
				const CMimicComponent* head = actor->GetMimicComponent();
				if ( head )
				{
					m_pose->SetPose( head );

					for ( Uint32 i=0; i<m_pose->m_numBones; ++i )
					{
#ifdef USE_HAVOK_ANIMATION
						m_pose->m_outputPose[ i ].setIdentity();
#else
						m_pose->m_outputPose[ i ].SetIdentity();
#endif
					}
				}
			}
		}
		else
		{
			m_pose->SetPose( ac );
		}
	}
}

void CAllocatedBehaviorGraphOutput::Cache( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseToCache )
{
	Cache( instance.GetAnimatedComponent(), poseToCache );
}

void CAllocatedBehaviorGraphOutput::Cache( const CAnimatedComponent* ac, const SBehaviorGraphOutput &poseToCache )
{
	if ( m_pose )
	{
		if ( m_mimic )
		{
			ASSERT( false, TXT("Not supported ATM") );
			// animated component is here to support head
		}
		else
		{
			m_pose->SetPose( poseToCache );
		}
	}
}

void CAllocatedBehaviorGraphOutput::CreateAndCache( CBehaviorGraphInstance& instance, Bool mimic )
{
	Create( instance, mimic );
	Cache( instance );
}

void CAllocatedBehaviorGraphOutput::Free( CBehaviorGraphInstance& instance )
{
	Free();
}

void CAllocatedBehaviorGraphOutput::Free()
{
	m_pose.Reset();
}

CAllocatedBehaviorGraphOutput& CAllocatedBehaviorGraphOutput::operator=( const CAllocatedBehaviorGraphOutput& rhs )
{
	// remove if there isn't pose or if mimic is different
	if ((m_pose && ! rhs.m_pose) || (m_mimic != rhs.m_mimic))
	{
		Free();
	}
	if (! m_pose && rhs.m_pose)
	{
		Create( rhs.m_poseAlloc, rhs.m_mimicAlloc, rhs.m_mimic );
	}
	if (m_pose && rhs.m_pose)
	{
		m_pose->operator =(*rhs.m_pose);
	}
	return *this;
}
