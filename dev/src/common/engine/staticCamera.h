/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "camera.h"

//////////////////////////////////////////////////////////////////////////

#ifndef NO_LOG

#define	SCAM_LOG( format, ... )		RED_LOG( StaticCamera, format, ## __VA_ARGS__ )
#define SCAM_WARN( format, ... )	RED_LOG( StaticCamera, format, ## __VA_ARGS__ )
#define SCAM_ERROR( format, ... )	RED_LOG( StaticCamera, format, ## __VA_ARGS__ )

#else

#define SCAM_LOG( format, ... )	
#define SCAM_WARN( format, ... )	
#define SCAM_ERROR( format, ... )

#endif

//////////////////////////////////////////////////////////////////////////

enum ECameraSolver
{
	CS_None,
	CS_LookAt,
	CS_Focus
};

BEGIN_ENUM_RTTI( ECameraSolver );
	ENUM_OPTION( CS_None );
	ENUM_OPTION( CS_LookAt );
	ENUM_OPTION( CS_Focus );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

class IStaticCameraListener
{
public:
	virtual ~IStaticCameraListener() {}
	virtual void OnRunStart( const CStaticCamera* camera ) = 0;
	virtual void OnRunEnd( const CStaticCamera* camera ) = 0;
	virtual void OnActivationFinished( const CStaticCamera* camera ) {}
	virtual void OnDeactivationStarted( const CStaticCamera* camera ) {}
};

class CStaticCamera : public CCamera
{
	DECLARE_ENGINE_CLASS( CStaticCamera, CCamera, 0 )

protected:
	Float					m_activationDuration;
	Float					m_deactivationDuration;
	Float					m_timeout;
	Float					m_zoom;
	Float					m_fov;
	Int32					m_animState;
	Int32					m_guiEffect;
	Bool					m_blockPlayer;
	Bool					m_resetPlayerCamera;
	ECameraSolver			m_solver;
	Float					m_fadeStartDuration;
	Float					m_fadeEndDuration;
	Color					m_fadeStartColor;
	Color					m_fadeEndColor;
	Bool					m_isFadeStartFadeIn;
	Bool					m_isFadeEndFadeIn;

protected:
	Float					m_timer;
	Bool					m_isFadeStart;
	Bool					m_isFadeEnd;
	TDynArray< IStaticCameraListener* > m_listeners;

public:
	CStaticCamera();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual Bool Update( Float timeDelta );

public: // For editor
	void SetDefaults();
	void SetNoSolver();
	void SetEditorPreviewSettings();

	Bool GetPositionFromView( const Vector& viewPos, const EulerAngles& viewRot, Vector& posOut, EulerAngles& rotOut ) const;
	Bool GetViewFromPosition( const Vector& camPos, const EulerAngles& camRot, Vector& viewPos, EulerAngles& viewRot ) const;

	void ForceEditorBehavior();

public: // For control
	Bool Run();
	Bool Run( IStaticCameraListener* list );
	Bool IsRunning() const;

	void BreakRunning();

	void AddListener( IStaticCameraListener* l );
	Bool RemoveListener( IStaticCameraListener* l );
	void RemoveAllListeners();
	Bool HasAnyListeners() const;

	Bool HasTimeout() const;
	Float GetDuration() const;

protected:
	void SetupBehavior( Bool withReset );
	void SetTarget( const Vector& target );
	Vector GetDefaultTarget() const;
	Bool AutoDeactivating() const;

	void OnRunStart();
	void OnRunEnd();
	void OnActivationFinished();
	void OnDeactivationStarted();

	Bool HasFadeStart() const;
	Bool HasFadeEnd() const;
	void SetFadeStart();
	void SetFadeEnd();
	void RevertFadeStart();
	void RevertFadeEnd();
	void ResetFadingFlags();
	Bool IsFadeStartInProgress() const;
	Bool IsFadeEndInProgress() const;

public: // Gameplay
	Int32 GetGuiEffect() const;
	Bool ShouldBlockPlayerButtonInteractions() const;

protected:
	void funcRun( CScriptStackFrame& stack, void* result );
	void funcRunAndWait( CScriptStackFrame& stack, void* result );
	void funcIsRunning( CScriptStackFrame& stack, void* result );
	void funcAutoDeactivating( CScriptStackFrame& stack, void* result );
	void funcStop( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CStaticCamera )
	PARENT_CLASS( CCamera );
	PROPERTY_EDIT( m_solver, TXT("") );
	PROPERTY_EDIT_RANGE( m_activationDuration, TXT(""), 0.f, 100.f );
	PROPERTY_EDIT_RANGE( m_deactivationDuration, TXT(""), 0.f, 100.f );
	PROPERTY_EDIT_RANGE( m_timeout, TXT(""), 0.f, 100.f );
	PROPERTY_EDIT_RANGE( m_zoom, TXT(""), 0.f, 10.f );
	PROPERTY_EDIT_RANGE( m_fov, TXT(""), 1.f, 180.f );
	PROPERTY_CUSTOM_EDIT( m_animState, TXT(""), TXT("ScriptedEnum_EStaticCameraAnimState") );
	PROPERTY_CUSTOM_EDIT( m_guiEffect, TXT(""), TXT("ScriptedEnum_EStaticCameraGuiEffect") );
	PROPERTY_EDIT( m_blockPlayer, TXT("") );
	PROPERTY_EDIT( m_resetPlayerCamera, TXT("Reset player camera after static camera") );
	PROPERTY_EDIT( m_fadeStartDuration, TXT("Fade start duration, 0 = inactive") );
	PROPERTY_EDIT( m_fadeStartColor, TXT("Fade start color") );
	PROPERTY_EDIT( m_isFadeStartFadeIn, TXT("Is fade start fade in type ( or fade out )") );
	PROPERTY_EDIT( m_fadeEndDuration, TXT("Fade end duration, 0 = inactive") );
	PROPERTY_EDIT( m_fadeEndColor, TXT("Fade end color") );
	PROPERTY_EDIT( m_isFadeEndFadeIn, TXT("Is fade end fade in type ( or fade out )") );
	NATIVE_FUNCTION( "Run", funcRun );
	NATIVE_FUNCTION( "RunAndWait", funcRunAndWait );
	NATIVE_FUNCTION( "IsRunning", funcIsRunning );
	NATIVE_FUNCTION( "AutoDeactivating", funcAutoDeactivating );
	NATIVE_FUNCTION( "Stop", funcStop );
END_CLASS_RTTI()
