/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNodeLookAt.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../engine/renderFrame.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeLookAt );

CBehaviorGraphConstraintNodeLookAt::CBehaviorGraphConstraintNodeLookAt()
	: m_forwardDir(A_Y)
	, m_localOffset(0,0,0)
	, m_horizontalLimitAngle( 90.f )
	, m_upLimitAngle(45)
	, m_downLimitAngle(45)
	, m_rangeLimitUpAxis(A_Z)
	, m_solverType( LST_Quat )
	, m_deadZone( 0.f )
	, m_deadZoneDist( 0.7f )
{
	
}

void CBehaviorGraphConstraintNodeLookAt::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_parentBoneIndex;
	compiler << i_deadZoneTarget;
	compiler << i_deadZoneSnappedTarget;
}

void CBehaviorGraphConstraintNodeLookAt::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_bone, instance );
	instance[ i_parentBoneIndex ] = FindBoneIndex( m_parentBone, instance );
	instance[ i_deadZoneTarget ] = Vector::ZERO_3D_POINT;
	instance[ i_deadZoneSnappedTarget ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphConstraintNodeLookAt::UpdateDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	if ( HasDeadZone() )
	{
		Vector& deadZoneTarget = instance[ i_deadZoneTarget ];
		IConstraintTarget::ApplyDeadZone( GetTargetCurr( instance ).GetPosition(), deadZoneTarget, m_deadZone );

		if ( HasDeadZoneSnapping() )
		{
			Vector& deadZoneSnappedTarget = instance[ i_deadZoneSnappedTarget ];
			IConstraintTarget::ApplyDeadZoneSnapping( deadZoneTarget, deadZoneSnappedTarget, m_deadZoneDist, 0.f );
		}
	}
}

Bool CBehaviorGraphConstraintNodeLookAt::HasDeadZone() const
{
	return m_deadZone > 0.f;
}

Bool CBehaviorGraphConstraintNodeLookAt::HasDeadZoneSnapping() const
{
	return m_deadZoneDist > 0.f;
}
#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphConstraintNodeLookAt::GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	hkVector4 hkVec;
	const Vector& vec = instance[ i_deadZoneTarget ];
	hkVec = TO_CONST_HK_VECTOR_REF( vec );
	return hkVec;
}
#else
RedVector4 CBehaviorGraphConstraintNodeLookAt::GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const
{
	RedVector4 redVec;
	redVec = reinterpret_cast< const RedVector4& >( instance[ i_deadZoneTarget ] );
	return redVec;
}
#endif

#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphConstraintNodeLookAt::GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const
{
	hkVector4 hkVec;
	const Vector& vec = instance[ i_deadZoneSnappedTarget ];
	hkVec = TO_CONST_HK_VECTOR_REF( vec );
	return hkVec;
}
#else
RedVector4 CBehaviorGraphConstraintNodeLookAt::GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const
{
	RedVector4 RedVec;
	RedVec = reinterpret_cast< const RedVector4& >( instance[ i_deadZoneSnappedTarget ] );
	return RedVec;
}
#endif
#ifdef USE_HAVOK_ANIMATION
hkVector4 CBehaviorGraphConstraintNodeLookAt::GetLookAtTarget( CBehaviorGraphInstance& instance ) const
{
	return HasDeadZone() ? HasDeadZoneSnapping() ? GetDeadZoneSnappedTarget( instance ) : GetDeadZoneTarget( instance ) : GetCurrentConstraintTransform( instance ).getTranslation();
}
#else
RedVector4 CBehaviorGraphConstraintNodeLookAt::GetLookAtTarget( CBehaviorGraphInstance& instance ) const
{
	return HasDeadZone() ? HasDeadZoneSnapping() ? GetDeadZoneSnappedTarget( instance ) : GetDeadZoneTarget( instance ) : GetCurrentConstraintTransform( instance ).GetTranslation();
}
#endif
void CBehaviorGraphConstraintNodeLookAt::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_parentBoneIndex );
}

#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphConstraintNodeLookAt::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	const Int32 headBoneIndex = instance[ i_boneIndex ];
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( headBoneIndex != -1 && m_targetObject )
	{
		// Calc model transform
		hkQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, headBoneIndex );

		// Calc new target position
		hkVector4 forward;
		COPY_VECTOR_TO_HK_VECTOR( BehaviorUtils::VectorFromAxis( m_forwardDir ), forward );

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
RedQsTransform CBehaviorGraphConstraintNodeLookAt::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	const Int32 headBoneIndex = instance[ i_boneIndex ];
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( headBoneIndex != -1 && m_targetObject )
	{
		// Calc model transform
		RedQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, headBoneIndex );

		// Calc new target position
		RedVector4 forward = BehaviorUtils::RedVectorFromAxis( m_forwardDir );

		forward.RotateDirection( boneModelTransform.GetRotation(), forward );

		// Distance to  target - prev target must have the same distance value because of damping process
		RedVector4 destTargetVec = reinterpret_cast< const RedVector4& >( GetTargetEnd( instance ).GetPosition() );
		Float distToTarget = Sub( destTargetVec, boneModelTransform.GetTranslation() ).Length3();

		SetMul( forward, distToTarget );

		SetAdd( forward , boneModelTransform.GetTranslation() );

		return RedQsTransform( forward, RedQuaternion::IDENTITY, RedVector4::ONES );
	}

	return RedQsTransform::IDENTITY;
}
#endif
void CBehaviorGraphConstraintNodeLookAt::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	const Int32 boneIndex = instance[ i_boneIndex ];
	const Int32 parentBoneIndex = instance[ i_parentBoneIndex ];

	if ( !IsConstraintActive( instance ) || boneIndex == -1 || parentBoneIndex == -1 ) 
		return;

	UpdateDeadZoneTarget( instance );

	if ( m_solverType == LST_Quat )
	{
		IQuatLookAtSolver::SolverData boneData = GetBoneDataQuat( instance, output );

		// Solve
		Bool isLimit = IQuatLookAtSolver::Solve( boneData, output.m_outputPose[ boneIndex ] );
		SetLimitFlag( instance, isLimit );
	}
	else
	{
		IEulerLookAtSolver::SolverData boneData = GetBoneDataEuler( instance, output );

		// Solve
		Bool isLimit = IEulerLookAtSolver::Solve( boneData, output.m_outputPose[ boneIndex ] );
		SetLimitFlag( instance, isLimit );
	}
}

IEulerLookAtSolver::SolverData CBehaviorGraphConstraintNodeLookAt::GetBoneDataEuler( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const
{
	IEulerLookAtSolver::SolverData boneData;

	const Int32 bone = instance[ i_boneIndex ];
	const Int32 parentBone = instance[ i_parentBoneIndex ];
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform boneParentMS = pose.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBone );
#else
	RedQsTransform boneParentMS = pose.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBone );
#endif
	boneData.m_weight = 1.f;
	boneData.m_boneInLS = pose.m_outputPose[ bone ];
	boneData.m_boneParentMS = boneParentMS;
	boneData.m_horizontalLimit = m_horizontalLimitAngle;
	boneData.m_upLimit = m_upLimitAngle;
	boneData.m_downLimit = m_downLimitAngle;
	boneData.m_targetMS = GetLookAtTarget( instance );

	return boneData;
}

IQuatLookAtSolver::SolverData CBehaviorGraphConstraintNodeLookAt::GetBoneDataQuat( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const
{
	IQuatLookAtSolver::SolverData boneData;

	const Int32 bone = instance[ i_boneIndex ];
	const Int32 parentBone = instance[ i_parentBoneIndex ];
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform boneParentMS = pose.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBone );
	hkQsTransform boneMS;
	boneMS.setMul( boneParentMS, pose.m_outputPose[ bone ] );
#else
	RedQsTransform boneParentMS = pose.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBone );
	RedQsTransform boneMS;
	boneMS.SetMul( boneParentMS, pose.m_outputPose[ bone ] );
#endif
	boneData.m_weight = 1.f;
	boneData.m_inOutBoneMS = boneMS;
	boneData.m_boneParentMS = boneParentMS;
	boneData.m_forward = m_forwardDir;
	boneData.m_up = m_rangeLimitUpAxis;
	boneData.m_horizontalLimit = m_horizontalLimitAngle;
	boneData.m_upLimit = m_upLimitAngle;
	boneData.m_downLimit = m_downLimitAngle;
	boneData.m_localOffset = &m_localOffset;
	boneData.m_targetMS = GetLookAtTarget( instance );

	return boneData;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNodeLookAt::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Look at - %s"), m_name.AsChar() );
	else
		return String( TXT("Look at") );
}

#endif

void CBehaviorGraphConstraintNodeLookAt::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	const Int32 boneIndex = instance[ i_boneIndex ];

	if( !m_generateEditorFragments || GetState( instance ) == CS_Deactivated || boneIndex == -1 ) return;

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	Vector forwardDir = BehaviorUtils::VectorFromAxis( m_forwardDir );

	Matrix headMatWM = ac->GetBoneMatrixWorldSpace( boneIndex );

	EngineQsTransform tempCurrTarget = GetTargetCurr( instance );

	Vector bonePosMS = instance.GetAnimatedComponent()->GetBoneMatrixModelSpace( boneIndex ).GetTranslation();
	Vector vecToTargetMS = tempCurrTarget.GetPosition() - bonePosMS;
	Float distToTarget = vecToTargetMS.Normalize3();

	// Head dir
	frame->AddDebugArrow( headMatWM, forwardDir, distToTarget, Color::LIGHT_YELLOW );
	frame->AddDebugSphere( tempCurrTarget.GetPosition(), 0.01f, Matrix::IDENTITY, Color::LIGHT_YELLOW );

	if ( HasDeadZone() )
	{
		const Matrix& localToWorld = ac->GetLocalToWorld();
		const Vector& deadZoneTarget = instance[ i_deadZoneTarget ];

		frame->AddDebugSphere( localToWorld.TransformPoint( deadZoneTarget ), 0.1f, Matrix::IDENTITY, Color::CYAN, false );

		if ( HasDeadZoneSnapping() )
		{
			const Matrix& localToWorld = ac->GetLocalToWorld();
			const Vector& deadZoneSnappedTarget = instance[ i_deadZoneSnappedTarget ];

			frame->AddDebugSphere( localToWorld.TransformPoint( deadZoneSnappedTarget ), 0.1f, Matrix::IDENTITY, Color::MAGENTA, false );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLookAtNode );

CBehaviorGraphLookAtNode::CBehaviorGraphLookAtNode()
	: m_axis( 0.0f, 0.0f, 1.0f )
	, m_useLimits( false )
	, m_limitAngle( 15.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
}

#endif

void CBehaviorGraphLookAtNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_weight;
	compiler << i_target;

#ifndef NO_EDITOR
	compiler << i_dirMS;
	compiler << i_boneMS;
	compiler << i_vecToTarget;
	compiler << i_vecToTargetLimited;
#endif
}

void CBehaviorGraphLookAtNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_weight ] = 0.f;
	instance[ i_target ] = Vector::ZERO_3D_POINT;
}

void CBehaviorGraphLookAtNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_weight );
	INST_PROP( i_target );
}

void CBehaviorGraphLookAtNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SimpleLookAt );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedValueNode->GetValue( instance );
	}
	else
	{
		instance[ i_weight ] = 0.f;
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Update( context, instance, timeDelta );

		const Matrix& l2w = instance.GetAnimatedComponent()->GetLocalToWorld();
		const Bool shouldUseFullInverted = Abs( l2w.GetAxisX().SquareMag3() - 1.0f ) > 0.01f; // may we at least assume that scale is uniform, please?
		const Matrix w2l = shouldUseFullInverted ? l2w.FullInverted() : l2w.Inverted();

		instance[ i_target ] = w2l.TransformPoint( m_cachedTargetNode->GetVectorValue( instance ) );
	}
	else
	{
		instance[ i_target ] = Vector::ZERO_3D_POINT;
	}
}

void CBehaviorGraphLookAtNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( SimpleLookAt );

	TBaseClass::Sample( context, instance, output );

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CSkeleton* skeleton = ac->GetSkeleton();
	if ( !skeleton )
	{
		return;
	}

	const Int32 boneIdx  = instance[ i_boneIndex ];
	const Int32 parentIdx = skeleton->GetParentBoneIndex( boneIdx );

	Int32 numBones = (Int32)output.m_numBones;

	if ( boneIdx != -1 && boneIdx < numBones && parentIdx != -1 && parentIdx < numBones )
	{
		// Weight
		const Float weight = instance[ i_weight ];

		
#ifdef USE_HAVOK_ANIMATION
		// Target
		const Vector& temp = instance[ i_target ];
		const hkVector4 targetMS = TO_CONST_HK_VECTOR_REF( temp );

		// Bone LS
		hkQsTransform& boneLS = output.m_outputPose[ boneIdx ];

		// Bone MS
		hkQsTransform parentMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), parentIdx );
		hkQsTransform boneMS; boneMS.setMul( parentMS, boneLS );

#ifndef NO_EDITOR
		instance[ i_boneMS ] = TO_CONST_VECTOR_REF( boneMS.m_translation );
#endif

		// Target dir
		hkVector4 vecToTargetMS;
		vecToTargetMS.setSub4( targetMS, boneMS.m_translation );
		hkReal targetDist; targetDist = vecToTargetMS.length3();
		vecToTargetMS.normalize3();

#ifndef NO_EDITOR
		instance[ i_vecToTarget ] = TO_CONST_VECTOR_REF( vecToTargetMS );
		instance[ i_vecToTargetLimited ] = TO_CONST_VECTOR_REF( vecToTargetMS );
#endif

		// Limits
		if ( m_useLimits )
		{
			hkVector4 limitLS; limitLS.setZero4();
			COPY_VECTOR_TO_HK_VECTOR( m_axis, limitLS );

			hkVector4 limitMS; 
			limitMS.setRotatedDir( parentMS.getRotation(), limitMS );
			limitMS.normalize3();

			Float limitAngleRad = DEG2RAD( m_limitAngle );
			hkReal cosLimitAngle = hkMath::cos( limitAngleRad );
			hkReal cosTargetAngle = vecToTargetMS.dot3( limitMS );
			if ( cosTargetAngle < cosLimitAngle )
			{
				hkVector4 cross;
				cross.setCross( limitMS, vecToTargetMS );
				cross.normalize3();

				hkQuaternion q;
				q.setAxisAngle( cross, limitAngleRad );

				vecToTargetMS.setRotatedDir( q, limitMS );
			}

#ifndef NO_EDITOR
			instance[ i_vecToTargetLimited ] = TO_CONST_VECTOR_REF( vecToTargetMS );
#endif
		}

		hkVector4 dirLS; dirLS.setZero4();
		COPY_VECTOR_TO_HK_VECTOR( m_axis, dirLS );

		hkVector4 dirMS; 
		dirMS.setRotatedDir( boneMS.getRotation(), dirLS );
		dirMS.normalize3();

		// Final calc
		hkVector4 cross;
		cross.setCross( dirMS, vecToTargetMS );
		cross.normalize3();

		hkReal cosAngle = vecToTargetMS.dot3( dirMS );
		hkReal angle = hkMath::acos( cosAngle ) * weight;

		hkQuaternion q;
		q.setAxisAngle( cross, angle );

		// Apply final rotation
		boneMS.m_rotation.setMul( q, boneMS.m_rotation );
		boneLS.setMulInverseMul( parentMS, boneMS );

#ifndef NO_EDITOR
		instance[ i_dirMS ] = TO_CONST_VECTOR_REF( dirMS );
#endif
	}

#else
		// Target
		const RedVector4 targetMS = reinterpret_cast< const RedVector4& >( instance[ i_target ] );

		// Bone LS
		RedQsTransform& boneLS = output.m_outputPose[ boneIdx ];

		// Bone MS
		RedQsTransform parentMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), parentIdx );
		RedQsTransform boneMS; boneMS.SetMul( parentMS, boneLS );

#ifndef NO_EDITOR
		instance[ i_boneMS ] = reinterpret_cast< const Vector& >( boneMS.Translation );
#endif

		// Target dir
		RedVector4 vecToTargetMS;
		vecToTargetMS = Sub( targetMS, boneMS.Translation );
		Float targetDist; targetDist = vecToTargetMS.Length3();
		vecToTargetMS.Normalize3();

#ifndef NO_EDITOR
		instance[ i_vecToTarget ] = reinterpret_cast< const Vector& >( vecToTargetMS );
		instance[ i_vecToTargetLimited ] =reinterpret_cast< const Vector& >( vecToTargetMS );
#endif

		// Limits
		if ( m_useLimits )
		{
			RedVector4 limitLS( reinterpret_cast< const RedVector4& >( m_axis ) ); 

			RedVector4 limitMS; 
			limitMS.RotateDirection( parentMS.GetRotation(), limitMS );
			limitMS.Normalize3();

			Float limitAngleRad = DEG2RAD( m_limitAngle );
			Float cosLimitAngle = Red::Math::MCos( limitAngleRad );
			Float cosTargetAngle = Dot3(vecToTargetMS, limitMS );
			if ( cosTargetAngle < cosLimitAngle )
			{
				RedVector4 cross( Cross( limitMS, vecToTargetMS ) );
				cross.Normalize3();

				RedQuaternion q;
				q.SetAxisAngle( cross, limitAngleRad );

				vecToTargetMS.RotateDirection( q, limitMS );
			}

#ifndef NO_EDITOR
			instance[ i_vecToTargetLimited ] = reinterpret_cast< const Vector& >( vecToTargetMS );
#endif
		}

		RedVector4 dirLS( reinterpret_cast< const RedVector4& >( m_axis ) ); 

		RedVector4 dirMS; 
		dirMS.RotateDirection( boneMS.GetRotation(), dirLS );
		dirMS.Normalize3();

		// Final calc
		RedVector4 cross( Cross( dirMS, vecToTargetMS ) );
		cross.Normalize3();

		Float cosAngle = Dot3( vecToTargetMS, dirMS );
		Float angle = Red::Math::MAcos_safe( cosAngle ) * weight;//hkMath::acos( cosAngle ) * weight;

		RedQuaternion q;
		q.SetAxisAngle( cross, angle );

		// Apply final rotation
		boneMS.Rotation.SetMul( q, boneMS.Rotation );
		boneLS.SetMulInverseMul( parentMS, boneMS );

#ifndef NO_EDITOR
		instance[ i_dirMS ] = reinterpret_cast< const Vector& >( dirMS );
#endif
	}

#endif
}


void CBehaviorGraphLookAtNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Activate( instance );
	}
}

void CBehaviorGraphLookAtNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->Deactivate( instance );
	}
}

void CBehaviorGraphLookAtNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedTargetNode = CacheVectorValueBlock( TXT("Target") );
}

void CBehaviorGraphLookAtNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetNode )
	{
		m_cachedTargetNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphLookAtNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_generateEditorFragments )
	{
		const Matrix& l2w = instance.GetAnimatedComponent()->GetLocalToWorld();

		const Vector& targetMS = instance[ i_target ];
		const Vector targetWS = l2w.TransformPoint( targetMS );

		frame->AddDebugSphere( targetWS, 0.05f, Matrix::IDENTITY, Color::RED );

#ifndef NO_EDITOR
		const Vector& boneMS = instance[ i_boneMS ];
		const Vector& dirMS = instance[ i_dirMS ];

		const Vector boneWS = l2w.TransformPoint( boneMS );
		const Vector& dirWS = l2w.TransformVector( dirMS );

		frame->AddDebugLine( boneWS, boneWS + dirMS * 5.f, Color::YELLOW );

		const Vector& vecToTarget = instance[ i_vecToTarget ];
		const Vector& vecToTargetLimited = instance[ i_vecToTargetLimited ];

		const Vector vecToTargetWS = l2w.TransformVector( vecToTarget );
		const Vector vecToTargetLimitedWS = l2w.TransformVector( vecToTargetLimited );

		frame->AddDebugLine( boneWS, boneWS + vecToTargetWS * 0.25f, Color::GREEN );
		frame->AddDebugLine( boneWS, boneWS + vecToTargetLimitedWS * 5.f, Color::GREEN );
#endif
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphIk2Node );

CBehaviorGraphIk2Node::CBehaviorGraphIk2Node()
	: m_hingeAxis( A_Y )
	, m_angleMax( 180.f )
	, m_angleMin( 0.f )
	, m_firstJointGain( 1.0f )
	, m_secondJointGain( 1.0f )
	, m_endJointGain( 1.f )
	, m_enforceEndPosition( true )
	, m_enforceEndRotation( false )
	, m_positionOffset( Vector::ZERO_3D_POINT )
	, m_rotationOffset( EulerAngles::ZEROS )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIk2Node::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetQuatRot ) ) );
}

#endif

void CBehaviorGraphIk2Node::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_firstJointIdx;
	compiler << i_secondJointIdx;
	compiler << i_endBoneIdx;

	compiler << i_weight;

	compiler << i_targetPos;
	compiler << i_targetRot;
}

void CBehaviorGraphIk2Node::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_firstJointIdx ] = FindBoneIndex( m_firstBone, instance );
	instance[ i_secondJointIdx ] = FindBoneIndex( m_secondBone, instance );
	instance[ i_endBoneIdx ] = FindBoneIndex( m_endBone, instance );

	instance[ i_weight ] = 0.f;
	instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	instance[ i_targetRot ] = Vector::ZERO_3D_POINT;

	CheckBones( instance );
}

void CBehaviorGraphIk2Node::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_firstJointIdx );
	INST_PROP( i_secondJointIdx );
	INST_PROP( i_endBoneIdx );
	INST_PROP( i_weight );
	INST_PROP( i_targetPos );
	INST_PROP( i_targetRot );
}

void CBehaviorGraphIk2Node::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SimpleIk2 );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedValueNode->GetValue( instance );
	}
	else
	{
		instance[ i_weight ] = 0.f;
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Update( context, instance, timeDelta );

		instance[ i_targetPos ] = m_cachedTargetPosNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Update( context, instance, timeDelta );

		instance[ i_targetRot ] = m_cachedTargetRotNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_targetRot ] = Vector::ZERO_3D_POINT;
	}
}

void CBehaviorGraphIk2Node::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( SimpleIk2 );

	TBaseClass::Sample( context, instance, output );

	const Int32 firstJointIdx = instance[ i_firstJointIdx ];

	//const Int32 secondJointIdx = instance[ i_secondJointIdx ];
	//const Int32 endBoneIdx = instance[ i_endBoneIdx ];

	//const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	//const CSkeleton* skeleton = ac->GetSkeleton();

	if ( firstJointIdx != -1 )
	{
#ifdef USE_HAVOK_ANIMATION
		hkaTwoJointsIkSolver::Setup solverSetup;
		SetupSolver( instance, solverSetup );

		const Vector& targetVec = instance[ i_targetPos ];
		solverSetup.m_endTargetMS = TO_CONST_HK_VECTOR_REF( targetVec );

		if ( m_enforceEndRotation )
		{
			const Vector& targetRotVec = instance[ i_targetRot ];
			solverSetup.m_endTargetRotationMS.m_vec = TO_CONST_HK_VECTOR_REF( targetRotVec );
		}
		else
		{
			solverSetup.m_endTargetRotationMS = hkQuaternion::getIdentity();
		}

		const hkaSkeleton *havokSkeleton = skeleton->GetHavokSkeleton();
		hkaPose pose( havokSkeleton );
		SyncPoseFromOutput( pose, output );

		Bool isLimit = hkaTwoJointsIkSolver::solve( solverSetup, pose );

		output.m_outputPose[ firstJointIdx ]	= pose.getBoneLocalSpace( firstJointIdx );
		output.m_outputPose[ secondJointIdx ]	= pose.getBoneLocalSpace( secondJointIdx );
		output.m_outputPose[ endBoneIdx ]		= pose.getBoneLocalSpace( endBoneIdx );
#else
		//hkaTwoJointsIkSolver::Setup solverSetup;
		//SetupSolver( instance, solverSetup );

		//const Vector& targetVec = instance[ i_targetPos ];
		//solverSetup.m_endTargetMS = TO_CONST_HK_VECTOR_REF( targetVec );

		//if ( m_enforceEndRotation )
		//{
		//	const Vector& targetRotVec = instance[ i_targetRot ];
		//	solverSetup.m_endTargetRotationMS.m_vec = TO_CONST_HK_VECTOR_REF( targetRotVec );
		//}
		//else
		//{
		//	solverSetup.m_endTargetRotationMS = hkQuaternion::getIdentity();
		//}

		//const hkaSkeleton *havokSkeleton = skeleton->GetHavokSkeleton();
		//hkaPose pose( havokSkeleton );
		//SyncPoseFromOutput( pose, output );

		//Bool isLimit = hkaTwoJointsIkSolver::solve( solverSetup, pose );

		//output.m_outputPose[ firstJointIdx ]	= pose.getBoneLocalSpace( firstJointIdx );
		//output.m_outputPose[ secondJointIdx ]	= pose.getBoneLocalSpace( secondJointIdx );
		//output.m_outputPose[ endBoneIdx ]		= pose.getBoneLocalSpace( endBoneIdx );
		//GPUAPI_LOG_WARNING "Needs to be implemented" ) );
#endif
	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphIk2Node::SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const
{
	hkArray<hkQsTransform>& poseLocalTrans = pose.accessUnsyncedPoseLocalSpace();

	Int32 numBones = Min( (Int32)output.m_numBones, poseLocalTrans.getSize() );

	for ( Int32 i=0; i<numBones; i++ )
	{
		poseLocalTrans[i] = output.m_outputPose[i];
	}

	pose.syncModelSpace();
}
#endif

void CBehaviorGraphIk2Node::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Activate( instance );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Activate( instance );
	}
}

void CBehaviorGraphIk2Node::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Deactivate( instance );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->Deactivate( instance );
	}
}

void CBehaviorGraphIk2Node::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedTargetPosNode = CacheVectorValueBlock( TXT("Target") );
	m_cachedTargetRotNode = CacheVectorValueBlock( TXT("TargetQuatRot") );
}

void CBehaviorGraphIk2Node::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetRotNode )
	{
		m_cachedTargetRotNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphIk2Node::SetupSolver( CBehaviorGraphInstance& instance, hkaTwoJointsIkSolver::Setup& solver ) const
{
	hkVector4 hinge;
	COPY_VECTOR_TO_HK_VECTOR( BehaviorUtils::VectorFromAxis( m_hingeAxis ), hinge );
	solver.m_hingeAxisLS = hinge;

	const Float weight = instance[ i_weight ];

	solver.m_firstJointIkGain = weight * m_firstJointGain;
	solver.m_secondJointIkGain = weight * m_secondJointGain;
	solver.m_endJointIkGain = weight * m_endJointGain;

	solver.m_cosineMinHingeAngle = MCos( DEG2RAD( m_angleMin ) );
	solver.m_cosineMaxHingeAngle = MCos( DEG2RAD( m_angleMax ) );

	solver.m_endBoneOffsetLS.set( m_positionOffset.X, m_positionOffset.Y, m_positionOffset.Z );

	hkQuaternion q;
	EulerAnglesToHavokQuaternion( m_rotationOffset, q );
	solver.m_endBoneRotationOffsetLS = q;

	solver.m_enforceEndPosition = m_enforceEndPosition;
	solver.m_enforceEndRotation = m_enforceEndRotation;

	solver.m_firstJointIdx = (hkInt16)instance[ i_firstJointIdx ];
	solver.m_secondJointIdx = (hkInt16)instance[ i_secondJointIdx ];
	solver.m_endBoneIdx = (hkInt16)instance[ i_endBoneIdx ];
}
#endif

void CBehaviorGraphIk2Node::CheckBones( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	Int32& firstJointIdx = instance[ i_firstJointIdx ];
	Int32& secondJointIdx = instance[ i_secondJointIdx ];
	Int32& endBoneIdx = instance[ i_endBoneIdx ];

	if ( animatedComponent && animatedComponent->GetSkeleton() )
	{
		const CSkeleton* skeleton = animatedComponent->GetSkeleton();

		Int32 secondDepth = -1;
		Int32 firstDepth = -1;
		Int32 depth = 0;
		Int32 currentParent = endBoneIdx;

		while ( currentParent!=-1 )
		{
			if ( currentParent == secondJointIdx )
			{
				secondDepth = depth;
			}
			if ( currentParent == firstJointIdx )
			{
				firstDepth = depth;
			}

			currentParent = skeleton->GetParentBoneIndex(currentParent);
			depth++;
		}

		if ( !( secondDepth > 0 && firstDepth > 0 && secondDepth < firstDepth ) )
		{
			BEH_WARN( TXT("Order of joints/bones is incorrect. Select bones from one chain ( CBehaviorGraphConstraintLookAtNode '%ls', graph '%ls' ).") , GetBlockName().AsChar(), GetGraph()->GetFriendlyName().AsChar() );

			// Prevent IK calc - reset bones
			firstJointIdx = -1;
			secondJointIdx = -1;
			endBoneIdx = -1;
		}
	}
	else
	{
		firstJointIdx = -1;
		secondJointIdx = -1;
		endBoneIdx = -1;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIk2Node::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_generateEditorFragments )
	{
		if ( m_enforceEndPosition )
		{
			frame->AddDebugSphere( instance[ i_targetPos ], 0.05f, Matrix::IDENTITY, Color::RED );
		}

		if ( m_enforceEndRotation )
		{
			Matrix rot;
			AnimQsTransform trans( AnimQsTransform::IDENTITY );

#ifdef USE_HAVOK_ANIMATION
			trans.m_rotation.m_vec = TO_CONST_HK_VECTOR_REF( instance[ i_targetRot ] );
			HavokTransformToMatrix_Renormalize( trans, &rot );
#else
			trans.Rotation.Quat = reinterpret_cast< const RedVector4& >( instance[ i_targetRot ] );
			RedMatrix4x4 conversionMatrix = trans.ConvertToMatrixNormalized();
			rot = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif

			frame->AddDebugAxis( instance[ i_targetPos ], rot, 0.1f );
		}
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphIk3Node );

CBehaviorGraphIk3Node::CBehaviorGraphIk3Node()
	: m_hingeAxis( A_Y )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIk3Node::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Target ) ) );
}

#endif

void CBehaviorGraphIk3Node::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_firstJointIdx;
	compiler << i_secondJointIdx;
	compiler << i_thirdJointIdx;
	compiler << i_endBoneIdx;

	compiler << i_weight;
	compiler << i_targetPos;
}

void CBehaviorGraphIk3Node::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_firstJointIdx ] = FindBoneIndex( m_firstBone, instance );
	instance[ i_secondJointIdx ] = FindBoneIndex( m_secondBone, instance );
	instance[ i_thirdJointIdx ] = FindBoneIndex( m_thirdBone, instance );
	instance[ i_endBoneIdx ] = FindBoneIndex( m_endBone, instance );

	instance[ i_weight ] = 0.f;
	instance[ i_targetPos ] = Vector::ZERO_3D_POINT;

	CheckBones( instance );
}

void CBehaviorGraphIk3Node::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_firstJointIdx );
	INST_PROP( i_secondJointIdx );
	INST_PROP( i_thirdJointIdx );
	INST_PROP( i_endBoneIdx );
	INST_PROP( i_weight );
	INST_PROP( i_targetPos );
}

void CBehaviorGraphIk3Node::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SimpleIk3 );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedValueNode->GetValue( instance );
	}
	else
	{
		instance[ i_weight ] = 0.f;
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Update( context, instance, timeDelta );

		instance[ i_targetPos ] = m_cachedTargetPosNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_targetPos ] = Vector::ZERO_3D_POINT;
	}
}

void CBehaviorGraphIk3Node::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( SimpleIk3 );

	TBaseClass::Sample( context, instance, output );

	const Int32 firstJointIdx = instance[ i_firstJointIdx ];
	//const Int32 secondJointIdx = instance[ i_secondJointIdx ];
	//const Int32 thirdJointIdx = instance[ i_thirdJointIdx ];
	//const Int32 endBoneIdx = instance[ i_endBoneIdx ];

	//const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	//const CSkeleton* skeleton = ac->GetSkeleton();

	if ( firstJointIdx != -1 )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkaSkeleton *havokSkeleton = skeleton->GetHavokSkeleton();
		hkaPose pose( havokSkeleton );
		SyncPoseFromOutput( pose, output );

		hkaThreeJointsIkSolver::Setup solverSetup;
		SetupSolver( instance, pose, solverSetup );
#else
		/*const hkaSkeleton *havokSkeleton = skeleton->GetHavokSkeleton();
		hkaPose pose( havokSkeleton );
		SyncPoseFromOutput( pose, output );

		hkaThreeJointsIkSolver::Setup solverSetup;
		SetupSolver( instance, pose, solverSetup );*/
		//GPUAPI_LOG_WARNING "Needs to be implemented" ) );
#endif
#ifdef USE_HAVOK_ANIMATION
		const Vector& targetVec = instance[ i_targetPos ];
		const hkVector4& target = TO_CONST_HK_VECTOR_REF( targetVec );
#else
		//const RedVector4& target = reinterpret_cast< const RedVector4& >( instance[ i_targetPos ] );
#endif
#ifdef USE_HAVOK_ANIMATION
		hkaThreeJointsIkSolver solver( solverSetup, pose );
		hkResult isLimit = solver.solve( target, pose );
		
		output.m_outputPose[ firstJointIdx ]	= pose.getBoneLocalSpace( firstJointIdx );
		output.m_outputPose[ secondJointIdx ]	= pose.getBoneLocalSpace( secondJointIdx );
		output.m_outputPose[ thirdJointIdx ]	= pose.getBoneLocalSpace( thirdJointIdx );
		output.m_outputPose[ endBoneIdx ]		= pose.getBoneLocalSpace( endBoneIdx );
#else
//		hkaThreeJointsIkSolver solver( solverSetup, pose );
//		hkResult isLimit = solver.solve( target, pose );

//		output.m_outputPose[ firstJointIdx ]	= pose.getBoneLocalSpace( firstJointIdx );
//		output.m_outputPose[ secondJointIdx ]	= pose.getBoneLocalSpace( secondJointIdx );
//		output.m_outputPose[ thirdJointIdx ]	= pose.getBoneLocalSpace( thirdJointIdx );
//		output.m_outputPose[ endBoneIdx ]		= pose.getBoneLocalSpace( endBoneIdx );
//		GPUAPI_LOG_WARNING "Needs to be implemented" ) );
#endif

	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphIk3Node::SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const
{
	hkArray<hkQsTransform>& poseLocalTrans = pose.accessUnsyncedPoseLocalSpace();

	Int32 numBones = Min( (Int32)output.m_numBones, poseLocalTrans.getSize() );

	for ( Int32 i=0; i<numBones; i++ )
	{
		poseLocalTrans[i] = output.m_outputPose[i];
	}

	pose.syncModelSpace();
}
#endif

void CBehaviorGraphIk3Node::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Activate( instance );
	}
}

void CBehaviorGraphIk3Node::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->Deactivate( instance );
	}
}

void CBehaviorGraphIk3Node::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedTargetPosNode = CacheVectorValueBlock( TXT("Target") );
}

void CBehaviorGraphIk3Node::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetPosNode )
	{
		m_cachedTargetPosNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphIk3Node::SetupSolver( CBehaviorGraphInstance& instance, const hkaPose &pose, hkaThreeJointsIkSolver::Setup& solver ) const
{
	hkVector4 hinge;
	COPY_VECTOR_TO_HK_VECTOR( BehaviorUtils::VectorFromAxis( m_hingeAxis ), hinge );

	const Float weight = instance[ i_weight ];

	const Int32 firstBoneIdx = instance[ i_firstJointIdx ];
	ASSERT( firstBoneIdx >= 0 );

	const hkQsTransform& boneMS = pose.getBoneModelSpace( firstBoneIdx );
	solver.m_hingeAxisMS.setRotatedDir( boneMS.m_rotation, hinge );

	solver.m_firstJointIdx = (hkInt16)instance[ i_firstJointIdx ];
	solver.m_secondJointIdx = (hkInt16)instance[ i_secondJointIdx ];
	solver.m_thirdJointIdx = (hkInt16)instance[ i_thirdJointIdx ];
	solver.m_endBoneIdx = (hkInt16)instance[ i_endBoneIdx ];
}
#endif

void CBehaviorGraphIk3Node::CheckBones( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	Int32& firstJointIdx = instance[ i_firstJointIdx ];
	Int32& secondJointIdx = instance[ i_secondJointIdx ];
	Int32& thirdJointIdx = instance[ i_thirdJointIdx ];
	Int32& endBoneIdx = instance[ i_endBoneIdx ];
	if ( animatedComponent && animatedComponent->GetSkeleton() )
	{
		const CSkeleton* skeleton = animatedComponent->GetSkeleton();
		Int32 thirdDepth = -1;
		Int32 secondDepth = -1;
		Int32 firstDepth = -1;
		Int32 depth = 0;
		Int32 currentParent = endBoneIdx;

		while ( currentParent!=-1 )
		{
			if ( currentParent == thirdJointIdx )
			{
				thirdDepth = depth;
			}
			if ( currentParent == secondJointIdx )
			{
				secondDepth = depth;
			}
			if ( currentParent == firstJointIdx )
			{
				firstDepth = depth;
			}

			currentParent = skeleton->GetParentBoneIndex(currentParent);
			depth++;
		}

		if ( !( secondDepth > 0 && firstDepth > 0 && thirdDepth> 0 && thirdDepth < secondDepth && secondDepth < firstDepth ) )
		{
			BEH_WARN( TXT("Order of joints/bones is incorrect. Select bones from one chain ( CBehaviorGraphConstraintLookAtNode '%ls', graph '%ls' ).") , GetBlockName().AsChar(), GetGraph()->GetFriendlyName().AsChar() );

			// Prevent IK calc - reset bones
			firstJointIdx = -1;
			secondJointIdx = -1;
			thirdJointIdx = -1;
			endBoneIdx = -1;
		}
	}
	else
	{
		firstJointIdx = -1;
		secondJointIdx = -1;
		thirdJointIdx = -1;
		endBoneIdx = -1;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIk3Node::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_generateEditorFragments )
	{
		frame->AddDebugSphere( instance[ i_targetPos ], 0.05f, Matrix::IDENTITY, Color::RED );
	}
}

#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
