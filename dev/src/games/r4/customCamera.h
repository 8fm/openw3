
#pragma once

#include "../../common/engine/cameraDirector.h"
#include "customCameraControllers.h"

struct SCameraAnimationDefinition
{
	DECLARE_RTTI_STRUCT( SCameraAnimationDefinition );

	CName	m_animation;	// Animation name
	Int32	m_priority;		// Priority of the animation
	Float	m_blendIn;		// Blend in time
	Float	m_blendOut;		// Blend out time
	Float	m_weight;		// Weight of the animation
	Float	m_speed;		// SpeedToPlayTheCamera animation
	Bool	m_loop;			// Is the animation looped
	Bool	m_reset;		// If the same animation is played second time should it be played from beginning
	Bool	m_additive;		// Is the animation additive
	Bool	m_exclusive;	// Is this the only animation that should be played
};

BEGIN_CLASS_RTTI( SCameraAnimationDefinition )
	PROPERTY( m_animation )
	PROPERTY( m_priority )
	PROPERTY( m_blendIn )
	PROPERTY( m_blendOut )
	PROPERTY( m_weight )
	PROPERTY( m_speed )
	PROPERTY( m_loop )
	PROPERTY( m_reset )
	PROPERTY( m_additive )
	PROPERTY( m_exclusive )
END_CLASS_RTTI()

class CCustomCameraAnimation
{
public:
	struct SAnimInstance
	{
		const CSkeletalAnimation*	m_animation;
		SCameraAnimationDefinition	m_definition;
		Float						m_weight;
		Float						m_time;

		SAnimInstance( const CSkeletalAnimation* anim, const SCameraAnimationDefinition& def )
			: m_animation( anim )
			, m_definition( def )
			, m_weight( 0.f )
			, m_time( 0.f ) {}

		SAnimInstance() : m_animation( NULL ) {}

		RED_INLINE Bool operator==( const SAnimInstance& other ) const { return m_definition.m_animation == other.m_definition.m_animation; }
	};

private:
	TDynArray< SAnimInstance > m_animations;

	Vector		m_translation;
	EulerAngles	m_rotation;
	Matrix		m_transfrom;

	TDynArray< AnimQsTransform >	m_bones;
	TDynArray< Float >				m_tracks;

public:
	CCustomCameraAnimation();

	void PlayAnimation( const CSkeletalAnimation* animation, const SCameraAnimationDefinition& definition );
	void StopAnimation( const CName& anim );
	void Update( Float timeDelta, CCustomCamera* camera );
	Bool IsAdditive();

	// Finishers handling, this should be unrelated to CCustomCameraAnimation, so can be considered as a HACK:
	//---------------------------------------------------------------------------------------------------------
	// Sets camera at the end of camera finisher animation (if any founded..).
	void SetCameraToTheEndOfFinisherAnimation( const SAnimInstance animInstance, CCustomCamera* camera ); 

	// Tries to predict player transform at the end of played finisher anim.
	AnimQsTransform FindPlayerWorldTransformAtEndOfFinisher( const String& cameraAnimName );

	// Setup movement data based on given parameters(not exact, some aproximations done to simplify the code).
	void SetupMovementData( SCameraMovementData& moveData, const AnimQsTransform& cameraTransformModelSpace, const AnimQsTransform& playerTransformWorldSpace );
	//----------------------------------------------------------------------------------------------------------

	void ApplyTransform( Vector& cameraPosition, EulerAngles& cameraRotation );

	RED_INLINE const EulerAngles&	GetRotation() const		{ return m_rotation; }
	RED_INLINE const Vector&		GetTranslation() const	{ return m_translation; }
	RED_INLINE const Matrix&		GetTransform() const	{ return m_transfrom; }
};

//////////////////////////////////////////////////////////////////////////

class CCustomCamera : public CEntity, public ICamera
{
	DECLARE_ENGINE_CLASS( CCustomCamera, CEntity, 0 )

private:
	TDynArray<ICustomCameraPivotPositionController*>	m_pivotPositionControllers;
	TDynArray<ICustomCameraPivotRotationController*>	m_pivotRotationControllers;
	TDynArray<ICustomCameraPivotDistanceController*>	m_pivotDistanceControllers;

	// Blending controller
	CCustomCameraBlendPPC*					m_blendPivotPositionController;

	SCameraMovementData						m_movementData;
	ICustomCameraPositionController*		m_activeCameraPositionController;

	Float									m_autoRotationHorTimer;
	Float									m_autoRotationVerTimer;
	Float									m_manualRotationHorTimeout;
	Float									m_manualRotationVerTimeout;
	Bool									m_allowAutoRotation;
	Bool									m_allowManualRotation;

	Float									m_fov;
	Float									m_forcedNearPlane;

	THandle< CSkeletalAnimationSet >		m_animSet;
	TDynArray< CSkeletalAnimationSet* >		m_dlcAnimSets;
	CCustomCameraAnimation					m_animation;

	TDynArray< SCustomCameraPreset >		m_presets;

	TDynArray< CCurve* >	m_curveSet;
	TDynArray< CName >		m_curveNames;

	Bool									m_updateInput;
	Bool									m_isResetScheduled;

	Float									m_blendInTime;
	Float									m_blendInTimeElapsed;

public:
	CCustomCamera();
	virtual ~CCustomCamera();

	virtual void OnPostLoad();
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	RED_INLINE SCameraMovementData& GetMoveData() { return m_movementData; }
	RED_INLINE const SCameraMovementData& GetMoveData() const { return m_movementData; }

	RED_INLINE ICustomCameraPositionController* GetActiveController() { return m_activeCameraPositionController; }

	virtual Bool IsManualControlledHor() const override { return m_autoRotationHorTimer < m_manualRotationHorTimeout; }
	virtual Bool IsManualControlledVer() const override { return m_autoRotationVerTimer < m_manualRotationVerTimeout; }
	RED_INLINE void ForceManualControlHorTimeout() { m_autoRotationHorTimer = m_manualRotationHorTimeout; }
	RED_INLINE void ForceManualControlVerTimeout() { m_autoRotationVerTimer = m_manualRotationVerTimeout; }

	RED_INLINE void SetNearPlane( Float nearPlane ) { m_forcedNearPlane = nearPlane; }

	RED_INLINE Float GetFoV() const { return m_fov; }

	Bool ChangePivotPositionController( const CName& ctrlName );
	Bool ChangePivotRotationController( const CName& ctrlName );
	Bool ChangePivotDistanceController( const CName& ctrlName );

	Bool BlendToPivotPositionController( const CName& ctrlName, Float blendTime );

	CCurve*	FindCurve( const CName& curveName ) const;

	void UpdateWithoutInput();

	void SetResetScheduled( Bool reset ) { m_isResetScheduled = reset; }

	Bool AddDLCAnimset( CSkeletalAnimationSet* animset );
	Bool RemoveDLCAnimset( CSkeletalAnimationSet* animset );

	//////////////////////////////////////////////////////////////////////////
	// ICamera implementation
	//////////////////////////////////////////////////////////////////////////
	
	virtual Bool Update( Float timeDelta ) override;
	virtual Bool GetData( Data& outData ) const override;
	Bool StartBlendFrom( const Data& data, Float blendTime );

	virtual void ResetCamera() override;
	virtual void OnActivate( const IScriptable* prevCameraObject, Bool resetCamera ) override;

private:
	void OnActivate( const SCameraMovementData* data );
	void UpdateInput( Float timeDelta );
	void SimulateCameraToDefaultPos();

private:
	void funcActivate( CScriptStackFrame& stack, void* result );
	void funcGetActivePivotPositionController( CScriptStackFrame& stack, void* result );
	void funcGetActivePivotRotationController( CScriptStackFrame& stack, void* result );
	void funcGetActivePivotDistanceController( CScriptStackFrame& stack, void* result );
	void funcGetActivePreset( CScriptStackFrame& stack, void* result );
	void funcChangePivotPositionController( CScriptStackFrame& stack, void* result );
	void funcChangePivotRotationController( CScriptStackFrame& stack, void* result );
	void funcChangePivotDistanceController( CScriptStackFrame& stack, void* result );
	void funcBlendToPivotPositionController( CScriptStackFrame& stack, void* result );
	void funcPlayAnimation( CScriptStackFrame& stack, void* result );
	void funcStopAnimation( CScriptStackFrame& stack, void* result );
	void funcFindCurve( CScriptStackFrame& stack, void* result );
	void funcSetManualRotationHorTimeout( CScriptStackFrame& stack, void* result );
	void funcSetManualRotationVerTimeout( CScriptStackFrame& stack, void* result );
	void funcGetManualRotationHorTimeout( CScriptStackFrame& stack, void* result );
	void funcGetManualRotationVerTimeout( CScriptStackFrame& stack, void* result );
	void funcIsManualControledHor( CScriptStackFrame& stack, void* result );
	void funcIsManualControledVer( CScriptStackFrame& stack, void* result );
	void funcEnableManualControl( CScriptStackFrame& stack, void* result );
	void funcForceManualControlHorTimeout( CScriptStackFrame& stack, void* result );
	void funcForceManualControlVerTimeout( CScriptStackFrame& stack, void* result );
	void funcChangePreset( CScriptStackFrame& stack, void* result );
	void funcNextPreset( CScriptStackFrame& stack, void* result );
	void funcPrevPreset( CScriptStackFrame& stack, void* result );
	void funcSetCollisionOffset( CScriptStackFrame& stack, void* result );
	void funcEnableScreenSpaceCorrection( CScriptStackFrame& stack, void* result );
	void funcSetAllowAutoRotation( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CCustomCamera )
	PARENT_CLASS( CEntity )
	PROPERTY_INLINED( m_pivotPositionControllers, String::EMPTY )
	PROPERTY_INLINED( m_pivotRotationControllers, String::EMPTY )
	PROPERTY_INLINED( m_pivotDistanceControllers, String::EMPTY )
	PROPERTY_INLINED( m_activeCameraPositionController, String::EMPTY )
	PROPERTY( m_blendPivotPositionController );
	PROPERTY_EDIT( m_allowAutoRotation, TXT("Defines if the camera can be controlled only by the user input") )
	PROPERTY_EDIT( m_manualRotationHorTimeout, TXT("Time after which the camera starts to rotate automatically horizontal") )
	PROPERTY_EDIT( m_manualRotationVerTimeout, TXT("Time after which the camera starts to rotate automatically vertical") )
	PROPERTY_EDIT( m_fov, TXT("FoV") )
	PROPERTY_EDIT( m_animSet, TXT("Animations used by the camera") )
	PROPERTY_EDIT( m_presets, TXT("Camera Presets") )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_curveSet, TXT("Curve set"), TXT("CurveSelection") )
	PROPERTY_EDIT( m_curveNames, TXT("Curve names") )
	NATIVE_FUNCTION( "Activate", funcActivate )
	NATIVE_FUNCTION( "GetActivePivotPositionController", funcGetActivePivotPositionController )
	NATIVE_FUNCTION( "GetActivePivotRotationController", funcGetActivePivotRotationController )
	NATIVE_FUNCTION( "GetActivePivotDistanceController", funcGetActivePivotDistanceController )
	NATIVE_FUNCTION( "ChangePivotPositionController", funcChangePivotPositionController )
	NATIVE_FUNCTION( "ChangePivotRotationController", funcChangePivotRotationController )
	NATIVE_FUNCTION( "ChangePivotDistanceController", funcChangePivotDistanceController )
	NATIVE_FUNCTION( "BlendToPivotPositionController", funcBlendToPivotPositionController )
	NATIVE_FUNCTION( "GetActivePreset", funcGetActivePreset )
	NATIVE_FUNCTION( "PlayAnimation", funcPlayAnimation )
	NATIVE_FUNCTION( "StopAnimation", funcStopAnimation )
	NATIVE_FUNCTION( "FindCurve", funcFindCurve )
	NATIVE_FUNCTION( "SetManualRotationHorTimeout", funcSetManualRotationHorTimeout )
	NATIVE_FUNCTION( "SetManualRotationVerTimeout", funcSetManualRotationVerTimeout )
	NATIVE_FUNCTION( "GetManualRotationHorTimeout", funcGetManualRotationHorTimeout )
	NATIVE_FUNCTION( "GetManualRotationVerTimeout", funcGetManualRotationVerTimeout )
	NATIVE_FUNCTION( "IsManualControledHor", funcIsManualControledHor )
	NATIVE_FUNCTION( "IsManualControledVer", funcIsManualControledVer )
	NATIVE_FUNCTION( "ForceManualControlHorTimeout", funcForceManualControlHorTimeout )
	NATIVE_FUNCTION( "ForceManualControlVerTimeout", funcForceManualControlVerTimeout )
	NATIVE_FUNCTION( "EnableManualControl", funcEnableManualControl )
	NATIVE_FUNCTION( "ChangePreset", funcChangePreset )
	NATIVE_FUNCTION( "NextPreset", funcNextPreset )
	NATIVE_FUNCTION( "PrevPreset", funcPrevPreset )
	NATIVE_FUNCTION( "SetCollisionOffset", funcSetCollisionOffset )
	NATIVE_FUNCTION( "EnableScreenSpaceCorrection", funcEnableScreenSpaceCorrection )
	NATIVE_FUNCTION( "SetAllowAutoRotation", funcSetAllowAutoRotation );
END_CLASS_RTTI()
