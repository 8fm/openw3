/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphStack.h"
#include "staticCamera.h"
#include "../core/scriptStackFrame.h"
#include "skeleton.h"
#include "animatedComponent.h"
#include "game.h"
#include "world.h"
#include "layer.h"
#include "cameraDirector.h"


IMPLEMENT_RTTI_ENUM( ECameraSolver );

IMPLEMENT_ENGINE_CLASS( CStaticCamera );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_NAMED_NAME( VAR_SOLVER, "solver" );
RED_DEFINE_NAMED_NAME( VAR_ANIM_STATE, "animState" );

CStaticCamera::CStaticCamera()
	: m_activationDuration( 0.f )
	, m_deactivationDuration( 0.f )
	, m_timeout( 0.f )
	, m_zoom( 0.f )
	, m_fov( 0.f )
	, m_animState( 0 )
	, m_solver( CS_Focus )
	, m_guiEffect( 0 )
	, m_blockPlayer( true )
	, m_resetPlayerCamera( true )
	, m_fadeStartDuration( 0.f )
	, m_fadeEndDuration( 0.f )
	, m_fadeStartColor( Color::BLACK )
	, m_fadeEndColor( Color::BLACK )
	, m_isFadeStart( false )
	, m_isFadeEnd( false )
	, m_isFadeStartFadeIn( true )
	, m_isFadeEndFadeIn( true )
{
	
}

void CStaticCamera::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	ASSERT( !IsFrozen() );

	Freeze();

	m_timer = 0.f;
	
	ResetFadingFlags();

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Areas );
}

void CStaticCamera::OnDetached( CWorld* world )
{
	if ( IsFrozen() )
	{
		Unfreeze();
	}

	RemoveAllListeners();

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Areas );

	TBaseClass::OnDetached( world );
}

Bool CStaticCamera::Update( Float timeDelta )
{
	TBaseClass::Update( timeDelta );

	PC_SCOPE_PIX( StaticCameraTick );

	if ( !GIsGame )
	{
		// For preview tool
		SetupBehavior( false );
	}

	if ( !GetRootAnimatedComponent() || !GetRootAnimatedComponent()->IsFrozen() )
	{
		m_timer += timeDelta;

		// Fading
		if ( HasFadeEnd() && m_timeout > 0.f && !IsFadeEndInProgress() && m_timer > m_timeout - m_fadeEndDuration )
		{
			SetFadeEnd();
		}

		// Blend out ended
		if ( m_activationDuration > 0.f && m_timer > m_activationDuration )
		{
			OnActivationFinished();
		}

		// Timeout
		if ( m_timeout > 0.f && m_timer > m_timeout - m_deactivationDuration )
		{
			m_timer = 0.f;

			OnRunEnd();
		}
	}

	return true;
}

Bool CStaticCamera::Run( IStaticCameraListener* list )
{
	AddListener( list );

	return Run();
}

Bool CStaticCamera::Run()
{
	m_timer = 0.f;

	ResetFadingFlags();

	Unfreeze();

	OnRunStart();

	ASSERT( IsOnStack() );

	return true;
}

Bool CStaticCamera::IsRunning() const
{
	return !IsFrozen();
}

void CStaticCamera::BreakRunning()
{
	SCAM_LOG( TXT("Static camera '%ls' is broken"), GetName().AsChar() );

	m_timer = 0.f;

	OnRunEnd();

	RemoveAllListeners();

	Freeze();

	ASSERT( !IsOnStack() );
}

void CStaticCamera::AddListener( IStaticCameraListener* l )
{
	m_listeners.PushBackUnique( l );
}

Bool CStaticCamera::RemoveListener( IStaticCameraListener* l )
{
	return m_listeners.Remove( l );
}

void CStaticCamera::RemoveAllListeners()
{
	m_listeners.Clear();
}

Bool CStaticCamera::HasAnyListeners() const
{
	return m_listeners.Size() > 0;
}

Bool  CStaticCamera::HasTimeout() const
{
	return m_timeout > 0.f;
}

Float CStaticCamera::GetDuration() const
{
	return m_timeout;
}

void CStaticCamera::OnRunStart()
{
	SCAM_LOG( TXT("Static camera '%ls' is running, ac time: %f, deac time: %f, timeout %f"), 
		GetName().AsChar(), m_activationDuration, m_deactivationDuration, m_timeout );

	// Run start
	SetupBehavior( true );

	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnRunStart( this );
	}

	static CName eventName( TXT("OnStarted") );
	CallEvent( eventName );

	ForceBehaviorCameraPose();

	SetActive( m_activationDuration );

	if ( HasFadeStart() )
	{
		SetFadeStart();
	}
}

void CStaticCamera::OnRunEnd()
{
	SCAM_LOG( TXT("Static camera '%ls' is stoping"), GetName().AsChar() );

	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnRunEnd( this );
	}

	OnDeactivationStarted();

	RemoveAllListeners();

	Freeze();
}

void CStaticCamera::OnActivationFinished()
{
	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		ASSERT( m_listeners[ i ] );
		m_listeners[ i ]->OnActivationFinished( this );
	}
}

void CStaticCamera::OnDeactivationStarted()
{
	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		ASSERT( m_listeners[ i ] );
		m_listeners[ i ]->OnDeactivationStarted( this );
	}
}

void CStaticCamera::ResetFadingFlags()
{
	m_isFadeStart = false;
	m_isFadeEnd = false;
}

Bool CStaticCamera::IsFadeStartInProgress() const
{
	return m_isFadeStart;
}

Bool CStaticCamera::IsFadeEndInProgress() const
{
	return m_isFadeEnd;
}

Bool CStaticCamera::HasFadeStart() const
{
	return m_fadeStartDuration > 0.f;
}

Bool CStaticCamera::HasFadeEnd() const
{
	return m_fadeEndDuration > 0.f;
}

void CStaticCamera::SetFadeStart()
{
	ASSERT( !m_isFadeStart );
	m_isFadeStart = true;

	//GGame->SetBlackscreen( m_isFadeStartFadeIn, TXT( "StaticCameraFade" ) );
	//GGame->StartFade( m_isFadeStartFadeIn, TXT( "StaticCameraFade" ), m_fadeStartDuration, m_fadeStartColor );
}

void CStaticCamera::SetFadeEnd()
{
	ASSERT( !m_isFadeEnd );
	m_isFadeEnd = true;

	//GGame->SetBlackscreen( m_isFadeEndFadeIn, TXT( "StaticCameraFade" ) );
	//GGame->StartFade( m_isFadeEndFadeIn, TXT( "StaticCameraFade" ), m_fadeEndDuration, m_fadeEndColor );
}

void CStaticCamera::RevertFadeStart()
{
	ASSERT( m_isFadeStart );
	m_isFadeStart = false;

	//GGame->SetBlackscreen( false, TXT( "StaticCameraFade" ) );
}

void CStaticCamera::RevertFadeEnd()
{
	ASSERT( m_isFadeEnd );
	m_isFadeEnd = false;

	//GGame->SetBlackscreen( false, TXT( "StaticCameraFade" ) );
}

Vector CStaticCamera::GetDefaultTarget() const
{
	return GetWorldPositionRef() + Vector( 0.f, 10.f, 1.f );
}

void CStaticCamera::SetTarget( const Vector& target )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();
		if ( stack )
		{
			Bool ret = stack->SetBehaviorVariable( CNAME( LOOK_AT_TARGET ), target );
			ASSERT( ret );
		}
	}
}

void CStaticCamera::SetupBehavior( Bool withReset )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		CBehaviorGraphStack* stack = ac->GetBehaviorStack();
		if ( stack )
		{
			if ( withReset )
			{
				stack->Reset();
			}

			Bool ret = stack->SetBehaviorVariable( CNAME( VAR_SOLVER ), (Float)m_solver );
			ASSERT( ret );

			ret = stack->SetBehaviorVariable( CNAME( VAR_ANIM_STATE ), (Float)m_animState );
			ASSERT( ret );

			if ( m_fov > 0.f )
			{
				SetFov( m_fov );
			}

			if ( m_zoom > 0.f )
			{
				SetZoom( m_zoom );
			}
		}
	}
}

void CStaticCamera::SetNoSolver()
{
	m_solver = CS_None;
}

void CStaticCamera::SetEditorPreviewSettings()
{
	SetNoSolver();

	m_activationDuration = 0.f;
	m_deactivationDuration = 0.f;
	m_timeout = 0.f;

	m_guiEffect = 0;
	m_blockPlayer = true;

	m_fadeStartDuration = 0.f;
	m_fadeEndDuration = 0.f;
}

void CStaticCamera::SetDefaults()
{
	m_activationDuration = 2.f;
	m_deactivationDuration = 0.f;
	m_timeout = 5.f;
	m_zoom = 0.f;
	m_fov = 0.f;
	m_animState = 0;
	m_solver = CS_None;
	m_guiEffect = 0;
	m_blockPlayer = true;
	m_fadeStartDuration = 0.f;
	m_fadeEndDuration = 0.f;
}

Bool CStaticCamera::AutoDeactivating() const
{
	return m_timeout > 0.f;
}

Bool CStaticCamera::GetPositionFromView( const Vector& viewPos, const EulerAngles& viewRot, Vector& posOut, EulerAngles& rotOut ) const
{
	CSkeleton* skeleton = GetRootAnimatedComponent() ? GetRootAnimatedComponent()->GetSkeleton() : NULL;
	if ( skeleton )
	{
		Matrix viewPoint = viewRot.ToMatrix();
		viewPoint.SetTranslation( viewPos );

		if ( skeleton->GetBonesNum() > m_boneEye )
		{
			Matrix boneMS = skeleton->GetBoneMatrixMS( m_boneEye );

			Matrix rootPos = boneMS.Inverted() * viewPoint;

			posOut = rootPos.GetTranslation();
			rotOut = rootPos.ToEulerAngles();

			return true;
		}
		else
		{
			ASSERT( skeleton->GetBonesNum() > m_boneEye );
		}
	}

	return false;
}

Bool CStaticCamera::GetViewFromPosition( const Vector& camPos, const EulerAngles& camRot, Vector& viewPos, EulerAngles& viewRot ) const
{
	CSkeleton* skeleton = GetRootAnimatedComponent() ? GetRootAnimatedComponent()->GetSkeleton() : NULL;
	if ( skeleton )
	{
		Matrix rootPoint = camRot.ToMatrix();
		rootPoint.SetTranslation( camPos );

		if ( skeleton->GetBonesNum() > m_boneEye )
		{
			Matrix boneMS = skeleton->GetBoneMatrixMS( m_boneEye );

			Matrix viewPoint = boneMS * rootPoint;

			viewPos = viewPoint.GetTranslation();
			viewRot = viewPoint.ToEulerAngles();

			return true;
		}
		else
		{
			ASSERT( skeleton->GetBonesNum() > m_boneEye );
		}
	}

	return false;
}

void CStaticCamera::ForceEditorBehavior()
{
	SetupBehavior( false );
}

Int32 CStaticCamera::GetGuiEffect() const
{
	return m_guiEffect;
}

void CStaticCamera::funcRun( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( Run() );
}

void CStaticCamera::funcIsRunning( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	const CWorld* world = GetLayer() ? GetLayer()->GetWorld() : NULL;
	RETURN_BOOL( world && world->GetCameraDirector()->GetTopmostCameraObject() == this );
}

void CStaticCamera::funcAutoDeactivating( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( AutoDeactivating() );
}

extern Bool GLatentFunctionStart;

void CStaticCamera::funcRunAndWait( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Starting
	if ( GLatentFunctionStart )
	{
		Bool ret = Run();
		ASSERT( ret );
	}

	if ( !IsRunning() )
	{
		RETURN_BOOL( true );
		return;
	}

	if ( HasTimeout() )
	{
		timeout += GetDuration();
	}

	const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
	if ( timeout > 0.f && waitedTime <= timeout )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( false );
}

void CStaticCamera::funcStop( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( HasFadeEnd() && !IsFadeEndInProgress() )
	{
		SetFadeEnd();
	}

	OnRunEnd();

	GGame->ActivateGameCamera( m_deactivationDuration );
}