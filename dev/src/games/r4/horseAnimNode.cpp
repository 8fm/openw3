
#include "build.h"

#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphValueNode.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/behaviorGraphSocket.h"

#include "horseAnimNode.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphHorseNode );
IMPLEMENT_ENGINE_CLASS( SHorseStateOffsets );

RED_DEFINE_STATIC_NAME( fY )
RED_DEFINE_STATIC_NAME( fZ )
RED_DEFINE_STATIC_NAME( bY )
RED_DEFINE_STATIC_NAME( bZ )

CBehaviorGraphHorseNode::CBehaviorGraphHorseNode()
	: m_firstBoneF( TXT("arm") )
	, m_secondBoneF( TXT("forearm") )
	, m_thirdBoneF( TXT("hand") )
	, m_endBoneF( TXT("f_hoof") )
	, m_hingeAxisF( A_NX )
	, m_firstBoneB( TXT("thigh") )
	, m_secondBoneB( TXT("shin") )
	, m_thirdBoneB( TXT("foot") )
	, m_endBoneB( TXT("b_hoof") )
	, m_hingeAxisB( A_X )
	, m_root( TXT("Root") )
	, m_pelvis( TXT("pelvis") )
	, m_headFirst( TXT("neck1") )
	, m_headSecond( TXT("neck2") )
	, m_headThird( TXT("head") )
	, m_speedStep( 0.5f )
	, m_hingeAxisHead( A_Z )
{

}

void CBehaviorGraphHorseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_slopeFB;
	compiler << i_slopeLR;

	compiler << i_speedWeight;

	compiler << i_rootIdx;
	compiler << i_pelvisIdx;

	compiler << i_headFirstIdx;
	compiler << i_headSecondIdx;
	compiler << i_headThirdIdx;

	compiler << i_bones;
	compiler << i_variables;
}

void CBehaviorGraphHorseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	/*
	instance[ i_slopeFB ] = 0.f;
	instance[ i_slopeLR ] = 0.f;

	instance[ i_speedWeight ] = 0;

	instance[ i_rootIdx ] = FindBoneIndex( m_root, instance );
	instance[ i_pelvisIdx ] = FindBoneIndex( m_pelvis, instance );

	instance[ i_headFirstIdx ] = FindBoneIndex( m_headFirst, instance );
	instance[ i_headSecondIdx ] = FindBoneIndex( m_headSecond, instance );
	instance[ i_headThirdIdx ] = FindBoneIndex( m_headThird, instance );

	if ( instance[ i_headSecondIdx ] == -1 || instance[ i_headThirdIdx ] == -1 )
	{
		instance[ i_headFirstIdx ] = -1;
	}

	TDynArray< Int32 >& bones = instance[ i_bones ];
	bones.Resize( 4 * HL_Last );

	COMPILE_ASSERT( 4 * HL_Last > 15 );

	bones[ 0 ] = FindBoneIndex( TXT("l_") + m_firstBoneF, instance );
	bones[ 4 ] = FindBoneIndex( TXT("r_") + m_firstBoneF, instance );
	bones[ 8 ] = FindBoneIndex( TXT("l_") + m_firstBoneB, instance );
	bones[ 12 ] = FindBoneIndex( TXT("r_") + m_firstBoneB, instance );

	bones[ 1 ] = FindBoneIndex( TXT("l_") + m_secondBoneF, instance );
	bones[ 5 ] = FindBoneIndex( TXT("r_") + m_secondBoneF, instance );
	bones[ 9 ] = FindBoneIndex( TXT("l_") + m_secondBoneB, instance );
	bones[ 13 ] = FindBoneIndex( TXT("r_") + m_secondBoneB, instance );

	bones[ 2 ] = FindBoneIndex( TXT("l_") + m_thirdBoneF, instance );
	bones[ 6 ] = FindBoneIndex( TXT("r_") + m_thirdBoneF, instance );
	bones[ 10 ] = FindBoneIndex( TXT("l_") + m_thirdBoneB, instance );
	bones[ 14 ] = FindBoneIndex( TXT("r_") + m_thirdBoneB, instance );

	bones[ 3 ] = FindBoneIndex( TXT("l_") + m_endBoneF, instance );
	bones[ 7 ] = FindBoneIndex( TXT("r_") + m_endBoneF, instance );
	bones[ 11 ] = FindBoneIndex( TXT("l_") + m_endBoneB, instance );
	bones[ 15 ] = FindBoneIndex( TXT("r_") + m_endBoneB, instance );

	TDynArray< Int32 >& vars = instance[ i_variables ];
	//vars.Resize( 2 * HL_Last );

	//vars[ HL_FL ] = instance.GetFloatValueId( )*/
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphHorseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ) ) );
}

void CBehaviorGraphHorseNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{

}

void CBehaviorGraphHorseNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	{
		m_walkFBP.m_maxValue = 30.f;
		m_walkFBP.m_speedValue = 0.5f;

		m_walkFBP.m_legFY = 0.0015f;
		m_walkFBP.m_legFZ = -0.2762f;

		m_walkFBP.m_legBY = 0.0015f;
		m_walkFBP.m_legBZ = -0.2762f;

		m_walkFBP.m_pelvisY = -0.19f;
		m_walkFBP.m_pelvisZ = -0.0684f;

		m_walkFBP.m_headFirstAngle = 13.f;
		m_walkFBP.m_headSecondAngle = 13.f;
		m_walkFBP.m_headThirdAngle = 5.f;
	}

	{
		m_walkFBN.m_maxValue = -30.f;
		m_walkFBN.m_speedValue = 0.5f;

		m_walkFBN.m_legFY = -0.0068f;
		m_walkFBN.m_legFZ = 0.4924f;

		m_walkFBN.m_legBY = -0.0141f;
		m_walkFBN.m_legBZ = 0.3746f;

		m_walkFBN.m_pelvisY = -0.1988f;
		m_walkFBN.m_pelvisZ = -0.0684f;

		m_walkFBN.m_headFirstAngle = -12.f;
		m_walkFBN.m_headSecondAngle = -19.f;
		m_walkFBN.m_headThirdAngle = 26.f;
	}
}

#endif

void CBehaviorGraphHorseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_slopeFB ] = instance.GetFloatValue( m_slopeFBVar );
	instance[ i_slopeLR ] = instance.GetFloatValue( m_slopeLRVar );

	if ( m_cachedSpeedValueNode )
	{
		Float& speedWeight = instance[ i_speedWeight ];

		m_cachedSpeedValueNode->Update( context, instance, timeDelta );

		speedWeight = m_cachedSpeedValueNode->GetValue( instance );
	}
}

void CBehaviorGraphHorseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	//----------------------------------------------------------------
	// Hack
	/*const CEntity* horse = instance.GetAnimatedComponent()->GetEntity();
	if ( horse->GetTransformParent() )
	{
		CPlayer* rider = static_cast< CPlayer* >( horse->GetTransformParent()->GetParent() );
		//rider->Hack_ApplyHorseMovement( output.m_deltaReferenceFrameLocal );
		output.m_deltaReferenceFrameLocal.setIdentity();
	}*/
	//----------------------------------------------------------------

	const Int32 rootIndex = instance[ i_rootIdx ];
	const Int32 pelvisIndex = instance[ i_pelvisIdx ];
	if ( pelvisIndex != -1 && rootIndex != -1 )
	{
		const Float slopeFB = instance[ i_slopeFB ];
		const Float slopeLR = instance[ i_slopeLR ];
		const Float speedWeight = instance[ i_speedWeight ];

		Float weightFB = 0.f;
		Float weightLR = 0.f;

		const SHorseStateOffsets& stateFB = FindOffsetStateFB( slopeFB, speedWeight, weightFB );
		const SHorseStateOffsets& stateLR = FindOffsetStateLR( slopeLR, speedWeight, weightLR );

		//ASSERT( weightFB >= 0.f && weightFB <= 1.f );
		//ASSERT( weightLR >= 0.f && weightLR <= 1.f );

		weightFB = Clamp( weightFB, 0.f, 1.f );
		weightLR = Clamp( weightLR, 0.f, 1.f );

		// 1.f Pelvis
		AnimQsTransform& pelvis = output.m_outputPose[ pelvisIndex ];
		TranslatePelvis( pelvis, stateFB, weightFB );

		// 2. Legs 
		ProcessLegsIK( instance, output, stateFB, weightFB, stateLR, weightLR );

		// 3. Head
		const Int32 headFirstIndex = instance[ i_headFirstIdx ];
		const Int32 headSecondIndex = instance[ i_headSecondIdx ];
		const Int32 headThirdIndex = instance[ i_headThirdIdx ];
		if ( headFirstIndex != -1 )
		{
			AnimQsTransform& h1 = output.m_outputPose[ headFirstIndex ];
			AnimQsTransform& h2 = output.m_outputPose[ headSecondIndex ];
			AnimQsTransform& h3 = output.m_outputPose[ headThirdIndex ];
			RotateHead( h1, h2, h3, stateFB, weightFB );
		}

		// 4. Root
		AnimQsTransform& root = output.m_outputPose[ rootIndex ];
		RotateRoot( root, stateFB, weightFB, stateLR, weightLR );
	}
}

void CBehaviorGraphHorseNode::RotateRoot( AnimQsTransform& bone, const SHorseStateOffsets& stateFB, Float weightFB, const SHorseStateOffsets& stateLR, Float weightLR ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkVector4 axisFB = BehaviorUtils::hkVectorFromAxis( m_axisRootFB );
	const hkVector4 axisLR = BehaviorUtils::hkVectorFromAxis( m_axisRootLR );

	hkQsTransform offsetFB( hkQsTransform::IDENTITY );
	offsetFB.m_rotation.setAxisAngle( axisFB, DEG2RAD( stateFB.m_maxValue ) * weightFB );

	//hkQsTransform offsetLR( hkQsTransform::IDENTITY );
	//offsetLR.m_rotation.setAxisAngle( axisLR, DEG2RAD( stateLR.m_maxValue ) * weightLR );

	bone.setMul( bone, offsetFB );
	//bone.setMul( bone, offsetLR );
#else
	const RedVector4 axisFB = BehaviorUtils::RedVectorFromAxis( m_axisRootFB );
	const RedVector4 axisLR = BehaviorUtils::RedVectorFromAxis( m_axisRootLR );

	RedQsTransform offsetFB( RedQsTransform::IDENTITY );
	offsetFB.Rotation.SetAxisAngle( axisFB, DEG2RAD( stateFB.m_maxValue ) * weightFB );

	//hkQsTransform offsetLR( hkQsTransform::IDENTITY );
	//offsetLR.m_rotation.setAxisAngle( axisLR, DEG2RAD( stateLR.m_maxValue ) * weightLR );

	bone.SetMul( bone, offsetFB );
	//bone.setMul( bone, offsetLR );
#endif
}

void CBehaviorGraphHorseNode::TranslatePelvis( AnimQsTransform& bone, const SHorseStateOffsets& state, Float weight ) const
{
	weight *= MSign( state.m_maxValue );

	AnimQsTransform offset( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
	offset.m_translation.set( 0.f, state.m_pelvisY * weight, state.m_pelvisZ * weight );
	bone.setMul( bone, offset );
#else
	offset.Translation.Set( 0.0f, state.m_pelvisY * weight, state.m_pelvisZ * weight, 0.0f );
	bone.SetMul( bone, offset );
#endif
}

void CBehaviorGraphHorseNode::RotateHead( AnimQsTransform& h1, AnimQsTransform& h2, AnimQsTransform& h3, const SHorseStateOffsets& state, Float weight ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkVector4 axis = BehaviorUtils::hkVectorFromAxis( m_hingeAxisHead );

	hkQsTransform offset1( hkQsTransform::IDENTITY );
	offset1.m_rotation.setAxisAngle( axis, DEG2RAD(state.m_headFirstAngle) * weight );

	hkQsTransform offset2( hkQsTransform::IDENTITY );
	offset2.m_rotation.setAxisAngle( axis, DEG2RAD(state.m_headSecondAngle) * weight );

	hkQsTransform offset3( hkQsTransform::IDENTITY );
	offset3.m_rotation.setAxisAngle( axis, DEG2RAD(state.m_headThirdAngle) * weight );

	h1.setMul( h1, offset1 );
	h2.setMul( h2, offset2 );
	h3.setMul( h3, offset3 );
#else
	const RedVector4 axis = BehaviorUtils::RedVectorFromAxis( m_hingeAxisHead );

	RedQsTransform offset1( RedQsTransform::IDENTITY );
	offset1.Rotation.SetAxisAngle( axis, DEG2RAD(state.m_headFirstAngle) * weight );

	RedQsTransform offset2( RedQsTransform::IDENTITY );
	offset2.Rotation.SetAxisAngle( axis, DEG2RAD(state.m_headSecondAngle) * weight );

	RedQsTransform offset3( RedQsTransform::IDENTITY );
	offset3.Rotation.SetAxisAngle( axis, DEG2RAD(state.m_headThirdAngle) * weight );

	h1.SetMul( h1, offset1 );
	h2.SetMul( h2, offset2 );
	h3.SetMul( h3, offset3 );
#endif
}

void CBehaviorGraphHorseNode::ProcessLegsIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SHorseStateOffsets& stateFB, Float weightFB, const SHorseStateOffsets& stateLR, Float weightLR ) const
{
	const TDynArray< Int32 >& bones = instance[ i_bones ];

	if ( bones.Size() < 16 )
	{
		return;
	}

	Float fY = stateFB.m_legFY * weightFB;
	Float fZ = stateFB.m_legFZ * weightFB;

	Float bY = stateFB.m_legBY * weightFB;
	Float bZ = stateFB.m_legBZ * weightFB;

	fY = instance.GetFloatValue( CNAME( fY ) );
	fZ = instance.GetFloatValue( CNAME( fZ ) );
	bY = instance.GetFloatValue( CNAME( bY ) );
	bZ = instance.GetFloatValue( CNAME( bZ ) );

	ProcessLegIK( output, fY, fZ, bones[ 0 ], bones[ 1 ], bones[ 2 ], bones[ 3 ] );
	ProcessLegIK( output, fY, fZ, bones[ 4 ], bones[ 5 ], bones[ 6 ], bones[ 7 ] );
	
	ProcessLegIK( output, bY, bZ, bones[ 8 ], bones[ 9 ], bones[ 10 ], bones[ 11 ] );
	ProcessLegIK( output, bY, bZ, bones[ 12 ], bones[ 13 ], bones[ 14 ], bones[ 15 ] );
}

void CBehaviorGraphHorseNode::ProcessLegIK( SBehaviorGraphOutput &output, Float y, Float z, Int32 boneA, Int32 boneB, Int32 boneC, Int32 boneD ) const
{
	/*if ( boneA == -1 || boneB == -1 || boneC == -1 || boneD == -1 )
	{
		return;
	}

	HorseIkSolver::Setup setup;

	setup.m_boneA = output.m_outputPose[ boneA ];
	setup.m_boneB = output.m_outputPose[ boneB ];
	setup.m_boneC = output.m_outputPose[ boneC ];
	setup.m_boneD = output.m_outputPose[ boneD ];

	setup.m_offsetX = 0.f;
	setup.m_offsetY = z;
	setup.m_offsetZ = y;

	HorseIkSolver::Solve( setup );

	output.m_outputPose[ boneA ] = setup.m_boneA;
	output.m_outputPose[ boneB ] = setup.m_boneB;
	output.m_outputPose[ boneC ] = setup.m_boneC;
	output.m_outputPose[ boneD ] = setup.m_boneD;*/
}

const SHorseStateOffsets& CBehaviorGraphHorseNode::FindOffsetState( Float slope, Float speedWeight, Float& weight, 
	const SHorseStateOffsets& canter, const SHorseStateOffsets& gallop, const SHorseStateOffsets& trot, const SHorseStateOffsets& walk ) const
{
	if ( speedWeight > canter.m_speedValue )
	{
		weight = CalcWeightForState( canter, slope, speedWeight );
		return canter;
	}
	else if ( speedWeight > gallop.m_speedValue )
	{
		weight = CalcWeightForState( gallop, slope, speedWeight );
		return gallop;
	}
	else if ( speedWeight > trot.m_speedValue )
	{
		weight = CalcWeightForState( trot, slope, speedWeight );
		return trot;
	}
	else
	{
		weight = CalcWeightForState( walk, slope, speedWeight );
		return walk;
	}
}

const SHorseStateOffsets& CBehaviorGraphHorseNode::FindOffsetStateFB( Float slope, Float speedWeight, Float& weight ) const
{
	if ( slope >= 0.f )
	{
		return FindOffsetState( slope, speedWeight, weight, m_canterFBP, m_gallopFBP, m_trotFBP, m_walkFBP );
	}
	else
	{
		return FindOffsetState( slope, speedWeight, weight, m_canterFBN, m_gallopFBN, m_trotFBN, m_walkFBN );
	}
}

const SHorseStateOffsets& CBehaviorGraphHorseNode::FindOffsetStateLR( Float slope, Float speedWeight, Float& weight ) const
{
	return FindOffsetState( slope, speedWeight, weight, m_canterLR, m_gallopLR, m_trotLR, m_walkLR );
}

Float CBehaviorGraphHorseNode::CalcWeightForState( const SHorseStateOffsets& step, Float slope, Float speedWeight ) const
{
	//const Float speedProp = ( speedWeight - step.m_speedValue ) / m_speedStep;
	//ASSERT( speedProp >= 0.f && speedProp <= 1.f );

	const Float slopeProp = Clamp( slope / step.m_maxValue, -1.f, 1.f );

	return slopeProp;
}

void CBehaviorGraphHorseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedSpeedValueNode )
	{
		m_cachedSpeedValueNode->Activate( instance );
	}
}

void CBehaviorGraphHorseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedSpeedValueNode )
	{
		m_cachedSpeedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphHorseNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedSpeedValueNode = CacheValueBlock( TXT("Speed") );
}
