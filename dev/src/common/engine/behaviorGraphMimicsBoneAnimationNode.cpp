
#include "build.h"
#include "behaviorGraphMimicsBoneAnimationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphContext.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsBoneAnimationNode );

CBehaviorGraphMimicsBoneAnimationNode::CBehaviorGraphMimicsBoneAnimationNode()	
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicsBoneAnimationNode::GetCaption() const
{
	if ( m_animationName.Empty() )
	{
		return TXT("Mimics Bone Anim"); 
	}

	return String::Printf( TXT( "Mimics Bone Anim [ %s ]" ), m_animationName.AsString().AsChar() );
}

#endif

void CBehaviorGraphMimicsBoneAnimationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	SMimicPostProcessData* mimicData = context.GetMimicPostProcessData();
	if ( mimicData )
	{
		FillMimicData( instance, mimicData, output );
	}
	else
	{
		ASSERT( mimicData );
	}

	//if ( context.ShouldCorrectPose() )
	//{
	//	context.SetPoseCorrectionIdentity( output );
	//}
}

Bool CBehaviorGraphMimicsBoneAnimationNode::ShouldDoPoseCorrection() const
{
	return false;
}

void CBehaviorGraphMimicsBoneAnimationNode::FillMimicData( CBehaviorGraphInstance& instance, SMimicPostProcessData* mimicData, const SBehaviorGraphOutput &pose ) const
{
	COMPILE_ASSERT( MIMIC_POSE_BONES_NUM == 0 );

	/*if ( const CMimicComponent* head = Cast< const CMimicComponent >( instance.GetAnimatedComponent() ) )
	{
		if ( const CMimicFace* face = head->GetMimicFace() )
		{
			Int32 headIdx = -1;
			Int32 neckIdx = -1;

			face->GetNeckAndHead( neckIdx, headIdx );

			const Int32 numBones = (Int32)pose.m_numBones;

			if ( headIdx != -1 && neckIdx != -1 && headIdx < numBones && neckIdx < numBones )
			{
				mimicData->m_mimicBones[ 0 ] = pose.m_outputPose[ neckIdx ];
				mimicData->m_mimicBones[ 1 ] = pose.m_outputPose[ headIdx ];
			}
		}
	}*/
}
