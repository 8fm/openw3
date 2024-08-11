#include "build.h"
#include "spawnTreeInitializerSetAppearance.h"
#include "actor.h"
#include "../core/dataError.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerSetAppearance );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerDrawWeapon
////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerSetAppearance::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	entityInfo.m_appearances.ResizeFast( 1 );
	entityInfo.m_appearances[ 0 ] = m_appearanceName;

#if !(defined NO_EDITOR) && !(defined NO_DATA_VALIDATION)
	// Check if the requested appearance is still valid
	if ( entityInfo.m_template.Get() && 
		 entityInfo.m_template.Get()->GetAppearance( m_appearanceName, true ) == nullptr )
	{
		// Try to guess the resource from the object hierarchy
		CResource* res = nullptr;
		CObject* parent = GetParent();
		while ( res == nullptr && parent != nullptr && !parent->IsA< CResource >() )
		{
			parent = parent->GetParent();
		}
		res = Cast< CResource >( parent );

		// Fall back to report the template since we cannot figure out where this spawn tree
		// initializer comes from
		if ( res == nullptr )
		{
			res = entityInfo.m_template.Get();
		}
		DATA_HALT( DES_Major, res, TXT("Spawn Tree"), TXT("Unknown appearance '%ls' in CSpawnTreeInitializerSetAppearance"), 
			m_appearanceName.AsChar() );
	}
#endif
}
Bool CSpawnTreeInitializerSetAppearance::Accept( CActor* actor ) const
{
	if ( m_onlySetOnSpawnAppearance && actor->GetAppearance() != m_appearanceName )
	{
		return false;
	}
	return true;
}
ISpawnTreeInitializer::EOutput CSpawnTreeInitializerSetAppearance::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	if ( m_onlySetOnSpawnAppearance  && reason != ISpawnTreeInitializer::EAR_GameIsRestored )
	{
		return actor->GetAppearance() == m_appearanceName ? OUTPUT_SUCCESS : OUTPUT_FAILED;
	}

	if( !m_appearanceName.Empty() )
	{
		actor->ApplyAppearance( m_appearanceName );
		
	}

	return OUTPUT_SUCCESS;
}

void CSpawnTreeInitializerSetAppearance::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
}

String CSpawnTreeInitializerSetAppearance::GetBlockCaption() const
{
	if ( m_onlySetOnSpawnAppearance )
	{
		return String::Printf( TXT("Spawn appearance: '%ls'"), m_appearanceName.AsString().AsChar() );
	}
	else
	{
		return String::Printf( TXT("Set dynamic appearance: '%ls'"), m_appearanceName.AsString().AsChar() );
	}
}	

String CSpawnTreeInitializerSetAppearance::GetEditorFriendlyName() const
{
	static String STR( TXT("SetApperance") );
	return STR;
}

Bool CSpawnTreeInitializerSetAppearance::CallActivateOnRestore() const
{
	return true;
}

Bool CSpawnTreeInitializerSetAppearance::CallActivateOnSpawn() const
{
	return false;
}
Bool CSpawnTreeInitializerSetAppearance::CallActivateOnPoolSpawn() const
{
	return false;
}

Bool CSpawnTreeInitializerSetAppearance::CallActivateWhenStealing() const
{
	return true;
}

#ifndef NO_EDITOR
CEntityTemplate* CSpawnTreeInitializerSetAppearance::Editor_GetEntityTemplate()
{
	return GetCreatureEntityTemplate();
}
#endif