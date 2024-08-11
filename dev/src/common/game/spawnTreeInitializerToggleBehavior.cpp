/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerToggleBehavior.h"

#include "behTreeNodeConditionExternalToggle.h"


IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializerToggleBehavior )

///////////////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializerToggleBehavior
///////////////////////////////////////////////////////////////////////////////
ISpawnTreeInitializerToggleBehavior::EOutput ISpawnTreeInitializerToggleBehavior::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	Int32 val = Int32( m_enableBehavior );
	actor->SignalGameplayEvent( m_behaviorSwitchName, val );
	return OUTPUT_SUCCESS;
}
void ISpawnTreeInitializerToggleBehavior::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	Int32 val = CBehTreeNodeConditionExternalToggleInstance::TOGGLE_RESET_TO_DEFAULTS;
	// nothing for now
	actor->SignalGameplayEvent( m_behaviorSwitchName, val );
}

String ISpawnTreeInitializerToggleBehavior::GetEditorFriendlyName() const
{
	if ( GetClass() == ISpawnTreeInitializerToggleBehavior::GetStaticClass() )
	{
		return TXT("ToggleBehavior");
	}
	{
		IScriptable* context = const_cast< ISpawnTreeInitializerToggleBehavior* >( this );
		String friendlyName;
		CallFunctionRet< String >( context, CNAME( GetEditorFriendlyName ), friendlyName );
		if ( !friendlyName.Empty() )
		{
			return friendlyName;
		}
	}
	return GetClass()->GetName().AsString();
}

Bool ISpawnTreeInitializerToggleBehavior::CallActivateOnRestore() const
{
	return true;
}
Bool ISpawnTreeInitializerToggleBehavior::CallActivateOnSpawn() const
{
	return true;
}
Bool ISpawnTreeInitializerToggleBehavior::CallActivateOnPoolSpawn() const
{
	return true;
}
Bool ISpawnTreeInitializerToggleBehavior::CallActivateWhenStealing() const
{
	return true;
}

Bool ISpawnTreeInitializerToggleBehavior::IsSpawnable() const
{
	return GetClass() != ISpawnTreeInitializerToggleBehavior::GetStaticClass();
}