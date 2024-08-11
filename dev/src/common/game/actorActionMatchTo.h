
#pragma once

#include "animationSlider.h"
#include "../../common/engine/animationTrajectorySync.h"

struct SActionMatchToSettings
{
	DECLARE_RTTI_STRUCT( SActionMatchToSettings );

	CName	m_animation;
	CName	m_slotName;
	Float	m_blendIn;
	Float	m_blendOut;
	Bool	m_useGameTimeScale;
	Bool	m_useRotationDeltaPolicy;

	CName	m_matchBoneName;
	CName	m_matchEventName;

	SActionMatchToSettings() : m_blendIn( 0.f ), m_blendOut( 0.f ), m_useGameTimeScale( true ), m_useRotationDeltaPolicy( false ) {}
};

BEGIN_CLASS_RTTI( SActionMatchToSettings );
	PROPERTY_EDIT( m_animation, String::EMPTY );
	PROPERTY_EDIT( m_slotName, String::EMPTY );
	PROPERTY_EDIT( m_blendIn, String::EMPTY );
	PROPERTY_EDIT( m_blendOut, String::EMPTY );
	PROPERTY_EDIT( m_useGameTimeScale, String::EMPTY );
	PROPERTY_EDIT( m_useRotationDeltaPolicy, String::EMPTY );
	PROPERTY_EDIT( m_matchBoneName, String::EMPTY );
	PROPERTY_EDIT( m_matchEventName, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SActionMatchToTarget
{
	DECLARE_RTTI_STRUCT( SActionMatchToTarget );

	Vector				m_vec;
	THandle< CNode >	m_node;
	Bool				m_isTypeStatic; // static = vec, dynamic = node
	Bool				m_useRot;
	Bool				m_useTrans;

	SActionMatchToTarget();

	void Set( const CNode* node, Bool trans = true, Bool rot = true );
	void Set( const Vector& vec, Float yaw );
	void Set( const Vector& vec );
	void Set( Float yaw );
	void Set( const Vector& vec, Float yaw, Bool trans, Bool rot );

	Bool Get( const Matrix& defaultVal, Matrix& target ) const;

	Bool IsRotationSet() const;
	Bool IsTranslationSet() const;
};

BEGIN_CLASS_RTTI( SActionMatchToTarget );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class ActorActionMatchTo : public ActorAction, public CMoveLocomotion::IController
{
private:
	EActorActionResult						m_status;
	THandle< CActionMoveAnimationProxy >	m_proxy;

public:
	ActorActionMatchTo( CActor* actor );

	Bool Start( const SActionMatchToSettings& settings, const SActionMatchToTarget& target );
	Bool Start( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy );

	virtual void Stop();
	virtual Bool Update( Float timeDelta );

	virtual Bool CanUseLookAt() const { return true; }
	virtual void OnBeingPushed( const Vector& direction, Float rotation, Float speed, EPushingDirection animDirection ) {}

	virtual void OnGCSerialize( IFile& file );

	// -------------------------------------------------------------------------
	// CMoveLocomotion::IController
	// -------------------------------------------------------------------------
	virtual void OnSegmentFinished( EMoveStatus status );
	virtual void OnControllerAttached();
	virtual void OnControllerDetached();

protected:
	void ResetAgentMovementData();
};
