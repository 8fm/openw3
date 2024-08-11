
#pragma once

/*
#include "../../common/engine/controlRigAlgorithms.h"

enum EControlRigEffector
{
	CRE_Hips,
	CRE_ChestOrigin,
	CRE_ChestEnd,
	CRE_Head,

	CRE_LeftShoulder,
	CRE_LeftElbow,
	CRE_LeftWrist,
	CRE_LeftHip,
	CRE_LeftKnee,
	CRE_LeftAnkle,
	CRE_LeftFoot,

	CRE_RightShoulder,
	CRE_RightElbow,
	CRE_RightWrist,
	CRE_RightHip,
	CRE_RightKnee,
	CRE_RightAnkle,
	CRE_RightFoot,

	CRE_Last
};

struct SControlRigSetup
{
	struct SHipRotation
	{
		Float m_rotationZ;
		Float m_offsetZ;

		SHipRotation() : m_rotationZ( 0.f ), m_offsetZ( 0.f ) {}

		RED_INLINE Bool IsSet() const { return IsRotSet() || IsTransSet(); }
		RED_INLINE Bool IsRotSet() const { return MAbs( m_rotationZ ) > 0.001f; }
		RED_INLINE Bool IsTransSet() const { return MAbs( m_offsetZ ) > 0.001f; }

	};

	struct SChestIk
	{
		Bool	m_isSet;
		Vector	m_offset;
		Float	m_weight;
		Float	m_weightSpine1;
		Float	m_weightSpine2;
		Float	m_weightSpine3;
		EAxis	m_boneDirection1;
		EAxis	m_boneDirection2;
		EAxis	m_boneDirection3;

		SChestIk() : m_isSet( false ), m_offset( Vector::ZERO_3D_POINT ), m_weight( 1.f ), m_weightSpine1( 1.f ), m_weightSpine2( 1.f ), m_weightSpine3( 1.f ), m_boneDirection1( A_X ), m_boneDirection2( A_X ) , m_boneDirection3( A_X )  {}

		RED_INLINE Bool IsSet() const { return m_isSet; }
		RED_INLINE void Set( const Vector& offset ) { m_offset = offset; m_isSet = true; }
		RED_INLINE void Reset() { m_isSet = false; }
	};

	struct SLegIk
	{
		Float	m_weight;
		Float	m_weightFirst;
		Float	m_weightSecond;
		Float	m_weightEnd;
		Bool	m_enforceRotation;
		Float	m_cosineMaxHingeAngle;
		Float	m_cosineMinHingeAngle;

		SLegIk() : m_weight( 1.f ), m_weightFirst( 1.f ), m_weightSecond( 1.f ), m_weightEnd( 1.f ), m_enforceRotation( false ), m_cosineMaxHingeAngle( -1.0f ), m_cosineMinHingeAngle( 1.0f ) {}
	};

	struct SArmIk
	{
		Float	m_weight;
		Float	m_weightFirst;
		Float	m_weightSecond;
		Float	m_weightEnd;
		Bool	m_enforceRotation;
		Float	m_cosineMaxHingeAngle;
		Float	m_cosineMinHingeAngle;
		Vector	m_targetMS;

		SArmIk() : m_weight( 1.f ), m_weightFirst( 1.f ), m_weightSecond( 1.f ), m_weightEnd( 1.f ), m_enforceRotation( false ), m_cosineMaxHingeAngle( -1.0f ), m_cosineMinHingeAngle( 1.0f ), m_targetMS( Vector::ZERO_3D_POINT ) {}
	};

	SHipRotation	m_hipRotation;

	SLegIk			m_leftLeg;
	SLegIk			m_rightLeg;

	SChestIk		m_chest;

	SArmIk			m_leftArm;
	SArmIk			m_rightArm;

	SControlRigSetup() {}
};

class CControlRig
{
public:
	struct Setup
	{
		const CSkeleton*	m_skeleton;

		Int32					m_hipsIdx;
		EAxis				m_hipsZ;

		Int32					m_leftHipIdx;
		Int32					m_leftKneeIdx;
		Int32					m_leftAnkleIdx;

		Int32					m_rightHipIdx;
		Int32					m_rightKneeIdx;
		Int32					m_rightAnkleIdx;
	};

private:
	const CSkeleton*	m_skeleton;

	EAxis				m_hipsZ;

	Ik2Solver::Setup	m_setupLegLeft;
	Ik2Solver::Setup	m_setupLegRight;

	Ik2Solver::Setup	m_setupArmLeft;
	Ik2Solver::Setup	m_setupArmRight;

	SphericalChainIk3Solver::Setup m_setupChest;

public:
	CControlRig( const Setup& setup );
	~CControlRig();

	Bool Solve( const SControlRigSetup& data, SBehaviorGraphOutput& pose );

private:
	void RotateBoneLS( hkQsTransform& bone, Float rotationDeg, EAxis axis ) const;
	void TranslateBoneLS( hkQsTransform& bone, Float offset, EAxis axis ) const;

	void FillLegSetup( Ik2Solver::Setup& ikSetup, const Setup& rigSetup, Bool left ) const;
	void FillArmSetup( Ik2Solver::Setup& ikSetup, const Setup& rigSetup, Bool left ) const;
	void FillChestSetup( SphericalChainIk3Solver::Setup& ikSetup, const Setup& rigSetup ) const;

	Bool ChestIk( const hkQsTransform& pelvisMS, SBehaviorGraphOutput& pose, const SControlRigSetup::SChestIk& chest, hkQsTransform& spineEndMS );
};
*/
