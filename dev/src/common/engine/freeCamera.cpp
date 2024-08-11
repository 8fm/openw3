/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "freeCamera.h"
#include "../core/clipboardBase.h"
#include "redGuiManager.h"
#include "viewport.h"
#include "inputBufferedInputEvent.h"
#include "game.h"
#include "inputKeys.h"
#include "entity.h"
#include "camera.h"
#include "rawInputManager.h"
#include "mesh.h"

CGameFreeCamera::CGameFreeCamera()
	: m_moveForwardBackward( 0.0f )
	, m_moveLeftRight( 0.0f )
	, m_lookUpDown( 0.0f )
	, m_lookLeftRight( 0.0f )
	, m_cameraRoll( 0.0f )
	, m_position( 0,0,0 )
	, m_rotation( 0,0,0 )
	, m_speed( 0.0f )
	, m_fov( 52.0f )
	, m_skipInputs( false )
	, m_attachedToPlayer( false )
	, m_attachOffset( Vector::ZERO_3D_POINT )
	, m_fovDistanceMultiplier( 1.f )
	, m_dofMultiplier( 0.0f )
	, m_dofFarOffset( 0.0f )
	, m_dofNearOffset( 0.0f )
{
	CacheFovDistanceMultiplier();
}

CGameFreeCamera::~CGameFreeCamera()
{
}

void CGameFreeCamera::MoveTo( const Vector& position, const EulerAngles& rotation )
{
	m_position = position;
	m_rotation = rotation;
	m_visiblePosition = position;
	m_visibleRotation = m_rotation;
	m_lastFreeCameraData = SRenderCameraLastFrameData::INVALID;
}

Bool CGameFreeCamera::CanProcessedInputs() const
{
	return !m_skipInputs;
}

Bool CGameFreeCamera::ProcessInput( enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef NO_RED_GUI
	if( GRedGui::GetInstance().GetEnabled() == true && GRedGui::GetInstance().GetInputManager()->IsControlPressed() == false )
	{
		return false;
	}
#endif
	const Float value = ( action == IACT_Release ) ? 0.0f : 5.0f;

	if ( key == IK_Pad_RightShoulder )
	{	
		m_skipInputs = action == IACT_Press;

		if ( m_skipInputs )
		{
			Reset();
		}

		return true;	
	}
	
	if ( m_attachedToPlayer && key == IK_Pad_RightThumb && action == IACT_Release )
	{
		m_attachedToPlayer = false;
		m_attachOffset = Vector::ZERO_3D_POINT;

		return true;
	}
	else if ( m_skipInputs && !m_attachedToPlayer && key == IK_Pad_RightThumb && action == IACT_Press && GGame->GetPlayerEntity() )
	{
		m_attachedToPlayer = true;
		m_attachOffset = m_position - GGame->GetPlayerEntity()->GetWorldPositionRef();

		return true;
	}

	if ( m_skipInputs && key == IK_Pad_LeftShoulder )
	{	
		m_skipInputs = action == IACT_Press;

		if ( m_skipInputs )
		{
			Reset();
		}

		return true;	
	}

	if ( m_skipInputs )
	{
		return false;
	}

	if( key == IK_Q || key == IK_Pad_Y_TRIANGLE )
	{
		if ( RIM_IS_KEY_DOWN( IK_Pad_LeftThumb ) )
		{
			m_dofMultiplier += value;
		}
		else
		{
			m_moveUpDown = value * 0.05f;
		}
	}

	if( key == IK_E || key == IK_Pad_A_CROSS )
	{
		if ( RIM_IS_KEY_DOWN( IK_Pad_LeftThumb ) )
		{
			m_dofMultiplier -= value;
		}
		else
		{
			m_moveUpDown = -value * 0.05f;
		}
	}

	if ( key == IK_W )
	{
		m_moveForwardBackward = value;
		return true;
	}
	else if( key == IK_S )
	{
		m_moveForwardBackward = -value;
		return true;
	}
	else if ( key == IK_A )
	{
		m_moveLeftRight = -value;
		return true;
	}
	else if ( key == IK_D )
	{
		m_moveLeftRight = +value;
		return true;
	}		
	else if ( key == IK_LShift )
	{
		m_speed = value;
		return true;
	}
	else if ( key == IK_MouseX )
	{
		m_lookLeftRight = -data;
		return true;
	}
	else if ( key == IK_MouseY )
	{
		m_lookUpDown = -data;
		return true;
	}
	else if ( key == IK_R || key == IK_Pad_DigitLeft )
	{
		m_cameraRoll = -data;
	}
	else if ( key == IK_F || key == IK_Pad_DigitRight )
	{
		m_cameraRoll = data;
	}
	else if ( key == IK_Pad_RightAxisX )
	{
		m_lookLeftRight = -data * 5.0f;
		return true;	
	}
	else if ( key == IK_Pad_RightAxisY )
	{
		m_lookUpDown = data * 5.0f;
		return true;	
	}
	else if ( key == IK_Pad_LeftAxisX )
	{
		m_moveLeftRight = data;
		return true;	
	}
	else if ( key == IK_Pad_LeftAxisY )
	{
		m_moveForwardBackward = data;
		return true;	
	}

	if ( key == IK_Pad_LeftTrigger )
	{		
		m_speed = data * 5.0f;
		return true;	
	}

	if ( key == IK_Pad_B_CIRCLE )
	{
		if ( RIM_IS_KEY_DOWN( IK_Pad_LeftThumb ) )
		{
			if ( RIM_IS_KEY_DOWN( IK_Pad_LeftShoulder ) )
			{
				m_dofNearOffset += value*5;
			}
			else
			{
				m_dofFarOffset += value*5;
			}
		}
		else
		{
			GGame->EnableFreeCamera( false );
		}
	}

	if ( key == IK_Pad_X_SQUARE && GGame->GetPlayerEntity() )
	{
		if ( RIM_IS_KEY_DOWN( IK_Pad_LeftThumb ) )
		{
			if ( RIM_IS_KEY_DOWN( IK_Pad_LeftShoulder ) )
			{
				m_dofNearOffset -= value*5;
			}
			else
			{
				m_dofFarOffset -= value*5;
			}
		}
		else
		{
			EulerAngles rot = EulerAngles::ZEROS;
			rot.Yaw = m_rotation.Yaw;
			GGame->GetPlayerEntity()->Teleport( m_position, rot );
		}
	}

#ifndef NO_EDITOR
	if ( GCameraClipboard )
	{
		// Copy camera view to clipboard
		if ( key == IK_C && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
		{
			GCameraClipboard->Copy( m_position, m_rotation );
			return true;
		}
		// Paste camera view from clipboard
		else if ( key == IK_V && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
		{
			GCameraClipboard->Paste( m_position, m_rotation );
			return true;
		}
		// Copy/paste to editor bookmark 0..9
		else if ( key >= IK_0 && key <= IK_9 && RIM_IS_KEY_DOWN( IK_Ctrl )  )
		{
			// Copy
			if ( RIM_IS_KEY_DOWN( IK_Alt ) )
			{
				GCameraClipboard->CopyToBookmark( key - IK_0, m_position, m_rotation );
			}
			// Paste
			else
			{
				if ( GCameraClipboard->PasteFromBookmark( key - IK_0, m_position, m_rotation ) )
				{
					// Teleport the player to the camera
					if ( RIM_IS_KEY_DOWN( IK_Shift ) && GGame->GetPlayerEntity() )
					{
						GGame->GetPlayerEntity()->Teleport( m_position, GGame->GetPlayerEntity()->GetWorldRotation() );
						GGame->EnableFreeCamera( false );
					}
				}
			}
			return true;
		}
	}
	else if ( GClipboard )	// No GCameraClipboard, but we still have a clipboard
	{
		if ( key == IK_C && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
		{
			if ( GClipboard != nullptr )
			{
				GClipboard->Copy( TXT("[[") + ToString( m_position ) + TXT("|") + ToString( m_rotation ) + TXT("]]") );
			}
			else
			{
				RED_LOG( FreeCamera, TXT("Camera View: %ls"), String( TXT("[[") + ToString( m_position ) + TXT("|") + ToString( m_rotation ) + TXT("]]") ).AsChar() );
			}
			return true;
		}
		else if ( key == IK_V && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
		{
			String text;
			if ( GClipboard != nullptr && GClipboard->Paste( text ) )
			{
				Vector position;
				EulerAngles rotation;
				if ( ParseCameraViewString( text, position, rotation ) )
				{
					m_position = position;
					m_rotation = rotation;
				}
			}
		}
	}
#else
	// Copy/paste camera view to clipboard (game edition, please keep the format in sync with editor)
#ifdef RED_PLATFORM_CONSOLE
	if ( key == IK_C && action == IACT_Press )
#else
	if ( key == IK_C && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
#endif // RED_PLATFORM_CONSOLE
	{
		if ( GClipboard != nullptr )
		{
			GClipboard->Copy( TXT("[[") + ToString( m_position ) + TXT("|") + ToString( m_rotation ) + TXT("]]") );
		}
		else
		{
			RED_LOG( FreeCamera, TXT("Camera View: %ls"), String( TXT("[[") + ToString( m_position ) + TXT("|") + ToString( m_rotation ) + TXT("]]") ).AsChar() );
		}
		return true;
	}
#ifdef RED_PLATFORM_CONSOLE
	else if ( key == IK_V && action == IACT_Press )
#else
	else if ( key == IK_V && RIM_IS_KEY_DOWN( IK_Ctrl ) && RIM_IS_KEY_DOWN( IK_Alt ) && action == IACT_Press )
#endif // RED_PLATFORM_CONSOLE
	{
		String text;
		if ( GClipboard != nullptr && GClipboard->Paste( text ) )
		{
			Vector position;
			EulerAngles rotation;
			if ( ParseCameraViewString( text, position, rotation ) )
			{
				m_position = position;
				m_rotation = rotation;
			}
		}
	}
#endif

	return false;
}

void CGameFreeCamera::Tick( Float timeDelta )
{
	timeDelta /= GGame->GetTimeScale();

	if ( !m_attachedToPlayer )
	{
		// Get camera vectors
		Vector forward, side;
		EulerAngles rotation;
		rotation = m_rotation;
		rotation.Normalize();
		rotation.ToAngleVectors( &forward, &side, NULL );

		// Calculate new position
		if ( Red::Math::MAbs( m_moveForwardBackward ) > 0.0f || 
			 Red::Math::MAbs( m_moveLeftRight ) > 0.0f || 
			 Red::Math::MAbs( m_moveUpDown ) > 0.0f )
		{
			const Float speed = 5.0f + m_speed * 5.0f;
			const Vector forwardBackwardDelta = forward * ( timeDelta * m_moveForwardBackward * speed );
			const Vector leftRightDelta = side * ( timeDelta * m_moveLeftRight * speed );
			const Vector upDownDelta = Vector::EZ * ( timeDelta * m_moveUpDown * speed );
			m_position += forwardBackwardDelta + leftRightDelta + upDownDelta;
		}

		// Calculate new rotation
		if ( Red::Math::MAbs( m_lookUpDown ) > 0.0f || 
			 Red::Math::MAbs( m_lookLeftRight ) > 0.0f ||
			 Red::Math::MAbs( m_cameraRoll ) > 0.0f )
		{
			const Float rotUpDownDelta = timeDelta * m_lookUpDown * 20.0f;
			const Float rotLeftRightDelta = timeDelta * m_lookLeftRight * 20.0f;
			const Float rotRollDelta = timeDelta * m_cameraRoll * 20.0f;
			m_rotation.Yaw += rotLeftRightDelta;
			m_rotation.Pitch += rotUpDownDelta;
			m_rotation.Roll += rotRollDelta;
		}

		// Smoothly update the visible position/rotation
		// (this depends on framerate, but we're always running at 60fps, right? riiight?)
		m_visiblePosition += ( m_position - m_visiblePosition )*0.6f;
		m_visibleRotation += ( m_rotation - m_visibleRotation )*0.6f;
	}
	else
	{
		// TODO:

		// Get camera vectors
		Vector forward, side;
		m_rotation.ToAngleVectors( &forward, &side, NULL );

		// Calculate new position
		const Float speed = 5.0f + m_speed * 5.0f;
		const Vector forwardBackwardDelta = forward * ( timeDelta * m_moveForwardBackward * speed );
		const Vector leftRightDelta = side * ( timeDelta * m_moveLeftRight * speed );

		// Update offset
		m_attachOffset += forwardBackwardDelta + leftRightDelta;

		Matrix temp;
		temp = m_rotation.ToMatrix();
		temp.SetTranslation( m_attachOffset );

		m_position = GGame->GetPlayerEntity()->GetWorldPositionRef() + temp.GetTranslationRef();

		// Calculate new rotation
		const Float rotUpDownDelta = timeDelta * m_lookUpDown * 20.0f;
		const Float rotLeftRightDelta = timeDelta * m_lookLeftRight * 20.0f;
		const Float rotRollDelta = timeDelta * m_cameraRoll * 20.0f;
		m_rotation.Yaw += rotLeftRightDelta;
		m_rotation.Pitch += rotUpDownDelta;
		m_rotation.Roll += rotRollDelta;

		// Update visible position/rotation directly from camera
		m_visiblePosition = m_position;
		m_visibleRotation = m_rotation;
	}

	m_lookUpDown = m_lookLeftRight = 0.f;

	// build cached camera
	{
		const Float aspect = GGame->GetViewport()->GetWidth() / (Float)GGame->GetViewport()->GetHeight();

		// Create new camera
		EulerAngles normalizedRotation = m_visibleRotation;
		normalizedRotation.Normalize();
		m_cachedCamera.Set( m_visiblePosition, normalizedRotation, m_fov, aspect, 0.01f, 1000.0f, 1.0f );

		// Restore last frame data from original camera
		m_cachedCamera.SetLastFrameData( m_lastFreeCameraData );

		// Save cached camera for the next frame
		m_lastFreeCameraData.Init( GGame->GetEngineTime() , m_cachedCamera);
	}
	
	if ( RIM_IS_KEY_DOWN( IK_Pad_DigitUp ) )
	{
		if ( GetFOV() < 150 ) SetFOV( GetFOV() + 1 );
	}
	else if ( RIM_IS_KEY_DOWN( IK_Pad_DigitDown ) )
	{
		if ( GetFOV() > 5 ) SetFOV( GetFOV() - 1 );
	}
}

void CGameFreeCamera::CalculateCamera( CRenderCamera &camera ) const
{
	// Simply copy cached camera
	camera = m_cachedCamera;
}

void CGameFreeCamera::ConfigureFrameInfo( CRenderFrameInfo& info ) const
{
	Float v = Clamp<Float>( m_dofMultiplier, 0, 1000 );
	info.m_envParametersArea.m_depthOfField.m_intensity.m_value *= Vector( v, v, v, v );
	v = m_dofNearOffset;
	info.m_envParametersArea.m_depthOfField.m_nearFocusDist.m_value += Vector( v, v, v, v );
	info.m_envParametersArea.m_depthOfField.m_nearBlurDist.m_value += Vector( v, v, v, v );
	v = m_dofFarOffset;
	info.m_envParametersArea.m_depthOfField.m_farFocusDist.m_value += Vector( v, v, v, v );
	info.m_envParametersArea.m_depthOfField.m_farBlurDist.m_value += Vector( v, v, v, v );
}

void CGameFreeCamera::Reset()
{
	m_moveForwardBackward = 0.f;
	m_moveLeftRight = 0.f;
	m_lookUpDown = 0.f;
	m_lookLeftRight = 0.f;
	m_speed = 0.f;
	m_visiblePosition = m_position;
	m_visibleRotation = m_rotation;
	m_dofMultiplier = 0.0f;
	m_dofFarOffset = 0.0f;
	m_dofNearOffset = 0.0f;
}

void CGameFreeCamera::CacheFovDistanceMultiplier()
{
	m_fovDistanceMultiplier = MeshUtilities::CalcFovDistanceMultiplier( GetFOV() );
}
