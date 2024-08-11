/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphAimingWithIKNode.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/behaviorGraphContext.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeleton.h"
#include "../engine/behaviorGraphSocket.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAimingWithIKNode );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( l_bicep );
RED_DEFINE_STATIC_NAME( l_forearm );
RED_DEFINE_STATIC_NAME( l_hand );
RED_DEFINE_STATIC_NAME( LookAtTargetDirMS );

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphAimingWithIKNode::CBehaviorGraphAimingWithIKNode()
{
	m_aimingBaseBoneName = CNAME( l_bicep );
	m_ik.m_upperBone.m_name = CNAME( l_bicep );
	m_ik.m_jointBone.m_name = CNAME( l_forearm );
	m_ik.m_lowerBone.m_name = CNAME( l_hand );
	m_ik.m_autoSetupDirs = false;
	m_ik.m_jointSideWeightUpper = 0.5f;
	m_ik.m_jointSideWeightJoint = 0.4f;
	m_ik.m_jointSideWeightLower = 0.1f;
	m_ik.m_sideDirUpperBS = Vector( 0.0f,  0.0f,  1.0f );
	m_ik.m_sideDirJointBS = Vector( 0.0f, -1.0f,  0.0f );
	m_ik.m_sideDirLowerBS = Vector( 0.0f, -1.0f,  0.0f );
	m_ik.m_bendDirUpperBS = Vector( 0.0f,  1.0f,  0.0f );
	m_ik.m_bendDirJointBS = Vector( 0.0f,  0.0f,  1.0f );
	m_ik.m_bendDirLowerBS = Vector( 0.0f,  0.0f,  1.0f );
}

void CBehaviorGraphAimingWithIKNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_aimingBaseBoneIdx;
	compiler << i_ik;
#ifndef NO_EDITOR
	compiler << i_debugLookAtTargetDirMS;
	compiler << i_debugBaseAimingBaseMS;
	compiler << i_debugBaseLowerMS;
	compiler << i_debugInputAimingBaseMS;
	compiler << i_debugOutputLowerMS;
#endif
}

void CBehaviorGraphAimingWithIKNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_ik ].Setup( instance, m_ik );
	instance[ i_aimingBaseBoneIdx ] = instance.GetAnimatedComponent()->GetSkeleton()->FindBoneByName( m_aimingBaseBoneName );
}

void CBehaviorGraphAimingWithIKNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	instance[ i_timeDelta ] = timeDelta;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedLookAtTargetDirMSInputNode && m_cachedLookAtTargetDirMSInputNode != m_cachedInputNode )
	{
		m_cachedLookAtTargetDirMSInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAimingWithIKNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	if ( m_cachedBaseInputNode )
	{
		CCacheBehaviorGraphOutput cachePoseBase( context );

		SBehaviorGraphOutput* tempBase = cachePoseBase.GetPose();

		if ( tempBase )
		{
			m_cachedBaseInputNode->Sample( context, instance, *tempBase );

			Float timeDelta = instance[ i_timeDelta ];
			STwoBonesIKSolver& ik = instance[ i_ik ];
			Int32 aimingBaseBoneIdx = instance[ i_aimingBaseBoneIdx ];
			const CAnimatedComponent * ac = instance.GetAnimatedComponent();

			// get relative location of <ik's lower bone> to <base bone> from "base"
			AnimQsTransform baseLowerTMS;
			ik.GetLowerTMS( instance, *tempBase, baseLowerTMS );
			AnimQsTransform baseAimingBaseTMS = tempBase->GetBoneModelTransform( ac, aimingBaseBoneIdx );
			AnimQsTransform inputAimingBaseTMS = output.GetBoneModelTransform( ac, aimingBaseBoneIdx );

			// we don't want to have any rotation around here!
			baseAimingBaseTMS.SetRotation( AnimQuaternion::IDENTITY );

			AnimQsTransform baseRelativeLowerToAimingBase;
			SetMulInverseMulTransform( baseRelativeLowerToAimingBase, baseAimingBaseTMS, baseLowerTMS );

			// get look at target dir MS from "look at target dir ms"
			Vector lookAtTargetDirMS = m_cachedLookAtTargetDirMSInputNode? m_cachedLookAtTargetDirMSInputNode->GetVectorValue( instance ) : Vector(0.0f, 1.0f, 0.0f);

			// rotate base to face look at target dir ms
			AnimVector4 fwdDirMS = VectorToAnimVector( lookAtTargetDirMS ).Normalized3();
			AnimVector4 upDirMS = AnimVector4(0.0f, 0.0f, 1.0f);
			AnimVector4 sideDirMS;
			SetCross( sideDirMS, fwdDirMS, upDirMS );
			sideDirMS.Normalize3();
			CalculateAnimTransformLFS( inputAimingBaseTMS, inputAimingBaseTMS.GetTranslation(), fwdDirMS, sideDirMS, AnimVector4(0.0f, 1.0f, 0.0f), AnimVector4(1.0f, 0.0f, 0.0f), AnimVector4(0.0f, 0.0f, 1.0f) );

			//requestedlowerBoneTMS.SetTranslation( inputAimingBaseTMS.GetTranslation() + inputAimingBaseTMS.GetRotation().Quat.ro)
			// multiply distance of relative location basing on <ik's lower bone> to <base bone> from "input"
			AnimQsTransform baseRelativeLowerToAimingBaseModified = baseRelativeLowerToAimingBase;
			const Float baseLength = baseRelativeLowerToAimingBaseModified.GetTranslation().Length3();
			const Float inputLength = baseRelativeLowerToAimingBaseModified.GetTranslation().Length3();
			const Float useLength = inputLength;
			baseRelativeLowerToAimingBaseModified.SetTranslation( Mul( baseRelativeLowerToAimingBaseModified.GetTranslation(), useLength / inputLength ) );

			// set lower bone to be in relative location calculated above
			AnimQsTransform requestedlowerBoneTMS;
			SetMulTransform( requestedlowerBoneTMS, inputAimingBaseTMS, baseRelativeLowerToAimingBaseModified );

			// process ik
			DEBUG_ANIM_TRANSFORM( requestedlowerBoneTMS );
			ik.SetTargetLowerTMS( requestedlowerBoneTMS );
			ik.UpdatePose( instance, output, m_ik, 1.0f, timeDelta );
		
#ifndef NO_EDITOR
			instance[ i_debugLookAtTargetDirMS ] = lookAtTargetDirMS;
			instance[ i_debugBaseAimingBaseMS ] = AnimQsTransformToMatrix( baseAimingBaseTMS );
			instance[ i_debugBaseLowerMS ] = AnimQsTransformToMatrix( baseLowerTMS );
			instance[ i_debugInputAimingBaseMS ] = AnimQsTransformToMatrix( inputAimingBaseTMS );
			instance[ i_debugOutputLowerMS ] = AnimQsTransformToMatrix( requestedlowerBoneTMS );
#endif
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAimingWithIKNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( LookAtTargetDirMS ) ) );
}
#endif

void CBehaviorGraphAimingWithIKNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheBlock( TXT("Input") );
	m_cachedBaseInputNode = CacheBlock( TXT("Base") );
	m_cachedLookAtTargetDirMSInputNode = CacheVectorValueBlock( TXT("LookAtTargetDirMS") );
}

void CBehaviorGraphAimingWithIKNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->Activate( instance );
	}
	
	if ( m_cachedLookAtTargetDirMSInputNode && m_cachedLookAtTargetDirMSInputNode != m_cachedInputNode )
	{
		m_cachedLookAtTargetDirMSInputNode->Activate( instance );
	}
}

void CBehaviorGraphAimingWithIKNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->Deactivate( instance );
	}

	if ( m_cachedLookAtTargetDirMSInputNode && m_cachedLookAtTargetDirMSInputNode != m_cachedInputNode )
	{
		m_cachedLookAtTargetDirMSInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphAimingWithIKNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAimingWithIKNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphAimingWithIKNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedBaseInputNode )
	{
		m_cachedBaseInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedLookAtTargetDirMSInputNode && m_cachedLookAtTargetDirMSInputNode != m_cachedInputNode )
	{
		m_cachedLookAtTargetDirMSInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphAimingWithIKNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool result = false;

	if ( m_cachedInputNode )
	{
		result |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	// input only!

	return result;
}

void CBehaviorGraphAimingWithIKNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( ! m_generateEditorFragments )
	{
		return;
	}

#ifndef NO_EDITOR
	Matrix l2w = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();
	Vector debugLookAtTargetDirWS = l2w.TransformVector( instance[ i_debugLookAtTargetDirMS ] );
	Vector aimingDirectionWS = l2w.TransformVector( Vector(0.0f, 1.0f, 0.0f) );
	Matrix debugBaseAimingBaseWS = instance[ i_debugBaseAimingBaseMS ] * l2w;
	Matrix debugBaseLowerWS = instance[ i_debugBaseLowerMS ] * l2w;
	Matrix debugInputAimingBaseWS = instance[ i_debugInputAimingBaseMS ] * l2w;
	Matrix debugOutputLowerWS = instance[ i_debugOutputLowerMS ] * l2w;
	DrawDebugMatrix( debugBaseAimingBaseWS, 0.1f, 180, frame, Color( 255, 0, 0, 150 ) );
	DrawDebugMatrix( debugBaseLowerWS, 0.1f, 180, frame, Color( 255, 0, 0, 150 ) );
	frame->AddDebugLineWithArrow( debugBaseAimingBaseWS.GetTranslation(), debugBaseLowerWS.GetTranslation(), 1.0f, 0.035f, 0.035f, Color( 255, 0, 0, 150 ), true, true );
	frame->AddDebugLineWithArrow( debugBaseAimingBaseWS.GetTranslation(), debugBaseAimingBaseWS.GetTranslation() + aimingDirectionWS, 1.0f, 0.075f, 0.075f, Color( 255, 0, 0, 150 ), true, true );
	DrawDebugMatrix( debugInputAimingBaseWS, 0.1f, 180, frame, Color( 0, 255, 0, 150 ) );
	DrawDebugMatrix( debugOutputLowerWS, 0.1f, 180, frame, Color( 0, 255, 0, 150 ) );
	frame->AddDebugLineWithArrow( debugInputAimingBaseWS.GetTranslation(), debugOutputLowerWS.GetTranslation(), 1.0f, 0.035f, 0.035f, Color( 0, 255, 0, 150 ), true, true );
	frame->AddDebugLineWithArrow( debugInputAimingBaseWS.GetTranslation(), debugInputAimingBaseWS.GetTranslation() + debugLookAtTargetDirWS, 1.0f, 0.075f, 0.075f, Color( 0, 255, 0, 150 ), true, true );
#endif
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
