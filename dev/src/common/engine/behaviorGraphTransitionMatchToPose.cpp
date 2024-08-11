
#include "build.h"
#include "behaviorGraphTransitionMatchToPose.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animSyncInfo.h"
#include "behaviorProfiler.h"
#include "../core/mathUtils.h"
#include "behaviorIncludes.h"
#include "behaviorGraphUtils.inl"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

class CBehaviorGraphStateNode;

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionMatchToPoseNode );

CBehaviorGraphStateTransitionMatchToPoseNode::CBehaviorGraphStateTransitionMatchToPoseNode()
	: m_useMathMethod( true )
{
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_poseCached;
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_poseCached ] = false;
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	if ( m_useMathMethod )
	{
		DestroyPose( instance );
	}
}

String CBehaviorGraphStateTransitionMatchToPoseNode::GetCaption() const
{
	if ( m_useMathMethod )
	{
		if ( !m_name.Empty() )
		{
			return String::Printf( TXT("Match to pose transition [%s]"), m_name.AsChar() );
		}
		else
		{
			return String( TXT("Match to pose transition") );
		}
	}
	else
	{
		return TBaseClass::GetCaption();
	}
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( TransitionMatchTo );

	if ( !m_useMathMethod )
	{
		TBaseClass::OnUpdate( context, instance, timeDelta );
		return;
	}

	Float& currTime = instance[ i_currentTime ];
	currTime += timeDelta;

	// synchronize children playback
	Synchronize( instance, timeDelta );

	{
		if ( m_cachedStartStateNode && !m_cachedStartStateNode->IsActive( instance ) )
		{
			BEH_DUMP_ERROR( instance, TXT("CBehaviorGraphStateTransitionBlendNode::OnUpdate - !m_cachedStartStateNode->IsActive( instance ) ") );

			if ( m_cachedEndStateNode )
			{
				BEH_ERROR( TXT("CBehaviorGraphStateTransitionBlendNode: From %s To %s"), m_cachedStartStateNode->GetName().AsChar(), m_cachedEndStateNode->GetName().AsChar()  );
			}

			ASSERT( !m_cachedStartStateNode->IsActive( instance ) );
			m_cachedStartStateNode->Activate( instance );
		}

		if ( m_cachedEndStateNode && !m_cachedEndStateNode->IsActive( instance ) )
		{
			ASSERT( m_cachedEndStateNode->IsActive( instance ) );
			m_cachedEndStateNode->Activate( instance );
		}
	}

	// update start state
	if ( m_cachedStartStateNode )
	{
		m_cachedStartStateNode->Update( context, instance, timeDelta );
	}

	// transition has finished
	if ( currTime >= m_transitionTime )
	{
		const Float restTime = currTime - m_transitionTime;
		ASSERT( restTime >= 0.f );

		m_cachedEndStateNode->Update( context, instance, restTime );

		CBehaviorGraphStateMachineNode *stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );

		DisableSynchronization( instance );

		Deactivate( instance );

		ASSERT( m_cachedEndStateNode );

		stateMachine->SwitchToState( m_cachedEndStateNode, instance );

		currTime = 0.0f;
	}
}

void CBehaviorGraphStateTransitionMatchToPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( TransitionMatchTo );

	ASSERT( IsActive( instance ) );

	if ( !m_useMathMethod )
	{
		TBaseClass::Sample( context, instance, output );
		return;
	}

	CCacheBehaviorGraphOutput tempPose( context );
	SBehaviorGraphOutput* temp = tempPose.GetPose();

	SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();

	if ( temp && cachedPose && m_cachedStartStateNode && m_cachedEndStateNode )
	{
		Bool& poseCached = instance[ i_poseCached ];
		if ( !poseCached )
		{
			SBehaviorGraphOutput* poseA = temp;
			SBehaviorGraphOutput* poseB = cachedPose;

			CSyncInfo infoP, infoN;
			m_cachedStartStateNode->GetSyncInfo( instance, infoP );
			infoN = infoP;
			infoN.m_currTime = infoN.m_currTime + m_transitionTime;
			infoN.m_prevTime = infoN.m_currTime;
			m_cachedStartStateNode->SynchronizeTo( instance, infoN );

			m_cachedStartStateNode->Sample( context, instance, *poseA );
			m_cachedEndStateNode->Sample( context, instance, *poseB );

			m_cachedStartStateNode->SynchronizeTo( instance, infoP );

			CachePose( instance, *cachedPose, *poseA, *poseB );
		}

		const Float alpha = GetAlpha( instance );

		// Sample end state
		m_cachedStartStateNode->Sample( context, instance, *temp );

		// Interpolate poses
#ifdef DISABLE_SAMPLING_AT_LOD3
		if ( context.GetLodLevel() <= BL_Lod2 )
#endif
		{
			AddCachePose( output, *temp, *cachedPose, alpha );
		}

		// Add motion
		output.m_deltaReferenceFrameLocal = temp->m_deltaReferenceFrameLocal;

		// Merge events and used anims
		output.MergeEventsAndUsedAnims( *temp, alpha );
	}
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_useMathMethod )
	{
		CreatePose( instance );
	}
}

void CBehaviorGraphStateTransitionMatchToPoseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	if ( m_useMathMethod )
	{
		DestroyPose( instance );
	}

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphStateTransitionMatchToPoseNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_poseCached ] );
	ASSERT( m_useMathMethod );

	instance[ i_pose ].Create( instance );
}

void CBehaviorGraphStateTransitionMatchToPoseNode::CachePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const
{
	ASSERT( !instance[ i_poseCached ] );
	ASSERT( m_useMathMethod );

#ifdef USE_HAVOK_ANIMATION
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		output.m_outputPose[ i ].setMulInverseMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
	}
#else
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		output.m_outputPose[ i ].SetMulInverseMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
	}
#endif

	instance[ i_poseCached ] = true;
}

void CBehaviorGraphStateTransitionMatchToPoseNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( m_useMathMethod );

	instance[ i_pose ].Free( instance );
	instance[ i_poseCached ] = false;
}

void CBehaviorGraphStateTransitionMatchToPoseNode::AddCachePose( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& animPose, const SBehaviorGraphOutput& cachedPose, Float alpha ) const
{
	ASSERT( m_useMathMethod );
	ASSERT( alpha >= 0.f && alpha <= 1.f );

	BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, animPose, cachedPose, alpha );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionMatchFromPoseNode );

CBehaviorGraphStateTransitionMatchFromPoseNode::CBehaviorGraphStateTransitionMatchFromPoseNode()
	: m_useMathMethod( true )
{

}

String CBehaviorGraphStateTransitionMatchFromPoseNode::GetCaption() const
{
	if ( m_useMathMethod && IsEnabled() )
	{
		if ( !m_name.Empty() )
		{
			return String::Printf( TXT("M [%s]"), m_name.AsChar() );
		}
		else
		{
			return String( TXT("M") );
		}
	}
	else
	{
		return TBaseClass::GetCaption();
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_poseCached;
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_poseCached ] = false;
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	if ( m_useMathMethod )
	{
		DestroyPose( instance );
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::CacheMatchFromPose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( m_useMathMethod );
	ASSERT( IsActive( instance ) );
	ASSERT( m_cachedStartStateNode );
	ASSERT( m_cachedStartStateNode->IsActive( instance ) );
	ASSERT( instance[ i_poseCached ] );

	SBehaviorGraphOutput* pose = instance[ i_pose ].GetPose();
	ASSERT( pose );

	if ( pose )
	{
		output = *pose;
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( TransitionMatchFrom );

	if ( !m_useMathMethod )
	{
		TBaseClass::OnUpdate( context, instance, timeDelta );
		return;
	}

	// synchronize children playback
	Synchronize( instance, timeDelta );

	ASSERT( m_cachedStartStateNode );
	ASSERT( m_cachedEndStateNode );

	{
		if ( !m_cachedStartStateNode->IsActive( instance ) )
		{
			BEH_DUMP_ERROR( instance, TXT("CBehaviorGraphStateTransitionBlendNode::OnUpdate - !m_cachedStartStateNode->IsActive( instance ) ") );

			BEH_ERROR( TXT("CBehaviorGraphStateTransitionBlendNode: From %s To %s"), m_cachedStartStateNode->GetName().AsChar(), m_cachedEndStateNode->GetName().AsChar()  );

			ASSERT( !m_cachedStartStateNode->IsActive( instance ) );
			m_cachedStartStateNode->Activate( instance );
		}

		if ( !m_cachedEndStateNode->IsActive( instance ) )
		{
			ASSERT( m_cachedEndStateNode->IsActive( instance ) );
			m_cachedEndStateNode->Activate( instance );
		}
	}

	m_cachedStartStateNode->Update( context, instance, timeDelta );
	m_cachedEndStateNode->Update( context, instance, timeDelta );

	if ( instance[ i_poseCached ] )
	{
		CBehaviorGraphStateMachineNode *stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );

		DisableSynchronization( instance );

		Deactivate( instance );

		stateMachine->SwitchToState( m_cachedEndStateNode, instance );
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( TransitionMatchFrom );

	ASSERT( IsActive( instance ) );

	if ( !m_useMathMethod )
	{
		TBaseClass::Sample( context, instance, output );
		return;
	}

	CCacheBehaviorGraphOutput tempPose( context );
	SBehaviorGraphOutput* temp = tempPose.GetPose();

	SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();
	ASSERT( cachedPose );

	if ( temp && cachedPose && m_cachedStartStateNode && m_cachedEndStateNode )
	{
		Bool& poseCached = instance[ i_poseCached ];
		if ( !poseCached )
		{
			m_cachedStartStateNode->Sample( context, instance, *cachedPose );

			poseCached = true;
		}

		m_cachedEndStateNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_useMathMethod )
	{
		CreatePose( instance );
	}
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	if ( m_useMathMethod )
	{
		DestroyPose( instance );
	}

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_poseCached ] );
	ASSERT( m_useMathMethod );

	instance[ i_pose ].Create( instance );
}

void CBehaviorGraphStateTransitionMatchFromPoseNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( m_useMathMethod );

	instance[ i_pose ].Free( instance );
	instance[ i_poseCached ] = false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMatchFromPoseNode );

CBehaviorGraphMatchFromPoseNode::CBehaviorGraphMatchFromPoseNode()
{

}

String CBehaviorGraphMatchFromPoseNode::GetCaption() const
{
	return TXT("Match from pose");
}

void CBehaviorGraphMatchFromPoseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_poseCached;
	compiler << i_additiveDuration;
	compiler << i_currTime;
	compiler << i_running;
}

void CBehaviorGraphMatchFromPoseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_poseCached ] = false;
	instance[ i_additiveDuration ] = 0.f;
	instance[ i_currTime ] = 0.f;
	instance[ i_running ] = false;
}

void CBehaviorGraphMatchFromPoseNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

void CBehaviorGraphMatchFromPoseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MatchFromPose );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Bool& running = instance[ i_running ];
	if ( running )
	{
		SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();
		ASSERT( cachedPose );

		const Float additiveDuration = instance[ i_additiveDuration ];
		ASSERT( additiveDuration > 0.f );

		Float& currTime = instance[ i_currTime ];

		currTime = Min( currTime + timeDelta, additiveDuration );

		if ( currTime >= additiveDuration )
		{
			DestroyPose( instance );
			running = false;
		}
	}
}

void CBehaviorGraphMatchFromPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Bool running = instance[ i_running ];
	if ( running
#ifdef DISABLE_SAMPLING_AT_LOD3
		&& context.GetLodLevel() <= BL_Lod2 
#endif
		)
	{
		SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();
		ASSERT( cachedPose );

		Bool& poseCached = instance[ i_poseCached ];
		if ( !poseCached )
		{
			if ( ! SuckPoseFromTransition( instance, *cachedPose ) )
			{
				// if couldn't get pose, bail out, instead of blending previous pose (that might be t-pose!)
				return;
			}

			SBehaviorGraphOutput* poseB = cachedPose;
			SBehaviorGraphOutput* poseA = &output;

			CachePose( instance, *cachedPose, *poseA, *poseB );
		}

		const Float weight = 1.f - instance[ i_currTime ] / instance[ i_additiveDuration ];

		AddCachePose( output, output, *cachedPose, weight );
	}
}

void CBehaviorGraphMatchFromPoseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	const CBehaviorGraphStateMachineNode* sm = FindParent< CBehaviorGraphStateMachineNode >();
	if ( sm )
	{
		const CBehaviorGraphNode* cn = sm->GetCurrentState( instance );
		const CBehaviorGraphStateTransitionMatchFromPoseNode* t = Cast< const CBehaviorGraphStateTransitionMatchFromPoseNode >( cn );
		if ( t && t->UseMatchMathod() && ( t->GetTransitionTime() > 0.f || m_minDuration > 0.0f ))
		{
			instance[ i_additiveDuration ] = Max( m_minDuration, t->GetTransitionTime() );
			instance[ i_currTime ] = 0.f;
			instance[ i_running ] = true;

			CreatePose( instance );
		}
	}
}

void CBehaviorGraphMatchFromPoseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	DestroyPose( instance );

	instance[ i_additiveDuration ] = 0.f;
	instance[ i_currTime ] = 0.f;
	instance[ i_running ] = false;

	TBaseClass::OnDeactivated( instance );
}

Bool CBehaviorGraphMatchFromPoseNode::SuckPoseFromTransition( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& pose ) const
{
	const CBehaviorGraphStateMachineNode* sm = FindParent< CBehaviorGraphStateMachineNode >();
	if ( sm )
	{
		const CBehaviorGraphNode* cn = sm->GetCurrentState( instance );
		const CBehaviorGraphStateTransitionMatchFromPoseNode* t = Cast< const CBehaviorGraphStateTransitionMatchFromPoseNode >( cn );
		if ( t && t->UseMatchMathod() && ( t->GetTransitionTime() > 0.f || m_minDuration > 0.0f  ) )
		{
			t->CacheMatchFromPose( instance, pose );
			return true;
		}
	}

	ASSERT( 0 );
	return false;
}

void CBehaviorGraphMatchFromPoseNode::CreatePose( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !instance[ i_poseCached ] );
	ASSERT( instance[ i_running ] );

	instance[ i_pose ].Create( instance );
}

void CBehaviorGraphMatchFromPoseNode::CachePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const
{
	ASSERT( !instance[ i_poseCached ] );
	ASSERT( instance[ i_running ] );

#ifdef USE_HAVOK_ANIMATION
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		output.m_outputPose[ i ].setMulInverseMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
	}
#else
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		output.m_outputPose[ i ].SetMulInverseMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
	}
#endif

	instance[ i_poseCached ] = true;
}

void CBehaviorGraphMatchFromPoseNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Free( instance );
	instance[ i_poseCached ] = false;
}

void CBehaviorGraphMatchFromPoseNode::AddCachePose( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& animPose, const SBehaviorGraphOutput& cachedPose, Float alpha ) const
{
	ASSERT( alpha >= 0.f && alpha <= 1.f );

	BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, animPose, cachedPose, alpha );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStatePelvisTransitionNode );

CBehaviorGraphStatePelvisTransitionNode::CBehaviorGraphStatePelvisTransitionNode()
	: m_usePelvisBlendMethod( true )
	, m_pelvisBoneName( TXT("pelvis") )
	, m_pelvisDirectionFwdLS( A_NX )
{

}

String CBehaviorGraphStatePelvisTransitionNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Pelvis transition [%s]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Pelvis transition") );
	}
}

void CBehaviorGraphStatePelvisTransitionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pelvisBoneIdx;
	compiler << i_cacheOffset;
	compiler << i_offsetME;
	compiler << i_offsetPelvisLS;
	compiler << i_timeDelta;
}

void CBehaviorGraphStatePelvisTransitionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_pelvisBoneIdx ] = FindBoneIndex( m_pelvisBoneName, instance );
	instance[ i_cacheOffset ] = false;
	instance[ i_offsetME ].Identity();
	instance[ i_offsetPelvisLS ].Identity();
	instance[ i_timeDelta ] = 0.f;
}

void CBehaviorGraphStatePelvisTransitionNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphStatePelvisTransitionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorGraphStatePelvisTransitionNode::InterpolatePoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const
{
	if ( m_usePelvisBlendMethod && !instance[ i_cacheOffset ] )
	{
		CalcAndCacheOffset( instance, poseA, poseB, alpha );
		instance[ i_cacheOffset ] = true;
	}

	const Int32 pelvisIdx = instance[ i_pelvisBoneIdx ];
	if ( m_usePelvisBlendMethod && pelvisIdx != -1 )
	{
		TBaseClass::InterpolatePoses( context, instance, output, poseA, poseB, alpha );

		/*const EngineQsTransform& engineOffsetPelvisLS = instance[ i_offsetPelvisLS ];
		const AnimQsTransform& offsetPelvisLS = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( engineOffsetPelvisLS );

		const AnimQsTransform& pelvisA = poseA.m_outputPose[ pelvisIdx ];
		const AnimQsTransform& pelvisB = poseB.m_outputPose[ pelvisIdx ];
		AnimQsTransform& pelvisOut = output.m_outputPose[ pelvisIdx ];

		pelvisOut

		root.SetMul( root, offsetLS );*/

		const EngineQsTransform& engineOffsetME = instance[ i_offsetME ];
		const AnimQsTransform& offsetME = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( engineOffsetME );

		Float& timeDelta = instance[ i_timeDelta ];
		const Float weight = timeDelta / m_transitionTime;
		timeDelta = 0.f;

		AnimQsTransform thisFrameOffsetME;
		thisFrameOffsetME.Lerp( AnimQsTransform::IDENTITY, offsetME, weight );

		output.m_deltaReferenceFrameLocal.SetMul( output.m_deltaReferenceFrameLocal, thisFrameOffsetME );
	}
	else
	{
		TBaseClass::InterpolatePoses( context, instance, output, poseA, poseB, alpha );
	}
}

void CBehaviorGraphStatePelvisTransitionNode::CalcAndCacheOffset( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const
{
	ASSERT( m_usePelvisBlendMethod );

	const Int32 pelvisIdx = instance[ i_pelvisBoneIdx ];
	if ( pelvisIdx != -1 )
	{
		// A - source
		// B - dest
		const AnimQsTransform& rootA_MS = poseA.m_outputPose[ 0 ];
		const AnimQsTransform& pelvisA_LS = poseA.m_outputPose[ pelvisIdx ];

		const AnimQsTransform& rootB_MS = poseB.m_outputPose[ 0 ];
		const AnimQsTransform& pelvisB_LS = poseB.m_outputPose[ pelvisIdx ];

		AnimQsTransform pelvisA_MS;
		pelvisA_MS.SetMul( rootA_MS, pelvisA_LS );
		AnimQsTransform pelvisB_MS;
		pelvisB_MS.SetMul( rootB_MS, pelvisB_LS );

		const AnimVector4 pelvisDirLS = BehaviorUtils::RedVectorFromAxis( m_pelvisDirectionFwdLS );
		AnimVector4 pelvisDisA_MS;
		AnimVector4 pelvisDisB_MS;
		pelvisDisA_MS.RotateDirection( pelvisA_MS.Rotation, pelvisDirLS );
		pelvisDisB_MS.RotateDirection( pelvisB_MS.Rotation, pelvisDirLS );

		const Vector pelvisDisVecA_MS = AnimVectorToVector( pelvisDisA_MS );
		const Vector pelvisDisVecB_MS = AnimVectorToVector( pelvisDisB_MS );

		const Float angleRad = MathUtils::VectorUtils::GetAngleRadAroundAxis( pelvisDisVecA_MS, pelvisDisVecB_MS, Vector( 0.f, 0.f, 1.f ) );
		const Float angleDed = RAD2DEG( angleRad );

		AnimQsTransform offset_MS( AnimQsTransform::IDENTITY );
		offset_MS.Translation = Sub( pelvisA_MS.Translation, pelvisB_MS.Translation );
		offset_MS.Translation.Z = 0.f;
		offset_MS.Rotation.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( A_Z ), angleRad );

		//AnimQsTransform offset_LS;
		//offset_LS.SetMulInverseMul( rootB_MS, offset_MS );

		instance[ i_offsetME ] = ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( offset_MS );
	}
}

void CBehaviorGraphStatePelvisTransitionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_cacheOffset ] = false;
}

void CBehaviorGraphStatePelvisTransitionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnDeactivated( instance );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
