/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorConstraintObject.h"
#include "behaviorGraphLookAtSystemNode.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/renderFrame.h"
#include "../engine/curve.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtSystemNode );

const Float CBehaviorGraphLookAtSystemNode::SWITCH_SMOOTH = 0.2f;
const CName CBehaviorGraphLookAtSystemNode::EVT_FORCE_STATE = CNAME( lookAtForce );

CBehaviorGraphLookAtSystemNode::CBehaviorGraphLookAtSystemNode()
	: m_deadZone( 0.f )
	, m_deadZoneDist( 0.f )
	, m_internalDampCurve( NULL )
	, m_limitDampTime( 0.5f )
	, m_levelDampTime( 1.f )
	, m_thirdWeight( 1.0f )
	, m_secondWeight( 0.5f )
	, m_firstWeight( 0.f )
	, m_thirdBone( TXT("head") )
	, m_secondBone( TXT("neck") )
	, m_firstBone( TXT("torso2") )
	, m_dampForFirstTarget( 0.f )
	, m_range( 160.f )
{
	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtSystemNode::OnSpawned(const GraphBlockSpawnInfo& info)
{
	TBaseClass::OnSpawned( info );
	m_internalDampCurve = CreateObject< CCurve >( this );
}

String CBehaviorGraphLookAtSystemNode::GetCaption() const
{
	return TXT("Look at system");
}

void CBehaviorGraphLookAtSystemNode::OnRebuildSockets()
{
	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Level ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Weights ), false ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Data ), false ) );
}

#endif

void CBehaviorGraphLookAtSystemNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedLevelVariableNode = CacheValueBlock( TXT("Level") );
	m_cachedWeightsVariableNode = CacheVectorValueBlock( TXT("Weights") );
	m_cachedCompressedDataVariableNode = CacheVectorValueBlock( TXT("Data") );
}

void CBehaviorGraphLookAtSystemNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_firstBoneIndex;
	compiler << i_secondBoneIndex;
	compiler << i_thirdBoneIndex;
	compiler << i_limitCurrWeight;
	compiler << i_limitDestWeight;
	compiler << i_currWeightFirst;
	compiler << i_destWeightFirst;
	compiler << i_currWeightSecAndThird;
	compiler << i_destWeightSecAndThird;
	compiler << i_weightsScale;
	compiler << i_deadZoneTarget;
	compiler << i_deadZoneSnappedTarget;
	compiler << i_dampedTarget;
	compiler << i_autoLimitDeact;
	compiler << i_limitStartRange;
	compiler << i_evtForceState;
	compiler << i_forceState;
}

void CBehaviorGraphLookAtSystemNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_firstBoneIndex ] = FindBoneIndex( m_firstBone, instance );
	instance[ i_secondBoneIndex ] = FindBoneIndex( m_secondBone, instance );
	instance[ i_thirdBoneIndex ] = FindBoneIndex( m_thirdBone, instance );

	instance[ i_deadZoneTarget ] = Vector::ZERO_3D_POINT;
	instance[ i_deadZoneSnappedTarget ] = Vector::ZERO_3D_POINT;
	instance[ i_dampedTarget ] = Vector::ZERO_3D_POINT;

	instance[ i_evtForceState ] = instance.GetEventId( EVT_FORCE_STATE );
	
	InternalReset( instance );

	ForceActivatedState( instance, false );
}

void CBehaviorGraphLookAtSystemNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_firstBoneIndex );
	INST_PROP( i_secondBoneIndex );
	INST_PROP( i_thirdBoneIndex );

	INST_PROP( i_currWeightFirst );
	INST_PROP( i_destWeightFirst );
	INST_PROP( i_currWeightSecAndThird );
	INST_PROP( i_destWeightSecAndThird );

	INST_PROP( i_limitCurrWeight );
	INST_PROP( i_limitDestWeight );
	INST_PROP( i_autoLimitDeact );

	INST_PROP( i_weightsScale );
}

void CBehaviorGraphLookAtSystemNode::UpdateDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDeadZone() )
	{
		Vector& deadZoneTarget = instance[ i_deadZoneTarget ];
		IConstraintTarget::ApplyDeadZone( GetTargetCurr( instance ).GetPosition(), deadZoneTarget, m_deadZone );

		if ( HasDeadZoneSnapping() )
		{
			Vector& deadZoneSnappedTarget = instance[ i_deadZoneSnappedTarget ];
			Bool limit = IConstraintTarget::ApplyDeadZoneSnapping( deadZoneTarget, deadZoneSnappedTarget, m_deadZoneDist, instance[ i_limitStartRange ] );

			if ( limit )
			{
				SetLimitFlag( instance, true );
			}
		}
	}
	else if ( HasDeadZoneSnapping() )
	{
		Vector& deadZoneSnappedTarget = instance[ i_deadZoneSnappedTarget ];
		Bool limit = IConstraintTarget::ApplyDeadZoneSnapping( GetTargetCurr( instance ).GetPosition(), deadZoneSnappedTarget, m_deadZoneDist, instance[ i_limitStartRange ] );

		if ( limit )
		{
			SetLimitFlag( instance, true );
		}
	}
}

Bool CBehaviorGraphLookAtSystemNode::HasDeadZone() const
{
	return m_deadZone > 0.f;
}

Bool CBehaviorGraphLookAtSystemNode::HasDeadZoneSnapping() const
{
	return m_deadZoneDist > 0.f;
}

void CBehaviorGraphLookAtSystemNode::UpdateDampedTarget( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( HasDampedTarget() )
	{
#ifdef USE_HAVOK_ANIMATION
		hkVector4 targetHk = GetLookAtTarget( instance );

		Vector target = TO_CONST_VECTOR_REF( targetHk );
#else
		RedVector4 targetRed = GetLookAtTarget( instance );

		Vector target = reinterpret_cast< const Vector& >( targetRed );
#endif
		Vector& dampedTarget = instance[ i_dampedTarget ];

		IConstraintTarget::ApplyProgressiveDamp( target, dampedTarget, m_dampForFirstTarget, timeDelta );
	}
}

Bool CBehaviorGraphLookAtSystemNode::HasDampedTarget() const
{
	return m_dampForFirstTarget > 0.f;
}
#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphLookAtSystemNode::GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	hkVector4 hkVec;
	const Vector& vec = instance[ i_deadZoneTarget ];
	hkVec = TO_CONST_HK_VECTOR_REF( vec );
	return hkVec;
}
#else
RedVector4 CBehaviorGraphLookAtSystemNode::GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	RedVector4 RedVec = reinterpret_cast< const RedVector4& >( instance[ i_deadZoneTarget ] );
	return RedVec;
}
#endif
#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphLookAtSystemNode::GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const
{
	hkVector4 hkVec;
	const Vector& vec = instance[ i_deadZoneSnappedTarget ];
	hkVec = TO_CONST_HK_VECTOR_REF( vec );
	return hkVec;
}
#else
RedVector4 CBehaviorGraphLookAtSystemNode::GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const
{
	RedVector4 RedVec = reinterpret_cast< const RedVector4& >( instance[ i_deadZoneSnappedTarget ] );
	return RedVec;
}
#endif
#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphLookAtSystemNode::GetDampedTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDampedTarget() )
	{
		hkVector4 hkVec;
		const Vector& vec = instance[ i_dampedTarget ];
		hkVec = TO_CONST_HK_VECTOR_REF( vec );
		return hkVec;
	}
	else
	{
		return GetLookAtTarget( instance );
	}
}
#else
RedVector4 CBehaviorGraphLookAtSystemNode::GetDampedTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDampedTarget() )
	{
		RedVector4 RedVec = reinterpret_cast< const RedVector4& >( instance[ i_dampedTarget ] );		
		return RedVec;
	}
	else
	{
		return GetLookAtTarget( instance );
	}
}
#endif

#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphLookAtSystemNode::GetLookAtTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDeadZone() )
	{
		if ( HasDeadZoneSnapping() )
		{
			return GetDeadZoneSnappedTarget( instance );
		}
		else
		{
			return GetDeadZoneTarget( instance );
		}
	}
	else if ( HasDeadZoneSnapping() )
	{
		return GetDeadZoneSnappedTarget( instance );
	}

	return GetCurrentConstraintTransform( instance ).getTranslation();
}
#else
RedVector4 CBehaviorGraphLookAtSystemNode::GetLookAtTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDeadZone() )
	{
		if ( HasDeadZoneSnapping() )
		{
			return GetDeadZoneSnappedTarget( instance );
		}
		else
		{
			return GetDeadZoneTarget( instance );
		}
	}
	else if ( HasDeadZoneSnapping() )
	{
		return GetDeadZoneSnappedTarget( instance );
	}

	return GetCurrentConstraintTransform( instance ).GetTranslation();
}
#endif

#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphLookAtSystemNode::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	const Int32 headBoneIndex = instance[ i_thirdBoneIndex ];
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( headBoneIndex != -1 && m_targetObject )
	{
		// Calc model transform
		hkQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, headBoneIndex );

		// Calc new target position
		hkVector4 forward;
		COPY_VECTOR_TO_HK_VECTOR( BehaviorUtils::VectorFromAxis( A_Y ), forward );

		forward.setRotatedDir( boneModelTransform.getRotation(), forward );

		// Distance to  target - prev target must have the same distance value because of damping process
		Vector destTargetVec = GetTargetEnd( instance ).GetPosition();
		Float distToTarget = ( destTargetVec - TO_CONST_VECTOR_REF( boneModelTransform.getTranslation() ) ).Mag3();

		forward.mul4( distToTarget );

		forward.add4( boneModelTransform.getTranslation() );

		return hkQsTransform( forward, hkQuaternion::getIdentity(), hkVector4(1,1,1,1) );
	}

	return hkQsTransform::getIdentity();
}
#else
RedQsTransform CBehaviorGraphLookAtSystemNode::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	const Int32 headBoneIndex = instance[ i_thirdBoneIndex ];
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( headBoneIndex != -1 && m_targetObject )
	{
		// Calc model transform
		RedQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, headBoneIndex );

		// Calc new target position
		RedVector4 forward = BehaviorUtils::RedVectorFromAxis( A_Y );

		forward.RotateDirection( boneModelTransform.GetRotation(), forward );

		// Distance to  target - prev target must have the same distance value because of damping process
		RedVector4 destTargetVec = reinterpret_cast< const RedVector4& >( GetTargetEnd( instance ).GetPosition() );
		Float distToTarget = Sub( destTargetVec, boneModelTransform.GetTranslation() ).Length3();

		SetMul( forward, distToTarget );

		SetAdd( forward, boneModelTransform.GetTranslation() );

		return RedQsTransform( forward, RedQuaternion::IDENTITY, RedVector4::ONES );
	}

	return RedQsTransform::IDENTITY;
}
#endif
void CBehaviorGraphLookAtSystemNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	SetLimitFlag( instance, false );

	instance[ i_limitCurrWeight ] = 0.f;
	instance[ i_limitDestWeight ] = 0.f;
	instance[ i_autoLimitDeact ] = true;
	instance[ i_limitStartRange ] = GetLimitStartFromRange( m_range );

	instance[ i_currWeightFirst ] = 0.f;
	instance[ i_destWeightFirst ] = 0.f;
	instance[ i_currWeightSecAndThird ] = 0.f;
	instance[ i_destWeightSecAndThird ] = 0.f;

	instance[ i_weightsScale ] = Vector( m_firstWeight, m_secondWeight, m_thirdWeight, 1.f );
}

void CBehaviorGraphLookAtSystemNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphLookAtSystemNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	InternalReset( instance );
}

Float CBehaviorGraphLookAtSystemNode::GetDampSpeed( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedCompressedDataVariableNode )
	{
		Vector var = m_cachedCompressedDataVariableNode->GetVectorValue( instance );
		if ( var.A[COMPRESSED_PARAM_DAMP_SPEED] > 0.f )
		{
			return var.A[COMPRESSED_PARAM_DAMP_SPEED];
		}
	}

	return TBaseClass::GetDampSpeed( instance );
}

Float CBehaviorGraphLookAtSystemNode::GetFollowSpeed( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedCompressedDataVariableNode )
	{
		Vector var = m_cachedCompressedDataVariableNode->GetVectorValue( instance );
		if ( var.A[COMPRESSED_PARAM_FOLLOW_SPEED] > 0.f )
		{
			return var.A[COMPRESSED_PARAM_FOLLOW_SPEED];
		}
	}

	return TBaseClass::GetFollowSpeed( instance );
}

void CBehaviorGraphLookAtSystemNode::OnConstraintActivated( CBehaviorGraphInstance& instance ) const 
{
	Bool forceState = instance[ i_forceState ];

	InternalReset( instance );

	if ( forceState )
	{
		SetState( instance, CS_Activated );
	}
}

void CBehaviorGraphLookAtSystemNode::OnConstraintDeactivated( CBehaviorGraphInstance& instance ) const 
{

}

void CBehaviorGraphLookAtSystemNode::ActivateInputs( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::ActivateInputs( instance );

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->Activate( instance );
	}

	if ( m_cachedWeightsVariableNode )
	{
		m_cachedWeightsVariableNode->Activate( instance );
	}

	if ( m_cachedCompressedDataVariableNode )
	{
		m_cachedCompressedDataVariableNode->Activate( instance );
	}

	if ( m_cachedLimitVariableNode )
	{
		m_cachedLimitVariableNode->Activate( instance );
	}
}

void CBehaviorGraphLookAtSystemNode::DeactivateInputs( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::DeactivateInputs( instance );

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->Deactivate( instance );
	}

	if ( m_cachedWeightsVariableNode )
	{
		m_cachedWeightsVariableNode->Deactivate( instance );
	}

	if ( m_cachedCompressedDataVariableNode )
	{
		m_cachedCompressedDataVariableNode->Deactivate( instance );
	}

	if ( m_cachedLimitVariableNode )
	{
		m_cachedLimitVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphLookAtSystemNode::ForceActivatedState( CBehaviorGraphInstance& instance, Bool flag ) const
{
	instance[ i_forceState ] = flag;
}

Bool CBehaviorGraphLookAtSystemNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	Uint32 forceEvent = instance[ i_evtForceState ];

	if ( forceEvent != CBehaviorEventsList::NO_EVENT && forceEvent == event.GetEventID() )
	{
		ForceActivatedState( instance, true );
		ret = true;
	}

	return ret;
}

void CBehaviorGraphLookAtSystemNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( ShouldUseInputs( instance ) )
	{
		if ( m_cachedLevelVariableNode )
		{
			m_cachedLevelVariableNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedWeightsVariableNode )
		{
			m_cachedWeightsVariableNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedCompressedDataVariableNode )
		{
			m_cachedCompressedDataVariableNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedLimitVariableNode )
		{
			m_cachedLimitVariableNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

void CBehaviorGraphLookAtSystemNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );

	ForceActivatedState( instance, false );
}

void CBehaviorGraphLookAtSystemNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

#ifndef LOOK_AT_POSE

	const Bool forceState = instance[ i_forceState ];
	if ( forceState )
	{
		// Beta hack
		EngineQsTransform target = m_targetObject->RefreshTransform( instance );

		SetFollowTimer( instance );
		SetProgress( instance, 1.f );

		SetTargetStart( instance, target );
		SetTargetEnd( instance, target );
	}

	if ( ShouldUseInputs( instance ) )
	{
		if ( m_cachedLevelVariableNode )
		{
			ASSERT( m_cachedLevelVariableNode->IsActive( instance ) );
			m_cachedLevelVariableNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedWeightsVariableNode )
		{
			ASSERT( m_cachedWeightsVariableNode->IsActive( instance ) );
			m_cachedWeightsVariableNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedCompressedDataVariableNode )
		{
			ASSERT( m_cachedCompressedDataVariableNode->IsActive( instance ) );
			m_cachedCompressedDataVariableNode->Update( context, instance, timeDelta );
			Vector value = m_cachedCompressedDataVariableNode->GetVectorValue( instance );
			instance[ i_autoLimitDeact ] = value.A[ COMPRESSED_PARAM_AUTO_LIMIT_DEACT ] > 0.5f ? true : false;
			instance[ i_limitStartRange ] = GetLimitStartFromRange( value.A[ COMPRESSED_PARAM_LIMIT_RANGE ] );
		}
		else
		{
			if ( m_cachedLimitVariableNode )
			{
				ASSERT( m_cachedLimitVariableNode->IsActive( instance ) );
				m_cachedLimitVariableNode->Update( context, instance, timeDelta );
				instance[ i_autoLimitDeact ] = m_cachedLimitVariableNode->GetValue( instance ) > 0.5f ? true : false;
			}
		}

		CheckWeights( instance );
	}

	UpdateDeadZoneTarget( instance );

	UpdateDampedTarget( instance, timeDelta );

	CheckLimits( instance );

	UpdateTimers( instance, timeDelta );

	ForceActivatedState( instance, false );

#endif
}

void CBehaviorGraphLookAtSystemNode::CheckLimits( CBehaviorGraphInstance& instance ) const
{
	const Bool isLimited = GetLimitFlag( instance ) && instance[ i_autoLimitDeact ];
	instance[ i_limitDestWeight ] = isLimited ? 1.f : 0.f;
}

void CBehaviorGraphLookAtSystemNode::CheckWeights( CBehaviorGraphInstance& instance ) const
{
	// Get level
	Int32 level = LL_Body;

	if ( m_cachedLevelVariableNode )
	{
		ASSERT( m_cachedLevelVariableNode->IsActive( instance ) );
		level = (Int32)m_cachedLevelVariableNode->GetValue( instance );
	}

	// Get weights scale
	Vector& weightsScale = instance[ i_weightsScale ];

	if ( m_cachedWeightsVariableNode )
	{
		ASSERT( m_cachedWeightsVariableNode->IsActive( instance ) );
		weightsScale = m_cachedWeightsVariableNode->GetVectorValue( instance );
	}

	// Destination weight
	Float& destWeightFirst = instance[ i_destWeightFirst ];
	Float& destWeightSecAndThird = instance[ i_destWeightSecAndThird];

	switch ( level )
	{
	case LL_Null:
	case LL_Eyes:
		destWeightFirst = 0.f;
		destWeightSecAndThird = 0.f;
		break;
	case LL_Head:
		destWeightFirst = 0.f;
		destWeightSecAndThird = 1.f;
		break;
	case LL_Body:
		destWeightFirst = 1.f;
		destWeightSecAndThird = 1.f;
		break;
	default:
		ASSERT( 0 );
	}

	ASSERT( destWeightSecAndThird >= destWeightFirst );
}

void CBehaviorGraphLookAtSystemNode::UpdateTimers( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Float&		currWeightFirst = instance[ i_currWeightFirst ];
	const Float destWeightFirst = instance[ i_destWeightFirst ];

	Float&		currWeightSecAndThird = instance[ i_currWeightSecAndThird ];
	const Float destWeightSecAndThird = instance[ i_destWeightSecAndThird ];

	Float&		currLimitWeight = instance[ i_limitCurrWeight ];
	const Float destLimitWeight = instance[ i_limitDestWeight ];

	// Solver
	currWeightFirst = CalcLookAtWeight( destWeightFirst, currWeightFirst, timeDelta, m_levelDampTime );
	currWeightSecAndThird = CalcLookAtWeight( destWeightSecAndThird, currWeightSecAndThird, timeDelta, m_levelDampTime );

	ASSERT( currWeightSecAndThird >= currWeightFirst );

	// Limits
	currLimitWeight = CalcLookAtWeight( destLimitWeight, currLimitWeight, timeDelta, m_limitDampTime );
}

Float CBehaviorGraphLookAtSystemNode::CalcLookAtWeight( const Float dest, const Float curr, const Float timeDelta, const Float timeScale ) const
{
	Float diff = dest - curr;
	Float absDiff = MAbs( diff );

	if ( absDiff > 0.01f )
	{
		Float sign = diff > 0.f ? 1.f : -1.f;

		Float delta = Clamp( timeDelta / timeScale, -absDiff, absDiff );

		return Clamp( curr + delta * sign, 0.f, 1.f );
	}
	else
	{
		return dest;
	}
}

Float CBehaviorGraphLookAtSystemNode::GetLimitWeight( CBehaviorGraphInstance& instance ) const
{
	return Clamp< Float >( m_internalDampCurve->GetFloatValue( instance[ i_limitCurrWeight ] ), 0.0f, 1.0f );
}

Float CBehaviorGraphLookAtSystemNode::GetSolverWeightFirst( CBehaviorGraphInstance& instance ) const
{
	return Clamp< Float >( m_internalDampCurve->GetFloatValue( instance[ i_currWeightFirst ] ), 0.0f, 1.0f );
}

Float CBehaviorGraphLookAtSystemNode::GetSolverWeightSecAndThird( CBehaviorGraphInstance& instance ) const
{
	return Clamp< Float >( m_internalDampCurve->GetFloatValue( instance[ i_currWeightSecAndThird ] ), 0.0f, 1.0f );
}

Float CBehaviorGraphLookAtSystemNode::GetLimitStartFromRange( Float range ) const
{
	if ( range > 0.f && range < 360.f )
	{
		Float temp = - range / 2.f + 90.f;
		return tan( (Float)( DEG2RAD(temp) ) );
	}
	else
	{
		return 0.1763f;
	}
}

void CBehaviorGraphLookAtSystemNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

#ifndef LOOK_AT_POSE

	if ( !IsConstraintActive( instance ) || m_targetObject == NULL ) 
	{
		return;
	}

	const Int32 firstBone = instance[ i_firstBoneIndex ];
	const Int32 secondBone = instance[ i_secondBoneIndex ];
	const Int32 thirdBone = instance[ i_thirdBoneIndex ];

	if ( firstBone == -1 || secondBone == -1 || thirdBone == -1 )
	{
		return;
	}

	// Get limit weight
	Float limitWeight = GetLimitWeight( instance );

	// Solver data
	ChainSolverData data = GetSolverData( instance, output );

	// Solve
	hkQsTransform transforms[ 3 ];
	Bool isLimit = Solve( data, transforms );
	SetLimitFlag( instance, isLimit );

	Float firstWeight = 1.f;
	Float secondWeight = 1.f;
	Float thirdWeight = 1.f;

	// Final smooth
	if ( IsTargetDamping( instance ) )
	{
		Float signedProgress = GetSingedProgress( instance );
		ASSERT( signedProgress >= 0.f );

		if ( signedProgress < SWITCH_SMOOTH )
		{
			Float p = signedProgress / SWITCH_SMOOTH;

			firstWeight = p;
			secondWeight = p;
			thirdWeight = p;
		}
	}

	// Limits
	if ( limitWeight > 0.f )
	{
		firstWeight *= 1.f - limitWeight;
		secondWeight *= 1.f - limitWeight;
		thirdWeight *= 1.f - limitWeight;

		//if ( data.m_boneData[ 0 ].m_weight > 0.f )
		//{
		//	output.m_outputPose[ firstBone ].setInterpolate4( transforms[ 0 ], output.m_outputPose[ firstBone ] ,limitWeight );
		//}
	}

	if ( thirdWeight == 1.f )
	{
		output.m_outputPose[ thirdBone ] = transforms[ 2 ];
	}
	else if ( thirdWeight < 1.f && thirdWeight > 0.f )
	{
		output.m_outputPose[ thirdBone ].setInterpolate4( output.m_outputPose[ thirdBone ], transforms[ 2 ], thirdWeight );
	}

	if ( secondWeight == 1.f )
	{
		output.m_outputPose[ secondBone ] = transforms[ 1 ];
	}
	else if ( secondWeight < 1.f && secondWeight > 0.f )
	{
		output.m_outputPose[ secondBone ].setInterpolate4( output.m_outputPose[ secondBone ], transforms[ 1 ], secondWeight );
	}

	if ( firstWeight == 1.f )
	{
		output.m_outputPose[ firstBone ] = transforms[ 0 ];
	}
	else if ( firstWeight < 1.f && firstWeight > 0.f )
	{
		output.m_outputPose[ firstBone ].setInterpolate4( output.m_outputPose[ firstBone ], transforms[ 0 ], firstWeight );
	}

#endif
}

CBehaviorGraphLookAtSystemNode::ChainSolverData CBehaviorGraphLookAtSystemNode::GetSolverData( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const
{
	ChainSolverData data;

	const Int32 thirdBone = instance[ i_thirdBoneIndex ];
	const Int32 secBone = instance[ i_secondBoneIndex ];
	const Int32 firstBone = instance[ i_firstBoneIndex ];
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform boneParentMS = pose.GetParentBoneModelTransform( instance.GetAnimatedComponent(), firstBone );
#else
	RedQsTransform boneParentMS = pose.GetParentBoneModelTransform( instance.GetAnimatedComponent(), firstBone );
#endif
	data.m_targetMS[ 0 ] = GetDampedTarget( instance );
	data.m_targetMS[ 1 ] = GetLookAtTarget( instance );
	data.m_targetMS[ 2 ] = data.m_targetMS[ 1 ];
	data.m_firstBoneParentMS = boneParentMS;
	data.m_firstBoneLS = pose.m_outputPose[ firstBone ];
	data.m_secondBoneLS = pose.m_outputPose[ secBone ];
	data.m_thirdBoneLS = pose.m_outputPose[ thirdBone ];
	data.m_localOffsetForLastBone = &m_localOffset;

	const Vector& weights = instance[ i_weightsScale ];
	const Float firstScale = GetSolverWeightFirst( instance );
	const Float secAndThirdScale = GetSolverWeightSecAndThird( instance );

	data.m_weightsScale.A[ 0 ] = weights.A[ 0 ] * firstScale;
	data.m_weightsScale.A[ 1 ] = weights.A[ 1 ] * secAndThirdScale;
	data.m_weightsScale.A[ 2 ] = weights.A[ 2 ] * secAndThirdScale;
	
	return data;
}

void CBehaviorGraphLookAtSystemNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

#ifndef LOOK_AT_POSE

	const Int32 boneIndex = instance[ i_thirdBoneIndex ];

	if( !m_generateEditorFragments || GetState( instance ) == CS_Deactivated || boneIndex == -1 ) return;

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	Vector forwardDir = BehaviorUtils::VectorFromAxis( A_Y );

	Matrix headMatWM = ac->GetBoneMatrixWorldSpace( boneIndex );

	EngineQsTransform tempCurrTarget = GetTargetCurr( instance );

	Vector bonePosMS = instance.GetAnimatedComponent()->GetBoneMatrixModelSpace( boneIndex ).GetTranslation();
	Vector vecToTargetMS = tempCurrTarget.GetPosition() - bonePosMS;
	Float distToTarget = vecToTargetMS.Normalize3();

	// Head dir
	frame->AddDebugArrow( headMatWM, forwardDir, distToTarget, Color::LIGHT_YELLOW );
	frame->AddDebugSphere( tempCurrTarget.GetPosition(), 0.01f, Matrix::IDENTITY, Color::LIGHT_YELLOW );

	const Matrix& localToWorld = ac->GetLocalToWorld();

	if ( HasDeadZone() )
	{
		const Vector& deadZoneTarget = instance[ i_deadZoneTarget ];

		frame->AddDebugSphere( localToWorld.TransformPoint( deadZoneTarget ), 0.1f, Matrix::IDENTITY, Color::CYAN, false );
	}

	if ( HasDeadZoneSnapping() )
	{
		const Matrix& localToWorld = ac->GetLocalToWorld();
		const Vector& deadZoneSnappedTarget = instance[ i_deadZoneSnappedTarget ];

		frame->AddDebugSphere( localToWorld.TransformPoint( deadZoneSnappedTarget ), 0.1f, Matrix::IDENTITY, Color::MAGENTA, false );
	}

	if ( HasDampedTarget() )
	{
		const Vector& dampedTarget = instance[ i_dampedTarget ];

		Box box( localToWorld.TransformPoint( dampedTarget ), 0.05f );

		frame->AddDebugBox( box, Matrix::IDENTITY, Color::LIGHT_BLUE, false );
	}

#endif
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicLookAtSystemNode );

CBehaviorGraphMimicLookAtSystemNode::CBehaviorGraphMimicLookAtSystemNode()
	: m_eyeHorMax( 90.f )
	, m_eyeVerMax( 90.f )
	, m_dampTime( 1.f )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicLookAtSystemNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Level ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

String CBehaviorGraphMimicLookAtSystemNode::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Mimics look at - %s"), m_name.AsChar() );
	else
		return String( TXT("Mimics look at") );
}

void CBehaviorGraphMimicLookAtSystemNode::OnSpawned(const GraphBlockSpawnInfo& info)
{
	TBaseClass::OnSpawned( info );
	m_dampCurve = CreateObject< CCurve >( this );
}

#endif

void CBehaviorGraphMimicLookAtSystemNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
	m_cachedTargetNode = CacheVectorValueBlock( TXT("Target") );
	m_cachedLevelVariableNode = CacheValueBlock( TXT("Level") );
}

CSkeleton* CBehaviorGraphMimicLookAtSystemNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetMimicSkeleton();
}

void CBehaviorGraphMimicLookAtSystemNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_eyeHorLeft;
	compiler << i_eyeHorRight;
	compiler << i_eyeVerLeft;
	compiler << i_eyeVerRight;
	compiler << i_eyeLeftPlacer;
	compiler << i_eyeRightPlacer;
	compiler << i_currWeight;
	compiler << i_destWeight;
	compiler << i_target;
	compiler << i_eyeLeftDirLS;
	compiler << i_eyeRightDirLS;
}

void CBehaviorGraphMimicLookAtSystemNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );

	instance[ i_eyeHorLeft ] = -1;
	instance[ i_eyeHorRight ] = -1;
	instance[ i_eyeVerLeft ] = -1;
	instance[ i_eyeVerRight ] = -1;
	instance[ i_eyeLeftPlacer ] = -1;
	instance[ i_eyeRightPlacer ] = -1;
	instance[ i_eyeLeftDirLS ] = Vector::ZERO_3D_POINT;
	instance[ i_eyeRightDirLS ] = Vector::ZERO_3D_POINT;

	CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	CSkeleton* mimicSkeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();

	if ( skeleton && mimicSkeleton )
	{
		// Tracks
		instance[ i_eyeHorLeft ] = FindTrackIndex( m_eyeHorLeftTrack, mimicSkeleton );
		instance[ i_eyeHorRight ] = FindTrackIndex( m_eyeHorRightTrack, mimicSkeleton );

		instance[ i_eyeVerLeft ] = FindTrackIndex( m_eyeVerLeftTrack, mimicSkeleton );
		instance[ i_eyeVerRight ] = FindTrackIndex( m_eyeVerRightTrack, mimicSkeleton );

		// Bones
		instance[ i_eyeLeftPlacer ] = FindBoneIndex( m_eyeLeftPlacerBone.AsChar(), skeleton );
		instance[ i_eyeRightPlacer ] = FindBoneIndex( m_eyeRightPlacerBone.AsChar(), skeleton );

		// Bones direction
		const Vector boneDir = BehaviorUtils::VectorFromAxis( A_X );

		Vector& eyeLeftDirLS = instance[ i_eyeLeftDirLS ];
		Vector& eyeRightDirLS = instance[ i_eyeRightDirLS ];

		eyeLeftDirLS = Vector::ZERO_3D_POINT;
		eyeRightDirLS = Vector::ZERO_3D_POINT;

		if ( instance[ i_eyeLeftPlacer ] != -1 )
		{
			Matrix boneMS = skeleton->GetBoneMatrixMS( instance[ i_eyeLeftPlacer ] );

			boneMS.SetTranslation( Vector::ZERO_3D_POINT );
			boneMS.Invert();

			eyeLeftDirLS = boneMS.TransformVector( boneDir ).Normalized3();
		}
	}
}

void CBehaviorGraphMimicLookAtSystemNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );
}

void CBehaviorGraphMimicLookAtSystemNode::InternalReset( CBehaviorGraphInstance& instance )const
{
	instance[ i_currWeight ] = 0.f;
	instance[ i_destWeight ] = 0.f;
	instance[ i_target ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphMimicLookAtSystemNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );
	InternalReset( instance );
}

void CBehaviorGraphMimicLookAtSystemNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
		instance[ i_destWeight ] = m_cachedControlVariableNode->GetValue( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Update( context, instance, timeDelta );
		instance[ i_target ] = m_cachedTargetNode->GetVectorValue( instance );
	}

	CheckLookAtLevel( instance );

	UpdateWeight( instance, timeDelta );
}

void CBehaviorGraphMimicLookAtSystemNode::CheckLookAtLevel( CBehaviorGraphInstance& instance )const
{
	if ( m_cachedLevelVariableNode )
	{
		Int32 level = (Int32)m_cachedLevelVariableNode->GetValue( instance );

		if ( level == LL_Null )
		{
			instance[ i_destWeight ] = 0.f;
		}
	}
}

void CBehaviorGraphMimicLookAtSystemNode::UpdateWeight( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Float& currWeight = instance[ i_currWeight ];
	const Float destWeight = instance[ i_destWeight ];

	Float diff = destWeight - currWeight;
	Float absDiff = MAbs( diff );

	if ( absDiff > 0.01f && m_dampTime > 0.f )
	{
		Float sign = diff > 0.f ? 1.f : -1.f;

		Float delta = Clamp( timeDelta / m_dampTime, -absDiff, absDiff );

		currWeight = Clamp( currWeight + delta * sign, 0.f, 1.f );
	}
}

Float CBehaviorGraphMimicLookAtSystemNode::GetLookAtWeight( CBehaviorGraphInstance& instance )const
{
	return Clamp( m_dampCurve->GetFloatValue( instance[ i_currWeight ] ), 0.f, 1.f );
}

void CBehaviorGraphMimicLookAtSystemNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	if ( !context.HasMimic() )
	{
		return;
	}

	// Get pose
	CCacheBehaviorGraphOutput cachePose( context );
	SBehaviorGraphOutput* animPose = cachePose.GetPose();

	// Solver data
	SolverData leftEyeData;
	SolverData rightEyeData;

	// Setup
	if ( animPose && GetSolversData( leftEyeData, rightEyeData, instance, *animPose ) )
	{
		// Solver
		Solve( leftEyeData );
		Solve( rightEyeData );

		// Set data do mimic pose
		const Int32 trackLeftHor = instance[ i_eyeHorLeft ];
		const Int32 trackLeftVer = instance[ i_eyeVerLeft ];
		const Int32 trackRightHor = instance[ i_eyeHorRight ];
		const Int32 trackRightVer = instance[ i_eyeVerRight ];

		output.m_floatTracks[ trackLeftHor ] = leftEyeData.m_horTrackValue;
		output.m_floatTracks[ trackLeftVer ] = leftEyeData.m_verTrackValue;
		output.m_floatTracks[ trackRightHor ] = rightEyeData.m_horTrackValue;
		output.m_floatTracks[ trackRightVer ] = rightEyeData.m_verTrackValue;
	}
}

Bool CBehaviorGraphMimicLookAtSystemNode::GetSolversData( CBehaviorGraphMimicLookAtSystemNode::SolverData& leftEye, 
														  CBehaviorGraphMimicLookAtSystemNode::SolverData& rightEye, 
														  CBehaviorGraphInstance& instance, 
														  const SBehaviorGraphOutput &animPose ) const
{
	const Float weight = GetLookAtWeight( instance );
	const Int32 eyeLeftPlacer = instance[ i_eyeLeftPlacer ];
	const Int32 eyeRightPlacer = instance[ i_eyeRightPlacer ];
	const Vector targetMS = instance[ i_target ];
#ifdef USE_HAVOK_ANIMATION
	const hkVector4 hkTargetMS = TO_CONST_HK_VECTOR_REF( targetMS );
#else
	const RedVector4 hkTargetMS = reinterpret_cast< const RedVector4& >( targetMS );
#endif
	if ( eyeLeftPlacer != -1 && eyeRightPlacer != -1 )
	{
		leftEye.m_placerMS = animPose.GetBoneModelTransform( instance.GetAnimatedComponent(), eyeLeftPlacer );
		leftEye.m_targetMS = hkTargetMS;
		leftEye.m_weight = weight;
		leftEye.m_horMax = m_eyeHorMax;
		leftEye.m_verMax = m_eyeVerMax;

		rightEye.m_placerMS = animPose.GetBoneModelTransform( instance.GetAnimatedComponent(), eyeRightPlacer );
		rightEye.m_targetMS = hkTargetMS;
		rightEye.m_weight = weight;
		rightEye.m_horMax = m_eyeHorMax;
		rightEye.m_verMax = m_eyeVerMax;

		return true;
	}
	else
	{
		return false;
	}
}

void CBehaviorGraphMimicLookAtSystemNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Activate( instance );
	}

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMimicLookAtSystemNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Deactivate( instance );
	}

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicLookAtSystemNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedLevelVariableNode )
	{
		m_cachedLevelVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicLookAtSystemNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if( !m_generateEditorFragments || GetLookAtWeight( instance ) < 0.01f ) return;

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	// Mark target position
	const Vector targetMS = instance[ i_target ];
	const Int32 eyeLeftPlacer = instance[ i_eyeLeftPlacer ];
	const Int32 eyeRightPlacer = instance[ i_eyeRightPlacer ];

	const Matrix& localToWorld = ac->GetLocalToWorld();
	const Vector targetPosWS = localToWorld.TransformPoint( targetMS );

	frame->AddDebugSphere( targetPosWS, 0.1f, Matrix::IDENTITY, Color::YELLOW, false);
	frame->AddDebugSphere( targetPosWS, 0.01f, Matrix::IDENTITY, Color::YELLOW, false);

	// Placers
	Matrix placerLeftWSMat = ac->GetBoneMatrixWorldSpace( eyeLeftPlacer );
	Matrix placerRightWSMat = ac->GetBoneMatrixWorldSpace( eyeRightPlacer );

	Vector placerLeftWS = placerLeftWSMat.GetTranslation();
	Vector placerRightWS = placerRightWSMat.GetTranslation();

	frame->AddDebugLine( placerLeftWS, targetPosWS, Color::LIGHT_YELLOW );
	frame->AddDebugLine( placerRightWS, targetPosWS, Color::LIGHT_YELLOW );

	frame->AddDebugAxis( placerLeftWS, placerLeftWSMat, 0.05f, true );
	frame->AddDebugAxis( placerRightWS, placerRightWSMat, 0.05f, true );

	Matrix placerChildLeftWS = FindChildBoneMatrixWS( instance, eyeLeftPlacer );
	Matrix placerChildRightWS = FindChildBoneMatrixWS( instance, eyeRightPlacer );

	frame->AddDebugAxis( placerChildLeftWS.GetTranslation(), placerChildLeftWS, 0.1f, true );
	frame->AddDebugAxis( placerChildRightWS.GetTranslation(), placerChildRightWS, 0.1f, true );
}
