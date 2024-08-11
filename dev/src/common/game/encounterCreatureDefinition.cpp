#include "build.h"
#include "encounterCreatureDefinition.h"

#include "../core/cooker.h"
#include "../core/depot.h"
#include "../core/gameSave.h"

#include "spawnTreeNode.h"

IMPLEMENT_ENGINE_CLASS( CEncounterCreatureDefinition );

//////////////////////////////////////////////////////////////
/// CEncounterCreatureDefinition
//////////////////////////////////////////////////////////////

#ifndef NO_RESOURCE_COOKING
void CEncounterCreatureDefinition::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// there's no entity template for the creature definition
	if ( nullptr == GDepot->FindFileUseLinks( m_entityTemplate.GetPath(), 0 ) )
	{
		cooker.CookingError( this, TXT("Missing creature entity template '%ls'"), m_entityTemplate.GetPath().AsChar() );
	}
}
#endif

Bool CEncounterCreatureDefinition::CheckEntityTemplate( CEntityTemplate* entityTemplate )
{
	if ( !entityTemplate )
	{
		return false;
	}

	const CObject* templateInstance = entityTemplate->GetTemplateInstance();
	if ( templateInstance && !templateInstance->IsA< CActor >() )
	{
		HALT( "ENCOUNTER ENTRY HAS TEMPLATE '%ls' THAT IS NOT AN 'CActor'!", entityTemplate->GetFile()->GetDepotPath().AsChar() );
		return false;
	}

	return true;
}

Bool CEncounterCreatureDefinition::MarkAppearanceAsUsed( CEntityTemplate* entityTemplate, CName appearance )
{
	const TDynArray< CName >& appearances = entityTemplate->GetEnabledAppearancesNames();
	Int32 apperancesCount = appearances.Size();
	if( apperancesCount > 0 )
	{
		// check if system was not initialized yet
		if ( m_usedAppearancesCount < 0 )
		{
			m_usedAppearances.Resize( apperancesCount );
			m_usedAppearances.SetZero();
			m_usedAppearancesCount = 0;
		}
		// Already used all available options? Reset!
		else if( m_usedAppearancesCount >= apperancesCount )
		{
			m_usedAppearances.SetZero();
			m_usedAppearancesCount = 0;
		}
		if ( Int32( m_usedAppearances.Size() ) == apperancesCount )
		{
			return false;
		}

		for ( Uint32 i = 0; i < appearances.Size(); ++i )
		{
			if ( appearances[i] == appearance )
			{
				if ( m_usedAppearances.Get( i ) )
				{
					return false;
				}

				m_usedAppearances.Set( i, 1 );
				return true;
			}
		}
	}

	return false;
}

CName CEncounterCreatureDefinition::SelectUniqueAppearance( CEntityTemplate* entityTemplate )
{
	if ( m_forcedAppearance )
	{
		return m_forcedAppearance;
	}
	CName appName = CName::NONE;
	const TDynArray< CName >& appearances = entityTemplate->GetEnabledAppearancesNames();
	Int32 apperancesCount = appearances.Size();
	if( apperancesCount > 0 )
	{
		// check if system was not initialized yet
		if ( m_usedAppearancesCount < 0 )
		{
			m_usedAppearances.Resize( apperancesCount );
			m_usedAppearances.SetZero();
			m_usedAppearancesCount = 0;
		}
		// Already used all available options? Reset!
		else if( m_usedAppearancesCount >= apperancesCount )
		{
			m_usedAppearances.SetZero();
			m_usedAppearancesCount = 0;
		}
		ASSERT( Int32( m_usedAppearances.Size() ) == apperancesCount );

		// Random attempt at selection
		Int32 index		= GEngine->GetRandomNumberGenerator().Get< Int32 >( apperancesCount );

		// Selection already used? Get next available
		while( m_usedAppearances.Get( index ) )
		{
			if( ++index >= apperancesCount )
			{
				index = 0;
			}
		}
		++m_usedAppearancesCount;

		appName = appearances[index];
		m_usedAppearances.Set( index, 1 );
	}

	return appName;
}

void CEncounterCreatureDefinition::Save( IGameSaver* saver )
{
	saver->WriteValue< Uint16 >( CNAME( dead ), m_bodyCount );
}

void CEncounterCreatureDefinition::Load( IGameLoader* loader )
{
	loader->ReadValue< Uint16 >( CNAME( dead ), m_bodyCount );
}

#ifndef NO_EDITOR
CEntityTemplate* CEncounterCreatureDefinition::Editor_GetEntityTemplate()
{
	return m_entityTemplate.Get();
}
#endif


//////////////////////////////////////////////////////////////////////////
// ICreatureDefinitionContainer
//////////////////////////////////////////////////////////////////////////
void ICreatureDefinitionContainer::CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& defs )
{
	TDynArray< CEncounterCreatureDefinition* >& definitions = InternalGetCreatureDefinitions();
	for( auto it = definitions.Begin(), end = definitions.End(); it != end; ++it )
	{
		CEncounterCreatureDefinition* d = *it;
		if ( d->Override() )
		{
			if ( defs.Find( d->m_definitionName ) == defs.End() )
			{
				defs.Insert( d );
			}
		}
	}

	ISpawnTreeBaseNode*	rootNode = InternalGetRootTreeNode();
	if ( rootNode )
	{
		rootNode->CompileCreatureDefinitions( defs );
	}

	for( auto it = definitions.Begin(), end = definitions.End(); it != end; ++it )
	{
		CEncounterCreatureDefinition* d = *it;
		if ( !d->Override() )
		{
			if ( defs.Find( d->m_definitionName ) == defs.End() )
			{
				defs.Insert( d );
			}
		}
	}

}
void ICreatureDefinitionContainer::FillUpCreatureDefinition()
{
	TSortedArray< CName > usedDefinitions;
	CObject* me = AsCObject();
	TDynArray< CEncounterCreatureDefinition* >& creatureDefinitions = InternalGetCreatureDefinitions();
	for ( Uint32 childId = 0, childCount = GetNumChildren(); childId < childCount; ++childId )
	{
		IEdSpawnTreeNode* edNode = GetChild( childId );
		if ( !edNode )
		{
			continue;
		}
		CObject* obj = edNode->AsCObject();
		ISpawnTreeBaseNode* treeNode = Cast< ISpawnTreeBaseNode >( obj );
		if ( !treeNode )
		{
			continue;
		}
		treeNode->FillUpDefaultCreatureDefinitions( creatureDefinitions, me );
		treeNode->CollectUsedCreatureDefinitions( usedDefinitions );
	}

	for( Uint32 i = 0, n = usedDefinitions.Size(); i != n; ++i )
	{
		CName definitionName = usedDefinitions[ i ];
		Bool isDefinitionUsed = false;
		for( Uint32 j = 0, m = creatureDefinitions.Size(); j != m; ++j )
		{
			if ( creatureDefinitions[ j ] && creatureDefinitions[ j ]->m_definitionName == definitionName )
			{
				isDefinitionUsed = true;
				break;
			}
		}
		if ( !isDefinitionUsed )
		{
			CEncounterCreatureDefinition* newDefinition = CreateObject< CEncounterCreatureDefinition >( me );
			newDefinition->m_definitionName = definitionName;
			creatureDefinitions.PushBack( newDefinition );
		}
	}
	EDITOR_QUEUE_EVENT( CNAME( SpawnTreeCreatureListChanged ), CreateEventData( me ) );
}

void ICreatureDefinitionContainer::ClearCreatureDefinitions()
{
	TSortedArray< CName > usedDefinitions;
	CObject* me = AsCObject();
	TDynArray< CEncounterCreatureDefinition* >& creatureDefinitions = InternalGetCreatureDefinitions();
	for ( Uint32 childId = 0, childCount = GetNumChildren(); childId < childCount; ++childId )
	{
		IEdSpawnTreeNode* edNode = GetChild( childId );
		if ( !edNode )
		{
			continue;
		}
		CObject* obj = edNode->AsCObject();
		ISpawnTreeBaseNode* treeNode = Cast< ISpawnTreeBaseNode >( obj );
		if ( !treeNode )
		{
			continue;
		}
		treeNode->CollectUsedCreatureDefinitions( usedDefinitions );
	}
	for( Uint32 i = 0; i < creatureDefinitions.Size(); )
	{
		Bool useDefinition = false;
		if ( creatureDefinitions[ i ] )
		{
			auto itFind = usedDefinitions.Find( creatureDefinitions[ i ]->m_definitionName );
			if ( itFind != usedDefinitions.End() )
			{
				useDefinition = true;
			}
		}
		if ( !useDefinition )
		{
			creatureDefinitions.RemoveAt( i );
		}
		else
		{
			++i;
		}
	}
	EDITOR_QUEUE_EVENT( CNAME( SpawnTreeCreatureListChanged ), CreateEventData( me ) );
}
void ICreatureDefinitionContainer::OnEditorOpened()
{
	FillUpCreatureDefinition();

	IEdSpawnTreeNode::OnEditorOpened();
}