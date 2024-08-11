/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "triggerActivatorComponent.h"
#include "../core/scriptStackFrame.h"
#include "renderFrame.h"
#include "triggerManager.h"
#include "world.h"


IMPLEMENT_ENGINE_CLASS( CTriggerActivatorComponent );

CTriggerActivatorComponent::CTriggerActivatorComponent()
	: m_radius( 0.3f )
	, m_height( 1.8f )
	, m_maxContinousDistance( 10.0f )
{
	m_channels = TC_Default;
}

void CTriggerActivatorComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CTriggerActivatorComponent_OnAttached );

	// Create trigger activator
	if ( NULL == m_activator && m_radius > 0.0f && m_height > 0.0f )
	{
		CTriggerActivatorInfo info;
		info.m_channels = m_channels;
		info.m_component = this;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		info.m_debugName = GetFriendlyName();
#endif
		info.m_extents = Vector( m_radius, m_radius, m_height );
		info.m_localToWorld = GetLocalToWorld();
		info.m_maxContinousDistance = m_maxContinousDistance;
		info.m_enableCCD = m_enableCCD;

		m_activator = world->GetTriggerManager()->CreateActivator( info );
	}

	// register for debug preview rendering but only in the preview world
	if ( world->GetPreviewWorldFlag() ) 
	{
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
	}
}

void CTriggerActivatorComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// fallback case for unregistered trigger
	if ( IsSelected() && m_height > 0.0f && m_radius > 0.0f )
	{
		// AABB bounding box
		Matrix drawMatrix;
		drawMatrix.SetIdentity();
		drawMatrix.SetTranslation( GetWorldPosition() );

		const Box triggerBox( Vector::ZEROS, Vector( m_radius, m_radius, m_height/2.0f ) );
		frame->AddDebugBox( triggerBox, drawMatrix, Color::YELLOW, false );

		// Cylinder
		const Vector top = GetWorldPosition() + Vector( 0, 0, m_height / 2.0f );
		const Vector bottom = GetWorldPosition() - Vector( 0, 0, m_height / 2.0f );
		frame->AddDebugWireframeTube( top, bottom, m_radius, m_radius, Matrix::IDENTITY, Color::YELLOW, Color::YELLOW, false );
	}
}

void CTriggerActivatorComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// remove from trigger scene
	if ( NULL != m_activator )
	{
		m_activator->Remove();
		m_activator->Release();
		m_activator = NULL;
	}

	// unregister from debug drawing categories
	if ( world->GetPreviewWorldFlag() ) 
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
	}
}

void CTriggerActivatorComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CTriggerActivatorComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// move the activator
	if ( NULL != m_activator )
	{
		// new position
		const IntegerVector4 newPos( GetWorldPosition() );

		// assume never teleported
		const Bool isTeleported = false;
		m_activator->Move( newPos, isTeleported );
	}
}

void CTriggerActivatorComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( NULL != m_activator )
	{
		if ( property->GetName() == TXT("channels") )
		{
			m_activator->SetMask( m_channels );
		}

		if ( property->GetName() == TXT("radius") || property->GetName() == TXT("height") )
		{
			m_activator->SetExtents( Vector( m_radius, m_radius, m_height ) );
		}

		if ( property->GetName() == TXT("enableCCD") )
		{
			m_activator->EnableCCD( m_enableCCD );
		}
	}
}

void CTriggerActivatorComponent::SetRadius( const Float radius )
{
	if ( NULL != m_activator )
	{
		if ( radius != m_radius && radius > 0.0f )
		{
			m_radius = radius;

			const Vector extents( m_radius, m_radius, m_height / 2.0f );
			m_activator->SetExtents( extents );
		}
	}
}

void CTriggerActivatorComponent::SetHeight( const Float height )
{
	if ( NULL != m_activator )
	{
		if ( height != m_height && height > 0.0f )
		{
			m_height = height;

			const Vector extents( m_radius, m_radius, m_height / 2.0f );
			m_activator->SetExtents( extents );
		}
	}
}

void CTriggerActivatorComponent::AddTriggerChannel( const ETriggerChannel channel )
{
	if ( 0 == ( m_channels & channel ) )
	{
		m_channels |= (Uint32) channel;
		if ( m_activator != nullptr )
		{
			m_activator->SetMask( m_channels );
		}
	}
}

void CTriggerActivatorComponent::RemoveTriggerChannel( const ETriggerChannel channel )
{
	if ( 0 != ( m_channels & channel ) )
	{
		m_channels &= ~(Uint32) channel;
		if ( m_activator != nullptr )
		{
			m_activator->SetMask( m_channels );
		}
	}
}

Bool CTriggerActivatorComponent::IsInsideTrigger( const class ITriggerObject* trigger ) const
{
	if ( m_activator != nullptr )
	{
		return m_activator->IsInsideTrigger( trigger );
	}
	return false;
}

void CTriggerActivatorComponent::funcSetRadius( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, radius, 0.3f ); 
	FINISH_PARAMETERS;

	SetRadius(radius);
}

void CTriggerActivatorComponent::funcSetHeight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, height, 1.0f );
	FINISH_PARAMETERS;

	SetHeight(height);
}

void CTriggerActivatorComponent::funcGetRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_radius );
}

void CTriggerActivatorComponent::funcGetHeight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_height );
}

void CTriggerActivatorComponent::funcAddTriggerChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, TC_Default );
	FINISH_PARAMETERS;

	AddTriggerChannel(channel);
}

void CTriggerActivatorComponent::funcRemoveTriggerChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, TC_Default );
	FINISH_PARAMETERS;

	RemoveTriggerChannel(channel);
}
