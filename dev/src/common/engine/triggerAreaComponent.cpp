/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "triggerActivatorComponent.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "game.h"
#include "triggerAreaComponent.h"
#include "triggerManager.h"
#include "world.h"
#include "layer.h"
#include "layerInfo.h"
#include "entity.h"


IMPLEMENT_ENGINE_CLASS( CTriggerAreaComponent );
IMPLEMENT_RTTI_BITFIELD( ETriggerChannel );

CTriggerAreaComponent::CTriggerAreaComponent()
	: m_isEnabled( true )
	, m_includedChannels( TC_Default | TC_Player )
	, m_excludedChannels( 0 )
	, m_triggerPriority( 0 )
{
	m_color = Color::GREEN;
}

CTriggerAreaComponent::~CTriggerAreaComponent()
{
	m_listeners.ClearFast();
}

Float CTriggerAreaComponent::CalcLineWidth() const
{
	return 1.0f;
}

void CTriggerAreaComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	// Pass to base class
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Shape of the area changed, in editor readd the trigger object itself
	if ( GGame->IsActive() && m_triggerObject )
	{
		m_triggerObject->SetPosition( GetLocalToWorld() );
	}
}

void CTriggerAreaComponent::OnAreaShapeChanged()
{
	// reattach in the world system
	CWorld* attachedWorld = GetWorld();
	if ( attachedWorld )
		ConditionalAttachToWorld( attachedWorld );
}

void CTriggerAreaComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// a little hack for encounters :(
	if ( file.GetVersion() < VER_IMPROVED_BITFIELD_SERIALIZATION )
	{
		if ( m_includedChannels == 0 && m_excludedChannels == 0 )
		{
			m_includedChannels = TC_Default | TC_Player;
		}
	}
}

void CTriggerAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	ConditionalAttachToWorld( world );	
}

void CTriggerAreaComponent::OnDetached( CWorld* world )
{
	m_listeners.ClearFast();
	TBaseClass::OnDetached( world );
	ConditionalAttachToWorld( world );
}

Bool CTriggerAreaComponent::TestPointOverlap( const Vector& worldPoint ) const
{
	if ( NULL == m_triggerObject )
	{
		return TBaseClass::TestPointOverlap( worldPoint );
	}
	else
	{
		return m_triggerObject->TestPoint( worldPoint );
	}
}

Bool CTriggerAreaComponent::TestBoxOverlap( const Box& box ) const
{
	if ( NULL == m_triggerObject )
	{
		return TBaseClass::TestBoxOverlap( box );
	}
	else
	{
		return m_triggerObject->TestBox( box.CalcCenter(), box.CalcExtents() );
	}
}

Bool CTriggerAreaComponent::TestBoxTrace( const Vector& worldStart, const Vector& worldEnd, const Vector& extents, Float& outEntryTime, Float& outExitTime ) const
{
	if ( NULL != m_triggerObject )
	{
		return m_triggerObject->TraceBox( worldStart, worldEnd, extents, outEntryTime, outExitTime );
	}

	return false;
}

void CTriggerAreaComponent::EnteredArea( CComponent* component )
{
	if ( component && GGame->IsActive() )
	{
		GetEntity()->OnAreaEnter( this, component );
		TDynArray< ITriggerAreaListener* >::iterator itEnd = m_listeners.End();
		for ( TDynArray< ITriggerAreaListener* >::iterator it = m_listeners.Begin(); it != itEnd; ++it )
		{
			(*it)->OnAreaEnter( this, component );
		}
	}
}

void CTriggerAreaComponent::ExitedArea( CComponent* component )
{
	if ( component && GGame->IsActive() )
	{
		GetEntity()->OnAreaExit( this, component );
		TDynArray< ITriggerAreaListener* >::iterator itEnd = m_listeners.End();
		for ( TDynArray< ITriggerAreaListener* >::iterator it = m_listeners.Begin(); it != itEnd; ++it )
		{
			(*it)->OnAreaExit( this, component );
		}
	}
}

Bool CTriggerAreaComponent::AddListener( ITriggerAreaListener* listener )
{
	return m_listeners.PushBackUnique( listener );
}

Bool CTriggerAreaComponent::RemoveListener( ITriggerAreaListener* listener )
{
	return m_listeners.Remove( listener );
}

Bool CTriggerAreaComponent::IsEnabled() const
{
	return m_isEnabled;
}

void CTriggerAreaComponent::SetEnabled( Bool enabled )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() && GetLayer()->GetLayerInfo() && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to enable or disable trigger '%ls' in an environment layer"), GetFriendlyName().AsChar() );
	}
#endif

	// Toggle
	if ( m_isEnabled != enabled )
	{
		SetShouldSave( true );

		// Save new flag
		m_isEnabled = enabled;

		// Reattach
		if ( IsAttached() )
		{
			CWorld* attachedWorld = GetWorld();
			ConditionalAttachToWorld( attachedWorld );
		}

		if ( GGame->IsActive() )
		{
			GetEntity()->OnAreaActivated( this, enabled );
		}
	}
}

void CTriggerAreaComponent::SetChannelMask( const Uint32 includedChannels, const Uint32 excludedChannels )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() != nullptr && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to change the channel mask for trigger '%ls' in an environmen"), GetFriendlyName().AsChar() );
	}
#endif

	if ( includedChannels != m_includedChannels || excludedChannels != m_excludedChannels )
	{
		m_includedChannels = includedChannels;
		m_excludedChannels = excludedChannels;

		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}
}

void CTriggerAreaComponent::RemoveIncludedChannel( const ETriggerChannel channel )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() != nullptr && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to remove included channel on trigger '%ls' in an environment"), GetFriendlyName().AsChar() );
	}
#endif

	if ( 0 != ( m_includedChannels & channel ) )
	{
		m_includedChannels &= ~ (Uint32)channel;

		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}
}

void CTriggerAreaComponent::AddIncludedChannel( const ETriggerChannel channel )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() != nullptr && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to add included channel on trigger '%ls' in an environment layer"), GetFriendlyName().AsChar() );
	}
#endif

	if ( 0 == ( m_includedChannels & channel ) )
	{
		m_includedChannels |= (Uint32)channel;

		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}
}

void CTriggerAreaComponent::RemoveExcludedChannel( const ETriggerChannel channel )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() != nullptr && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to remove excluded channel on trigger '%ls' in an environment layer"), GetFriendlyName().AsChar() );
	}
#endif

	if ( 0 != ( m_excludedChannels & channel ) )
	{
		m_excludedChannels &= ~ (Uint32)channel;

		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}
}

void CTriggerAreaComponent::AddExcludedChannel( const ETriggerChannel channel )
{
#ifndef NO_DATA_ASSERTS
	if ( GetLayer() != nullptr && GetLayer()->GetLayerInfo()->IsEnvironment() )
	{
		DATA_HALT( DES_Uber, GetLayer(), TXT("World"), TXT("Tried to add excluded channel on trigger '%ls' in an environment layer"), GetFriendlyName().AsChar() );
	}
#endif

	if ( 0 == ( m_excludedChannels & channel ) )
	{
		m_excludedChannels |= (Uint32)channel;

		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}
}

void CTriggerAreaComponent::ConditionalAttachToWorld( CWorld* world )
{
	if( !world )
		return;

	// Remove previous shit
	if (NULL != m_triggerObject)
	{
		m_triggerObject->Remove();
		m_triggerObject->Release();
		m_triggerObject = NULL;
		m_listeners.ClearFast();
	}

	// Add if valid
	const Bool canAdd = IsAttached() && IsEnabled();
	if ( canAdd )
	{
		CTriggerObjectInfo initInfo;
		initInfo.m_callback = this;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		initInfo.m_debugName = GetFriendlyName();
#endif
		initInfo.m_component = this;
		initInfo.m_excludeChannels = m_excludedChannels;
		initInfo.m_includeChannels = m_includedChannels;
		initInfo.m_localToWorld = GetLocalToWorld();
		initInfo.m_shape = const_cast< CAreaShape* >( &GetCompiledShape() );
		initInfo.m_allowBelowTerrain = (m_terrainSide == ATS_AboveAndBelowTerrain) || (m_terrainSide == ATS_OnlyBelowTerrain);
		initInfo.m_allowAboveTerrain = (m_terrainSide == ATS_AboveAndBelowTerrain) || (m_terrainSide == ATS_OnlyAboveTerrain);
		initInfo.m_useCCD = m_enableCCD;

		// Register the trigger object inside the trigger manager
		m_triggerObject = world->GetTriggerManager()->CreateTrigger(initInfo);
	}
}

#ifndef NO_DATA_VALIDATION
void CTriggerAreaComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );
}
#endif // NO_DATA_VALIDATION

Bool CTriggerAreaComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if( propertyName == TXT( "priority" ) )
	{
		readValue.AsType( m_triggerPriority );
		return true;
	}

	return false;
}

void CTriggerAreaComponent::OnPropertyPostChange( CProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	// trigger channels changes
	if ( prop->GetName() == TXT("excludedChannels") || prop->GetName() == TXT("includedChannels") )
	{
		if ( NULL != m_triggerObject )
		{
			m_triggerObject->SetMask( m_includedChannels, m_excludedChannels );
		}
	}

	// triggers CCD settings changes
	if ( prop->GetName() == TXT("enableCCD") )
	{
		if ( NULL != m_triggerObject )
		{
			m_triggerObject->EnableCCD( m_enableCCD );
		}
	}

	// changed trigger terrain filtering
	if ( prop->GetName() == TXT("terrainSide") )
	{
		if ( NULL != m_triggerObject )
		{
			const Bool allowAbove = (m_terrainSide == ATS_AboveAndBelowTerrain) || (m_terrainSide == ATS_OnlyAboveTerrain);
			const Bool allowBelow = (m_terrainSide == ATS_AboveAndBelowTerrain) || (m_terrainSide == ATS_OnlyBelowTerrain);
			m_triggerObject->SetTerrainMask( allowAbove, allowBelow );
		}
	}
}

void CTriggerAreaComponent::OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	if ( m_triggerObject == object )
	{
		CComponent* component = activator ? activator->GetComponent() : NULL;
		EnteredArea( component );
	}
	else
	{
		WARN_ENGINE( TXT("OnActivatorEntered: invalid trigger object") );
	}

}

void CTriggerAreaComponent::OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	if ( m_triggerObject == object )
	{
		CComponent* component = activator ? activator->GetComponent() : NULL;
		ExitedArea( component );
	}
	else
	{
		WARN_ENGINE( TXT("OnActivatorExited: invalid trigger object") );
	}
}

//---------------------------------------------------------------------------

void CTriggerAreaComponent::funcSetChannelMask( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, includedChannels, m_includedChannels );
	GET_PARAMETER( Uint32, excludedChannels, m_excludedChannels );
	FINISH_PARAMETERS;

	SetChannelMask( includedChannels, excludedChannels );
}

void CTriggerAreaComponent::funcAddIncludedChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, (ETriggerChannel)0 );
	FINISH_PARAMETERS;

	AddIncludedChannel( channel );
}

void CTriggerAreaComponent::funcRemoveIncludedChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, (ETriggerChannel)0 );
	FINISH_PARAMETERS;

	RemoveIncludedChannel( channel );
}

void CTriggerAreaComponent::funcAddExcludedChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, (ETriggerChannel)0 );
	FINISH_PARAMETERS;

	AddExcludedChannel( channel );
}

void CTriggerAreaComponent::funcRemoveExcludedChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, (ETriggerChannel)0 );
	FINISH_PARAMETERS;

	RemoveExcludedChannel( channel );
}
