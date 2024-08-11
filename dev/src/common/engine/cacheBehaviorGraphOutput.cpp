
#include "build.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"

CCacheBehaviorGraphOutput::CCacheBehaviorGraphOutput( SBehaviorSampleContext& context, Bool mimic )
	: m_context( context )
	, m_mimic( mimic )
{
	if ( !m_mimic )
	{
		m_pose = m_context.GetPose();
	}
	else
	{
		m_pose = m_context.GetMimicPose();
	}

#ifdef DEBUG_CORRUPT_TRANSFORMS
	if ( m_pose )
	{
		const Uint32 num = m_pose->m_numBones;
		for ( Uint32 i=0; i<num; ++i )
		{
			RED_ASSERT( m_pose->m_outputPose[i].Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be corrupt!") );
			RED_ASSERT( m_pose->m_outputPose[i].Translation.IsOk(), TXT("Animation translation data appears to be corrupt!") );
			RED_ASSERT( m_pose->m_outputPose[i].Scale.IsOk(), TXT("Animation scale data appears to be corrupt!") );
		}
	}
#endif
}

CCacheBehaviorGraphOutput::~CCacheBehaviorGraphOutput()
{
#ifdef DEBUG_CORRUPT_TRANSFORMS
	if ( m_pose )
	{
		const Uint32 num = m_pose->m_numBones;
		for ( Uint32 i=0; i<num; ++i )
		{
			RED_ASSERT( m_pose->m_outputPose[i].Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be courrpt!") );
			RED_ASSERT( m_pose->m_outputPose[i].Translation.IsOk(), TXT("Animation translation data appears to be courrpt!") );
			RED_ASSERT( m_pose->m_outputPose[i].Scale.IsOk(), TXT("Animation scale data appears to be courrpt!") );
		}
	}
#endif

	m_pose.Reset();
}
