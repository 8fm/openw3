#include "build.h"
#include "..\..\..\build.h" // hack for using precompiled header on PS4 (only the case when you have cpp file in subfolders, hope that engine team will fix this correctly)
#include "behaviorGraphFootStepDetectorNode.h"
#include "..\..\..\..\engine\behaviorIncludes.h"
#include "..\..\..\..\core\instanceDataLayoutCompiler.h"
#include "..\..\..\..\engine\behaviorGraphOutput.h"
#include "..\..\..\..\engine\behaviorGraphContext.h"
#include "..\..\..\extAnimFootstepEvent.h"
#include "..\..\..\..\engine\behaviorGraphInstance.h"
#include "..\..\PostActions\behaviorGraphPostActions.h"
#include "..\..\..\..\core\object.h"
#include "..\..\..\..\engine\entity.h"
#include "..\..\..\..\engine\actorInterface.h"
#include "..\..\..\..\engine\visualDebug.h"

IMPLEMENT_ENGINE_CLASS( SFootDetectionBoneInfo );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFootStepDetectorNode );

RED_DEFINE_STATIC_NAME( l_toe )
RED_DEFINE_STATIC_NAME( r_toe )

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const 
{
	TBaseClass::Sample( context, instance, output );

	Float dt = instance[ i_dt ];

	if ( dt < 0.00001f )
	{
		return; // do not update if dt is 0
	}

	Vector deltaRef = BehaviorUtils::ConvertToVector( output.m_deltaReferenceFrameLocal.GetTranslation() );
	Float deltaRefFrameAvgSpeed = deltaRef.Mag3() / dt;
	
	// left foot step detection:
	UpdateFootDetectionForBone( context, instance, output, instance[ i_leftFootBoneInfo ], deltaRef, deltaRefFrameAvgSpeed, dt, ESide::S_Left);

	// right foot step detection:
	UpdateFootDetectionForBone( context, instance, output, instance[ i_rightFootBoneInfo ], deltaRef, deltaRefFrameAvgSpeed, dt, ESide::S_Right);
}

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const 
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_dt ] = timeDelta;
}

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_rightFootBoneInfo;
	compiler << i_leftFootBoneInfo;

	compiler << i_dt;
}

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::OnInitInstance( CBehaviorGraphInstance& instance ) const 
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_leftFootBoneInfo ].m_boneIdx = FindBoneIndex( CNAME( l_toe ), instance );
	instance[ i_rightFootBoneInfo ].m_boneIdx = FindBoneIndex( CNAME( r_toe ), instance );

	ASSERT( instance[ i_leftFootBoneInfo ].m_boneIdx  != -1 );
	ASSERT( instance[ i_rightFootBoneInfo ].m_boneIdx != -1 );
}

//////////////////////////////////////////////////////////////////////////
Bool CBehaviorGraphFootStepDetectorNode::DetectFootStep(const Vector& localBonePosThisFrame, const Vector& localBonePosePrevFrame, const Vector& delatRefFrame, Float deltaRefFrameAvgSpeed, Float dt) const
{
	// Simple detection algorithm
	Vector localboneDiff = localBonePosThisFrame - localBonePosePrevFrame; 

	Vector boneDisplacmentMS = localboneDiff + delatRefFrame;

	Float boneAvgSpeedMS = boneDisplacmentMS.Mag3() / dt;

	// So, foot step is detected, when foot has max 20% of reference frame, or simply the speed is low.
	// This means that the faster skeleton is moving, the tolerance for step is higher.
	// Remember that this operates on average speed not instant speed as we would like!

	// Values are chosen based on experiments.
	if ( ( ( boneAvgSpeedMS / deltaRefFrameAvgSpeed ) < 0.2f) || ( boneAvgSpeedMS < 0.1f ) )
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::GenerateFootStepAction( Int32 boneIdx, SBehaviorSampleContext& context ) const
{
	CFootStepAction* action = context.GetOneFrameAllocator().CreateOnStack<CFootStepAction>();

	if ( action )
	{
		action->m_boneIndex = boneIdx;
		context.AddPostAction( action );
	}
}

//////////////////////////////////////////////////////////////////////////
Bool CBehaviorGraphFootStepDetectorNode::UpdateFootStepActionForBone(Int32 boneIdx, SBehaviorSampleContext& context, Bool shouldSendThisFrame, Uint16& eventFrameCounter) const
{
	// [Notes]: rethink this, maybe this should be based on dist between foots ?
	Bool ret = false;
	if ( shouldSendThisFrame )
	{
		if ( eventFrameCounter == 0 )
		{
			// Generate action only if it wasn't generated yet in previous frame!
			// Foot detection works on frame basis, so ideally if foot is on the ground - algorithm should detect this every frame.
			// In practice, the algorithm can fail on single frame, so here we compensate for this.
			GenerateFootStepAction( boneIdx, context );
			ret = true;
		}
		eventFrameCounter = 2;
	}
	else
	{
		eventFrameCounter = eventFrameCounter > 0 ? --eventFrameCounter : 0;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::OnGenerateFragments(CBehaviorGraphInstance& instance, CRenderFrame* frame) const 
{
	TBaseClass::OnGenerateFragments(instance, frame);

#ifndef RED_FINAL_BUILD

	RenderDebug(instance.GetAnimatedComponent(), instance[ i_leftFootBoneInfo ], Color::RED);
	RenderDebug(instance.GetAnimatedComponent(), instance[ i_rightFootBoneInfo ], Color::GREEN);

#endif
}

//////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::UpdateFootDetectionForBone(SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output, SFootDetectionBoneInfo& boneInfo, const Vector& deltaRefFrame, Float deltaRefAvgSpeed, Float dt, ESide side) const
{
	Int32 leg_id = boneInfo.m_boneIdx;
	AnimQsTransform local_transform = output.GetBoneModelTransform( instance.GetAnimatedComponent(), leg_id );
	Vector pos = BehaviorUtils::ConvertToVector( local_transform.GetTranslation() );

	// try to detect footstep
	Bool isFootStepDetected = DetectFootStep( pos, boneInfo.m_prevFrameLocalPos, deltaRefFrame, deltaRefAvgSpeed, dt );

	// check if we have forced step
	isFootStepDetected |= DoesEventOccured( output, side );

	// decide about generating appropriate action
_DBG_ONLY_CODE_( boneInfo.m_debugFoundStepThisFrame = ) UpdateFootStepActionForBone( leg_id, context, isFootStepDetected, boneInfo.m_eventFrameCounter );

	boneInfo.m_prevFrameLocalPos = pos;
} 

//////////////////////////////////////////////////////////////
Bool CBehaviorGraphFootStepDetectorNode::DoesEventOccured(const SBehaviorGraphOutput &output, ESide side ) const
{
	for ( Uint32 i = 0; i < output.m_numEventsFired; ++i )
	{
		const CExtAnimEvent* event = output.m_eventsFired[ i ].m_extEvent;
		if ( IsType< CExtForcedLogicalFootstepAnimEvent >( event ) )
		{
			const CExtForcedLogicalFootstepAnimEvent* wantedEvent = static_cast< const CExtForcedLogicalFootstepAnimEvent* > ( event );
			if ( wantedEvent->GetSide() == side )
			{
				return true;
			}
		}
	}
	return false;
}

#ifndef RED_FINAL_BUILD
//////////////////////////////////////////////////////////////
void CBehaviorGraphFootStepDetectorNode::RenderDebug(const CAnimatedComponent* ac, const SFootDetectionBoneInfo& boneInfo, const Color& color) const
{
	if ( boneInfo.m_debugFoundStepThisFrame )
	{
		IActorInterface* actorInter = ac->GetEntity()->QueryActorInterface();
		if ( actorInter && actorInter->GetVisualDebug() )
		{
			Vector pos = ac->GetBoneMatrixWorldSpace( boneInfo.m_boneIdx).GetTranslation();
			actorInter->GetVisualDebug()->AddSphere( CName::NONE, 0.1f, pos, true, color, 15.0f );
		}
	}
}
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif