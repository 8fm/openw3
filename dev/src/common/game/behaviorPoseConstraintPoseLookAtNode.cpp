/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorPoseConstraintPoseLookAtNode.h"

#include "lookAtParam.h"
#include "bgNpc.h"

#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphOutput.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/behaviorGraphSocket.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/animatedIterators.h"
#include "../engine/curve.h"
#include "../core/mathUtils.h"
#include "../engine/graphConnectionRebuilder.h"

//#define DENUG_POSE_LOOK_AT
#ifdef DENUG_POSE_LOOK_AT
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SPoseLookAtSegmentData );
IMPLEMENT_ENGINE_CLASS( SPoseLookAtSegment );

IMPLEMENT_ENGINE_CLASS( IBehaviorPoseConstraintPoseLookAtModifier );
IMPLEMENT_ENGINE_CLASS( CBPCPoseLookAtCurveTrajModifier );

SPoseLookAtSegment::SPoseLookAtSegment()
	: m_data( NULL )
	, m_hAngle( 0.f )
	, m_vAngle( 0.f )
	, m_progress( 0.f )
	, m_hPrevAngle( 0.f )
	, m_vPrevAngle( 0.f )
	, m_limited( false )
	, m_num( 0 )
{

}

SPoseLookAtSegment::SPoseLookAtSegment( Uint32 num, const SPoseLookAtSegmentData* data, const CSkeleton* skeleton )
	: m_data( data )
	, m_hAngle( 0.f )
	, m_vAngle( 0.f )
	, m_progress( 0.f )
	, m_hPrevAngle( 0.f )
	, m_vPrevAngle( 0.f )
	, m_limited( false )
	, m_num( num )
{
	for ( BoneChainIterator it( skeleton, m_data->m_boneNameFirst.AsChar(), m_data->m_boneNameLast.AsChar() ); it; ++it )
	{
		m_bones.PushBack( it.GetIndex() );
	}
}

Bool SPoseLookAtSegment::IsValid() const
{
	if  ( !m_data || m_bones.Size() < 2 )
	{
		return false;
	}

	for ( Uint32 i=0; i<m_bones.Size(); ++i )
	{
		if ( m_bones[ i ] == -1 )
		{
			return false;
		}
	}

	return true;
}

Vector SPoseLookAtSegment::GetReferenceLookDir( const AnimQsTransform& boneLS ) const
{
#ifdef USE_HAVOK_ANIMATION
	hkVector4 hkRefLookDir;
	hkRefLookDir.setTransformedPos( boneLS, BehaviorUtils::hkVectorFromAxis( GetFront() ) );
	hkRefLookDir.normalize3();
	return TO_CONST_VECTOR_REF( hkRefLookDir );
#else
	RedVector4 refLookDir; 
	refLookDir.SetTransformedPos( boneLS, BehaviorUtils::RedVectorFromAxis( GetFront() ) );
	refLookDir.Normalize3();
	return reinterpret_cast< const Vector& >( refLookDir );
#endif
}

Vector SPoseLookAtSegment::GetReferenceUpDir( const AnimQsTransform& boneLS ) const
{
#ifdef USE_HAVOK_ANIMATION
	hkVector4 hkRefUpDir;
	hkRefUpDir.setTransformedPos( boneLS, BehaviorUtils::hkVectorFromAxis( GetUp() ) );
	hkRefUpDir.normalize3();
	return TO_CONST_VECTOR_REF( hkRefUpDir );
#else
	RedVector4 refUpDir; 
	refUpDir.SetTransformedPos( boneLS, BehaviorUtils::RedVectorFromAxis( GetUp() ) );
	refUpDir.Normalize3();
	return reinterpret_cast< const Vector& >( refUpDir );
#endif
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseConstraintPoseLookAtNode );

CBehaviorGraphPoseConstraintPoseLookAtNode::CBehaviorGraphPoseConstraintPoseLookAtNode()
{

}

void CBehaviorGraphPoseConstraintPoseLookAtNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_owner;
	compiler << i_valid;
	compiler << i_segments;
	compiler << i_targetId;
	compiler << i_active;
	compiler << i_isLimit;
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_owner ] = instance.GetAnimatedComponent()->GetEntity();
	instance[ i_valid ] = instance[ i_owner ] && CreateSegments( instance );

	InternalReset( instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphPoseConstraintPoseLookAtNode::GetCaption() const
{
	return TXT("Pose look at");
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();
}
#endif

void CBehaviorGraphPoseConstraintPoseLookAtNode::CacheConnections()
{
	TBaseClass::CacheConnections();
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("modifiers") )
	{
		for ( Uint32 i=0; i<m_modifiers.Size(); ++i )
		{
			IBehaviorPoseConstraintPoseLookAtModifier* m = m_modifiers[ i ];
			if ( m )
			{
				m->Reload();
			}
		}
	}
}

namespace 
{
	Float Interpolate( Float from, Float to, Float t )
	{
		return from + ( to - from ) * t;
	}
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	if ( !IsValid( instance ) )
	{
		return;
	}

	const CLookAtStaticParam* staticParam = NULL;
	const CLookAtDynamicParam* dynamicParam = NULL;
	CLookAtContextParam contextParam;

	if ( !GetLookAtParams( instance, dynamicParam, staticParam, contextParam ) )
	{
		return;
	}

	const Int32 currTargetId = dynamicParam ? dynamicParam->GetID() : -1;
	Int32& prevTargetId = instance[ i_targetId ];

	Bool hasTarget = dynamicParam != NULL;

	Bool& isAnySegmentActive = instance[ i_active ];

	if ( contextParam.m_reset )
	{
		if ( isAnySegmentActive )
		{
			TDynArray< SPoseLookAtSegment >& allSegments = instance[ i_segments ];
			for ( Uint32 i=0; i<allSegments.Size(); ++i )
			{
				SPoseLookAtSegment& segment = allSegments[ i ];
				segment.m_hAngle = 0;
				segment.m_vAngle = 0;
				segment.m_hPrevAngle = 0;
				segment.m_vPrevAngle = 0;
				segment.m_progress = 0.f;
				segment.m_limited = false;
			}
		}

		isAnySegmentActive = false;
	}

	if ( !hasTarget && !isAnySegmentActive )
	{
		prevTargetId = currTargetId;
		return;
	}
	else if ( hasTarget )
	{
		//ASSERT( MAbs( contextParam.m_speed - 1.f ) < NumericLimits< Float >::Epsilon() );
	}

	// Is target changed
	Bool isTargetChanged = prevTargetId != currTargetId;

	// Target
	Vector targetWS = dynamicParam ? dynamicParam->GetTarget() : Vector::ZERO_3D_POINT;
	AnimVector4 lookDirWS = reinterpret_cast< const AnimVector4&>( targetWS );
	instance[ i_targetPos ] = targetWS;

	// Instant
	Bool instant = isTargetChanged && dynamicParam && dynamicParam->IsInstant();

	// Reset before next upate process
	isAnySegmentActive = false;

	// Limit
	Bool& isLimit = instance[ i_isLimit ];
	Bool limitConstraint = false;

	// Get level
	ELookAtLevel level = Max( contextParam.m_actorLevel, GetLevel( staticParam, dynamicParam ) );

	TDynArray< SPoseLookAtSegment >& allSegments = instance[ i_segments ];
	for ( Uint32 i=0; i<allSegments.Size(); ++i )
	{
		SPoseLookAtSegment& segment = allSegments[ i ];

		const TDynArray< Int32 >& bones = segment.m_bones;
		ASSERT( bones.Size() >= 2 );

		Float weight = segment.GetWeight();
		if ( staticParam )
		{
			weight *= staticParam->GetWeight( i );	
		}
		weight *= contextParam.m_headRotationRatio;

		weight = Clamp( weight, 0.f, 1.f );

		const Int32 firstBone = bones[ 0 ]; // np. Head
		const Int32 parentBone = bones[ 1 ];
		AnimQsTransform& firstBoneLS = output.m_outputPose[ firstBone ];
		Float hAngle = 0.f;
		Float vAngle = 0.f;

		Float hAnglePose = 0.f;
		Float vAnglePose = 0.f;

		//////////////////////////////////////////////////////////////////////////
		// Calc angles

		Vector referenceLookDir = segment.GetReferenceLookDir( firstBoneLS );
		Vector referenceUpDir = segment.GetReferenceUpDir( firstBoneLS );

		Bool shouldBeUsed = level <= segment.GetSegmentLevel();

		if ( hasTarget && shouldBeUsed )
		{
			const CAnimatedComponent* ac = instance.GetAnimatedComponent();

			AnimQsTransform parentBoneMS = output.GetBoneModelTransform( ac, parentBone );
			AnimQsTransform l2w = MatrixToAnimQsTransform( ac->GetThisFrameTempLocalToWorld() );
			AnimQsTransform parentBoneWS;
			parentBoneWS.SetMul( l2w, parentBoneMS );

			AnimQsTransform firstBoneWS;

			firstBoneWS.SetMul( parentBoneWS, firstBoneLS );

			// Parent space
			RedVector4 lookDirGoal; 
			lookDirGoal.SetTransformedInversePos( parentBoneWS, lookDirWS );
			lookDirGoal.Normalize3();
			Vector lookDirGoalVec = reinterpret_cast< const Vector&>( lookDirGoal );

			hAngle = MathUtils::VectorUtils::GetAngleDegAroundAxis( referenceLookDir, lookDirGoalVec, referenceUpDir );

			Vector rightOfTarget = Vector::Cross( referenceUpDir, lookDirGoalVec ).Normalized3();

			Vector lookDirGoalinHPlane = lookDirGoalVec - Vector::Project( lookDirGoalVec, referenceUpDir );
			lookDirGoalinHPlane.Normalize3();

			vAngle = MathUtils::VectorUtils::GetAngleDegAroundAxis( lookDirGoalinHPlane, lookDirGoalVec, rightOfTarget );		

 			if ( segment.UseRefPoseLimit() )
 			{
 				Vector referenceRefLookDir = BehaviorUtils::VectorFromAxis( segment.GetFront() );
 				Vector referenceRefUpDir = BehaviorUtils::VectorFromAxis( segment.GetUp() );
 
 				hAnglePose = MathUtils::VectorUtils::GetAngleDegAroundAxis( referenceRefLookDir, referenceLookDir, referenceRefUpDir );
 
 				Vector rightOfTargetRef = Vector::Cross( referenceRefUpDir, referenceLookDir ).Normalized3();
 
 				Vector lookDirGoalinHPlaneRef = referenceLookDir - Vector::Project( referenceLookDir, referenceRefUpDir );
 				lookDirGoalinHPlaneRef.Normalize3();
 
 				vAnglePose = MathUtils::VectorUtils::GetAngleDegAroundAxis( lookDirGoalinHPlaneRef, referenceLookDir, rightOfTargetRef );
 			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Limits

		if ( dynamicParam )
		{
			segment.m_limited = LimitAngles( hAngle, vAngle, hAnglePose, vAnglePose, staticParam, dynamicParam, segment );
			if ( i == allSegments.Size()-1 && segment.m_limited && dynamicParam->IsAutoLimitDeact() )
			{
				limitConstraint = true;
			}
		}
		
		if ( isLimit )
		{
			hAngle = 0.f;
			vAngle = 0.f;
		}

		/////////////////////////////////////////////////////////////////////////
		// Damping

		if ( instant )
		{
			ClampAngles( hAngle, vAngle, staticParam, dynamicParam, segment );

			segment.m_hAngle = DEG2RAD( hAngle );
			segment.m_vAngle = DEG2RAD( vAngle );
			segment.m_hPrevAngle = DEG2RAD( hAngle );
			segment.m_vPrevAngle = DEG2RAD( vAngle );
			segment.m_progress = 1.f;
		}

		for ( Uint32 i=0; i<m_modifiers.Size(); ++i )
		{
			IBehaviorPoseConstraintPoseLookAtModifier* m = m_modifiers[ i ];
			if ( m )
			{
				m->PreModify( hAngle, vAngle, isTargetChanged, segment );
			}
		}

		CalcAnglesForSegment(	instance, 
								hAngle, vAngle, 
								staticParam, dynamicParam, contextParam,
								isTargetChanged, instant,
								segment );

		for ( Uint32 i=0; i<m_modifiers.Size(); ++i )
		{
			IBehaviorPoseConstraintPoseLookAtModifier* m = m_modifiers[ i ];
			if ( m )
			{
				m->PostModify( hAngle, vAngle, isTargetChanged, segment );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Apply angles

		//BEH_LOG( TXT("h %f"), hAngle );

		if	( MAbs( hAngle ) > NumericLimits< Float >::Epsilon() || MAbs( vAngle ) > NumericLimits< Float >::Epsilon() ||
			  MAbs( segment.m_hAngle ) > NumericLimits< Float >::Epsilon() || MAbs( segment.m_vAngle ) > NumericLimits< Float >::Epsilon() )
		{
			isAnySegmentActive = true;

			Vector referenceRightDir = Vector::Cross( referenceUpDir, referenceLookDir );
#ifdef USE_HAVOK_ANIMATION
			hkQuaternion goalOrient;

			referenceUpDir.Normalize3();
			referenceRightDir.Normalize3();
			hkQuaternion goalOrientH; goalOrientH.setAxisAngle( TO_CONST_HK_VECTOR_REF( referenceUpDir ), segment.m_hAngle );
			hkQuaternion goalOrientV; goalOrientV.setAxisAngle( TO_CONST_HK_VECTOR_REF( referenceRightDir ), segment.m_vAngle );

			goalOrient.setMul( goalOrientH, goalOrientV );
#else
			RedQuaternion goalOrient;

			referenceUpDir.Normalize3();
			referenceRightDir.Normalize3();
			RedQuaternion goalOrientH; 
			goalOrientH.SetAxisAngle( reinterpret_cast< const RedVector4& >( referenceUpDir ), segment.m_hAngle );
			RedQuaternion goalOrientV; 
			goalOrientV.SetAxisAngle( reinterpret_cast< const RedVector4& >( referenceRightDir ), segment.m_vAngle );

			goalOrient.SetMul( goalOrientH, goalOrientV );
#endif
			//////////////////////////////////////////////////////////////////////////

			Vector upDirGoal = referenceUpDir;

			// TODO
			// Damp...

			//////////////////////////////////////////////////////////////////////////
#ifdef USE_HAVOK_ANIMATION
			// Weight
			hkQuaternion lookRotGoal; 
			lookRotGoal.setSlerp( hkQuaternion::getIdentity(), goalOrient, weight );

			Float partWeight = segment.OnlyFirstBone() ? 1.f : 1.f / (Float)bones.Size();

			hkQuaternion dividedRotation;
			dividedRotation.setSlerp( hkQuaternion::getIdentity(), lookRotGoal, partWeight );
			firstBoneLS.m_rotation.setMul( dividedRotation, firstBoneLS.m_rotation );
#else
			// Weight
			RedQuaternion lookRotGoal; 
			lookRotGoal.SetSlerp( RedQuaternion::IDENTITY, goalOrient, weight );

			Float partWeight = segment.OnlyFirstBone() ? 1.f : 1.f / (Float)bones.Size();

			RedQuaternion dividedRotation;
			dividedRotation.SetSlerp( RedQuaternion::IDENTITY, lookRotGoal, partWeight );
			firstBoneLS.Rotation.SetMul( dividedRotation, firstBoneLS.Rotation );
#endif


			if ( !segment.OnlyFirstBone() )
			{
				if ( segment.UsePropagation() )
				{
					for ( Int32 i=bones.Size()-1; i>=1; --i )
					{
						AnimQsTransform& boneInChainLS = output.m_outputPose[ bones[ i ] ];

#ifdef USE_HAVOK_ANIMATION
						boneInChainLS.m_rotation.setMul( dividedRotation, boneInChainLS.m_rotation );
#else
						boneInChainLS.Rotation.SetMul( dividedRotation, boneInChainLS.Rotation );
#endif
					}
				}
				else
				{
#ifdef USE_HAVOK_ANIMATION
					hkVector4 axisUpLS = BehaviorUtils::hkVectorFromAxis( segment.GetUp() );
					hkVector4 axisRightLS; axisRightLS.setCross( BehaviorUtils::hkVectorFromAxis( segment.GetUp() ), BehaviorUtils::hkVectorFromAxis( segment.GetFront() ) );
#else
					RedVector4 axisUpLS = BehaviorUtils::RedVectorFromAxis( segment.GetUp() );
					RedVector4 axisRightLS; 
					axisRightLS = Cross( BehaviorUtils::RedVectorFromAxis( segment.GetUp() ), BehaviorUtils::RedVectorFromAxis( segment.GetFront() ) );

#endif
					if ( partWeight * weight > 0.f )
					{
						Float partAngleH = segment.m_hAngle * partWeight * weight;
						Float partAngleV = 0.33f * segment.m_vAngle * partWeight * weight;

						for ( Int32 i=bones.Size()-1; i>=1; --i )
						{
							AnimQsTransform& boneInChainLS = output.m_outputPose[ bones[ i ] ];
							if ( partAngleH != 0.f )
							{
								BehaviorUtils::RotateBoneLS( boneInChainLS, partAngleH, axisUpLS );
							}

							if ( partAngleV != 0.f )
							{
								BehaviorUtils::RotateBoneLS( boneInChainLS, partAngleV, axisRightLS );
							}
						}
					}
				}
			}
		}
	}

	prevTargetId = currTargetId;
	isLimit = limitConstraint;
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_targetId ] = -1;
	instance[ i_active ] = false;
	instance[ i_isLimit ] = false;
}

Bool CBehaviorGraphPoseConstraintPoseLookAtNode::LimitAngles(	Float& hAngle, Float& vAngle, 
																Float hAnglePose, Float vAnglePose, 
																const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam,
																const SPoseLookAtSegment& segment ) const
{
	Bool ret = false;

	Float hAccAngle = hAngle + hAnglePose;
	Float vAccAngle = vAngle + vAnglePose;

	Float hMaxAngle = segment.GetMaxAngleHorToRefPose();
	if ( dynParam )
	{
		hMaxAngle = Min( hMaxAngle, dynParam->GetRange() / 2.f );
	}
	if ( staticParam )
	{
		hMaxAngle = Min( hMaxAngle, staticParam->GetMaxAngleHor() );
	}

	if ( hMaxAngle > 0.f && MAbs( hAccAngle ) > hMaxAngle )
	{
		hAngle = MSign( hAccAngle ) * ( hMaxAngle - hAnglePose );
		ret = true;
	}

	if ( MAbs( hAngle ) > segment.GetAngleMaxHor() )
	{
		hAngle = MSign( hAngle ) * segment.GetAngleMaxHor();
		ret = true;
	}

	Float vMaxAngle = segment.GetMaxAngleVerToRefPose();
	if ( staticParam )
	{
		vMaxAngle = Min( vMaxAngle, staticParam->GetMaxAngleVer() );
	}

	if ( vMaxAngle > 0.f && MAbs( vAccAngle ) > vMaxAngle )
	{
		vAngle = MSign( vAccAngle ) * ( vMaxAngle - vAnglePose );
		ret = true;
	}

	if ( MAbs( vAngle ) > segment.GetAngleMaxVer() )
	{
		vAngle = MSign( vAngle ) * segment.GetAngleMaxVer();
		ret = true;
	}

	return ret;
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::ClampAngles( Float& hAngle, Float& vAngle, const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, const SPoseLookAtSegment& segment ) const
{
	// Max angles
	Float maxHor = segment.GetAngleMaxHor();
	Float maxVer = segment.GetAngleMaxVer();
	if ( staticParam )
	{
		maxHor = Min( maxHor, staticParam->GetMaxAngleHor() );
		maxVer = Min( maxVer, staticParam->GetMaxAngleVer() );
	}
	if ( dynParam )
	{
		maxHor = Min( maxHor, dynParam->GetRange()/2.f );
	}

	hAngle = Clamp( hAngle, -maxHor, maxHor );
	vAngle = Clamp( vAngle, -maxVer, maxVer );
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::CalcAnglesForSegment(	CBehaviorGraphInstance& instance, 
																		Float hAngle, Float vAngle, 
																		const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, CLookAtContextParam& contextParam,
																		Bool isTargetChanged, Bool instant,
																		SPoseLookAtSegment& segment ) const
{
	Float dt = GetTimeDelta( instance );

	Float& angleH = segment.m_hAngle;
	Float& angleV = segment.m_vAngle;

	Float multiplier = segment.GetMultiplier();

	Float hAngleThr = CalcAngleThreshold( hAngle, segment.GetAngleThresholdDiffHor() );
	Float vAngleThr = CalcAngleThreshold( vAngle, segment.GetAngleThresholdDiffVer() );

	hAngle = DampAngle( hAngle, hAngleThr, segment.GetMaxAngleDiffHor(), multiplier );
	vAngle = DampAngle( vAngle, vAngleThr, segment.GetMaxAngleDiffVer(), multiplier );

	// Clamp
	ClampAngles( hAngle, vAngle, staticParam, dynParam, segment );

	// Responsiveness
	Float responsiveness = segment.GetResponsiveness();

	if ( dynParam && dynParam->GetSpeedOverride() > 0.f )
	{
		// Special hack for dialog designers
		Float factor = segment.m_num == 0 ? 2.f : 3.5f;
		responsiveness = factor * dynParam->GetSpeedOverride();
	}
	else if ( dynParam && dynParam->GetSpeed() > 0.f )
	{
		responsiveness *= dynParam->GetSpeed();
	}

	if ( contextParam.m_speed > 0.f )
	{
		// Special hack for dialog designers
		Float factor = segment.m_num == 0 ? 2.f : 3.5f;
		responsiveness = factor * contextParam.m_speed;
	}

	if ( staticParam )
	{
		responsiveness *= staticParam->GetResponsiveness();
	}

	// Interpolate
	const Float w = Clamp( dt * responsiveness, 0.f, 1.f );
	angleH = Interpolate( angleH, DEG2RAD( hAngle ), w );
	angleV = Interpolate( angleV, DEG2RAD( vAngle ), w );
}

ELookAtLevel CBehaviorGraphPoseConstraintPoseLookAtNode::GetLevel( const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam ) const
{
	ELookAtLevel level = LL_Null;

	if ( dynParam )
	{
		level = dynParam->GetLevel();
	}

	if ( staticParam )
	{
		level = Max( staticParam->GetLevel(), level );
	}

	return level;
}

Bool CBehaviorGraphPoseConstraintPoseLookAtNode::GetLookAtParams( CBehaviorGraphInstance& instance, const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const
{
	const CEntity* entity = instance[ i_owner ];

	const CActor* actor = Cast< const CActor >( entity );
	if ( actor )
	{
		actor->GetLookAtParams( dynamicParam, staticParam, contextParam );

		return true;
	}
	else
	{
		const CBgNpc* npc = Cast< const CBgNpc >( entity );
		{
			if ( npc )
			{
				npc->GetLookAtParams( dynamicParam, staticParam, contextParam );

				return true;
			}
		}
	}

	return false;
}

void CBehaviorGraphPoseConstraintPoseLookAtNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( m_generateEditorFragments )
	{
		const Vector& targetWS = GetTargetPos( instance );

		Color color = instance[ i_isLimit ] ? Color::RED : Color::YELLOW;

		frame->AddDebugSphere( targetWS, 0.01f, Matrix::IDENTITY, color );
		frame->AddDebugSphere( targetWS, 0.1f, Matrix::IDENTITY, color );
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphPoseConstraintPoseLookAtNode::IsValid( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_valid ];
}

Bool CBehaviorGraphPoseConstraintPoseLookAtNode::CreateSegments( CBehaviorGraphInstance& instance ) const
{
	TDynArray< SPoseLookAtSegment >& segments = instance[ i_segments ];

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	if ( skeleton )
	{
		for ( Uint32 i=0; i<m_dataSegments.Size(); ++i )
		{
			const SPoseLookAtSegmentData& seg = m_dataSegments[ i ];

			if ( !seg.m_boneNameFirst.Empty() && !seg.m_boneNameLast.Empty() )
			{
				segments.PushBack( SPoseLookAtSegment( i, &seg, skeleton ) );
			}
		}
	}

	for ( Uint32 i=0; i<segments.Size(); ++i )
	{
		if ( !segments[ i ].IsValid() )
		{
			return false;
		}
	}

	return !segments.Empty();
}

Float CBehaviorGraphPoseConstraintPoseLookAtNode::CalcAngleThreshold( Float angle, Float thresholdAngleDifference ) const
{
	return Max( 0.f, Abs( angle ) - thresholdAngleDifference ) * MSign( angle );
	//return ( Abs( angle ) >= thresholdAngleDifference ) ? angle : 0.f;
}

Float CBehaviorGraphPoseConstraintPoseLookAtNode::DampAngle( Float angle, Float angleThr, Float maxAngleDifference, Float bendingMultiplier ) const
{
	return Max( Abs( angleThr ) * Abs( bendingMultiplier ), Abs( angle ) - maxAngleDifference ) * MSign( angle ) * MSign( bendingMultiplier );
	//return ( ( Abs( angleThr ) > maxAngleDifference ) ? maxAngleDifference * MSign( angle ) : angleThr ) * bendingMultiplier;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SPoseLookAtSegmentDampData );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseConstraintPoseCurveLookAtNode );

SPoseLookAtSegmentDampData::SPoseLookAtSegmentDampData()
{
}

CBehaviorGraphPoseConstraintPoseCurveLookAtNode::CBehaviorGraphPoseConstraintPoseCurveLookAtNode()
	: m_useCurve( true )
	, m_curve( NULL )
	, m_doubleDamp( true )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphPoseConstraintPoseCurveLookAtNode::GetCaption() const
{
	return TXT("Pose curve look at");
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_curve = CreateObject< CCurve >( this );
}

#endif

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_segmentsDampData;
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	CreateSegmentsDampData( instance );
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::InternalReset( CBehaviorGraphInstance& instance ) const
{

}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::CreateSegmentsDampData( CBehaviorGraphInstance& instance ) const
{
	TDynArray< SPoseLookAtSegmentDampData >& dampData = instance[ i_segmentsDampData ];
	dampData.Resize( instance[ i_segments ].Size() );
}

void CBehaviorGraphPoseConstraintPoseCurveLookAtNode::CalcAnglesForSegment( CBehaviorGraphInstance& instance, 
																			Float hAngle, Float vAngle, 
																			const CLookAtStaticParam* staticParam, const CLookAtDynamicParam* dynParam, CLookAtContextParam& contextParam,
																			Bool isTargetChanged, Bool instant,
																			SPoseLookAtSegment& segment ) const
{
	if ( !m_useCurve )
	{
		TBaseClass::CalcAnglesForSegment( instance, hAngle, vAngle, staticParam, dynParam, contextParam, isTargetChanged, instant, segment );
		return;
	}

	const Float dt = GetTimeDelta( instance );

	if ( isTargetChanged )
	{
		// Reset progress
		segment.m_progress = 0.f;
		segment.m_hPrevAngle = segment.m_hAngle;
		segment.m_vPrevAngle = segment.m_vAngle;
	}

	Float speed = segment.GetResponsiveness();
	if ( dynParam && dynParam->GetSpeedOverride() > 0.f )
	{
		// Special hack for dialog designers
		Float factor = segment.m_num == 0 ? 2.f : 3.5f;
		speed = factor * dynParam->GetSpeedOverride();
	}
	else if ( dynParam && dynParam->GetSpeed() > 0.f )
	{
		speed *= dynParam->GetSpeed();
	}
		
	if ( contextParam.m_speed > 0.f )
	{
		Float factor = segment.m_num == 0 ? 2.f : 3.5f;
		speed = factor * contextParam.m_speed;
	}
	if ( staticParam )
	{
		speed *= staticParam->GetResponsiveness();
	}

	segment.m_progress = Clamp( segment.m_progress + dt * speed, 0.f, 1.f );

	Float progress = Clamp( m_curve->GetFloatValue( segment.m_progress ), 0.f, 1.f );

	Float hDampAngle = Interpolate( RAD2DEG( segment.m_hPrevAngle ), hAngle, progress );
	Float vDampAngle = Interpolate( RAD2DEG( segment.m_vPrevAngle ), vAngle, progress );

	if ( m_doubleDamp )
	{
		TBaseClass::CalcAnglesForSegment( instance, hDampAngle, vDampAngle, staticParam, dynParam, contextParam, isTargetChanged, instant, segment );
	}
}

#ifdef DENUG_POSE_LOOK_AT
#pragma optimize("",on)
#endif
