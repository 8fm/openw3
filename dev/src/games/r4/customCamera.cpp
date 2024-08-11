
#include "build.h"
#include "customCamera.h"
#include "../../common/engine/behaviorGraphUtils.inl"
#include "../../common/engine/curve.h"
#include "CameraTools.h"

// finishers debug staff:
//#define CAMERA_FINISHER_DEBUG_MODE

#ifdef CAMERA_FINISHER_DEBUG_MODE
#define _DEBUG_DRAW_( ... ) __VA_ARGS__
#include "../../common/game/commonGame.h" 
#include "../../common/engine/visualDebug.h" 
#else
#define _DEBUG_DRAW_( ... )
#endif
//----------------------------------

static const String CAMERA_FINISHER_MARK( TXT( "_camera_" ) );

IMPLEMENT_ENGINE_CLASS( SCameraAnimationDefinition );
IMPLEMENT_ENGINE_CLASS( CCustomCamera );

//////////////////////////////////////////////////////////////////////////

CCustomCamera::CCustomCamera()
	: m_activeCameraPositionController( NULL )
	, m_blendPivotPositionController( NULL )
	, m_autoRotationHorTimer( 0.f )
	, m_autoRotationVerTimer( 0.f )
	, m_manualRotationHorTimeout( 3.f )
	, m_manualRotationVerTimeout( 3.f )
	, m_allowAutoRotation( true )
	, m_allowManualRotation( true )
	, m_fov( 60.f )
	, m_forcedNearPlane( 0.f )
	, m_updateInput( true )
	, m_isResetScheduled( true )
	, m_blendInTime( 0.0f )
	, m_blendInTimeElapsed( 0.0f )
{
}

CCustomCamera::~CCustomCamera()
{
}

void CCustomCamera::OnPostLoad()
{
	// We need some defaults just in case. It's still better then sanity checks in loops imo
	if( m_pivotPositionControllers.Empty() )
	{
		m_pivotPositionControllers.PushBack( CreateObject< CCustomCameraPlayerPPC >( this ) );
	}

	if( m_pivotRotationControllers.Empty() )
	{
		m_pivotRotationControllers.PushBack( CreateObject< CCustomCameraDefaultPRC >( this ) );
	}

	if( m_pivotDistanceControllers.Empty() )
	{
		m_pivotDistanceControllers.PushBack( CreateObject< CCustomCameraDefaultPDC >( this ) );
	}

	if( !m_activeCameraPositionController )
	{
		m_activeCameraPositionController = CreateObject< CCustomCameraSimplePositionController >( this );
	}

	m_movementData.m_camera = this;
	m_movementData.m_pivotPositionController = m_pivotPositionControllers[0];
	m_movementData.m_pivotRotationController = m_pivotRotationControllers[0];
	m_movementData.m_pivotDistanceController = m_pivotDistanceControllers[0];

	if( m_presets.Empty() )
	{
		SCustomCameraPreset ccp;
		ccp.m_name = CNAME( Default );
		ccp.m_explorationDistance = 3.f;
		m_presets.PushBack( ccp );

		ccp.m_name = CNAME( Near );
		ccp.m_explorationDistance = 1.5f;
		m_presets.PushBack( ccp );

		ccp.m_name = CNAME( Far );
		ccp.m_explorationDistance = 4.5f;
		m_presets.PushBack( ccp );
	}

	m_activeCameraPositionController->SetPreset( m_presets[0] );
}

void CCustomCamera::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("curveSet") )
	{
		m_curveNames.Resize( m_curveSet.Size() );
	}
	else if ( property->GetName() == TXT("curveNames") )
	{
		m_curveSet.Resize( m_curveNames.Size() );
	}

	const Uint32 size = m_curveSet.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( !m_curveSet[ i ] )
		{
			m_curveSet[ i ] = CreateObject< CCurve >( this );
			m_curveSet[ i ]->GetCurveData().AddPoint( 0.f, 0.f );
			m_curveSet[ i ]->GetCurveData().AddPoint( 1.f, 1.f );
		}
	}
}

void CCustomCamera::OnActivate( const SCameraMovementData* data )
{
	if ( data )
	{
		// We don't need to blend any more, we are reactivating camera. Remove the blend controller
		if( m_movementData.m_pivotPositionController.Get() == m_blendPivotPositionController )
		{
			m_movementData.m_pivotPositionController = m_blendPivotPositionController->GetDestinationController();
		}

		m_activeCameraPositionController->Activate( m_movementData, *data );
	}
	else
	{
		m_activeCameraPositionController->Activate( m_movementData );
	}

	m_autoRotationHorTimer = m_manualRotationHorTimeout;
	m_autoRotationVerTimer = m_manualRotationVerTimeout;

	// Force update
	m_activeCameraPositionController->PreUpdate( *this, 0.0f );
	m_activeCameraPositionController->Update( m_movementData, 0.0f );

	SetPosition( m_activeCameraPositionController->GetPosition() );
	SetRotation( m_activeCameraPositionController->GetRotation() );

	ForceUpdateTransformNodeAndCommitChanges();
}

void CCustomCamera::OnActivate( const IScriptable* prevCameraObject, Bool resetCamera )
{
	if ( !resetCamera )
	{
		return;
	}

	const CCustomCamera* prevCam = Cast<const CCustomCamera>( prevCameraObject );
	OnActivate( prevCam ? &prevCam->GetMoveData() : nullptr );
}

void CCustomCamera::ResetCamera()
{
	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		if ( world->GetCameraDirector()->IsCameraResetDisabled() )
		{
			return;
		}
	}

	m_activeCameraPositionController->Reset();
	m_isResetScheduled = true;
}

void CCustomCamera::UpdateWithoutInput()
{
	m_updateInput = false;
	Update( 0.1f );
	m_updateInput = true;
}

void CCustomCamera::UpdateInput( Float timeDelta )
{
	if( m_allowManualRotation )
	{
		Bool horizontal = false, vertical = false;

		m_movementData.m_pivotRotationController.Get()->UpdateInput( horizontal, vertical );

		if( horizontal ) m_autoRotationHorTimer = 0.f;
		if( vertical ) m_autoRotationVerTimer = 0.f;
	}

	if( m_allowAutoRotation )
	{
		if( m_autoRotationHorTimer < m_manualRotationHorTimeout )
		{
			m_autoRotationHorTimer += timeDelta;

			if( !m_movementData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
			{
				m_movementData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_HorizontalRotation );
			}
		}

		if( m_autoRotationVerTimer < m_manualRotationVerTimeout )
		{
			m_autoRotationVerTimer += timeDelta;

			if( !m_movementData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualVertical ) )
			{
				m_movementData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_VerticalRotation );
			}
		}
	}
	else
	{
		if( !m_movementData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
		{
			m_movementData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_HorizontalRotation );
		}

		if( !m_movementData.m_pivotRotationController.Get()->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualVertical ) )
		{
			m_movementData.m_pivotRotationController.Get()->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_VerticalRotation );
		}
	}
}

Bool CCustomCamera::Update( Float timeDelta )
{
	Float scaledTimeDelta = 0;
	if ( timeDelta > 0 )
	{
		// timeDelta is already time scaled (CBaseEngine::Tick)
		// if it's > 0, we rather need to use timescale-independent value from engine multiplied by camera-related time scale, otherwise use 0 so nothing is stepped
		scaledTimeDelta = GEngine->GetLastTimeDelta() * GGame->GetTimeScale( true );
	}

	if ( m_blendInTimeElapsed < m_blendInTime )
	{
		m_blendInTimeElapsed += timeDelta;
		if ( m_blendInTimeElapsed > m_blendInTime )
		{
			m_blendInTime = m_blendInTimeElapsed = 0.0f;
		}
		else
		{
			const Float progress = m_blendInTimeElapsed / m_blendInTime;
			const Float timeDeltaScale = 1.0f - Red::Math::MSqr( 1.0f - progress );
			scaledTimeDelta *= timeDeltaScale;
		}
	}

	if( m_movementData.m_pivotPositionController.Get() == m_blendPivotPositionController && m_blendPivotPositionController->IsFinishedBlending() )
	{
		m_movementData.m_pivotPositionController = m_blendPivotPositionController->GetDestinationController();
	}

	m_activeCameraPositionController->PreUpdate( *this, scaledTimeDelta );

	if ( m_updateInput )
	{
		UpdateInput( scaledTimeDelta );
	}

	m_animation.Update( timeDelta, this );
	m_updateInput = m_animation.IsAdditive();

	m_activeCameraPositionController->Update( m_movementData, scaledTimeDelta );

	Vector		newPosition = m_activeCameraPositionController->GetPosition();
	EulerAngles	newRotation = m_activeCameraPositionController->GetRotation();

	m_animation.ApplyTransform( newPosition, newRotation );

	SetPosition( newPosition );
	SetRotation( newRotation );

	if ( m_isResetScheduled )
	{
		m_isResetScheduled = false;

		// Performs 5 secs worth of updates (without input update)

		m_updateInput = false;
		for ( Uint32 i = 0; i < 50; i++ )
		{
			Update( 0.1f );
		}
		m_updateInput = true;
	}

	return true;
}

Bool CCustomCamera::GetData( Data& outData ) const
{
	outData.m_position = GetPosition();
	outData.m_rotation = GetRotation();
	outData.m_fov = m_fov;

	outData.m_hasFocus = true;
	outData.m_focus = m_movementData.m_pivotPositionValue + m_movementData.m_cameraOffset;

	if( m_forcedNearPlane > 0.f )
	{
		outData.m_nearPlane = m_forcedNearPlane;
		outData.m_forceNearPlane = true;
	}
	else
	{
		outData.m_nearPlane = 0.25f;
		outData.m_forceNearPlane = false;
	}

	outData.m_farPlane = 1000.f;
	outData.m_forceFarPlane = false;
	outData.m_dofParams.Reset();
	outData.m_dofParams.dofIntensity = 0.0f;

	return true;
}

Bool CCustomCamera::StartBlendFrom( const Data& data, Float blendTime )
{
	SCameraMovementData newData = m_movementData;
	m_activeCameraPositionController->OnBeforeStartBlendFrom( newData, data );

	m_blendInTime = blendTime;
	m_blendInTimeElapsed = 0.0f;

	OnActivate( &newData );

	return true;
}

struct CtrlNamePred : public Red::System::NonCopyable
{
	const CName& name;

	CtrlNamePred( const CName& _name ) : name( _name ) {}

	Bool operator()( const ICustomCameraBaseController* ctrl ) const
	{
		ASSERT( ctrl, TXT( "ICustomCameraBaseController in NULL!" ) );
		return *ctrl == name;
	}
};

Bool CCustomCamera::ChangePivotPositionController( const CName& ctrlName )
{
	if( *m_movementData.m_pivotPositionController.Get() == ctrlName )
		return true;

	TDynArray< ICustomCameraPivotPositionController* >::iterator it = FindIf( m_pivotPositionControllers.Begin(), m_pivotPositionControllers.End(), CtrlNamePred( ctrlName ) );
	if( it != m_pivotPositionControllers.End() )
	{
		m_movementData.m_pivotPositionController->Deactivate();
		(*it)->Activate( m_movementData.m_pivotPositionValue, m_movementData.m_pivotPositionController.Get()->GetCurrentZOffset() );
		m_movementData.m_pivotPositionController = *it;
		return true;
	}

	return false;
}

Bool CCustomCamera::ChangePivotRotationController( const CName& ctrlName )
{
	if( *m_movementData.m_pivotRotationController.Get() == ctrlName )
		return true;

	TDynArray< ICustomCameraPivotRotationController* >::iterator it = FindIf( m_pivotRotationControllers.Begin(), m_pivotRotationControllers.End(), CtrlNamePred( ctrlName ) );
	if( it != m_pivotRotationControllers.End() )
	{
		m_movementData.m_pivotRotationController->Deactivate();
		(*it)->Activate( m_movementData.m_pivotRotationValue, m_movementData.m_pivotRotationController.Get()->GetRotationFlags() );
		m_movementData.m_pivotRotationController = *it;
		return true;
	}

	return false;
}

Bool CCustomCamera::ChangePivotDistanceController( const CName& ctrlName )
{
	if( *m_movementData.m_pivotDistanceController.Get() == ctrlName )
		return true;

	TDynArray< ICustomCameraPivotDistanceController* >::iterator it = FindIf( m_pivotDistanceControllers.Begin(), m_pivotDistanceControllers.End(), CtrlNamePred( ctrlName ) );
	if( it != m_pivotDistanceControllers.End() )
	{
		m_movementData.m_pivotDistanceController->Deactivate();
		(*it)->Activate( m_movementData.m_pivotDistanceValue );
		m_movementData.m_pivotDistanceController = *it;
		return true;
	}

	return false;
}

Bool CCustomCamera::BlendToPivotPositionController( const CName& ctrlName, Float blendTime )
{
	if( *m_movementData.m_pivotPositionController.Get() == ctrlName )
		return true;

	TDynArray< ICustomCameraPivotPositionController* >::iterator it = FindIf( m_pivotPositionControllers.Begin(), m_pivotPositionControllers.End(), CtrlNamePred( ctrlName ) );
	if( it != m_pivotPositionControllers.End() )
	{
		if( m_movementData.m_pivotPositionController.Get() == m_blendPivotPositionController )
		{
			// If we are already blending then we need to blend from the blending controller
			CCustomCameraBlendPPC* newBlendController = CreateObject< CCustomCameraBlendPPC >( this );
			m_blendPivotPositionController->SetOwner( *newBlendController );

			newBlendController->Setup( m_blendPivotPositionController, *it, blendTime );
			newBlendController->Activate( m_movementData.m_pivotPositionValue, m_blendPivotPositionController->GetCurrentZOffset() );
			m_blendPivotPositionController = newBlendController;
		}
		else
		{
			if( !m_blendPivotPositionController )
			{
				m_blendPivotPositionController = CreateObject< CCustomCameraBlendPPC >( this );
			}

			m_blendPivotPositionController->Setup( m_movementData.m_pivotPositionController.Get(), *it, blendTime );
			m_blendPivotPositionController->Activate( m_movementData.m_pivotPositionValue, m_movementData.m_pivotPositionController.Get()->GetCurrentZOffset() );
		}

		m_movementData.m_pivotPositionController = m_blendPivotPositionController;
		return true;
	}

	return false;
}

CCurve* CCustomCamera::FindCurve( const CName& curveName ) const
{
	ASSERT( m_curveSet.Size() == m_curveNames.Size() );

	if ( curveName != CName::NONE )
	{
		const Uint32 size = m_curveSet.Size();
		for ( Uint32 i = 0; i < size; ++i )
		{
			if ( m_curveNames[ i ] == curveName )
			{
				return m_curveSet[ i ];
			}
		}
	}

	return nullptr;
}

void CCustomCamera::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Camera ) )
	{
		m_movementData.m_pivotPositionController.Get()->GenerateDebugFragments( frame );
		m_movementData.m_pivotRotationController.Get()->GenerateDebugFragments( frame );
		m_movementData.m_pivotDistanceController.Get()->GenerateDebugFragments( frame );
		m_activeCameraPositionController->GenerateDebugFragments( frame );
	}
}

Bool CCustomCamera::AddDLCAnimset( CSkeletalAnimationSet* animset )
{
	return m_dlcAnimSets.PushBackUnique( animset );
}

Bool CCustomCamera::RemoveDLCAnimset( CSkeletalAnimationSet* animset )
{
	return m_dlcAnimSets.Remove( animset ); 
}

//////////////////////////////////////////////////////////////////////////

void CCustomCamera::funcActivate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Float, blendTime, 0.f );
	FINISH_PARAMETERS;

	if( GGame->GetActiveWorld() )
	{
		GGame->GetActiveWorld()->GetCameraDirector()->ActivateCamera( this, this, blendTime );
	}
}

void CCustomCamera::funcGetActivePivotPositionController( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_movementData.m_pivotPositionController.Get() );
}

void CCustomCamera::funcGetActivePivotRotationController( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_movementData.m_pivotRotationController.Get() );
}

void CCustomCamera::funcGetActivePivotDistanceController( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_movementData.m_pivotDistanceController.Get() );
}

void CCustomCamera::funcGetActivePreset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( SCustomCameraPreset, m_activeCameraPositionController->GetPreset() );
}

void CCustomCamera::funcChangePivotPositionController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( ChangePivotPositionController( name ) );
}

void CCustomCamera::funcChangePivotRotationController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( ChangePivotRotationController( name ) );
}

void CCustomCamera::funcChangePivotDistanceController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( ChangePivotDistanceController( name ) );
}

void CCustomCamera::funcBlendToPivotPositionController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Float, blendTime, 1.f );
	FINISH_PARAMETERS;

	RETURN_BOOL( BlendToPivotPositionController( name, blendTime ) );
}

void CCustomCamera::funcPlayAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SCameraAnimationDefinition, def, SCameraAnimationDefinition() );
	FINISH_PARAMETERS;

	const CSkeletalAnimationSetEntry* animEntry = m_animSet ? m_animSet->FindAnimation( def.m_animation ) : NULL;
	const CSkeletalAnimation* anim = animEntry ? animEntry->GetAnimation() : NULL;
	if( anim )
	{
		m_animation.PlayAnimation( anim, def );
		return;
	}

	for( const CSkeletalAnimationSet* animSet : m_dlcAnimSets )
	{
		animEntry = animSet ? animSet->FindAnimation( def.m_animation ) : NULL;
		anim = animEntry ? animEntry->GetAnimation() : NULL;
		if( anim )
		{
			m_animation.PlayAnimation( anim, def );
			return;
		}
	}
}

void CCustomCamera::funcStopAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, anim, CName::NONE );
	FINISH_PARAMETERS;

	m_animation.StopAnimation( anim );
}

void CCustomCamera::funcFindCurve( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_OBJECT( FindCurve( name ) );
}

void CCustomCamera::funcSetManualRotationHorTimeout( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, timeout, 1.f );
	FINISH_PARAMETERS;

	// Set the timer appropriately to the new timeout
	if( m_autoRotationHorTimer >= m_manualRotationHorTimeout )
	{
		m_autoRotationHorTimer = timeout;
	}

	m_manualRotationHorTimeout = timeout;
}

void CCustomCamera::funcSetManualRotationVerTimeout( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, timeout, 1.f );
	FINISH_PARAMETERS;

	// Set the timer appropriately to the new timeout
	if( m_autoRotationVerTimer >= m_manualRotationVerTimeout )
	{
		m_autoRotationVerTimer = timeout;
	}

	m_manualRotationVerTimeout = timeout;
}

void CCustomCamera::funcGetManualRotationHorTimeout( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( m_manualRotationHorTimeout );
}

void CCustomCamera::funcGetManualRotationVerTimeout( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( m_manualRotationVerTimeout );
}

void CCustomCamera::funcIsManualControledHor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool res = false;
	ICustomCameraPivotRotationController* controller = m_movementData.m_pivotRotationController.Get();
	if( controller )
	{
		res = m_autoRotationHorTimer < m_manualRotationHorTimeout;
	}
	RETURN_BOOL( res );
}

void CCustomCamera::funcIsManualControledVer( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool res = false;
	ICustomCameraPivotRotationController* controller = m_movementData.m_pivotRotationController.Get();
	if( controller )
	{
		res = m_autoRotationVerTimer < m_manualRotationVerTimeout;
	}
	RETURN_BOOL( res );
}

void CCustomCamera::funcEnableManualControl( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	if( !enable )
	{
		ICustomCameraPivotRotationController* rotCtrl = m_movementData.m_pivotRotationController.Get();
		if( rotCtrl->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal ) )
		{
			rotCtrl->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualHorizontal  | ICustomCameraPivotRotationController::ECCPRF_HorizontalRotation | ICustomCameraPivotRotationController::ECCPRF_AbsoluteHorizontal );
		}

		if( rotCtrl->HasRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualVertical ) )
		{
			rotCtrl->ClearRotationFlag( ICustomCameraPivotRotationController::ECCPRF_ManualVertical  | ICustomCameraPivotRotationController::ECCPRF_VerticalRotation | ICustomCameraPivotRotationController::ECCPRF_AbsoluteVertical );
		}

		ForceManualControlHorTimeout();
		ForceManualControlVerTimeout();
	}

	m_allowManualRotation = enable;
}

void CCustomCamera::funcForceManualControlHorTimeout( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ForceManualControlHorTimeout();
}

void CCustomCamera::funcForceManualControlVerTimeout( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ForceManualControlVerTimeout();
}

void CCustomCamera::funcChangePreset( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	struct Pred : public Red::System::NonCopyable
	{
		const CName m_name;
		Pred( const CName& name ) : m_name( name ) {}

		Bool operator()( const SCustomCameraPreset& preset ) const { return preset.m_name == m_name; }
	};

	TDynArray< SCustomCameraPreset >::const_iterator it = FindIf( m_presets.Begin(), m_presets.End(), Pred( name ) );
	if( it != m_presets.End() )
	{
		m_activeCameraPositionController->SetPreset( *it );
	}
}

void CCustomCamera::funcNextPreset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const TDynArray< SCustomCameraPreset >::const_iterator end = m_presets.End();
	TDynArray< SCustomCameraPreset >::const_iterator it;
	for( it = m_presets.Begin(); it != end; ++it )
	{
		if( it->m_name == m_activeCameraPositionController->GetPreset().m_name )
		{
			++it;
			break;
		}
	}

	if( it == end ) it = m_presets.Begin();

	m_activeCameraPositionController->SetPreset( *it );
}

void CCustomCamera::funcPrevPreset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const TDynArray< SCustomCameraPreset >::const_iterator end = m_presets.End();
	TDynArray< SCustomCameraPreset >::const_iterator it;
	for( it = m_presets.Begin(); it != end; ++it )
	{
		if( it->m_name == m_activeCameraPositionController->GetPreset().m_name )
		{
			if( it == m_presets.Begin() )
			{
				it = end;
			}

			--it;
			break;
		}
	}

	RED_ASSERT( it != end );

	m_activeCameraPositionController->SetPreset( *it );
}

void CCustomCamera::funcSetCollisionOffset( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, offset, Vector::ZEROS );
	FINISH_PARAMETERS;

	m_activeCameraPositionController->SetColisionOriginOffset( offset );
}

void CCustomCamera::funcEnableScreenSpaceCorrection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	m_activeCameraPositionController->EnableScreenSpaceCorrections( enable );
}

void CCustomCamera::funcSetAllowAutoRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, allow, true );
	FINISH_PARAMETERS;
	m_allowAutoRotation = allow;
}

//////////////////////////////////////////////////////////////////////////
// CAMERA ANIMATION
//////////////////////////////////////////////////////////////////////////

CCustomCameraAnimation::CCustomCameraAnimation()
	: m_rotation( EulerAngles::ZEROS )
	, m_translation( Vector::ZEROS )
	, m_transfrom( Matrix::IDENTITY )
{
}

void CCustomCameraAnimation::PlayAnimation( const CSkeletalAnimation* animation, const SCameraAnimationDefinition& definition )
{
	ASSERT( animation );

	SAnimInstance newAnim( animation, definition );
	Uint32 index = 0;

	const TDynArray< SAnimInstance >::const_iterator end = m_animations.End();
	for( TDynArray< SAnimInstance >::iterator it = m_animations.Begin(); it != end; ++it )
	{
		if( *it == newAnim )
		{
			if( newAnim.m_definition.m_priority >= it->m_definition.m_priority )
			{
				TDynArray< SAnimInstance >::const_iterator next = it + 1;
				TDynArray< SAnimInstance >::const_iterator to = it;
				while( next != end && newAnim.m_definition.m_priority >= next->m_definition.m_priority )
				{
					to = next;
					++next;
				}

				// should we keep current weight and time?
				if( !newAnim.m_definition.m_reset )
				{
					newAnim.m_time = it->m_time;
					newAnim.m_weight = it->m_weight;
				}

				*it = newAnim;

				while( it != to )
				{
					Swap( *(it+1), *it );
					++it;
				}
			}
			else
			{
				TDynArray< SAnimInstance >::const_iterator head = m_animations.Begin() - 1;
				TDynArray< SAnimInstance >::const_iterator prev = it - 1;
				TDynArray< SAnimInstance >::const_iterator to = it;
				while( prev != head && prev->m_definition.m_priority > newAnim.m_definition.m_priority )
				{
					to = prev;
					--prev;
				}

				// should we keep current weight and time?
				if( !newAnim.m_definition.m_reset )
				{
					newAnim.m_time = it->m_time;
					newAnim.m_weight = it->m_weight;
				}

				*it = newAnim;

				while( it != to )
				{
					Swap( *(it-1), *it );
					--it;
				}
			}

			return;
		}
		else if( newAnim.m_definition.m_priority >= it->m_definition.m_priority )
		{
			++index;
		}
	}

	m_animations.Insert( index, newAnim );
}

void CCustomCameraAnimation::StopAnimation( const CName& anim )
{
	const TDynArray< SAnimInstance >::const_iterator end = m_animations.End();
	for( TDynArray< SAnimInstance >::iterator it = m_animations.Begin(); it != end; ++it )
	{
		if( it->m_definition.m_animation == anim )
		{
			m_animations.Erase( it );
			break;
		}
	}
}

void CCustomCameraAnimation::SetupMovementData( SCameraMovementData& moveData, const AnimQsTransform& cameraTransformModelSpace, const AnimQsTransform& playerTransformWorldSpace )
{
	AnimQsTransform finalWorldCameraTransform;

	finalWorldCameraTransform.SetMul( playerTransformWorldSpace, cameraTransformModelSpace );
	Matrix finalWorldCameraMatrix = AnimQsTransformToMatrix( finalWorldCameraTransform );
	EulerAngles finalWorldCameraEuler = finalWorldCameraMatrix.ToEulerAnglesFull(); // probably there is no method to convert form AnimQsTransform to EulerAngles:D TODO: add such method!

	moveData.m_pivotRotationValue.Yaw = finalWorldCameraEuler.Yaw;
	moveData.m_pivotRotationValue.Pitch = finalWorldCameraEuler.Pitch;
	moveData.m_pivotRotationVelocity = EulerAngles::ZEROS;

	Vector oldPos = moveData.m_pivotPositionValue;
	moveData.m_pivotPositionValue = AnimVectorToVector( playerTransformWorldSpace.GetTranslation() );
	moveData.m_pivotPositionValue.Z = oldPos.Z;
	moveData.m_pivotPositionVelocity = Vector::ZEROS;

	moveData.m_cameraOffset = Vector::ZEROS;
	moveData.m_cameraLocalSpaceOffset = Vector::ZEROS;
	moveData.m_cameraLocalSpaceOffsetVel = Vector::ZEROS;

	moveData.m_pivotDistanceValue = ( moveData.m_pivotPositionValue.DistanceTo( finalWorldCameraMatrix.GetTranslation() ) );
	moveData.m_pivotDistanceVelocity = 0.0f;

_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddLine( CName::NONE, moveData.m_pivotPositionValue, finalWorldCameraMatrix.GetTranslation(), true, Color::WHITE, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddLine( CName::NONE, finalWorldCameraMatrix.GetTranslation(), finalWorldCameraMatrix.GetTranslation()+Vector::EZ*0.5f, true, Color::WHITE, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddSphere( CName::NONE, 0.01f, finalWorldCameraMatrix.GetTranslation()+Vector::EZ*0.5f, true, Color::WHITE, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddAxis( CName::NONE, 0.2f, finalWorldCameraMatrix.GetTranslation()+Vector::EZ*0.5f, finalWorldCameraMatrix.ToEulerAnglesFull(), true, 60, Color::WHITE ); )
}

AnimQsTransform CCustomCameraAnimation::FindPlayerWorldTransformAtEndOfFinisher( const String& cameraAnimName )
{
	AnimQsTransform currPlayerTransform = MatrixToAnimQsTransform( GGame->GetPlayerEntity()->GetLocalToWorld() );
	AnimQsTransform playerTransformAtAnimEndWorldSpace = currPlayerTransform; // in case that we don't find finisher anim on player..

	// extract anim name that should be played on entity(finishers name convention..):
	size_t index;
	cameraAnimName.FindSubstring( CAMERA_FINISHER_MARK, index );
	String animSearchString = cameraAnimName.LeftString( index ); 

	// Find matching finisher anim among recently used anims on player:
	const SBehaviorUsedAnimationData* selectedAnimData = CamTools::FindRecentlyUsedAnim( *( GGame->GetPlayerEntity() ),  animSearchString );

	if ( selectedAnimData )
	{
		Float animEndTime = selectedAnimData->m_animation->GetAnimation()->GetDuration();
		playerTransformAtAnimEndWorldSpace = CamTools::GetFutureTransformBasedOnAnim( *( selectedAnimData->m_animation->GetAnimation() ), selectedAnimData->m_currTime, animEndTime, currPlayerTransform );
	}
	else
	{
		RED_ASSERT( false, TXT("Anim Finisher: %s not found among recently used anims on player!"), animSearchString.AsChar() );
	}

	return playerTransformAtAnimEndWorldSpace;
}

void CCustomCameraAnimation::Update( Float timeDelta, CCustomCamera* camera )
{
	if( m_animations.Empty() ) return;

	TDynArray< AnimQsTransform > transforms;

	// Update primary animation
	{
		SAnimInstance& anim = m_animations.Back();

		while( !m_animations.Empty() )
		{
			anim = m_animations.Back();
			anim.m_time += timeDelta * anim.m_definition.m_speed;

			const Float duration = anim.m_animation->GetDuration();

			if( anim.m_time > duration )
			{
				if( anim.m_definition.m_loop )
				{
					anim.m_time -= duration;
				}
				else
				{
					m_animations.PopBackFast();
					continue;
				}
			}

			// Manage finisher case:
			if ( !anim.m_definition.m_additive && CamTools::IsBetween(anim.m_time, anim.m_definition.m_blendIn, anim.m_definition.m_blendIn + timeDelta ) )
			{
				// Note that we set camera at the begin of animation, cuz camera animation can be longer than corresponding anim that should be 
				// played on player.
				// Assumption that if camera anim name contains _camera_, then we are playing finisher, according to finishers name convention..
				if ( anim.m_animation->GetName().AsString().ContainsSubstring( CAMERA_FINISHER_MARK ) ) 
				{
					SetCameraToTheEndOfFinisherAnimation( anim, camera ); 
				}
			}

			const Float startBlendOutTime = duration - anim.m_definition.m_blendOut;
			if( !anim.m_definition.m_loop && anim.m_time > startBlendOutTime )
			{
				const Float remainingTime = Max( duration - anim.m_time, 0.f );
				anim.m_weight = (remainingTime / anim.m_definition.m_blendOut) * anim.m_definition.m_weight;
			}
			else if( anim.m_weight != anim.m_definition.m_weight )
			{
				if( anim.m_definition.m_blendIn > 0.0001f )
				{
					anim.m_weight = Min( anim.m_weight + timeDelta / anim.m_definition.m_blendIn, anim.m_definition.m_weight );
				}
				else
				{
					anim.m_weight = anim.m_definition.m_weight;
				}
			}

			break;
		}

		if( !m_animations.Empty() )
		{
			anim.m_animation->Sample( anim.m_time, m_bones, m_tracks );

			ASSERT( !m_bones.Empty() );

			if( anim.m_definition.m_additive )
			{
				AnimQsTransform& bone = m_bones[ m_bones.Size() - 1 ];

#ifdef USE_HAVOK_ANIMATION
				COMPILE_ASSERT( false );
#else
				SetMul(bone.Translation, anim.m_weight);
				bone.Rotation.SetSlerp( RedQuaternion::IDENTITY, bone.GetRotation(), anim.m_weight );
#endif

				transforms.PushBack( bone );
			}
			else
			{
				Uint32 currBone = m_bones.Size() - 1;
				AnimQsTransform transform = m_bones[currBone];
				--currBone;

				while( currBone != -1 )
				{			
					transform.SetMul( m_bones[currBone], transform );
					--currBone;
				}

				transforms.PushBack( transform );
			}
		}
	}

	// Update secondary
	Int32 i = (m_animations.Empty() || m_animations.Back().m_definition.m_exclusive) ? -1 : m_animations.SizeInt() - 2;

	while( i >= 0 )
	{
		SAnimInstance& anim = m_animations[i];

		anim.m_time += timeDelta;
		const Float duration = anim.m_animation->GetDuration();

		if( anim.m_time > duration )
		{
			if( anim.m_definition.m_loop )
			{
				anim.m_time -= duration;
			}
			else
			{
				m_animations.RemoveAt( i );
				--i;
				continue;
			}
		}

		// TODO: Playing non additive animations as secondary. Currently no such use case so ignore it
		if( !anim.m_definition.m_additive )
		{
			anim.m_weight = 0.f;
			--i;
			continue;
		}

		const Float startBlendOutTime = duration - anim.m_definition.m_blendOut;

		// Play exclusive animation only if primary
		if( (!anim.m_definition.m_loop && anim.m_time > startBlendOutTime) || anim.m_definition.m_exclusive )
		{
			anim.m_weight = Max( anim.m_weight - (timeDelta / anim.m_definition.m_blendOut) * anim.m_definition.m_weight, 0.f );
		}
		else if( anim.m_weight != anim.m_definition.m_weight )
		{
			if( anim.m_definition.m_blendIn > 0.0001f )
			{
				anim.m_weight = Min( anim.m_weight + (timeDelta / anim.m_definition.m_blendIn) * anim.m_definition.m_weight, anim.m_definition.m_weight );
			}
			else
			{
				anim.m_weight = anim.m_definition.m_weight;
			}
		}

		if( anim.m_weight > 0.f )
		{
			anim.m_animation->Sample( anim.m_time, m_bones, m_tracks );

			ASSERT( !m_bones.Empty() );
			AnimQsTransform& bone = m_bones[ m_bones.Size() - 1 ];

#ifdef USE_HAVOK_ANIMATION
			COMPILE_ASSERT( false );
#else
			SetMul(bone.Translation, anim.m_weight);
			bone.Rotation.SetSlerp( RedQuaternion::IDENTITY, bone.GetRotation(), anim.m_weight );
#endif

			transforms.PushBack( bone );
		}

		--i;
	}

#ifdef USE_HAVOK_ANIMATION
	COMPILE_ASSERT( false );
#else
	const Uint32 size = transforms.Size();

	if( size > 0 )
	{
		AnimQsTransform temp = transforms[0];

		for( Uint32 i = 1; i < size; ++i )
		{
			temp.SetMul( temp, transforms[i] );
		}

		m_transfrom = AnimQsTransformToMatrix( temp );
		m_translation = m_transfrom.GetTranslation();
		m_rotation = m_transfrom.ToEulerAnglesFull();
	}
	else
	{
		m_transfrom.SetIdentity();
		m_translation = Vector::ZERO_3D_POINT;
		m_rotation = EulerAngles::ZEROS;
	}
#endif
}

//////////////////////////////////////////////////////////////
void CCustomCameraAnimation::SetCameraToTheEndOfFinisherAnimation( const SAnimInstance animInstance, CCustomCamera* camera )
{
	String currCameraFinisherAnim = animInstance.m_animation->GetName().AsString();

	AnimQsTransform playerWorldTransformFinisherEnd = FindPlayerWorldTransformAtEndOfFinisher( currCameraFinisherAnim ); 
	AnimQsTransform cameraLocalTransformFinisherEnd = CamTools::GetCameraTransformFromAnim( *animInstance.m_animation, animInstance.m_animation->GetDuration(), m_bones, m_tracks );

	SCameraMovementData& moveData = camera->GetMoveData();
	SetupMovementData( moveData, cameraLocalTransformFinisherEnd, playerWorldTransformFinisherEnd );
	camera->GetActiveController()->ResetColliisons();
}

Bool CCustomCameraAnimation::IsAdditive()
{
	// Treat no animation as IDENTITY additive
	if( m_animations.Empty() ) return true;

	return m_animations.Back().m_definition.m_additive;
}

void CCustomCameraAnimation::ApplyTransform( Vector& cameraPosition, EulerAngles& cameraRotation )
{
	if( IsAdditive() )
	{
		cameraPosition = cameraPosition + cameraRotation.TransformPoint( GetTranslation() );
		cameraRotation = cameraRotation + GetRotation();
	}
	else
	{
		SAnimInstance& desc = m_animations.Back();
        const Matrix& ltw = GGame->GetPlayerEntity()->GetLocalToWorld(); 

_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddSphere( CName::NONE, 0.02f, cameraPosition, true, Color::YELLOW, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddAxis( CName::NONE, 0.2f, cameraPosition, cameraRotation, true, 60, Color::YELLOW); )

		Matrix result = GetTransform() * ltw;
		cameraPosition = Vector::Interpolate( cameraPosition, result.GetTranslation(), desc.m_weight );
		cameraRotation = EulerAngles::Interpolate( cameraRotation, result.ToEulerAnglesFull(), desc.m_weight );

_DEBUG_DRAW_( Color col = desc.m_weight < 0.95f ? Color::RED : Color::LIGHT_BLUE; )

_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddSphere( CName::NONE, 0.01f, result.GetTranslation(), true, col, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddAxis( CName::NONE, 0.2f, result.GetTranslation(), result.ToEulerAnglesFull(), true, 60, col ); )

_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddSphere( CName::NONE, 0.03f, cameraPosition, true, Color::GREEN, 60); )
_DEBUG_DRAW_( GCommonGame->GetVisualDebug()->AddAxis( CName::NONE, 0.2f, cameraPosition, cameraRotation, true, 60, Color::GREEN ); )
	}
}
