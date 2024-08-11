
#pragma once

#include "comboDefinition.h"
#include "../../common/engine/behaviorGraphAnimationManualSlot.h"
#include "../../common/game/animationSlider.h"
#include "../../common/engine/visualDebug.h"
#include "../../common/core/set.h"

enum EComboAttackType
{
	ComboAT_Normal,
	ComboAT_Directional,
	ComboAT_Restart,
	ComboAT_Stop,
};

BEGIN_ENUM_RTTI( EComboAttackType );
	ENUM_OPTION( ComboAT_Normal )
	ENUM_OPTION( ComboAT_Directional )
	ENUM_OPTION( ComboAT_Restart )
	ENUM_OPTION( ComboAT_Stop )
END_ENUM_RTTI();

struct SComboAttackCallbackInfo
{
	DECLARE_RTTI_STRUCT( SComboAttackCallbackInfo )

	EAttackDirection	m_outDirection;
	EAttackDistance		m_outDistance;
	Bool                m_outShouldRotate;
    Float				m_outRotateToEnemyAngle;
    Vector				m_outSlideToPosition;
    Bool                m_outShouldTranslate;
	Bool				m_outLeftString;

	EComboAttackType	m_outAttackType;

	CName				m_inAspectName;
	Bool				m_prevDirAttack;
	Int32				m_inGlobalAttackCounter;
	Int32				m_inStringAttackCounter;
	Int32				m_inAttackId;
	
	SComboAttackCallbackInfo() 
		: m_outDirection( AD_Front ), m_outDistance( ADIST_Small ), m_outAttackType( ComboAT_Normal )
		, m_inGlobalAttackCounter( 0 ), m_inStringAttackCounter( 0 ), m_prevDirAttack( false )
		, m_outRotateToEnemyAngle( 0.f ), m_inAttackId( -1 ), m_outLeftString( true )
		, m_outShouldRotate( false ), m_outShouldTranslate( false ) {}

	void CheckData() const
	{
		if ( m_outShouldTranslate )
		{
			ASSERT( !Vector::Near3( m_outSlideToPosition, Vector::ZERO_3D_POINT ) );

			ASSERT( !Red::Math::NumericalUtils::IsNan( m_outSlideToPosition.X ) );
			ASSERT( !Red::Math::NumericalUtils::IsNan( m_outSlideToPosition.Y ) );
			ASSERT( !Red::Math::NumericalUtils::IsNan( m_outSlideToPosition.Z ) );

			ASSERT( m_outSlideToPosition.X == m_outSlideToPosition.X );
			ASSERT( m_outSlideToPosition.Y == m_outSlideToPosition.Y );
			ASSERT( m_outSlideToPosition.Y == m_outSlideToPosition.Y );
		}
	}
};

BEGIN_CLASS_RTTI( SComboAttackCallbackInfo );
	PROPERTY( m_outDirection );
	PROPERTY( m_outDistance );
	PROPERTY( m_outAttackType );
	PROPERTY( m_inAspectName );
	PROPERTY( m_inGlobalAttackCounter );
	PROPERTY( m_inStringAttackCounter );
	PROPERTY( m_inAttackId );
	PROPERTY( m_prevDirAttack );
    PROPERTY( m_outRotateToEnemyAngle );
    PROPERTY( m_outSlideToPosition );
    PROPERTY( m_outShouldTranslate );
	PROPERTY( m_outShouldRotate );
	PROPERTY( m_outLeftString );
END_CLASS_RTTI();

struct SComboPlayerState
{
	Int32						m_id;

	Int32						m_aspectIndex;
	Int32						m_animationIndex;

	TSet< CName >				m_animationsUsed;

	SAnimationState				m_animationState;
	Float						m_animationDuration;

	Float						m_inputStartA;
	Float						m_inputEndA;

	Float						m_inputStartB;
	Float						m_inputEndB;

	Float						m_internalBlendStart;
	Float						m_internalBlendEnd;

	Float						m_externalBlendStart;
	Float						m_externalBlendEnd;

	Float						m_blendingTimer;
	Float						m_blendDuration;

	Bool						m_useRotation;
	Bool                        m_useTranslation;

	Float						m_rotateToEnemyAngle;
	Vector						m_outSlideToPosition;
	Bool						m_useDeltaRotatePolicy;
	Bool						m_useRotationScaling;

	Bool						m_isSliderInit;
	Bool						m_isSliderOk;
	AnimSlider					m_slider;
	//AnimationSlider2			m_slider2;

	SComboPlayerState() 
		: m_id( -1 ), m_aspectIndex( -1 ),m_animationIndex( -1 )
		, m_animationDuration( 1.f )
		, m_inputStartA( 0.f ), m_inputEndA( 0.5f ), m_inputStartB( -1.f ), m_inputEndB( -1.f )
		, m_internalBlendStart( 0.f ), m_internalBlendEnd( 0.5f )
		, m_externalBlendStart( 0.f ), m_externalBlendEnd( 0.5f )
		, m_blendingTimer( 0.f ), m_blendDuration( 0.2f )
		, m_useRotation( false ), m_useTranslation( false ), m_rotateToEnemyAngle( 0.f )
		, m_useDeltaRotatePolicy( false ), m_useRotationScaling( true )
		, m_isSliderInit( false ), m_isSliderOk( false )
	{}

	RED_INLINE void Reset( Float durationBlend )
	{
		m_id = -1;
		m_aspectIndex = -1;
		m_animationIndex = -1;
		m_animationState.m_currTime = 0.f;
		m_animationState.m_prevTime = 0.f;
		m_animationState.m_animation = CName::NONE;
		m_blendingTimer = 0.f;
		m_useRotation = false;
		m_useDeltaRotatePolicy = false;
		m_blendDuration = durationBlend;
		m_slider = AnimSlider();
		//m_slider2 = AnimationSlider2();
		m_isSliderInit = false;
		m_isSliderOk = false;
	}

	RED_INLINE Bool IsSet() const
	{
		Bool ret = m_aspectIndex != -1;
		if ( ret )
		{
			ASSERT( m_id != -1 );
		}
		return ret;
	}
};

class CComboPlayer : public CObject, public IVisualDebugInterface
{

	DECLARE_ENGINE_CLASS( CComboPlayer, CObject, 0 );

	const CComboDefinition*			m_definition;
	CEntity*						m_entity;

	CName							m_slotName;
	CName							m_eventAllowInput;
	CName							m_eventAllowIntenalBlend;
	CName							m_eventAllowExternalBlend;
	CName							m_eventAnimationFinished;
	CName							m_externalBlendVarName;

	CName							m_cachedInstanceName;
	CBehaviorManualSlotInterface	m_slot;

	SComboPlayerState				m_currState;
	SComboPlayerState				m_nextState;

	Uint32							m_globalAttackCounter;
	Uint32							m_stringAttackCounter;
	Int32							m_attackId;

	static const Int32				NO_ANIMATION = -1;
	static const Int32				DIR_ANIMATION = -4;

	static const String				COMBO_STATE_NAME;

    Float                           m_defaultBlendDuration;

	Bool							m_sliderPaused;
	Bool							m_paused;

public:
	CComboPlayer();

	Bool Build( const CComboDefinition* definition, CEntity* entity );

	Bool Init();
	void Deinit();

	Bool Update( Float dt );

	void Pause();
	void Unpause();
	Bool IsPaused() const;

	void PauseSlider();
	void UnpauseSlider();
	Bool IsSliderPaused() const;

	Bool PlayAttack( const CName& aspectName );

	Bool PlayHit();

public: // IVisualDebugInterface
	virtual void Render( CRenderFrame* frame, const Matrix& matrix );

private:
	Bool ScheduleAttack( const CName& aspectName );
	Bool PlayScheduledAttack( Float timeToSync );

	Bool IsInstanceValid( const CName& name ) const;
	Bool IsPlayingAttack() const;
	Bool IsPlayingDirAttack() const;
	Bool HasNextAttackScheduled() const;
	Bool IsBlendingAttacks() const;

	Bool UpdateAttackAnimation( Float dt, Float& timeRest );
	void FinishAttackAction();
	void ProcessAttackFinishedEvent();

	Bool ProcessInternalBlend( Float dt, Float& timeRest );
	void FinishInternalBlendAction( Float timeRest );

	void UpdateSlider( SComboPlayerState& state );

	void FindNextAttack( const SComboPlayerState& curr, SComboPlayerState& next, const CName& aspectName );
	Bool FindNextAttack_ContinueString( const SComboPlayerState& curr, SComboPlayerState& state );
	Bool FindNextAttack_RandomAndStartNewString( SComboPlayerState& state );
	void FindNextAttack_StartNewAspect( SComboPlayerState& state, const CName& aspectName );
	void FillStateEvents( SComboPlayerState& state, const CSkeletalAnimationSetEntry* animEntry ) const;

	Bool SetAllowExternalBlendVariable( Bool state );
	Bool IsAllowExternalBlendVariableSet() const;
	Bool IsBehaviorComboStateActive() const;
	Bool SendComboEventToBehaviorGraph();

	Bool IsInputTime( const SComboPlayerState& state ) const;
	Bool IsInternalBlendTime( const SComboPlayerState& state ) const;
	Bool IsExteralBlendTime( const SComboPlayerState& state ) const;

	Bool IsInputTime() const;
	Bool IsInternalBlendTime() const;
	Bool IsExteralBlendTime() const;

	Bool WillBeInternalBlendTime( Float dt ) const;
	Bool WillBeExternalBlendTime( Float dt ) const;

	const CSkeletalAnimationSetEntry* FindAnimation( const CName& animationName ) const;
	Int32 FindAspectIndex( const CName& aspectName ) const;
	const CComboAspect* FindAspect( const CName& aspectName ) const;

	void UpdateDebugBars() const;
	void UpdateDebugBar( const SComboPlayerState& state, Int32 x, Int32 y, Int32 h, Int32 h2, Int32 w, const CName& n1, const CName& n2, const CName& n3, const CName& n4 ) const;
    void RemoveDebugBars();

	Bool ShouldRecalcTargetPosition( const Vector& prevTargetPos, const Vector& newTargetPos ) const;

private:
    void funcPlayAttack( CScriptStackFrame& stack, void* result );
    void funcStopAttack( CScriptStackFrame& stack, void* result );
	void funcPlayHit( CScriptStackFrame& stack, void* result );
	void funcBuild( CScriptStackFrame& stack, void* result );
    void funcInit( CScriptStackFrame& stack, void* result );
	void funcDeinit( CScriptStackFrame& stack, void* result );
    void funcUpdate( CScriptStackFrame& stack, void* result );
	void funcPause( CScriptStackFrame& stack, void* result );
	void funcUnpause( CScriptStackFrame& stack, void* result );
	void funcIsPaused( CScriptStackFrame& stack, void* result );
	void funcPauseSlider( CScriptStackFrame& stack, void* result );
	void funcUnpauseSlider( CScriptStackFrame& stack, void* result );
	void funcIsSliderPaused( CScriptStackFrame& stack, void* result );

    void funcSetDurationBlend( CScriptStackFrame& stack, void* result );
    void funcUpdateTarget( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CComboPlayer );
    PARENT_CLASS( CObject );
    NATIVE_FUNCTION( "PlayAttack", funcPlayAttack );
    NATIVE_FUNCTION( "StopAttack", funcStopAttack );
	NATIVE_FUNCTION( "PlayHit", funcPlayHit );
	NATIVE_FUNCTION( "Build", funcBuild );
    NATIVE_FUNCTION( "Init", funcInit );
	NATIVE_FUNCTION( "Deinit", funcDeinit );
    NATIVE_FUNCTION( "Update", funcUpdate );
	NATIVE_FUNCTION( "Pause", funcPause );
	NATIVE_FUNCTION( "Unpause", funcUnpause );
	NATIVE_FUNCTION( "IsPaused", funcIsPaused );
	NATIVE_FUNCTION( "PauseSlider", funcPauseSlider );
	NATIVE_FUNCTION( "UnpauseSlider", funcUnpauseSlider );
	NATIVE_FUNCTION( "IsSliderPaused", funcIsSliderPaused );
    NATIVE_FUNCTION( "SetDurationBlend", funcSetDurationBlend );
    NATIVE_FUNCTION( "UpdateTarget", funcUpdateTarget );
END_CLASS_RTTI();
