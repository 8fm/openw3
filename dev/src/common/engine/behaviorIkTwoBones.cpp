/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorIkTwoBones.h"
#include "behaviorGraphUtils.inl"
#include "behaviorGraphInstance.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "entity.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( STwoBonesIKSolverBoneData );
IMPLEMENT_ENGINE_CLASS( STwoBonesIKSolverData );
IMPLEMENT_ENGINE_CLASS( STwoBonesIKSolver );

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

STwoBonesIKSolverBoneData::STwoBonesIKSolverBoneData()
{
}

//////////////////////////////////////////////////////////////////////////

STwoBonesIKSolverData::STwoBonesIKSolverData()
	: m_nextDirUpperBS( 1.0f, 0.0f, 0.0f )
	, m_nextDirJointBS( 1.0f, 0.0f, 0.0f )
	, m_nextDirLowerBS( 1.0f, 0.0f, 0.0f )
	, m_sideDirUpperBS( 0.0f, 0.0f, 1.0f )
	, m_sideDirJointBS( 0.0f, 0.0f, 1.0f )
	, m_sideDirLowerBS( 0.0f, 0.0f, 1.0f )
	, m_bendDirUpperBS( 0.0f, 1.0f, 0.0f )
	, m_bendDirJointBS( 0.0f, 1.0f, 0.0f )
	, m_bendDirLowerBS( 0.0f, 1.0f, 0.0f )
	, m_jointSideWeightUpper( 0.5f )
	, m_jointSideWeightJoint( 0.3f )
	, m_jointSideWeightLower( 0.2f )
	, m_reverseBend( false )
	, m_autoSetupDirs( true )
	, m_allowToLock( false )
	, m_limitToLengthPercentage( 0.99f )
{
}

//////////////////////////////////////////////////////////////////////////

STwoBonesIKSolver::STwoBonesIKSolver()
	: m_parentBone( INDEX_NONE )
	, m_upperBone( INDEX_NONE )
	, m_jointBone( INDEX_NONE )
	, m_subLowerBone( INDEX_NONE )
	, m_lowerBone( INDEX_NONE )
	, m_ikBone( INDEX_NONE )
	, m_additionalSideDirMS( AnimVector4::ZEROS )
{
}

void STwoBonesIKSolver::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_upperBone == INDEX_NONE ||
		 m_jointBone == INDEX_NONE ||
		 m_lowerBone == INDEX_NONE )
	{
#ifdef DEBUG_TWO_BONES_IK_SOLVER
		if ( m_upperBone != INDEX_NONE )
		{
			frame->AddDebugSphere( m_upperMatWS.GetTranslation(), 0.2f, Matrix::IDENTITY, Color( 255,0,0 ), true );
		}
		if ( m_jointBone != INDEX_NONE )
		{
			frame->AddDebugSphere( m_jointMatWS.GetTranslation(), 0.2f, Matrix::IDENTITY, Color( 0,255,0 ), true );
		}
		if ( m_lowerBone != INDEX_NONE )
		{
			frame->AddDebugSphere( m_lowerMatWS.GetTranslation(), 0.2f, Matrix::IDENTITY, Color( 0,0,255 ), true );
		}
#endif
		return;
	}

	/*
	frame->AddDebugLine( m_upperMatWS.GetTranslation(), m_jointMatWS.GetTranslation(), Color( 0,255,0,40 ), true );
	frame->AddDebugLine( m_jointMatWS.GetTranslation(), m_lowerMatWS.GetTranslation(), Color( 255,0,0,40 ), true );
	frame->AddDebugLine( m_upperMatWSPost.GetTranslation(), m_jointMatWSPost.GetTranslation(), Color( 60,255,60,180 ), true );
	frame->AddDebugLine( m_jointMatWSPost.GetTranslation(), m_lowerMatWSPost.GetTranslation(), Color( 255,60,60,180 ), true );
	*/

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	frame->AddDebugSphere( m_debugTargetWS.GetTranslation(), 0.01f, Matrix::IDENTITY, Color( 255,255,255 ), true );
	frame->AddDebugSphere( m_debugTargetWS.GetTranslation(), 0.002f, Matrix::IDENTITY, Color( 255,255,255 ), true );

	frame->AddDebugLine( m_debugBoneAWS_1.GetTranslation(), m_debugBoneBWS_1.GetTranslation(), Color( 0, 255, 0 ), true, true );
	frame->AddDebugLine( m_debugBoneBWS_1.GetTranslation(), m_debugBoneCWS_1.GetTranslation(), Color( 0, 255, 0 ), true, true );
	frame->AddDebugLine( m_debugBoneCWS_1.GetTranslation(), m_debugBoneDWS_1.GetTranslation(), Color( 0, 255, 0 ), true, true );

	frame->AddDebugLine( m_debugBoneAWS_2.GetTranslation(), m_debugBoneBWS_2.GetTranslation(), Color( 255, 0, 0 ), true, true );
	frame->AddDebugLine( m_debugBoneBWS_2.GetTranslation(), m_debugBoneCWS_2.GetTranslation(), Color( 255, 0, 0 ), true, true );
	frame->AddDebugLine( m_debugBoneCWS_2.GetTranslation(), m_debugBoneDWS_2.GetTranslation(), Color( 255, 0, 0 ), true, true );

	//frame->AddDebugLine( m_debugBoneAWS_3.GetTranslation(), m_debugBoneBWS_3.GetTranslation(), Color( 255, 255, 255 ), true, true );
	//frame->AddDebugLine( m_debugBoneBWS_3.GetTranslation(), m_debugBoneCWS_3.GetTranslation(), Color( 255, 255, 255 ), true, true );
	//frame->AddDebugLine( m_debugBoneCWS_3.GetTranslation(), m_debugBoneDWS_3.GetTranslation(), Color( 255, 255, 255 ), true, true );

	Float dirLen = 0.1f;
	frame->AddDebugLine( m_upperMatWS.GetTranslation(), m_upperMatWS.GetTranslation() + m_upperMatWS.TransformVector(AnimVectorToVector(m_nextDirUpperBS)) * dirLen, Color(255,0,0,128), true, true );
	frame->AddDebugLine( m_upperMatWS.GetTranslation(), m_upperMatWS.GetTranslation() + m_upperMatWS.TransformVector(AnimVectorToVector(m_sideDirUpperBS)) * dirLen, Color(0,255,0,128), true, true );
	frame->AddDebugLine( m_upperMatWS.GetTranslation(), m_upperMatWS.GetTranslation() + m_upperMatWS.TransformVector(AnimVectorToVector(m_bendDirUpperBS)) * dirLen, Color(0,0,255,128), true, true );
	frame->AddDebugLine( m_jointMatWS.GetTranslation(), m_jointMatWS.GetTranslation() + m_jointMatWS.TransformVector(AnimVectorToVector(m_nextDirJointBS)) * dirLen, Color(255,0,0,128), true, true );
	frame->AddDebugLine( m_jointMatWS.GetTranslation(), m_jointMatWS.GetTranslation() + m_jointMatWS.TransformVector(AnimVectorToVector(m_sideDirJointBS)) * dirLen, Color(0,255,0,128), true, true );
	frame->AddDebugLine( m_jointMatWS.GetTranslation(), m_jointMatWS.GetTranslation() + m_jointMatWS.TransformVector(AnimVectorToVector(m_bendDirJointBS)) * dirLen, Color(0,0,255,128), true, true );
	frame->AddDebugLine( m_lowerMatWS.GetTranslation(), m_lowerMatWS.GetTranslation() + m_lowerMatWS.TransformVector(AnimVectorToVector(m_nextDirLowerBS)) * dirLen, Color(255,0,0,128), true, true );
	frame->AddDebugLine( m_lowerMatWS.GetTranslation(), m_lowerMatWS.GetTranslation() + m_lowerMatWS.TransformVector(AnimVectorToVector(m_sideDirLowerBS)) * dirLen, Color(0,255,0,128), true, true );
	frame->AddDebugLine( m_lowerMatWS.GetTranslation(), m_lowerMatWS.GetTranslation() + m_lowerMatWS.TransformVector(AnimVectorToVector(m_bendDirLowerBS)) * dirLen, Color(0,0,255,128), true, true );
	dirLen = 0.05f;
	frame->AddDebugLine( m_upperMatWSPost.GetTranslation(), m_upperMatWSPost.GetTranslation() + m_upperMatWSPost.TransformVector(AnimVectorToVector(m_nextDirUpperBS)) * dirLen, Color(255,0,0,255), true, true );
	frame->AddDebugLine( m_upperMatWSPost.GetTranslation(), m_upperMatWSPost.GetTranslation() + m_upperMatWSPost.TransformVector(AnimVectorToVector(m_sideDirUpperBS)) * dirLen, Color(0,255,0,255), true, true );
	frame->AddDebugLine( m_upperMatWSPost.GetTranslation(), m_upperMatWSPost.GetTranslation() + m_upperMatWSPost.TransformVector(AnimVectorToVector(m_bendDirUpperBS)) * dirLen, Color(0,0,255,255), true, true );
	frame->AddDebugLine( m_jointMatWSPost.GetTranslation(), m_jointMatWSPost.GetTranslation() + m_jointMatWSPost.TransformVector(AnimVectorToVector(m_nextDirJointBS)) * dirLen, Color(255,0,0,255), true, true );
	frame->AddDebugLine( m_jointMatWSPost.GetTranslation(), m_jointMatWSPost.GetTranslation() + m_jointMatWSPost.TransformVector(AnimVectorToVector(m_sideDirJointBS)) * dirLen, Color(0,255,0,255), true, true );
	frame->AddDebugLine( m_jointMatWSPost.GetTranslation(), m_jointMatWSPost.GetTranslation() + m_jointMatWSPost.TransformVector(AnimVectorToVector(m_bendDirJointBS)) * dirLen, Color(0,0,255,255), true, true );
	frame->AddDebugLine( m_lowerMatWSPost.GetTranslation(), m_lowerMatWSPost.GetTranslation() + m_lowerMatWSPost.TransformVector(AnimVectorToVector(m_nextDirLowerBS)) * dirLen, Color(255,0,0,255), true, true );
	frame->AddDebugLine( m_lowerMatWSPost.GetTranslation(), m_lowerMatWSPost.GetTranslation() + m_lowerMatWSPost.TransformVector(AnimVectorToVector(m_sideDirLowerBS)) * dirLen, Color(0,255,0,255), true, true );
	frame->AddDebugLine( m_lowerMatWSPost.GetTranslation(), m_lowerMatWSPost.GetTranslation() + m_lowerMatWSPost.TransformVector(AnimVectorToVector(m_bendDirLowerBS)) * dirLen, Color(0,0,255,255), true, true );
	/*
	DrawDebugMatrix( m_upperMatWS, 0.1f, 40, frame );
	DrawDebugMatrix( m_jointMatWS, 0.1f, 40, frame );
	DrawDebugMatrix( m_lowerMatWS, 0.1f, 40, frame );
	DrawDebugMatrix( m_upperMatWSPost, 0.1f, 180, frame );
	DrawDebugMatrix( m_jointMatWSPost, 0.1f, 180, frame );
	DrawDebugMatrix( m_lowerMatWSPost, 0.1f, 180, frame );
	//
	DrawDebugMatrix( m_lowerMatWS, 0.05f, 255, frame, Color( 0,255,255,228 ) );
	DrawDebugMatrix( AnimQsTransformToMatrix( m_targetLowerTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld(), 0.05f, 255, frame, Color( 255,0,255,228 ) );
	{
		Vector a2end = (m_lowerMatWS.GetTranslation() - m_jointMatWS.GetTranslation()).Normalized3();
		Vector a = m_jointMatWS.GetTranslation();
		Vector b = a + Vector::Cross( m_jointMatWS.TransformVector( m_debugJointSideDirUpperBS ), a2end );
		Vector c = b + Vector::Cross( m_upperMatWS.TransformVector( m_debugJointSideDirJointBS ), a2end );
		Vector d = c + Vector::Cross( AnimQsTransformToMatrix( m_targetLowerTMS ).TransformVector( m_debugJointSideDirLowerBS ), a2end );
		frame->AddDebugLine( a, b, Color( 255,155,0,70 ), true );
		frame->AddDebugLine( b, c, Color( 255,155,0,70 ), true );
		frame->AddDebugLine( c, d, Color( 255,155,0,70 ), true );
		frame->AddDebugLine( a, d, Color( 255,155,0,180 ), true );
	}
	{
		Vector a2end = (m_lowerMatWSPost.GetTranslation() - m_jointMatWSPost.GetTranslation()).Normalized3();
		Vector a = m_jointMatWSPost.GetTranslation();
		Vector b = a + Vector::Cross( instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().TransformVector( m_debugSideDirMS ), a2end );
		Vector c = a + Vector::Cross( instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().TransformVector( m_debugRawSideDirMS ), a2end );
		Vector d = a + Vector::Cross( instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().TransformVector( m_debugJointSideMS ), a2end );
		frame->AddDebugLine( a, b, Color( 255,255,0,180 ), true );
		frame->AddDebugLine( a, c, Color( 180,255,0,180 ), true );
		frame->AddDebugLine( a, d, Color( 0,255,100,180 ), true );
	}
	*/
	/*Uint32 xDispl = 0;
	Uint32 yDispl = 1;
	frame->AddDebugText( m_upperMatWS.GetTranslation(), String::Printf( TXT("A:%.3f"), m_debugA ), xDispl, yDispl*2, true, Color(200,200,200), Color(0,0,0,128) );
	++ yDispl;
	frame->AddDebugText( m_upperMatWS.GetTranslation(), String::Printf( TXT("B:%.3f"), m_debugB ), xDispl, yDispl*2, true, Color(200,200,200), Color(0,0,0,128) );
	++ yDispl;
	frame->AddDebugText( m_upperMatWS.GetTranslation(), String::Printf( TXT("C:%.3f"), m_debugC ), xDispl, yDispl*2, true, Color(200,200,200), Color(0,0,0,128) );
	++ yDispl;
	frame->AddDebugText( m_upperMatWS.GetTranslation(), String::Printf( TXT("D:%.3f"), m_debugD ), xDispl, yDispl*2, true, Color(200,200,200), Color(0,0,0,128) );
	++ yDispl;
	frame->AddDebugText( m_upperMatWS.GetTranslation(), String::Printf( TXT("E:%.3f"), m_debugE ), xDispl, yDispl*2, true, Color(200,200,200), Color(0,0,0,128) );
	++ yDispl;*/
#endif
}

#ifndef USE_HAVOK_ANIMATION
static void TryDir( Float & bestInDir, AnimMatrix44 const & boneMS, AnimVector4 & bestDirBS, AnimVector4 const & tryDirBS, AnimVector4 const& inDirMS )
{
	const AnimVector4 tryDirMS = TransformVector( boneMS, tryDirBS );
	const Float tryInDir = RedMath::SIMD::Dot( tryDirMS, inDirMS );
	const Float absTryInDir = Abs( tryInDir );
	if ( absTryInDir > bestInDir )
	{
		bestInDir = absTryInDir;
		bestDirBS = tryInDir < 0.0f? tryDirBS.Negated() : tryDirBS;
	}
}

static void AutoSetupDir( const CSkeleton* skeleton, Int32 boneIdx, AnimVector4 & jointSideDirBS, AnimVector4 const& inDirMS )
{
	AnimQsTransform boneTransform = skeleton->GetBoneMS( boneIdx );
	AnimMatrix44 boneMatrixMS = boneTransform.ConvertToMatrix();
	Float bestInDir = 0.0f;
	jointSideDirBS = AnimVector4( 0.0f, 0.0f, 0.0f );

	TryDir( bestInDir, boneMatrixMS, jointSideDirBS, AnimVector4( 1.0f, 0.0f, 0.0f ), inDirMS );
	TryDir( bestInDir, boneMatrixMS, jointSideDirBS, AnimVector4( 0.0f, 1.0f, 0.0f ), inDirMS );
	TryDir( bestInDir, boneMatrixMS, jointSideDirBS, AnimVector4( 0.0f, 0.0f, 1.0f ), inDirMS );
}

static void AutoSetupDirExact( const CSkeleton* skeleton, Int32 boneIdx, AnimVector4 & jointSideDirBS, AnimVector4 const& inDirMS )
{
	AnimQsTransform boneTransform = skeleton->GetBoneMS( boneIdx );
	AnimMatrix44 boneMatrixMS = boneTransform.ConvertToMatrix();
	jointSideDirBS = AnimVector4( 0.0f, 0.0f, 0.0f );

	jointSideDirBS = TransformVector( boneMatrixMS.Inverted(), inDirMS ).Normalized3();
}
#endif

void STwoBonesIKSolver::Setup( CBehaviorGraphInstance& instance, const STwoBonesIKSolverData& data )
{
	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	m_upperBone = skeleton->FindBoneByName( data.m_upperBone.m_name );
	m_jointBone = skeleton->FindBoneByName( data.m_jointBone.m_name );
	m_subLowerBone = skeleton->FindBoneByName( data.m_subLowerBone.m_name );
	m_lowerBone = skeleton->FindBoneByName( data.m_lowerBone.m_name );
	m_ikBone = skeleton->FindBoneByName( data.m_ikBone.m_name );
	m_nextDirUpperBS = VectorToAnimVector( data.m_nextDirUpperBS );
	m_nextDirJointBS = VectorToAnimVector( data.m_nextDirJointBS );
	m_nextDirLowerBS = VectorToAnimVector( data.m_nextDirLowerBS );
	m_sideDirUpperBS = VectorToAnimVector( data.m_sideDirUpperBS );
	m_sideDirJointBS = VectorToAnimVector( data.m_sideDirJointBS );
	m_sideDirLowerBS = VectorToAnimVector( data.m_sideDirLowerBS );
	m_bendDirUpperBS = VectorToAnimVector( data.m_bendDirUpperBS );
	m_bendDirJointBS = VectorToAnimVector( data.m_bendDirJointBS );
	m_bendDirLowerBS = VectorToAnimVector( data.m_bendDirLowerBS );
	m_jointSideWeightUpper = data.m_jointSideWeightUpper;
	m_jointSideWeightJoint = data.m_jointSideWeightJoint;
	m_jointSideWeightLower = data.m_jointSideWeightLower;

	if ( data.m_autoSetupDirs && m_upperBone != INDEX_NONE && m_jointBone != INDEX_NONE && m_lowerBone != INDEX_NONE )
	{
		// find bone below lower bone
		Int32 belowLowerBone = m_lowerBone + 1;
		while ( belowLowerBone < skeleton->GetBonesNum() )
		{
			if ( skeleton->GetParentBoneIndex( belowLowerBone ) == m_lowerBone )
			{
				break;
			}
			++ belowLowerBone;
		}
		if ( belowLowerBone >= skeleton->GetBonesNum() )
		{
			belowLowerBone = INDEX_NONE;
		}
#ifndef USE_HAVOK_ANIMATION
		const AnimVector4 upperBoneLoc = GetTranslation( skeleton->GetBoneMS( m_upperBone ) );
		const AnimVector4 jointBoneLoc = GetTranslation( skeleton->GetBoneMS( m_jointBone ) );
		const AnimVector4 lowerBoneLoc = GetTranslation( skeleton->GetBoneMS( m_lowerBone ) );
		const AnimVector4 subLowerBoneLoc = m_subLowerBone != INDEX_NONE ? GetTranslation( skeleton->GetBoneMS( m_subLowerBone ) ) : lowerBoneLoc;
		const AnimVector4 belowLowerBoneLoc = belowLowerBone != INDEX_NONE? GetTranslation( skeleton->GetBoneMS( belowLowerBone ) ) : Sub( lowerBoneLoc, AnimVector4(0.0f, 0.0f, 1.0f) );

		const AnimVector4 upperToNextDirMS = Sub( jointBoneLoc, upperBoneLoc );
		const AnimVector4 jointToNextDirMS = Sub( subLowerBoneLoc /* or lowerBoneLoc */, jointBoneLoc );
		const AnimVector4 lowerToNextDirMS = Sub( belowLowerBoneLoc, lowerBoneLoc );
		const AnimVector4 sideDirMS = AnimVector4( 1.0f, 0.0f, 0.0f );

		// to next (down doesn't work as sometimes we point more backward than down and then we choose axis responsible for bending as "to next")
		AutoSetupDirExact( skeleton, m_upperBone, m_nextDirUpperBS, upperToNextDirMS );
		AutoSetupDirExact( skeleton, m_jointBone, m_nextDirJointBS, jointToNextDirMS );
		AutoSetupDirExact( skeleton, m_lowerBone, m_nextDirLowerBS, lowerToNextDirMS );

		// to right (in model space)
		AutoSetupDir( skeleton, m_upperBone, m_sideDirUpperBS, sideDirMS ); // to right 
		AutoSetupDir( skeleton, m_jointBone, m_sideDirJointBS, sideDirMS );
		AutoSetupDir( skeleton, m_lowerBone, m_sideDirLowerBS, sideDirMS );

		// cross product in proper direction (forwardish)
		m_bendDirUpperBS = Cross( m_sideDirUpperBS, m_nextDirUpperBS ); // just cross product
		m_bendDirJointBS = Cross( m_sideDirJointBS, m_nextDirJointBS );
		m_bendDirLowerBS = Cross( m_sideDirLowerBS, m_nextDirLowerBS );
#endif
	}

	m_parentBone = m_upperBone != INDEX_NONE? skeleton->GetParentBoneIndex( m_upperBone ) : INDEX_NONE;

	if ( m_jointBone != INDEX_NONE && m_jointBone < skeleton->GetBonesNum() &&
		 m_lowerBone != INDEX_NONE && m_lowerBone < skeleton->GetBonesNum() )
	{
		m_upperLength = GetLength3( GetTranslation( skeleton->GetReferencePoseLS()[ m_jointBone ] ) );
		m_jointLength = GetLength3( GetTranslation( skeleton->GetReferencePoseLS()[ m_lowerBone ] ) );
	}
	
	m_limitToLengthPercentage = data.m_limitToLengthPercentage;

	if ( m_parentBone != INDEX_NONE &&
		 m_upperBone != INDEX_NONE && m_upperBone < skeleton->GetBonesNum() &&
		 m_jointBone != INDEX_NONE && m_jointBone < skeleton->GetBonesNum() &&
		 m_lowerBone != INDEX_NONE && m_lowerBone < skeleton->GetBonesNum() )
	{
		// check for parentBone don't make much sense here, but as they are ASSERTs and things are changing, let's keep them
		// make sure that joint is child of upper and lower is child of joint
		ASSERT( m_parentBone < m_upperBone &&
				m_upperBone < m_jointBone &&
				m_jointBone < m_lowerBone, TXT( "Bones are not in correct order, upper is parent of joint, joint is parent of lower." ) );
		// make sure that they are direct children
		if ( m_subLowerBone != INDEX_NONE )
		{
			ASSERT( skeleton->GetParentBoneIndex( m_upperBone ) == m_parentBone &&
					skeleton->GetParentBoneIndex( m_jointBone ) == m_upperBone &&
					skeleton->GetParentBoneIndex( m_subLowerBone ) == m_jointBone &&
					skeleton->GetParentBoneIndex( m_lowerBone ) == m_subLowerBone, TXT( "Bones don't have direct child-parent in correct order relationship." ) );
		}
		else
		{
			ASSERT( skeleton->GetParentBoneIndex( m_upperBone ) == m_parentBone &&
					skeleton->GetParentBoneIndex( m_jointBone ) == m_upperBone &&
					skeleton->GetParentBoneIndex( m_lowerBone ) == m_jointBone, TXT( "Bones don't have direct child-parent in correct order relationship." ) );
		}
	}

	m_lock = 0.0f;
}

void STwoBonesIKSolver::GetUpperAndLowerIKTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper, AnimQsTransform& outLower ) const
{
	outUpper = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_upperBone );
	if ( m_ikBone == INDEX_NONE )
	{
		AnimQsTransform jointBoneTMS;
		SetMulTransform( jointBoneTMS, outUpper, output.m_outputPose[ m_jointBone ] );
		if ( m_subLowerBone == INDEX_NONE )
		{
			SetMulTransform( outLower, jointBoneTMS, output.m_outputPose[ m_lowerBone ] );
		}
		else
		{
			AnimQsTransform subLowerBoneTMS;
			SetMulTransform( subLowerBoneTMS, jointBoneTMS, output.m_outputPose[ m_subLowerBone ] );
			SetMulTransform( outLower, subLowerBoneTMS, output.m_outputPose[ m_lowerBone ] );
		}
	}
	else
	{
		outLower = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_ikBone );
	}
}

void STwoBonesIKSolver::GetUpperAndLowerTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper, AnimQsTransform& outLower ) const
{
	outUpper = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_upperBone );
	AnimQsTransform jointBoneTMS;
	SetMulTransform( jointBoneTMS, outUpper, output.m_outputPose[ m_jointBone ] );
	if ( m_subLowerBone == INDEX_NONE )
	{
		SetMulTransform( outLower, jointBoneTMS, output.m_outputPose[ m_lowerBone ] );
	}
	else
	{
		AnimQsTransform subLowerBoneTMS;
		SetMulTransform( subLowerBoneTMS, jointBoneTMS, output.m_outputPose[ m_subLowerBone ] );
		SetMulTransform( outLower, subLowerBoneTMS, output.m_outputPose[ m_lowerBone ] );
	}
}

void STwoBonesIKSolver::GetLowerTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outLower ) const
{
	outLower = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_lowerBone );
}

void STwoBonesIKSolver::GetUpperTMS( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, AnimQsTransform& outUpper ) const
{
	outUpper = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_upperBone );
}

void STwoBonesIKSolver::GetRefLowerTMS( CBehaviorGraphInstance& instance, AnimQsTransform& outLower ) const
{
	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton() )
	{
		outLower = skeleton->GetBoneMS( m_lowerBone );
	}
	else
	{
#ifndef USE_HAVOK_ANIMATION
		outLower = AnimQsTransform::IDENTITY;
#endif
	}
}

void STwoBonesIKSolver::SetTargetLowerTMS( const AnimQsTransform& targetTMS )
{
	m_targetLowerTMS = targetTMS;
}

void STwoBonesIKSolver::SetAdditionalSideDirMS( const AnimVector4& additionalSideDirMS )
{
	m_additionalSideDirMS = additionalSideDirMS;
}

void STwoBonesIKSolver::UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& data, Float weight, Float timeDelta )
{
	if ( m_parentBone == INDEX_NONE ||
		 m_upperBone == INDEX_NONE || ( Uint32 )m_upperBone >= output.m_numBones ||
		 m_jointBone == INDEX_NONE || ( Uint32 )m_jointBone >= output.m_numBones ||
		 m_lowerBone == INDEX_NONE || ( Uint32 )m_lowerBone >= output.m_numBones )
	{
		return;
	}

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	AnimQsTransform boneAParent_MS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_parentBone );
	AnimQsTransform boneA_LS_1 = output.m_outputPose[ m_upperBone ];
	AnimQsTransform boneB_LS_1 = output.m_outputPose[ m_jointBone ];
	AnimQsTransform boneC_LS_1 = output.m_outputPose[ m_lowerBone ];
	AnimQsTransform boneD_LS_1 = output.m_outputPose[ m_lowerBone+1 ];

	m_debugTargetWS = ( AnimQsTransformToMatrix( m_targetLowerTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld() );
#endif

	// assumptions:
	//	depends on bones being in order one after another (checked in Setup)
	//	depends on X facing next bone (fwd), Y facing to bend dir (up), Z facing to side (right ( ! )) (XYZ for reference, where it is used in lower parts) TODO code check

	AnimQsTransform parentBoneTMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_parentBone );
	AnimQsTransform upperBoneTMS;
	SetMulTransform( upperBoneTMS, parentBoneTMS, output.m_outputPose[ m_upperBone ] );
	AnimQsTransform jointBoneTMS;
	SetMulTransform( jointBoneTMS, upperBoneTMS, output.m_outputPose[ m_jointBone ] );
	AnimQsTransform subLowerBoneTMS;
	AnimQsTransform lowerBoneTMS;
	if ( m_subLowerBone == INDEX_NONE )
	{
		SetMulTransform( lowerBoneTMS, jointBoneTMS, output.m_outputPose[ m_lowerBone ] );
	}
	else
	{
		SetMulTransform( subLowerBoneTMS, jointBoneTMS, output.m_outputPose[ m_subLowerBone ] );
		SetMulTransform( lowerBoneTMS, subLowerBoneTMS, output.m_outputPose[ m_lowerBone ] );
	}

	AnimVector4 sideDirUJMS; // upper and joint
	AnimVector4 sideDirMS;
	{	// calculate bend dir by utilizing offsets for different bones
		TransformVectorNoScale( sideDirUJMS, jointBoneTMS, m_sideDirJointBS );
		MulVector( sideDirUJMS, m_jointSideWeightJoint );
		AnimVector4 temp;
		TransformVectorNoScale( temp, upperBoneTMS, m_sideDirUpperBS );
		AddVectorMul( sideDirUJMS, temp, m_jointSideWeightUpper );
		sideDirMS = sideDirUJMS;
		AnimVector4 fromLower;
		TransformVectorNoScale( fromLower, m_targetLowerTMS, m_sideDirLowerBS );
		AddVectorMul( sideDirMS, fromLower, m_jointSideWeightLower );
		Normalize3( sideDirMS );
	}
#ifdef DEBUG_TWO_BONES_IK_SOLVER
	m_debugRawSideDirMS = AnimVectorToVector( sideDirMS );
#endif

	/**														where:
	 *						b									a	upper
	 *						|									b	joint
	 *				  A		E	   B							c	lower
	 *						|
	 *			a ----D-----d------------ c
	 *						C
	 *		we know:
	 *			A = |ab|
	 *			B = |bc|
	 *			C = |ac|
	 *		we need:
	 *			D = |ad|
	 *			E = |bd|
	 *		let's go!
	 *			E^2 + D^2 = A^2
	 *			( C-D )^2 + E^2 = B^2
	 *		E^2 is the same:
	 *			E^2 = A^2 - D^2
	 *			E^2 = B^2 - ( C-D )^2
	 *		continue to find D:
	 *			A^2 - D^2 = B^2 - ( C-D )^2
	 *			A^2 - D^2 = B^2 - ( C^2 - 2CD + D^2 )
	 *			A^2 - D^2 = B^2 - C^2 + 2CD - D^2
	 *			A^2 - B^2 + C^2 = 2CD
	 *			2CD = C^2 + A^2 - B^2
	 *			    C^2 + A^2 - B^2
	 *			D = ---------------
	 *			          2C
	 *		and now we can have E:
	 *				  __________
	 *			E = \/A^2 - D^2'
	 */
	// calculate them basing on frame as reference skeleton might be messed up or they actually stretch! TODO check what's going on there - maybe reference skeleton is not modified and animations are for bigger skeleton?!
	m_upperLength = Sub( upperBoneTMS.GetTranslation(), jointBoneTMS.GetTranslation() ).Length3();
	m_jointLength = Sub( lowerBoneTMS.GetTranslation(), jointBoneTMS.GetTranslation() ).Length3();
	const AnimFloat A = m_upperLength;
	const AnimFloat B = m_jointLength;
		  AnimFloat C = GetDist3( GetTranslation( m_targetLowerTMS ), GetTranslation( upperBoneTMS ) ); // requested distance
	const AnimFloat maxLength = A + B;
	const AnimFloat allowedLength = maxLength * m_limitToLengthPercentage;
	if ( C > allowedLength )
	{
		const AnimFloat lengthWindow = maxLength - allowedLength;
		C = C - allowedLength;
		C = maxLength - lengthWindow / ( 1.0f + C );
	}
	const AnimFloat Asquare = A * A;
	const AnimFloat D = Min( A, C != 0.0f? ( C * C + Asquare - B * B ) / ( 2.0f * C ) : 0.0f ); // don't stretch!
		  AnimFloat E = Sqrt( Max( 0.0f, Asquare - D * D ) ) * ( data.m_reverseBend ? -1.0f : 1.0f );
	
	// there are no checks ATM and no corrections!
	// for time being I don't want to introduce them as they look ugly and system should not allow to end in such state
	// only checks are above for calculating C and D and below (to make sure 'c' is not further than C allows to)
	// TODO maybe remove this extra code as we don't want to end in such cases anyway!

	// find points from picture above
	AnimVector4 a = GetTranslation( upperBoneTMS );
	AnimVector4 c = GetTranslation( m_targetLowerTMS );
	AnimVector4 a2c = PointToPoint( a, c );
	Normalize3( a2c );

	if ( m_additionalSideDirMS.SquareLength3() > 0.0f )
	{
		AddVector( sideDirMS, m_additionalSideDirMS );
		Normalize3( sideDirMS );
	}
	{	// we need to have side dir perpendicular to a2c - remove part that goes along a2c
		AnimFloat sideDir_a2c;
		sideDir_a2c = RedMath::SIMD::Dot3( sideDirMS, a2c );
		SubVectorMul( sideDirMS, a2c, sideDir_a2c );
		Normalize3( sideDirMS );
	}
	AnimVector4 bendDirMS;
	SetCross( bendDirMS, sideDirMS, a2c );

	// take care of C exceeding A + B
	c = a;
	AddVectorMul( c, a2c, C );

	AnimVector4 d = a;
	AddVectorMul( d, a2c, D );

	if ( data.m_allowToLock )
	{
		AnimFloat desiredLock = 0.0f;

		AnimVector4 curr_a2c = PointToPoint( GetTranslation( upperBoneTMS ), GetTranslation( lowerBoneTMS ) );

		AnimFloat a2cLen = GetLength3( PointToPoint( a, c ) );
		AnimFloat curr_a2cLen = GetLength3( curr_a2c );
		if ( Abs( a2cLen - curr_a2cLen ) < curr_a2cLen * 0.05f ) // are more or less same length
		{
			// this is similar to what's going above but it calculates if leg is bended backwards to know if we should bend backwards too
			Normalize3( curr_a2c );

			AnimVector4 curr_bendDirMS;

			AnimVector4 curr_sideDirMS = sideDirUJMS;
			AnimVector4 fromLower;
			TransformVectorNoScale( fromLower, lowerBoneTMS, m_sideDirLowerBS );
			AddVectorMul( curr_sideDirMS, fromLower, m_jointSideWeightLower );
			Normalize3( curr_sideDirMS );

			AnimFloat curr_sideDir_a2c;
			curr_sideDir_a2c = RedMath::SIMD::Dot3( curr_sideDirMS, curr_a2c );
			SubVectorMul( curr_sideDirMS, curr_a2c, curr_sideDir_a2c );
			Normalize3( curr_sideDirMS );

			SetCross( curr_bendDirMS, curr_sideDirMS, curr_a2c );

			AnimVector4 upToJointMS = Sub( GetTranslation( jointBoneTMS ), GetTranslation( upperBoneTMS ) );
			if ( RedMath::SIMD::Dot3( curr_bendDirMS, upToJointMS ) < 0.0f )
			{
				desiredLock = 1.0f;
			}
		}

		m_lock = BlendOnOffWithSpeedBasedOnTime( m_lock, desiredLock, 0.2f, timeDelta );
		E *= ( 1.0f - 2.0f * m_lock );
	}

	AnimVector4 b = d;
	AddVectorMul( b, bendDirMS, E );

	AnimVector4 a2b = PointToPoint( a, b );
	Normalize3( a2b );

	AnimVector4 b2c = PointToPoint( b, c );
	Normalize3( b2c );

	// calculate bend direction to side
	AnimVector4 bendSideMS;
	bendSideMS = sideDirMS;
	Normalize3( bendSideMS );

	// compose transforms ( upper and lower now )
	AnimQsTransform outUpperBoneTMS = upperBoneTMS;
	CalculateAnimTransformLFS( outUpperBoneTMS, a, a2b, bendSideMS, m_nextDirUpperBS, m_sideDirUpperBS, m_bendDirUpperBS ); // XYZ

	AnimQsTransform outLowerBoneTMS = m_targetLowerTMS;
	SetTranslation( outLowerBoneTMS, c ); // use provided rotation and only change location

	// for joint-lower try to be between lower and upper dir
	AnimQsTransform outJointBoneTMS = jointBoneTMS;
	AnimVector4 jointSideMS = bendSideMS;
	{	// align joint with lower
		// TODO - option?
		AnimVector4 lowerSideMS;
		TransformVectorNoScale( lowerSideMS, outLowerBoneTMS, m_sideDirLowerBS ); // XYZ
		AddVector( jointSideMS, lowerSideMS );
		Normalize3( jointSideMS );
		// we need it to be orthogonal
		AnimFloat jointSide_b2c;
		jointSide_b2c = RedMath::SIMD::Dot3( jointSideMS, b2c );
		SubVectorMul( jointSideMS, b2c, jointSide_b2c );
		Normalize3( jointSideMS );
		/*
		AnimVector4 bend;
		SetCross( bend, jointSideMS, b2c );
		SetCross( jointSideMS, b2c, bend );
		*/
	}
#ifdef DEBUG_TWO_BONES_IK_SOLVER
	m_debugJointSideMS = AnimVectorToVector( jointSideMS );
#endif
	CalculateAnimTransformLFS( outJointBoneTMS, b, b2c, jointSideMS, m_nextDirJointBS, m_sideDirJointBS, m_bendDirJointBS ); // XYZ

	AnimQsTransform outSubLowerBoneTMS;
	if ( m_subLowerBone != INDEX_NONE )
	{
		// now joint points at lower - but we want it to point at sublower
		// first - how joint would be if it pointed at lower?
		AnimQsTransform preJointBoneAtLowerTMS;
		AnimVector4 preJointSideMS;
		TransformVectorNoScale( preJointSideMS, jointBoneTMS, m_sideDirJointBS ); // XYZ
		AnimVector4 preJointAtLowerFwdMS = Sub( lowerBoneTMS.GetTranslation(), jointBoneTMS.GetTranslation() ).Normalized3();
		AnimVector4 preJointAtLowerSideMS = preJointSideMS;
		{
			AnimFloat sideFwdDot;
			sideFwdDot = RedMath::SIMD::Dot3( preJointAtLowerSideMS, preJointAtLowerFwdMS );
			SubVectorMul( preJointAtLowerSideMS, preJointAtLowerFwdMS, sideFwdDot );
			Normalize3( preJointAtLowerSideMS );
		}
		CalculateAnimTransformLFS( preJointBoneAtLowerTMS, jointBoneTMS.GetTranslation(), preJointAtLowerFwdMS, preJointAtLowerSideMS, m_nextDirJointBS, m_sideDirJointBS, m_bendDirJointBS ); // XYZ
		AnimQsTransform localJointAtLowerOff;
		SetMulInverseMulTransform( localJointAtLowerOff, preJointBoneAtLowerTMS, jointBoneTMS );
		// move outJointBoneTMS by that value to point at sub lower after everything
		SetMulTransform( outJointBoneTMS, outJointBoneTMS, localJointAtLowerOff );
		// keep same relative for sub lower
		SetMulTransform( outSubLowerBoneTMS, outJointBoneTMS, output.m_outputPose[ m_subLowerBone ] );
	}

	// calculate local but as full IK
	AnimQsTransform fullIKUpperBoneQT;
	AnimQsTransform fullIKJointBoneQT;
	AnimQsTransform fullIKSubLowerBoneQT;
	AnimQsTransform fullIKLowerBoneQT;
	SetMulInverseMulTransform( fullIKUpperBoneQT, parentBoneTMS, outUpperBoneTMS );
	SetMulInverseMulTransform( fullIKJointBoneQT, outUpperBoneTMS, outJointBoneTMS );
	if ( m_subLowerBone == INDEX_NONE )
	{
		SetMulInverseMulTransform( fullIKLowerBoneQT, outJointBoneTMS, outLowerBoneTMS );
	}
	else
	{
		SetMulInverseMulTransform( fullIKSubLowerBoneQT, outJointBoneTMS, outSubLowerBoneTMS );
		SetMulInverseMulTransform( fullIKLowerBoneQT, outSubLowerBoneTMS, outLowerBoneTMS );
	}

	// blend
	AnimQsTransform& outputIKUpperBoneQT = output.m_outputPose[ m_upperBone ];
	AnimQsTransform& outputIKJointBoneQT = output.m_outputPose[ m_jointBone ];
	AnimQsTransform& outputIKLowerBoneQT = output.m_outputPose[ m_lowerBone ];
	if ( weight > 0.999f )
	{
		outputIKUpperBoneQT = fullIKUpperBoneQT;
		outputIKJointBoneQT = fullIKJointBoneQT;
		outputIKLowerBoneQT = fullIKLowerBoneQT;

		if ( m_subLowerBone != INDEX_NONE )
		{
			AnimQsTransform& outputIKSubLowerBoneQT = output.m_outputPose[ m_subLowerBone ];
			outputIKSubLowerBoneQT = fullIKSubLowerBoneQT;
		}
	}
	else
	{
		BlendTwoTransforms( outputIKUpperBoneQT, outputIKUpperBoneQT, fullIKUpperBoneQT, weight );
		BlendTwoTransforms( outputIKJointBoneQT, outputIKJointBoneQT, fullIKJointBoneQT, weight );
		BlendTwoTransforms( outputIKLowerBoneQT, outputIKLowerBoneQT, fullIKLowerBoneQT, weight );

		if ( m_subLowerBone != INDEX_NONE )
		{
			AnimQsTransform& outputIKSubLowerBoneQT = output.m_outputPose[ m_subLowerBone ];
			BlendTwoTransforms( outputIKSubLowerBoneQT, outputIKSubLowerBoneQT, fullIKSubLowerBoneQT, weight );
		}
	}

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	AnimQsTransform boneA_LS_2 = output.m_outputPose[ m_upperBone ];
	AnimQsTransform boneB_LS_2 = output.m_outputPose[ m_jointBone ];
	AnimQsTransform boneC_LS_2 = output.m_outputPose[ m_lowerBone ];
	AnimQsTransform boneD_LS_2 = output.m_outputPose[ m_lowerBone+1 ];

	AnimQsTransform boneA_LS_3 = boneA_LS_1;
	AnimQsTransform boneB_LS_3 = boneB_LS_1;
	AnimQsTransform boneC_LS_3 = boneC_LS_1;
	AnimQsTransform boneD_LS_3 = boneD_LS_1;

	{
		Matrix l2w = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();

		{
			AnimQsTransform boneA_MS;
			boneA_MS.SetMul( boneAParent_MS, boneA_LS_1 );
			AnimQsTransform boneB_MS;
			boneB_MS.SetMul( boneA_MS, boneB_LS_1 );
			AnimQsTransform boneC_MS;
			boneC_MS.SetMul( boneB_MS, boneC_LS_1 );
			AnimQsTransform boneD_MS;
			boneD_MS.SetMul( boneC_MS, boneD_LS_1 );

			m_debugBoneAWS_1 = AnimQsTransformToMatrix( boneA_MS ) * l2w;
			m_debugBoneBWS_1 = AnimQsTransformToMatrix( boneB_MS ) * l2w;
			m_debugBoneCWS_1 = AnimQsTransformToMatrix( boneC_MS ) * l2w;
			m_debugBoneDWS_1 = AnimQsTransformToMatrix( boneD_MS ) * l2w;
		}

		{
			AnimQsTransform boneA_MS;
			boneA_MS.SetMul( boneAParent_MS, boneA_LS_2 );
			AnimQsTransform boneB_MS;
			boneB_MS.SetMul( boneA_MS, boneB_LS_2 );
			AnimQsTransform boneC_MS;
			boneC_MS.SetMul( boneB_MS, boneC_LS_2 );
			AnimQsTransform boneD_MS;
			boneD_MS.SetMul( boneC_MS, boneD_LS_2 );

			m_debugBoneAWS_2 = AnimQsTransformToMatrix( boneA_MS ) * l2w;
			m_debugBoneBWS_2 = AnimQsTransformToMatrix( boneB_MS ) * l2w;
			m_debugBoneCWS_2 = AnimQsTransformToMatrix( boneC_MS ) * l2w;
			m_debugBoneDWS_2 = AnimQsTransformToMatrix( boneD_MS ) * l2w;
		}

		{
			AnimQsTransform boneA_MS;
			boneA_MS.SetMul( boneAParent_MS, boneA_LS_3 );
			AnimQsTransform boneB_MS;
			boneB_MS.SetMul( boneA_MS, boneB_LS_3 );
			AnimQsTransform boneC_MS;
			boneC_MS.SetMul( boneB_MS, boneC_LS_3 );
			AnimQsTransform boneD_MS;
			boneD_MS.SetMul( boneC_MS, boneD_LS_3 );

			m_debugBoneAWS_3 = AnimQsTransformToMatrix( boneA_MS ) * l2w;
			m_debugBoneBWS_3 = AnimQsTransformToMatrix( boneB_MS ) * l2w;
			m_debugBoneCWS_3 = AnimQsTransformToMatrix( boneC_MS ) * l2w;
			m_debugBoneDWS_3 = AnimQsTransformToMatrix( boneD_MS ) * l2w;
		}
	}

	m_debugSideDirMS = AnimVectorToVector( sideDirMS );
	m_upperMatWSPost = ( AnimQsTransformToMatrix( outUpperBoneTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld() );
	m_jointMatWSPost = ( AnimQsTransformToMatrix( outJointBoneTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld() );
	m_lowerMatWSPost = ( AnimQsTransformToMatrix( outLowerBoneTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld() );
	m_debugTargetWS = ( AnimQsTransformToMatrix( m_targetLowerTMS ) * instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld() );
	m_debugA = A;
	m_debugB = B;
	m_debugC = C;
	m_debugD = D;
	m_debugE = E;
#endif
}

#ifdef DEBUG_TWO_BONES_IK_SOLVER
void STwoBonesIKSolver::GatherDebugData( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& data )
{
	m_debugJointSideDirUpperBS = data.m_sideDirUpperBS;
	m_debugJointSideDirJointBS = data.m_sideDirJointBS;
	m_debugJointSideDirLowerBS = data.m_sideDirLowerBS;
	if ( m_upperBone == INDEX_NONE ||
		 m_jointBone == INDEX_NONE ||
		 m_lowerBone == INDEX_NONE )
	{
		if ( m_upperBone != INDEX_NONE )
		{
			m_upperMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_upperBone );
		}
		if ( m_jointBone != INDEX_NONE )
		{
			m_jointMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_jointBone );
		}
		if ( m_lowerBone != INDEX_NONE )
		{
			m_lowerMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_lowerBone );
		}
		return;
	}

	m_upperMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_upperBone );
	m_jointMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_jointBone );
	m_lowerMatWS = output.GetBoneWorldMatrix( instance.GetAnimatedComponent(), m_lowerBone );
}
#endif

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
