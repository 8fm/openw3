/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNodeRoll.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphValueNode.h"
#include "cacheBehaviorGraphOutput.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../engine/renderFrame.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeRoll );

CBehaviorGraphConstraintNodeRoll::CBehaviorGraphConstraintNodeRoll()
	: m_weight( 1.0f )
{

}

void CBehaviorGraphConstraintNodeRoll::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_boneIndex;
}

void CBehaviorGraphConstraintNodeRoll::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_controlValue ] = m_weight;
	instance[ i_boneIndex ].Clear();

	CacheBoneIndex( instance );
}

void CBehaviorGraphConstraintNodeRoll::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_boneIndex );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintNodeRoll::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphConstraintNodeRoll::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNodeRoll::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Roll deformation - %s"), m_name.AsChar() );
	else
		return String( TXT("Roll deformation") );;
}

#endif

void CBehaviorGraphConstraintNodeRoll::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( m_generateEditorFragments && animatedComponent && CheckBones( instance ) )
	{
		const TDynArray< Int32 >& boneIndex = instance[ i_boneIndex ];

		// Disp axis for roll bones

		// Left
		DisplayAxisForBone( boneIndex[ BI_Hand_Left ],			animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_HandRoll_Left ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ElbowRoll_Left ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ForearmRoll1_Left ],	animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ForearmRoll2_Left ],	animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_Bicep2_Left ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ShoulderRoll_Left ],	animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_KneeRoll_Left ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_LegRoll2_Left ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_LegRoll_Left ],		animatedComponent, frame );

		// Right
		DisplayAxisForBone( boneIndex[ BI_Hand_Right ],			animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_HandRoll_Right ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ElbowRoll_Right ],	animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ForearmRoll1_Right ], animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ForearmRoll2_Right ], animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_Bicep2_Right ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_ShoulderRoll_Right ], animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_KneeRoll_Right ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_LegRoll2_Right ],		animatedComponent, frame );
		DisplayAxisForBone( boneIndex[ BI_LegRoll_Right ],		animatedComponent, frame );
	}
}

void CBehaviorGraphConstraintNodeRoll::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphConstraintNodeRoll::OnReset( CBehaviorGraphInstance& /*instance*/ ) const
{
}

void CBehaviorGraphConstraintNodeRoll::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintNodeRoll::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphConstraintNodeRoll::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Roll );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update control value
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		instance[ i_controlValue ] = m_cachedControlValueNode->GetValue( instance );
	}
	else
	{
		instance[ i_controlValue ] = m_weight;
	}

	// Clamp value
	instance[ i_controlValue ] = ( instance[ i_controlValue ] > 0.5 ) ? 1.0f : 0.0f;
}

void CBehaviorGraphConstraintNodeRoll::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	BEH_NODE_SAMPLE( Roll );

	TBaseClass::Sample( context, instance, output );

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const TDynArray< Int32 >& boneIndex = instance[ i_boneIndex ];

	if ( instance[ i_controlValue ] > 0.5f && CheckBones( instance ) )
	{
		// Shoulder Roll
		Float shoulderRollWeightLeft = CalcShoulderRollWeight( boneIndex, output, BI_Bicep_Left, 1.f );
		Float shoulderRollWeightRight = CalcShoulderRollWeight( boneIndex, output, BI_Bicep_Right, -1.f );

		SetBoneRotation( boneIndex, ac, output, BI_ShoulderRoll_Left, BI_Bicep_Left, BI_Shoulder_Left, shoulderRollWeightLeft );
		SetBoneRotation( boneIndex, ac, output, BI_ShoulderRoll_Right, BI_Bicep_Right, BI_Shoulder_Right, shoulderRollWeightRight );

		// Bicep 2
		SetBoneRoll( boneIndex, output, BI_Bicep2_Left, BI_Bicep_Left, -0.65f );
		SetBoneRoll( boneIndex, output, BI_Bicep2_Right, BI_Bicep_Right, -0.65f );

		// Elbow Roll
		SetBoneRotationWithOffset( boneIndex, ac, output, BI_ElbowRoll_Left, BI_Bicep_Left, BI_Forearm_Left, 90,  0.5f );
		SetBoneRotationWithOffset( boneIndex, ac, output, BI_ElbowRoll_Right, BI_Bicep_Right, BI_Forearm_Right, -90, 0.5f );

		// Forearm 1
		SetBoneRoll( boneIndex, output, BI_ForearmRoll1_Left, BI_Hand_Left, 0.3f );
		SetBoneRoll( boneIndex, output, BI_ForearmRoll1_Right, BI_Hand_Right, 0.3f );

		// Forearm 2
		SetBoneRoll( boneIndex, output, BI_ForearmRoll2_Left, BI_Hand_Left, 0.6f );
		SetBoneRoll( boneIndex, output, BI_ForearmRoll2_Right, BI_Hand_Right, 0.6f );

		// Hand Roll
		SetBoneRotation( boneIndex, ac, output, BI_HandRoll_Left, BI_ForearmRoll2_Left, BI_Hand_Left, 0.5f );
		SetBoneRotation( boneIndex, ac, output, BI_HandRoll_Right, BI_ForearmRoll2_Right, BI_Hand_Right, 0.5f );

		// Leg Roll
		SetBoneRotation( boneIndex, ac, output, ac->GetSkeleton(), BI_LegRoll_Left, BI_Thigh_Left, 0.5f );
		SetBoneRotation( boneIndex, ac, output, ac->GetSkeleton(), BI_LegRoll_Right, BI_Thigh_Right, 0.5f );

		// Leg Roll 2
		SetBoneRotationEqualInMS ( boneIndex, ac, output, BI_LegRoll2_Left, BI_LegRoll_Left );
		SetBoneRotationEqualInMS ( boneIndex, ac, output, BI_LegRoll2_Right, BI_LegRoll_Right );

		// Knee Roll
		SetBoneRotation( boneIndex, ac, output, BI_KneeRoll_Left, BI_Thigh_Left, BI_Shin_Left, 0.5f );
		SetBoneRotation( boneIndex, ac, output, BI_KneeRoll_Right, BI_Thigh_Right, BI_Shin_Right, 0.5f );
	}
}

void CBehaviorGraphConstraintNodeRoll::SetBoneRoll( const TDynArray< Int32 >& boneIndex, SBehaviorGraphOutput& pose,
												    EBoneIndex boneOutIdx, EBoneIndex boneInIdx, Float weight ) const
{
	// Get bones transform
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	const hkQsTransform& boneIn = pose.m_outputPose[ boneIndex[ boneInIdx ] ];

	static const hkVector4 xAxis = hkVector4( 1.f, 0.f, 0.f, 0.f );
	hkReal inRollAngle;
	hkQuaternion quatRest;

	// Get input roll angle
	boneIn.getRotation().decomposeRestAxis( xAxis, quatRest, inRollAngle );

	// Set output roll angle
	Float outRollAngle = inRollAngle * weight;

	// Calc final transform
	boneOut.m_rotation.setAxisAngle( xAxis, outRollAngle );
#else
	RedQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	const RedQsTransform& boneIn = pose.m_outputPose[ boneIndex[ boneInIdx ] ];

	static const RedVector4 xAxis = RedVector4( 1.0f, 0.0f, 0.0f, 0.0f );
	float inRollAngle;
	RedQuaternion quatRest;

	// Get input roll angle
	boneIn.GetRotation().DecomposeRestAxis( xAxis, quatRest, inRollAngle );

	// Set output roll angle
	Float outRollAngle = inRollAngle * weight;

	// Calc final transform
	boneOut.Rotation.SetAxisAngle( xAxis, outRollAngle );
#endif
}

void CBehaviorGraphConstraintNodeRoll::SetBoneRotationWithOffset( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, 
																  SBehaviorGraphOutput& pose, EBoneIndex boneOutIdx, 
																  EBoneIndex boneAIdx, 
																  EBoneIndex boneBIdx, Float angleOffset, 
																  Float weight ) const
{
	Float halfAngle = DEG2RAD( angleOffset )/2.f;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion rotationQuat = hkQuaternion( sinf( halfAngle ), 0, 0, cosf( halfAngle ) );
	const hkQsTransform offset( hkVector4( 0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	const hkaSkeleton* havokSkeleton = animatedComponent->GetSkeleton()->GetHavokSkeleton();

	const hkQsTransform parentBoneMS = pose.GetBoneModelTransform(	havokSkeleton->m_parentIndices[ boneIndex[ boneOutIdx ] ],
																	havokSkeleton->m_parentIndices );
	

	const hkQsTransform boneAMS = pose.GetBoneModelTransform(	boneIndex[ boneAIdx ],
																havokSkeleton->m_parentIndices );

	hkQsTransform boneBMS = pose.GetBoneModelTransform( boneIndex[ boneBIdx ],
														havokSkeleton->m_parentIndices );

	// Add offset
	boneBMS.setMul( boneBMS, offset );

	// Check quaternion polarity
	Float signedWeight = ( boneAMS.m_rotation.m_vec.dot4( boneBMS.m_rotation.m_vec ) < 0.0f ) ? -weight : weight;

	// Rotation in model space
	hkQuaternion out;
	out.m_vec.setMul4( signedWeight, boneBMS.m_rotation.m_vec );
	out.m_vec.addMul4( 1.0f - weight, boneAMS.m_rotation.m_vec );
	out.normalize();

	hkQsTransform parentBoneMSInv;
	parentBoneMSInv.setInverse( parentBoneMS );

	hkQsTransform boneMS;
	boneMS.setMul( parentBoneMS, pose.m_outputPose[ boneIndex[ boneOutIdx ] ] );

	boneMS.m_rotation = out;

	hkQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.setMul( parentBoneMSInv, boneMS );
#else
	const CSkeleton* skeleton = animatedComponent->GetSkeleton();
	
	RedQuaternion rotationQuat = RedQuaternion( sinf( halfAngle ), 0, 0, cosf( halfAngle ) );
	const RedQsTransform offset( RedVector4::ZEROS, rotationQuat, RedVector4::ONES );

	const RedQsTransform parentBoneMS = pose.GetBoneModelTransform(	skeleton->GetParentBoneIndex( boneIndex[ boneOutIdx ] ),
		skeleton->GetParentIndices() );
	
	const RedQsTransform boneAMS = pose.GetBoneModelTransform( boneIndex[ boneAIdx ],
		skeleton->GetParentIndices() );

	RedQsTransform boneBMS = pose.GetBoneModelTransform( boneIndex[ boneBIdx ],
		skeleton->GetParentIndices() );

	// Add offset
	boneBMS.SetMul( boneBMS, offset );

	// Check quaternion polarity
	const Float signedWeight = ( Dot( boneAMS.Rotation.Quat, boneBMS.Rotation.Quat ) < 0.0f ) ? -weight : weight;

	// Rotation in model space
	RedQuaternion out;
	out.Quat = Mul( boneBMS.Rotation.Quat, signedWeight );
	SetAdd( out.Quat, Mul( boneAMS.Rotation.Quat, 1.0f - weight ) );
	out.Normalize();

	RedQsTransform parentBoneMSInv;
	parentBoneMSInv.SetInverse( parentBoneMS );

	RedQsTransform boneMS;
	boneMS.SetMul( parentBoneMS, pose.m_outputPose[ boneIndex[ boneOutIdx ] ] );

	boneMS.Rotation = out;

	RedQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.SetMul( parentBoneMSInv, boneMS );
#endif
}

void CBehaviorGraphConstraintNodeRoll::SetBoneRotation( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, 
													    SBehaviorGraphOutput& pose, EBoneIndex boneOutIdx, 
													    EBoneIndex boneAIdx, EBoneIndex boneBIdx, Float weight ) const
{
	const CSkeleton* skeleton = animatedComponent->GetSkeleton();

	const AnimQsTransform boneAMS = pose.GetBoneModelTransform( boneIndex[ boneAIdx ],
															skeleton->GetParentIndices() );

	const AnimQsTransform boneBMS = pose.GetBoneModelTransform( boneIndex[ boneBIdx ],
															skeleton->GetParentIndices() );

	const AnimQsTransform parentBoneMS = pose.GetBoneModelTransform( skeleton->GetParentBoneIndex( boneIndex[ boneOutIdx ] ),
															skeleton->GetParentIndices() );

#ifdef USE_HAVOK_ANIMATION
	// Check quaternion polarity
	Float signedWeight = ( boneAMS.m_rotation.m_vec.dot4( boneBMS.m_rotation.m_vec ) < 0.0f ) ? -weight : weight;

	// Rotation in model space
	hkQuaternion out;
	out.m_vec.setMul4( signedWeight, boneBMS.m_rotation.m_vec );
	out.m_vec.addMul4( 1.0f - weight, boneAMS.m_rotation.m_vec );
	out.normalize();

	hkQsTransform parentBoneMSInv;
	parentBoneMSInv.setInverse( parentBoneMS );

	hkQsTransform boneMS;
	boneMS.setMul( parentBoneMS, pose.m_outputPose[ boneIndex[ boneOutIdx ] ] );

	boneMS.m_rotation = out;
	
	hkQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.setMul( parentBoneMSInv, boneMS );
#else
	// Check quaternion polarity
	const Float signedWeight = ( Dot( boneAMS.Rotation.Quat, boneBMS.Rotation.Quat ) < 0.0f ) ? -weight : weight;

	// Rotation in model space
	RedQuaternion out;
	out.Quat = Mul( boneBMS.Rotation.Quat, signedWeight );
	SetAdd( out.Quat, Mul( boneAMS.Rotation.Quat, 1.0f - weight ) );
	out.Normalize();

	RedQsTransform parentBoneMSInv;
	parentBoneMSInv.SetInverse( parentBoneMS );

	RedQsTransform boneMS;
	boneMS.SetMul( parentBoneMS, pose.m_outputPose[ boneIndex[ boneOutIdx ] ] );

	boneMS.Rotation = out;

	RedQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.SetMul( parentBoneMSInv, boneMS );
#endif
}

void CBehaviorGraphConstraintNodeRoll::SetBoneRotation( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* /*animatedComponent*/, 
													    SBehaviorGraphOutput& pose, const CSkeleton* refSkeleton, 
													    EBoneIndex boneOutIdx, EBoneIndex boneInIdx, Float weight ) const
{
#ifdef USE_HAVOK_ANIMATION
	// Get bone transform in local space
	const hkQsTransform& boneALS = refSkeleton->GetReferencePose()[ boneIndex[ boneOutIdx ] ];
	const hkQsTransform& boneBLS = pose.m_outputPose[ boneIndex[ boneInIdx ] ];
	// Check quaternion polarity
	Float signedWeight = ( boneALS.m_rotation.m_vec.dot4( boneBLS.m_rotation.m_vec ) < 0.0f ) ? -weight : weight;

	// Rotation in local space
	hkQuaternion out;
	out.m_vec.setMul4( signedWeight, boneBLS.m_rotation.m_vec );
	out.m_vec.addMul4( 1.0f - weight, boneALS.m_rotation.m_vec );
	out.normalize();

	hkQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.m_rotation = out;

#else
	// Get bone transform in local space	
	const RedQsTransform& boneALS = refSkeleton->GetReferencePoseLS()[ boneIndex[ boneOutIdx ] ];
	const RedQsTransform& boneBLS = pose.m_outputPose[ boneIndex[ boneInIdx ] ];
		// Check quaternion polarity
	Float signedWeight = ( Dot( boneALS.Rotation.Quat, boneBLS.Rotation.Quat ) < 0.0f ) ? -weight : weight;

	// Rotation in local space
	RedQuaternion out;
	out.Quat = Mul( boneBLS.Rotation.Quat, signedWeight );
	SetAdd( out.Quat, Mul( boneALS.Rotation.Quat, 1.0f - weight ) );
	out.Normalize();

	RedQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.Rotation = out;
#endif
}

Float CBehaviorGraphConstraintNodeRoll::CalcShoulderRollWeight( const TDynArray< Int32 >& boneIndex, const SBehaviorGraphOutput& pose, 
															    EBoneIndex bicepBone, Float mulFactor ) const
{
#ifdef USE_HAVOK_ANIMATION
	// Get bicep transform
	const hkQsTransform& bicepLeft = pose.m_outputPose[ boneIndex[ bicepBone ] ];

	// Get Y angle
	static const hkVector4 yAxis = hkVector4( 0.f, 1.f, 0.f, 0.f );
	hkReal angle;
	hkQuaternion quatRest;

	// Get input roll angle
	bicepLeft.getRotation().decomposeRestAxis( yAxis, quatRest, angle );
#else
	// Get bicep transform
	const RedQsTransform& bicepLeft = pose.m_outputPose[ boneIndex[ bicepBone ] ];

	// Get Y angle
	static const RedVector4 yAxis = RedVector4( 0.0f, 1.0f, 0.0f, 0.0f );
	Float angle;
	RedQuaternion quatRest;

	// Get input roll angle
	bicepLeft.GetRotation().DecomposeRestAxis( yAxis, quatRest, angle );
#endif
	// Convert to deg
	Float angleDeg = RAD2DEG( angle ) * mulFactor;

	// Magic formula - biecepWeight
 	Float bicepWeight = angleDeg * (-2.f) + 140.f;
	bicepWeight = ::Clamp< Float >( bicepWeight, 0.f, 100.f );
	bicepWeight /= 100.f;
	
	Float shoulderWeight = 1.f;

	// Calc normalize weight
	Float bicepWeightNorm = bicepWeight / ( bicepWeight + shoulderWeight );
	Float shoulderWeightNorm = shoulderWeight / ( bicepWeight + shoulderWeight );

	ASSERT( bicepWeightNorm + shoulderWeightNorm < 1.01f && bicepWeightNorm + shoulderWeightNorm > 0.01f );

	return shoulderWeightNorm;
}

void CBehaviorGraphConstraintNodeRoll::SetBoneRotationEqualInMS( const TDynArray< Int32 >& boneIndex, const CAnimatedComponent* animatedComponent, 
																 SBehaviorGraphOutput& pose, EBoneIndex boneOutIdx, EBoneIndex boneRefIdx ) const
{
	const CSkeleton* skeleton = animatedComponent->GetSkeleton();

	// Get bones transform in model space
	AnimQsTransform boneOutMS = pose.GetBoneModelTransform(	boneIndex[ boneOutIdx ],
		skeleton->GetParentIndices() );

	const AnimQsTransform boneRefMS = pose.GetBoneModelTransform( boneIndex[ boneRefIdx ],
		skeleton->GetParentIndices() );

	const AnimQsTransform parentBoneOutMS = pose.GetBoneModelTransform(	skeleton->GetParentBoneIndex( boneIndex[ boneOutIdx ] ),
		skeleton->GetParentIndices() );

	AnimQsTransform parentBoneOutMSInv;

#ifdef USE_HAVOK_ANIMATION
	parentBoneOutMSInv.setInverse( parentBoneOutMS );

	// Set equal in model space
	boneOutMS.m_rotation = boneRefMS.m_rotation;

	hkQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.setMul( parentBoneOutMSInv, boneOutMS );
#else
	parentBoneOutMSInv.SetInverse( parentBoneOutMS );

	// Set equal in model space
	boneOutMS.Rotation = boneRefMS.Rotation;

	AnimQsTransform& boneOut = pose.m_outputPose[ boneIndex[ boneOutIdx ] ];
	boneOut.SetMul( parentBoneOutMSInv, boneOutMS );
#endif
}

void CBehaviorGraphConstraintNodeRoll::CacheBoneIndex( CBehaviorGraphInstance& instance ) const
{
	TDynArray< Int32 >& boneIndex = instance[ i_boneIndex ];

	// Clear current index table
	boneIndex.Clear();

	if ( !instance.GetAnimatedComponent() ) return;

	boneIndex.Reserve( BI_Last-1 );

	/* Use bone index EBoneIndex		
		BI_Pelvis,
		BI_Shoulder_Left,			BI_Shoulder_Right,
		BI_Bicep_Left,				BI_Bicep_Right,
		BI_Forearm_Left,			BI_Forearm_Right,
		BI_Hand_Left,				BI_Hand_Right,
		BI_ElbowRoll_Left,			BI_ElbowRoll_Right,
		BI_ForearmRoll1_Left,		BI_ForearmRoll1_Right,
		BI_ForearmRoll2_Left,		BI_ForearmRoll2_Right,
		BI_HandRoll_Left,			BI_HandRoll_Right,
		BI_Bicep2_Left,				BI_Bicep2_Right,
		BI_ShoulderRoll_Left,		BI_ShoulderRoll_Right,
		BI_Thigh_Left,				BI_Thigh_Right,
		BI_Shin_Left,				BI_Shin_Right,
		BI_KneeRoll_Left,			BI_KneeRoll_Right,
		BI_LegRoll2_Left,			BI_LegRoll2_Right,
		BI_LegRoll_Left,			BI_LegRoll_Right,
	*/

	String boneTable[] = { 
		TXT("pelvis"),
		TXT("l_shoulder"),		TXT("r_shoulder"),
		TXT("l_bicep"),			TXT("r_bicep"),
		TXT("l_forearm"),		TXT("r_forearm"),
		TXT("l_hand"),			TXT("r_hand"),
		TXT("l_elbowRoll"),		TXT("r_elbowRoll"),
		TXT("l_forearmRoll1"),	TXT("r_forearmRoll1"),
		TXT("l_forearmRoll2"),	TXT("r_forearmRoll2"),
		TXT("l_handRoll"),		TXT("r_handRoll"),
		TXT("l_bicep2"),		TXT("r_bicep2"),
		TXT("l_shoulderRoll"),	TXT("r_shoulderRoll"),
		TXT("l_thigh"),			TXT("r_thigh"), 
		TXT("l_shin"),			TXT("r_shin"), 
		TXT("l_kneeRoll"),		TXT("r_kneeRoll"),
		TXT("l_legRoll2"),		TXT("r_legRoll2"), 
		TXT("l_legRoll"),		TXT("r_legRoll"), 
		};

		Uint32 boneTableSize = ARRAY_COUNT( boneTable );

		// Fill bone index table
		for ( Uint32 i=0; i<boneTableSize; i++ )
		{
			Int32 index = FindBoneIndex( boneTable[i], instance );

			if ( index == -1 )
			{
				boneIndex.Clear();

				WARN_ENGINE( TXT("CBehaviorGraphConstraintNodeRoll: Couldn't find bone %s. Roll constraint can't work."), boneTable[i].AsChar() );

				return;
			}

			boneIndex.PushBack( index );
		}

		if ( boneIndex.Size() > 0 )
		{
			ASSERT( boneIndex.Size() == BI_Last );
		}
}

Bool CBehaviorGraphConstraintNodeRoll::CheckBones( CBehaviorGraphInstance& instance ) const
{
	return ( instance[ i_boneIndex ].Size() == BI_Last ) ? true : false;
}

void CBehaviorGraphConstraintNodeRoll::DisplayAxisForBone( Uint32 bone, const CAnimatedComponent* animatedComponent, CRenderFrame* frame ) const
{
	Matrix transform = animatedComponent->GetBoneMatrixWorldSpace( bone );
	frame->AddDebugAxis( transform.GetTranslation(), transform, 0.1f, true );
}