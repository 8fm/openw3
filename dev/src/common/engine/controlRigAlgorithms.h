
#pragma once
#include "behaviorIncludes.h"

class Ik2Solver
{
public:
	struct Input
	{
		RedQsTransform	m_firstJointParentTransformMS;

		RedQsTransform	m_firstJointLS;
		RedQsTransform	m_secondJointLS;
		RedQsTransform	m_endBoneLS;

		RedVector4		m_hingeAxisLS;

		Float			m_cosineMaxHingeAngle;
		Float			m_cosineMinHingeAngle;

		Float			m_firstJointIkGain;
		Float			m_secondJointIkGain;
		Float			m_endJointIkGain;

		Float			m_weight;

		RedVector4		m_endTargetMS;
		RedQuaternion	m_endTargetRotationMS;

		RedVector4		m_endBoneOffsetLS;
		RedQuaternion	m_endBoneRotationOffsetLS;

		Bool			m_enforceEndPosition;
		Bool			m_enforceEndRotation;

		Input()
			: m_firstJointLS( RedQsTransform::IDENTITY )
			, m_secondJointLS( RedQsTransform::IDENTITY )
			, m_endBoneLS( RedQsTransform::IDENTITY )
			, m_hingeAxisLS( 1.0f, 0.0f, 0.0f )
			, m_cosineMaxHingeAngle( -1.0f )
			, m_cosineMinHingeAngle( 1.0f )
			, m_firstJointIkGain( 1.0f )
			, m_secondJointIkGain( 1.0f )
			, m_endJointIkGain( 1.0f )
			, m_endTargetMS( 0.0f, 0.0f, 0.0f )
			, m_endTargetRotationMS( RedQuaternion::IDENTITY )
			, m_endBoneOffsetLS( 0.0f, 0.0f, 0.0f )
			, m_endBoneRotationOffsetLS( RedQuaternion::IDENTITY )
			, m_enforceEndPosition( true )
			, m_enforceEndRotation( false )
			, m_firstJointParentTransformMS( RedQsTransform::IDENTITY )
			, m_weight( 1.f )
		{}
	};

	struct Output
	{
		RedQsTransform	m_firstJointMS;
		RedQsTransform	m_secondJointMS;
		RedQsTransform	m_endBoneMS;

		RedQsTransform	m_firstJointLS;
		RedQsTransform	m_secondJointLS;
		RedQsTransform	m_endBoneLS;

		Output()
			: m_firstJointMS( RedQsTransform::IDENTITY )
			, m_secondJointMS( RedQsTransform::IDENTITY )
			, m_endBoneMS( RedQsTransform::IDENTITY )
			, m_firstJointLS( RedQsTransform::IDENTITY )
			, m_secondJointLS( RedQsTransform::IDENTITY )
			, m_endBoneLS( RedQsTransform::IDENTITY )
		{

		}
	};

	static Bool Solve( const Input& input, Output& output );

	//static Bool CalcEndBoneMS( const Setup& setup, const SBehaviorGraphOutput& poseLSInOut, hkQsTransform& out );
};

//////////////////////////////////////////////////////////////////////////

class LookAtSolver
{
public:
	struct Setup
	{

		RedQsTransform	m_parentMS;

		EAxis			m_boneDirection;
		Float			m_weight;
		Vector			m_localChildBoneOffset;
		Vector			m_targetMS;

		Bool			m_useLimits;
		Float			m_limitAngle;
		EAxis			m_limitBoneFwdDirection;

		Bool			m_useRangeLimits;
		Float			m_limitAngleUp;   // [ -pi/2, pi/2 ]
		Float			m_limitAngleDown; // [ -pi/2, pi/2 ]
		Float			m_limitAngleLeft;  // [ -pi, pi ]
		Float			m_limitAngleRight; // [ -pi, pi ]
		EAxis			m_limitBoneUpDirection;

		Setup() 
			: m_parentMS( RedQsTransform::IDENTITY )
			, m_localChildBoneOffset( Vector::ZERO_3D_POINT)
			, m_boneDirection( A_X ), m_limitBoneFwdDirection( A_X ), m_limitBoneUpDirection( A_Z )
			, m_useLimits( false ), m_limitAngle( 0.f )
			, m_weight( 1.f ), m_targetMS( Vector::ZERO_3D_POINT )
			, m_useRangeLimits( false ) 
		{}
	};

	static Bool Solve( const Setup& setup, RedQsTransform& boneInOutLS );
	static Bool Solve( const Setup& setup, RedQsTransform& boneInOutLS, RedQsTransform& boneOutMS );
};

//////////////////////////////////////////////////////////////////////////

class SphericalChainIk3Solver
{
public:
	struct Setup
	{
		Int32			m_firstJointIdx;
		Int32			m_secondJointIdx;
		Int32			m_endBoneIdx;

		EAxis			m_boneDirection1;
		EAxis			m_boneDirection2;
		EAxis			m_boneDirection3;
		RedQsTransform	m_firstJointParentTransformMS;

		Vector			m_endTargetMS;

		Float			m_firstJointIkGain;
		Float			m_secondJointIkGain;
		Float			m_endJointIkGain;

		Float			m_weight;

		Setup()
			: m_firstJointIdx( -1 )
			, m_secondJointIdx( -1 )
			, m_endBoneIdx( -1 )
			, m_boneDirection1( A_X )
			, m_boneDirection2( A_X )
			, m_boneDirection3( A_X )
			, m_endTargetMS( Vector::ZERO_3D_POINT )
			, m_firstJointParentTransformMS( RedQsTransform::IDENTITY )
			, m_firstJointIkGain( 1.f )
			, m_secondJointIkGain( 1.f )
			, m_endJointIkGain( 1.f )
			, m_weight( 1.f )
		{}
	};

	static Bool Solve( const Setup& setup, SBehaviorGraphOutput& poseLSInOut );
	static Bool Solve( const Setup& setup, SBehaviorGraphOutput& poseLSInOut, RedQsTransform& endBoneMSOut );
};

//////////////////////////////////////////////////////////////////////////

class Ik2Baker
{
public:
	struct Input
	{
		RedQsTransform	m_firstJointParentTransformMS;

		RedQsTransform	m_firstJointLS;
		RedQsTransform	m_secondJointLS;
		RedQsTransform	m_endBoneLS;

		RedVector4		m_hingeAxisLS;

		RedVector4		m_endTargetMS;
		RedQuaternion	m_endTargetRotationMS;

		Bool			m_enforceEndPosition;
		Bool			m_enforceEndRotation;

		Input()
			: m_firstJointParentTransformMS( RedQsTransform::IDENTITY )

			, m_firstJointLS( RedQsTransform::IDENTITY )
			, m_secondJointLS( RedQsTransform::IDENTITY )
			, m_endBoneLS( RedQsTransform::IDENTITY )

			, m_hingeAxisLS( 1.0f, 0.0f, 0.0f )

			, m_endTargetMS( 0.0f, 0.0f, 0.0f )
			, m_endTargetRotationMS( RedQuaternion::IDENTITY )

			, m_enforceEndPosition( true )
			, m_enforceEndRotation( false )
		{}
	};

	struct Output
	{
		RedQsTransform	m_firstBoneAdditive;
		RedQsTransform	m_secondBoneAdditive;
		RedQsTransform	m_endBoneAdditive;

		Output()
			: m_firstBoneAdditive( RedQsTransform::IDENTITY )
			, m_secondBoneAdditive( RedQsTransform::IDENTITY )
			, m_endBoneAdditive( RedQsTransform::IDENTITY )
		{

		}
	};

	static Bool Bake( const Input& input, Output& output );
};
