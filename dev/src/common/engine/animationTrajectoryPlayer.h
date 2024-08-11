
#pragma once

#include "animationSelectors.h"
#include "behaviorGraphAnimationManualSlot.h"
#include "animationPlayerTimeController.h"
#include "animationTrajectorySync.h"
#include "visualDebug.h"

class CSkeletalAnimationAttackTrajectoryParam;
class AnimationTrajectoryPlayer_State;
class IActorInterface;

//////////////////////////////////////////////////////////////////////////

enum EAnimationTrajectorySelectorType
{
	ATST_None,
	ATST_IK,
	ATST_Blend2,
	ATST_Blend3,
	ATST_Blend2Direction,
};

BEGIN_ENUM_RTTI( EAnimationTrajectorySelectorType );
	ENUM_OPTION( ATST_None );
	ENUM_OPTION( ATST_IK );
	ENUM_OPTION( ATST_Blend2 );
	ENUM_OPTION( ATST_Blend3 );
	ENUM_OPTION( ATST_Blend2Direction );
END_ENUM_RTTI();

struct SAnimationTrajectoryPlayerInput
{
	DECLARE_RTTI_STRUCT( SAnimationTrajectoryPlayerInput );

	Matrix								m_localToWorld;
	Vector								m_pointWS;
	Vector								m_directionWS;
	CName								m_tagId;
	EAnimationTrajectorySelectorType	m_selectorType;
	EActionMoveAnimationSyncType		m_proxySyncType;
	THandle< CActionMoveAnimationProxy > m_proxy;
	
	SAnimationTrajectoryPlayerInput()
		: m_pointWS( Vector::ZERO_3D_POINT ), m_proxySyncType( AMAST_None ) {}
};

BEGIN_CLASS_RTTI( SAnimationTrajectoryPlayerInput );
	PROPERTY_EDIT( m_localToWorld, String::EMPTY );
	PROPERTY_EDIT( m_pointWS, String::EMPTY );
	PROPERTY_EDIT( m_directionWS, String::EMPTY );
	PROPERTY_EDIT( m_tagId, String::EMPTY );
	PROPERTY_EDIT( m_selectorType, String::EMPTY );
	PROPERTY_EDIT( m_proxySyncType, String::EMPTY );
	PROPERTY_EDIT( m_proxy, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SAnimationTrajectoryPlayerToken
{
	DECLARE_RTTI_STRUCT( SAnimationTrajectoryPlayerToken );

	Bool		m_isValid;
	Vector		m_pointWS;
	Float		m_weightA;
	Float		m_weightB;
	Vector		m_syncPointMS;
	Float		m_syncPointDuration;
	Float		m_timeFactor;
	Float		m_blendIn;
	Float		m_blendOut;
	Float		m_duration;
	Float		m_syncTime;
	CName		m_tagId;

	EAnimationTrajectorySelectorType				m_selectorType;
	EActionMoveAnimationSyncType					m_proxySyncType;
	THandle< CActionMoveAnimationProxy >			m_proxy;

	const CSkeletalAnimationSetEntry*				m_animationA;
	const CSkeletalAnimationSetEntry*				m_animationB;
	const CSkeletalAnimationSetEntry*				m_animationC;
	const CSkeletalAnimationSetEntry*				m_animationD;

	const CSkeletalAnimationAttackTrajectoryParam*	m_trajectoryParamA;
	const CSkeletalAnimationAttackTrajectoryParam*	m_trajectoryParamB;
	const CSkeletalAnimationAttackTrajectoryParam*	m_trajectoryParamC;
	const CSkeletalAnimationAttackTrajectoryParam*	m_trajectoryParamD;

	SAnimationTrajectoryPlayerToken() 
		: m_isValid( false ), m_pointWS( Vector::ZERO_3D_POINT ), m_syncPointMS( Vector::ZERO_3D_POINT )
		, m_syncPointDuration( 0.f ), m_timeFactor( 1.f )
		, m_blendIn( 0.0f ), m_blendOut( 0.0f ), m_duration( 1.f ), m_syncTime( 0.f )
		, m_weightA( 0.f ), m_weightB( 0.f )
		, m_animationA( NULL ), m_animationB( NULL ), m_animationC( NULL ), m_animationD( NULL )
		, m_trajectoryParamA( NULL ), m_trajectoryParamB( NULL ), m_trajectoryParamC( NULL ), m_trajectoryParamD( NULL )
		, m_selectorType( ATST_None ), m_proxySyncType( AMAST_None ) {}
};

BEGIN_CLASS_RTTI( SAnimationTrajectoryPlayerToken );
	PROPERTY_EDIT( m_isValid, String::EMPTY );
	PROPERTY_EDIT( m_pointWS, String::EMPTY );
	PROPERTY_EDIT( m_syncPointMS, String::EMPTY );
	PROPERTY_EDIT( m_timeFactor, String::EMPTY );
	PROPERTY_EDIT( m_syncPointDuration, String::EMPTY );
	PROPERTY_EDIT( m_blendIn, String::EMPTY );
	PROPERTY_EDIT( m_blendOut, String::EMPTY );
	PROPERTY_EDIT( m_duration, String::EMPTY );
	PROPERTY_EDIT( m_syncTime, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class AnimationTrajectoryPlayer
{
	THandle< CAnimatedComponent >		m_component;

	CName								m_slotName;
	CBehaviorManualSlotInterface		m_slot;

	AnimationTrajectoryPlayer_State*	m_currAnimation;
	AnimationTrajectoryPlayer_State*	m_nextAnimation;

public:
	AnimationTrajectoryPlayer();
	~AnimationTrajectoryPlayer();

	Bool Init( const CEntity* entity, const CName& slotName = CName::NONE );
	void Deinit();

	void Tick( Float dt );

	Float GetTime() const;

	Bool IsPlayingAnimation() const;
	Bool IsBeforeSyncTime() const;

	Bool SelectAnimation( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const;
	Bool PlayAnimation( const SAnimationTrajectoryPlayerToken& token );
	Bool Stop();

	void UpdatePoint( const Vector& pointWS );
	void UpdatePoint( const Matrix& l2w, const Vector& pointWS );

	void GenerateFragments( CRenderFrame* frame );

private:
	RED_INLINE Bool IsValid() const { return m_component.Get() != NULL; }

	void SelectAnimation_IK( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const;
	void SelectAnimation_Blend2( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const;
	void SelectAnimation_Blend3( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const;
	void SelectAnimation_Blend2Direction( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const;

	void ClearState( AnimationTrajectoryPlayer_State*& state );
	void CopyState( AnimationTrajectoryPlayer_State*& src, AnimationTrajectoryPlayer_State*& dest );

	Bool SetupSlot( CAnimatedComponent* ac );
	void CloseSlot();
};

//////////////////////////////////////////////////////////////////////////

class AnimationTrajectoryPlayerScriptWrapper : public CObject, public IVisualDebugInterface
{
	DECLARE_ENGINE_CLASS( AnimationTrajectoryPlayerScriptWrapper, CObject, 0 );

protected:
	AnimationTrajectoryPlayer	m_player;
	IActorInterface*			m_actor;

public:
	AnimationTrajectoryPlayerScriptWrapper();

	virtual void Render( CRenderFrame* frame, const Matrix& matrix );

protected:
	void funcInit( CScriptStackFrame& stack, void* result );
	void funcDeinit( CScriptStackFrame& stack, void* result );
	void funcSelectAnimation( CScriptStackFrame& stack, void* result );
	void funcPlayAnimation( CScriptStackFrame& stack, void* result );
	void funcTick( CScriptStackFrame& stack, void* result );
	void funcIsPlayingAnimation( CScriptStackFrame& stack, void* result );
	void funcIsBeforeSyncTime( CScriptStackFrame& stack, void* result );
	void funcUpdateCurrentPoint( CScriptStackFrame& stack, void* result );
	void funcUpdateCurrentPointM( CScriptStackFrame& stack, void* result );
	void funcGetTime( CScriptStackFrame& stack, void* result );

	void funcWaitForSyncTime( CScriptStackFrame& stack, void* result );
	void funcWaitForFinish( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( AnimationTrajectoryPlayerScriptWrapper );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "Init", funcInit );
	NATIVE_FUNCTION( "Deinit", funcDeinit );
	NATIVE_FUNCTION( "SelectAnimation", funcSelectAnimation );
	NATIVE_FUNCTION( "PlayAnimation", funcPlayAnimation );
	NATIVE_FUNCTION( "Tick", funcTick );
	NATIVE_FUNCTION( "IsPlayingAnimation", funcIsPlayingAnimation );
	NATIVE_FUNCTION( "IsBeforeSyncTime", funcIsBeforeSyncTime );
	NATIVE_FUNCTION( "UpdateCurrentPoint", funcUpdateCurrentPoint );
	NATIVE_FUNCTION( "UpdateCurrentPointM", funcUpdateCurrentPointM );
	NATIVE_FUNCTION( "GetTime", funcGetTime );

	NATIVE_FUNCTION( "WaitForSyncTime", funcWaitForSyncTime );
	NATIVE_FUNCTION( "WaitForFinish", funcWaitForFinish );
END_CLASS_RTTI();
