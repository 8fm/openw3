/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "r4LootDefinitions.h"
#include "r4LootManager.h"
#include "r4Player.h"
#include "../../common/game/definitionsHelpers.h"
#include "../../common/game/lootDefinitions.h"
#include "../../common/core/xmlReader.h"
#include "../../common/core/xmlWriter.h"

RED_DEFINE_STATIC_NAME( minLootParamNumber )
RED_DEFINE_STATIC_NAME( maxLootParamNumber )

namespace
{

const String NODE_LOOT_CONTAINER	= TXT( "loot" );
const String NODE_LOOT_ENTRY		= TXT( "loot_entry" );
const String ATTR_NAME				= TXT( "name" );
const String ATTR_CHANCE			= TXT( "chance" );
const String ATTR_QUANTITY_MIN		= TXT( "quantity_min" );
const String ATTR_QUANTITY_MAX		= TXT( "quantity_max" );
const String ATTR_PLAYER_LEVEL_MIN	= TXT( "player_level_min" );
const String ATTR_PLAYER_LEVEL_MAX	= TXT( "player_level_max" );
const String ATTR_CRAFTER_LEVEL_MIN	= TXT( "crafter_level_min" );
const String ATTR_CRAFTER_LEVEL_MAX	= TXT( "crafter_level_max" );
const String ATTR_RESPAWN_TIME		= TXT( "respawn_time" );

class CR4LootDefinitionValidator
{
	Uint32	m_playerLevel;

public:

	CR4LootDefinitionValidator()
	{
		m_playerLevel = 1; // default player level
		CR4Player* player = Cast< CR4Player >( GCommonGame->GetPlayer() );
		if ( player != nullptr )
		{
			// calling script method  W3PlayerWitcher::GetLevel (W3PayerWitcher extends CR4Player)
			CallFunctionRet< Uint32 >( player, CName( TXT( "GetLevel" ) ), m_playerLevel );
		}
	}

	template < typename T >
	Bool operator()( const T* definition ) const
	{
		if ( definition->GetChance() != -1.0f && definition->GetChance() <= 0.0f )
		{
			return false;
		}
		if ( m_playerLevel != 0 && !definition->CheckPlayerLevel( m_playerLevel ) )
		{
			return false;
		}
		return true;
	}
};

}

IMPLEMENT_ENGINE_CLASS( CR4LootDefinitionBase );
IMPLEMENT_ENGINE_CLASS( CR4LootItemDefinition );
IMPLEMENT_ENGINE_CLASS( CR4LootContainerDefinition );
IMPLEMENT_ENGINE_CLASS( CR4LootContainerParam );
IMPLEMENT_ENGINE_CLASS( CR4LootParam );
IMPLEMENT_ENGINE_CLASS( SR4LootNameProperty );

//////////////////////////////////////////////////////////////////////////

CR4LootDefinitionBase::CR4LootDefinitionBase()
	: m_quantityMin( 1 )
	, m_quantityMax( 1 )
	, m_playerLevelMin( 0 )
	, m_playerLevelMax( 0 )
	, m_crafterLevelMin( 0 )
	, m_crafterLevelMax( 0 )
{
}

CR4LootDefinitionBase::CR4LootDefinitionBase( const CName& name )
	: CLootDefinitionBase( name )
	, m_quantityMin( 1 )
	, m_quantityMax( 1 )
	, m_playerLevelMin( 0 )
	, m_playerLevelMax( 0 )
	, m_crafterLevelMin( 0 )
	, m_crafterLevelMax( 0 )
{
}

Uint32 CR4LootDefinitionBase::GetRandomQuantity( Uint32 seed ) const
{
	if ( m_quantityMax > m_quantityMin )
	{
		if( seed != 0 )
		{
			Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( seed );
			return noiseMaker.Get< Uint32 >( m_quantityMin , m_quantityMax + 1 );
		}

		return GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_quantityMin , m_quantityMax + 1 );
	}
	return m_quantityMin;
}

Bool CR4LootDefinitionBase::CheckPlayerLevel( Uint32 playerLevel ) const
{
	if ( playerLevel < m_playerLevelMin )
	{
		return false;
	}
	if ( m_playerLevelMax != 0 && playerLevel > m_playerLevelMax )
	{
		return false;
	}
	return true;
}

Bool CR4LootDefinitionBase::CheckCrafterLevel( Uint32 crafterLevel ) const
{
	if ( crafterLevel < m_crafterLevelMin )
	{
		return false;
	}
	if ( m_crafterLevelMax != 0 && crafterLevel > m_crafterLevelMax )
	{
		return false;
	}
	return true;
}

Bool CR4LootDefinitionBase::ReadBaseAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag )
{
	String val;

	if ( !reader->Attribute( ATTR_NAME, val ) )
	{
		WARN_R4( TXT("Cannot parse loot definition entry name in %s"), reader->GetFileNameForDebug() );
		return false;
	}

	m_name = CName( val );
	m_creatorTag = creatorTag;

	if ( reader->Attribute( ATTR_QUANTITY_MIN, val ) )
	{
		FromString< Uint32 >( val, m_quantityMin );
	}
	if ( reader->Attribute( ATTR_QUANTITY_MAX, val ) )
	{
		FromString< Uint32 >( val, m_quantityMax );
	}
	if ( reader->Attribute( ATTR_PLAYER_LEVEL_MIN, val ) )
	{
		FromString< Uint32 >( val, m_playerLevelMin );
	}
	if ( reader->Attribute( ATTR_PLAYER_LEVEL_MAX, val ) )
	{
		FromString< Uint32 >( val, m_playerLevelMax );
	}
	if ( reader->Attribute( ATTR_CRAFTER_LEVEL_MIN, val ) )
	{
		FromString< Uint32 >( val, m_crafterLevelMin );
	}
	if ( reader->Attribute( ATTR_CRAFTER_LEVEL_MAX, val ) )
	{
		FromString< Uint32 >( val, m_crafterLevelMax );
	}

	if ( m_quantityMin > m_quantityMax )
	{
		WARN_R4( TXT("Loot definition entry %s has minimum quantity higher that maximum, ENTRY ignored. Fix this!"), m_name.AsString().AsChar() );
		return false;
	}
	if ( m_playerLevelMax != 0 && m_playerLevelMin > m_playerLevelMax )
	{
		WARN_R4( TXT("Loot definition entry %s has minimum player level higher that maximum, ENTRY ignored. Fix this!"), m_name.AsString().AsChar() );
		return false;
	}
	if ( m_crafterLevelMax != 0 && m_crafterLevelMin > m_crafterLevelMax )
	{
		WARN_R4( TXT("Loot definition entry %s has minimum crafter level higher that maximum, ENTRY ignored. Fix this!"), m_name.AsString().AsChar() );
		return false;
	}
	return true;
}

Bool CR4LootDefinitionBase::WriteBaseAttributes( CXMLWriter* writer )
{
	Validate();

	String name = m_name.AsString();
	String quantityMin = ToString( m_quantityMin );
	String quantityMax = ToString( m_quantityMax );
	String playerLevelMin = ToString( m_playerLevelMin );
	String playerLevelMax = ToString( m_playerLevelMax );
	String crafterLevelMin = ToString( m_crafterLevelMin );
	String crafterLevelMax = ToString( m_crafterLevelMax );
	writer->Attribute( ATTR_NAME, name );
	writer->Attribute( ATTR_QUANTITY_MIN, quantityMin );
	writer->Attribute( ATTR_QUANTITY_MAX, quantityMax );
	writer->Attribute( ATTR_PLAYER_LEVEL_MIN, playerLevelMin );
	writer->Attribute( ATTR_PLAYER_LEVEL_MAX, playerLevelMax );
	writer->Attribute( ATTR_PLAYER_LEVEL_MIN, crafterLevelMin );
	writer->Attribute( ATTR_PLAYER_LEVEL_MAX, crafterLevelMax );

	return true;
}

void CR4LootDefinitionBase::Validate()
{
	if ( m_quantityMin > m_quantityMax )
	{
		m_quantityMax = m_quantityMin;
	}
	if ( m_playerLevelMax != 0 && m_playerLevelMin > m_playerLevelMax )
	{
		m_playerLevelMax = m_playerLevelMin;
	}
	if ( m_crafterLevelMax != 0 && m_crafterLevelMin > m_crafterLevelMax )
	{
		m_crafterLevelMax = m_crafterLevelMin;
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CR4LootItemDefinition::ReadAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag )
{
	if ( !ReadBaseAttributes( reader, creatorTag ) )
	{
		return false;
	}
	String val;
	if ( reader->Attribute( ATTR_CHANCE, val ) )
	{
		FromString< Float >( val, m_chance );
	}
	return true;
}

Bool CR4LootItemDefinition::WriteAttributes( CXMLWriter* writer )
{
	WriteBaseAttributes( writer );
	Clamp( m_chance, 0.0f, 100.0f );
	String chance = ToString( m_chance );
	writer->Attribute( ATTR_CHANCE, chance );
	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool CR4LootContainerDefinition::GetItems( TDynArray< const CR4LootItemDefinition* > & items ) const
{
	items.Clear();
	for ( TDynArray< CR4LootItemDefinition >::const_iterator it = m_items.Begin(); it != m_items.End(); ++it )
	{
		items.PushBack( &(*it) );
	}
	return items.Size() > 0;
}

const TDynArray< CR4LootItemDefinition > & CR4LootContainerDefinition::GetItems() const
{
	return m_items;
}

Bool CR4LootContainerDefinition::ReadAttributes( CXMLReader* reader, const Red::System::GUID& creatorTag )
{
	if ( !ReadBaseAttributes( reader, creatorTag ) ) 
	{
		return false;
	}
	String val;
	if ( reader->Attribute( ATTR_RESPAWN_TIME, val ) )
	{
		FromString< Uint32 >( val, m_respawnTime );
	}
	while ( reader->BeginNode( NODE_LOOT_ENTRY ) )
	{
		CR4LootItemDefinition def;
		if ( def.ReadAttributes( reader, creatorTag ) )
		{
			m_items.PushBack( def );
		}
		reader->EndNode();
	}
	m_items.Shrink();
	return true;
}

Bool CR4LootContainerDefinition::WriteAttributes( CXMLWriter* writer )
{
	struct CItemDefsByNameSorter
	{
		Bool operator()( const CR4LootItemDefinition* p1, const CR4LootItemDefinition* p2 ) const
		{
			const Bool p1IsNull = ( p1 == nullptr );
			const Bool p2IsNull = ( p2 == nullptr );			
			if ( p1IsNull && p2IsNull )
			{
				return false;
			}
			if ( p1IsNull != p2IsNull )
			{
				// lets put all nulls at the end
				return p2IsNull;
			}

			const Bool p1IsEmpty = p1->GetName().Empty();
			const Bool p2IsEmpty = p2->GetName().Empty();
			if ( p1IsEmpty && p2IsEmpty )
			{
				return false;
			}
			if ( p1IsEmpty != p2IsEmpty )
			{
				// lets put all "None" at the end
				return p2IsEmpty;
			}

			return Red::System::StringCompareNoCase( p1->GetName().AsChar(), p2->GetName().AsChar() ) < 0;
		}
	};

	// sort items definitions by name
	const Uint32 size = m_items.Size();
	TDynArray< CR4LootItemDefinition* > sortedItems( size );
	for ( Uint32 i = 0; i < size; ++i )
	{
		sortedItems[ i ] = &m_items[ i ];
	}
	Sort( sortedItems.Begin(), sortedItems.End(), CItemDefsByNameSorter() );


	WriteBaseAttributes( writer );
	String respawnTime = ToString( m_respawnTime );
	writer->Attribute( ATTR_RESPAWN_TIME, respawnTime );
	TDynArray< CR4LootItemDefinition* >::iterator itEnd = sortedItems.End();
	for ( TDynArray< CR4LootItemDefinition* >::iterator it = sortedItems.Begin(); it != itEnd; ++it )
	{
		if ( *it != nullptr )
		{
			writer->BeginNode( NODE_LOOT_ENTRY );
			( *it )->WriteAttributes( writer );
			writer->EndNode();
		}
	}
	return true;
}

void CR4LootContainerDefinition::CopyItems( const CR4LootContainerDefinition& lootContainerDefinition )
{
	m_items.PushBack( lootContainerDefinition.m_items );
}

Bool CR4LootContainerDefinition::RemoveDefinitions( const Red::System::GUID& creatorTag )
{
	for ( Int32 i = m_items.SizeInt() - 1; i >= 0; --i )
	{
		if( m_items[i].GetCreatorTag() == creatorTag )
		{
			m_items.Erase( m_items.Begin() + i );
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

CLootDefinitionBase* CR4LootDefinitions::GetDefinition( const CName& name )
{
	return m_containers.FindPtr( name );
}

const CLootDefinitionBase* CR4LootDefinitions::GetDefinition( const CName& name ) const
{
	return m_containers.FindPtr( name );
}

Bool CR4LootDefinitions::GetDefinitions( TDynArray< CLootDefinitionBase* > & definitions )
{
	definitions.Clear();
	for ( TContainersMap::iterator it = m_containers.Begin(); it != m_containers.End(); ++it )
	{
		definitions.PushBack( &it->m_second );
	}
	return definitions.Size() > 0;		
}

Bool CR4LootDefinitions::GetDefinitions( TDynArray< const CLootDefinitionBase* > & definitions ) const
{
	definitions.Clear();
	for ( TContainersMap::const_iterator it = m_containers.Begin(); it != m_containers.End(); ++it )
	{
		definitions.PushBack( &it->m_second );
	}
	return definitions.Size() > 0;		
}

Bool CR4LootDefinitions::GetDefinitionsNames( TDynArray< CName > & names ) const
{
	names.Clear();
	for ( TContainersMap::const_iterator it = m_containers.Begin(); it != m_containers.End(); ++it )
	{
		names.PushBack( it->m_first );
	}
	return names.Size() > 0;
}

Bool CR4LootDefinitions::ChooseLootEntries( const CEntity* entity, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const
{
	Int32 minLootParamNumber = -1, maxLootParamNumber = -1;
	CProperty* minLootParamNumberProperty = entity->GetClass()->FindProperty( CNAME( minLootParamNumber ) );
	CProperty* maxLootParamNumberProperty = entity->GetClass()->FindProperty( CNAME( maxLootParamNumber ) );

	if ( minLootParamNumberProperty )
	{
		minLootParamNumberProperty->Get( entity, &minLootParamNumber );
	}
	if ( maxLootParamNumberProperty )
	{
		maxLootParamNumberProperty->Get( entity, &maxLootParamNumber );
	}

	Uint32 seed = inventory ? inventory->GenerateSeed() : 0;

	Int32 lootParamNumber = -1;
	if ( minLootParamNumber > -1 && maxLootParamNumber > -1 && minLootParamNumber <= maxLootParamNumber )
	{
		if( inventory )
		{
			Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( seed );
			lootParamNumber = noiseMaker.Get< Uint32 >( minLootParamNumber, maxLootParamNumber );
		}
		else
		{
			lootParamNumber = GEngine->GetRandomNumberGenerator().Get< Int32 >( minLootParamNumber, maxLootParamNumber );
		}
	}

	const CEntityTemplate* templ = Cast< CEntityTemplate >( entity->GetTemplate() );
	if ( templ != nullptr )
	{
		TDynArray< CGameplayEntityParam*> params;
		params.Reserve( 10 ); // presumably there shouldn't be more of them
		templ->CollectGameplayParams( params, CR4LootParam::GetStaticClass() );

		if ( lootParamNumber > -1 )
		{
			// get only number equal to lootParamNumber
			// but include all params that have m_alwaysPresent flag set
			TDynArray< CGameplayEntityParam*> alwaysPresentParams;
			alwaysPresentParams.Reserve( 10 ); // presumably there shouldn't be more of them

			// move all 'always present' params to different array
			for ( Uint32 i = 0; i < params.Size(); )
			{
				CR4LootParam* param = static_cast< CR4LootParam* >( params[ i ] );
				if ( param && param->IsAlwaysPresent() )
				{
					alwaysPresentParams.PushBack( param );
					params.RemoveAtFast( i );
				}
				else
				{
					i++;
				}
			}
			// move required number of random params from remaining ones
			Int32 remainingParamsNumber = lootParamNumber - alwaysPresentParams.Size();
			for ( Int32 i = 0; i < remainingParamsNumber; i++ )
			{
				if ( params.Empty() )
				{
					break;
				}
				Uint32 paramIndex = 0;
				if( inventory )
				{
					Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( seed + i );
					paramIndex =  noiseMaker.Get< Uint32 >( params.SizeInt() );
				}
				else
				{
					paramIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( params.SizeInt() );
				}

				alwaysPresentParams.PushBack( params[ paramIndex ] );
				params.RemoveAtFast( paramIndex );
			}
			// copy to previous array
			params.SwapWith( alwaysPresentParams );
		}
		for ( TDynArray< CGameplayEntityParam* >::iterator it = params.Begin(); it != params.End(); ++it )
		{
			// we're sure that pointers are of type CR4LootParam so we safely perform static_cast
			ChooseLootEntries( static_cast< CR4LootParam* >( *it ), entries, inventory );
		}
	}

	// merge duplicated entries
	if ( entries.Size() > 0 )
	{
		Uint32 mergeFrom = entries.Size() - 1;
		while ( mergeFrom > 0 )
		{
			for ( Uint32 mergeTo = 0; mergeTo < mergeFrom; mergeTo++ )
			{
				if ( entries[ mergeFrom ].m_itemName == entries[ mergeTo ].m_itemName )
				{
					entries[ mergeTo ].m_quantityMin += entries[ mergeFrom ].m_quantityMin;
					entries[ mergeTo ].m_quantityMax += entries[ mergeFrom ].m_quantityMax;
					// if both have non-zero respawn time (both should be respawned) we take shorter respawn time
					if ( entries[ mergeFrom ].m_respawnTime > 0 && entries[ mergeTo ].m_respawnTime > 0 )
					{
						entries[ mergeTo ].m_respawnTime = Min( entries[ mergeFrom ].m_respawnTime, entries[ mergeTo ].m_respawnTime );
					}
					// otherwise let's take max from both, either:
					// - one will be positive - and will determine respawn time
					// - there will be two zeros - no respawn
					else
					{
						entries[ mergeTo ].m_respawnTime = Max( entries[ mergeFrom ].m_respawnTime, entries[ mergeTo ].m_respawnTime );
					}
					entries.RemoveAtFast( mergeFrom );
					break;
				}
			}
			mergeFrom--;
		}
	}

	return ( entries.Size() > 0 );
}

Bool CR4LootDefinitions::ChooseLootEntries( const CLootDefinitionBase* definition, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const
{
	const CR4LootContainerDefinition* containerDef = Cast< CR4LootContainerDefinition >( definition );
	if ( containerDef == nullptr )
	{
		return false;
	}

	TDynArray< SR4LootContainerDescriptor* > chosenContainers;
	chosenContainers.PushBack( new SR4LootContainerDescriptor( containerDef ) );
	ChooseLootEntries( chosenContainers, NumericLimits< Uint32 >::Max(), entries, inventory );
	chosenContainers.ClearPtr();

	return entries.Size() > 0;
	return false;
}

Bool CR4LootDefinitions::ChooseLootEntries( TDynArray< SR4LootContainerDescriptor* > & containers, Uint32 maxContainersCount, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory ) const
{
	CR4LootDefinitionValidator validator;
	TDynArray< SR4LootContainerDescriptor* > chosenContainers;

	Uint32 seed = inventory ? inventory->GenerateSeed() : 0;

	CDefinitionsHelpers::ChooseDefinitions( containers, chosenContainers, maxContainersCount, validator, seed );

	TDynArray< SR4LootItemDescriptor > items;
	TDynArray< SR4LootItemDescriptor > validItems;
	TDynArray< SR4LootItemDescriptor > chosenItems;

	for ( Uint32 i = 0; i < chosenContainers.Size(); i++ )
	{
		const CR4LootContainerDefinition* container = chosenContainers[i]->m_definition;
		ASSERT( container != nullptr );

		CreateItemsDescriptors( container->GetItems(), items );
		
		CDefinitionsHelpers::ValidateDefinitions( items, validItems );

		Uint32 newSeed = seed == 0 ? 0 : seed + i;
		CDefinitionsHelpers::ChooseDefinitions( validItems, chosenItems, container->GetRandomQuantity( newSeed ), validator, newSeed );

		Uint32 respawnTime = containers[i]->GetRespawnTime();
		entries.Reserve( chosenItems.Size() );
		for ( Uint32 j = 0; j < chosenItems.Size(); j++ )
		{
			const SR4LootItemDescriptor& item = chosenItems[j];
			SLootEntry entry;
			entry.m_itemName = item.GetName();
			entry.m_quantityMin = item.GetQuantityMin();
			entry.m_quantityMax = item.GetQuantityMax();
			entry.m_respawnTime = respawnTime;
			entry.m_nextRespawnTime = EngineTime::ZERO;
			entry.m_hasAreaLimit = item.HasAreaLimit();
			entries.PushBack( entry );
		}
		items.ClearFast();
		validItems.ClearFast();
		chosenItems.ClearFast();
	}

	return entries.Size() > 0;
}

Bool CR4LootDefinitions::ChooseLootEntries( const CR4LootParam* lootParam, TDynArray< SLootEntry > & entries, const CInventoryComponent* inventory  ) const
{
	Uint32 usedContainersCount = lootParam->GetUsedContainersCount( inventory ? inventory->GenerateSeed() : 0 );

	TDynArray< const CR4LootContainerParam* > containersParams;
	lootParam->GetContainersParams( containersParams );
	if ( usedContainersCount == 0 || containersParams.Size() == 0 )
	{
		return false;
	}

	TDynArray< SR4LootContainerDescriptor* > containers;
	CreateContainersDescriptors( containersParams, containers );
	ChooseLootEntries( containers, usedContainersCount, entries, inventory );
	containers.ClearPtr();

	return entries.Size() > 0;
}

Bool CR4LootDefinitions::CreateItemsDescriptors( const TDynArray< CR4LootItemDefinition > & itemsDefs, TDynArray< SR4LootItemDescriptor > & items ) const
{
	CR4LootManager* lootManager = GR4Game->GetSystem< CR4LootManager >();
	RED_ASSERT( lootManager != nullptr );
	items.ClearFast();
	for ( TDynArray< CR4LootItemDefinition >::const_iterator it = itemsDefs.Begin(); it != itemsDefs.End(); ++it )
	{
		Int32 maxCount = lootManager->GetItemMaxCount( it->GetName() );
		// if max count for this item is less than item min quantity, we skip item definition
		if ( maxCount >= 0 && static_cast< Uint32 >( maxCount ) < it->GetQuantityMin() )
		{
			continue;
		}
		items.PushBack( SR4LootItemDescriptor( &(*it), maxCount ) );
	}
	return items.Size() > 0;
}

Bool CR4LootDefinitions::CreateContainersDescriptors( const TDynArray< const CR4LootContainerParam* > & params, TDynArray< SR4LootContainerDescriptor* > & containers ) const
{
	containers.ClearPtr();
	for ( TDynArray< const CR4LootContainerParam* >::const_iterator it = params.Begin(); it != params.End(); ++it )
	{
		const CR4LootContainerDefinition* definition = m_containers.FindPtr( (*it)->GetName() );
		if ( definition != nullptr )
		{
			containers.PushBack( new SR4LootContainerDescriptor( *it, definition ) );
		}
	}
	return containers.Size() > 0;
}

Bool CR4LootDefinitions::AddNewDefinition( const CName& name, CLootDefinitionBase** newDefinition )
{
	if ( !IsDefinitionNameUnique( name ) )
	{
		*newDefinition = nullptr;
		return false;
	}
	CR4LootContainerDefinition container( name );
	m_containers.Insert( name, container );
	*newDefinition = m_containers.FindPtr( name );
	return true;
}

Bool CR4LootDefinitions::RemoveDefinition( const CName& name )
{
	return m_containers.Erase( name );
}

Bool CR4LootDefinitions::RemoveDefinitions( const Red::System::GUID& creatorTag )
{
	TDynArray<CName> m_definitionsToRemove;
	 for( auto& container : m_containers )
	 {
		 if( container.m_second.GetCreatorTag() == creatorTag )
		 {
			 m_definitionsToRemove.PushBack( container.m_first );
		 }
		 else
		 {
			 container.m_second.RemoveDefinitions( creatorTag );
		 }
	 }


	for( auto& removeDefinitionName : m_definitionsToRemove )
	{
		m_containers.Erase( removeDefinitionName );
	}

	return true;
}

Bool CR4LootDefinitions::Clear()
{
	m_containers.Clear();
	return true;
}

Bool CR4LootDefinitions::Load( CXMLReader* reader, const Red::System::GUID& creatorTag )
{
	if ( reader->BeginNode( NODE_ROOT ) )
	{
		if ( reader->BeginNode( NODE_DEFINITIONS, true ) )
		{
			ReadNode( reader, creatorTag );
			reader->EndNode( false );
			return true;
		}
		reader->EndNode( false );
	}
	return false;
}

Bool CR4LootDefinitions::ReadNode( CXMLReader* reader, const Red::System::GUID& creatorTag )
{
	if ( reader->BeginNode( NODE_LOOT_DEFINITIONS, true ) )
	{
		while ( reader->BeginNode( NODE_LOOT_CONTAINER ) )
		{
			CR4LootContainerDefinition def;
			if ( def.ReadAttributes( reader, creatorTag ) )
			{
				const CName& defName = def.GetName();
				auto foundDefContainer = m_containers.Find( defName );
				if( foundDefContainer != m_containers.End() )
				{
					foundDefContainer->m_second.CopyItems( def );
				}
				else
				{
					m_containers.Insert( def.GetName(), def );
				}				
			}
			reader->EndNode();
		}
		reader->EndNode( false );
		m_containers.Shrink();
		return true;
	}
	return false;
}

Bool CR4LootDefinitions::Save( CXMLWriter* writer )
{
	struct CNamesSorter
	{
		Bool operator()( const CName& p1, const CName& p2 ) const
		{
			const Bool p1IsEmpty = p1.Empty();
			const Bool p2IsEmpty = p2.Empty();
			if ( p1IsEmpty && p2IsEmpty )
			{
				return false;
			}
			if ( p1IsEmpty != p2IsEmpty )
			{
				// lets put all "None" at the end
				return p2IsEmpty;
			}
			return Red::System::StringCompareNoCase( p1.AsChar(), p2.AsChar() ) < 0;
		}
	};

	// sorting containers definitions by name
	TDynArray< CName > sortedKeys;
	m_containers.GetKeys( sortedKeys );
	Sort( sortedKeys.Begin(),sortedKeys.End(), CNamesSorter() );

	writer->BeginNode( NODE_ROOT );
	writer->BeginNode( NODE_DEFINITIONS );
	writer->BeginNode( NODE_LOOT_DEFINITIONS );
	TDynArray< CName >::iterator itEnd = sortedKeys.End();
	for ( TDynArray< CName >::iterator it = sortedKeys.Begin(); it != itEnd; ++it )
	{
		CR4LootContainerDefinition* container = m_containers.FindPtr( *it );
		if ( container != nullptr )
		{
			writer->BeginNode( NODE_LOOT_CONTAINER );
			container->WriteAttributes( writer );
			writer->EndNode();
		}
		else
		{
			RED_ASSERT( container != nullptr, TXT( "Cannot find a value in THashMap for key obtained from THashMap.GetKeys" ) );
		}
	}
	writer->EndNode();
	writer->EndNode();
	writer->EndNode();
	writer->Flush();
	return true;
}

Bool CR4LootDefinitions::UpdateDefinitionName( const CName& oldName, const CName& newName )
{
	ASSERT( IsDefinitionNameUnique( newName ) );
	CR4LootContainerDefinition* container = m_containers.FindPtr( oldName );
	if ( container == nullptr )
	{
		return false;
	}
	container->SetName( newName );
	m_containers.Insert( newName, *container );
	m_containers.Erase( oldName );
	return true;
}

//////////////////////////////////////////////////////////////////////////

CR4LootDefinitions::SR4LootItemDescriptor::SR4LootItemDescriptor()
	: m_definition( nullptr )
	, m_areaMaxCount( -1 )
{}

CR4LootDefinitions::SR4LootItemDescriptor::SR4LootItemDescriptor( const CR4LootItemDefinition* definition, Int32 areaMaxCount )
	: m_definition( definition )
	, m_areaMaxCount( areaMaxCount )
{}

CName CR4LootDefinitions::SR4LootItemDescriptor::GetName() const
{
	return m_definition->GetName();
}

Float CR4LootDefinitions::SR4LootItemDescriptor::GetChance() const
{
	return m_definition->GetChance();
}

Bool CR4LootDefinitions::SR4LootItemDescriptor::CheckPlayerLevel( Uint32 playerLevel ) const
{
	return m_definition->CheckPlayerLevel( playerLevel );
}

Bool CR4LootDefinitions::SR4LootItemDescriptor::CheckCrafterLevel( Uint32 crafterLevel ) const
{
	return m_definition->CheckCrafterLevel( crafterLevel );
}

Uint32 CR4LootDefinitions::SR4LootItemDescriptor::GetQuantityMin() const
{
	if ( m_areaMaxCount < 0 )
	{
		return m_definition->GetQuantityMin();
	}
	return Min( m_definition->GetQuantityMin(), static_cast< Uint32 >( m_areaMaxCount ) );
}

Uint32 CR4LootDefinitions::SR4LootItemDescriptor::GetQuantityMax() const
{
	if ( m_areaMaxCount < 0 )
	{
		return m_definition->GetQuantityMax();
	}
	return Min( m_definition->GetQuantityMax(), static_cast< Uint32 >( m_areaMaxCount ) );
}

Bool CR4LootDefinitions::SR4LootItemDescriptor::HasAreaLimit() const
{
	return ( m_areaMaxCount >= 0 );
}

//////////////////////////////////////////////////////////////////////////

CR4LootDefinitions::SR4LootContainerDescriptor::SR4LootContainerDescriptor() 
	: m_param( nullptr )
	, m_definition( nullptr )
{}

CR4LootDefinitions::SR4LootContainerDescriptor::SR4LootContainerDescriptor( const CR4LootContainerDefinition* definition ) 
	: m_param( nullptr )
	, m_definition( definition )
{}

CR4LootDefinitions::SR4LootContainerDescriptor::SR4LootContainerDescriptor( const CR4LootContainerParam* param, const CR4LootContainerDefinition* definition ) 
	: m_param( param )
	, m_definition( definition )
{}

CName CR4LootDefinitions::SR4LootContainerDescriptor::GetName() const
{
	return m_definition->GetName();
}

Float CR4LootDefinitions::SR4LootContainerDescriptor::GetChance() const
{
	return ( m_param != nullptr ) ? m_param->GetChance() : -1.0f;
}

Bool CR4LootDefinitions::SR4LootContainerDescriptor::CheckPlayerLevel( Uint32 playerLevel ) const
{
	return m_definition->CheckPlayerLevel( playerLevel );
}

Bool CR4LootDefinitions::SR4LootContainerDescriptor::CheckCrafterLevel( Uint32 playerLevel ) const
{
	return m_definition->CheckCrafterLevel( playerLevel );
}

Uint32 CR4LootDefinitions::SR4LootContainerDescriptor::GetRespawnTime()
{
	return ( m_param != nullptr && m_param->GetRespawnTime() != 0 ) ? m_param->GetRespawnTime() : m_definition->GetRespawnTime();
}

void CR4LootDefinitions::ValidateLootDefinitions( bool listAllItemDefs )
{
	TDynArray< CName > definitionsNames;

	CLootDefinitions* lootDefs = GCommonGame->GetDefinitionsManager()->GetLootDefinitions();

	if ( lootDefs )
	{
		lootDefs->GetDefinitionsNames( definitionsNames );

		for ( TDynArray< CName >::iterator it = definitionsNames.Begin(); it != definitionsNames.End(); ++it )
		{
			CR4LootContainerDefinition* lootContainerDef = Cast< CR4LootContainerDefinition >( lootDefs->GetDefinition( *it ) );
			if ( lootContainerDef )
			{
				const TDynArray< CR4LootItemDefinition >& lootItemDefs = lootContainerDef->GetItems();
				for ( Uint32 j = 0; j < lootItemDefs.Size(); j++ )
				{
					const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( lootItemDefs[ j ].GetName() );

					if ( nullptr == itemDef )
					{
						ERR_GAME( TXT( ">>>>> ERROR >>>>> Loot Def: %s cannot find Item Definition for %s" ), lootContainerDef->GetName().AsChar(), lootItemDefs[ j ].GetName().AsChar() );
					}
					else if ( listAllItemDefs )
					{
						ERR_GAME( TXT( "Loot Def: %s found %s Item Definition" ), lootContainerDef->GetName().AsChar(), lootItemDefs[ j ].GetName().AsChar() );
					}
				}
			}
		}
	}

	ERR_GAME( TXT( "----------------> LOOT DEFINITIONS REPORT COMPLETE" ) );
}

//////////////////////////////////////////////////////////////////////////

CR4LootParam::CR4LootParam()
	: m_usedContainersMin( 1 )
	, m_usedContainersMax( 1 )
	, m_alwaysPresent( false )
{}

Uint32 CR4LootParam::GetUsedContainersCount( Uint32 seed ) const
{
	if ( m_usedContainersMax > m_usedContainersMin )
	{
		if( seed != 0 )
		{
			Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( seed );
			return noiseMaker.Get< Uint32 >( m_usedContainersMin , m_usedContainersMax + 1 );
		}

		return GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_usedContainersMin , m_usedContainersMax + 1 );
	}
	return m_usedContainersMin;
}

Bool CR4LootParam::GetContainersParams( TDynArray< const CR4LootContainerParam* > & containers ) const
{
	containers.Clear();
	for ( TDynArray< CR4LootContainerParam >::const_iterator it = m_containers.Begin(); it != m_containers.End(); ++it )
	{
		containers.PushBack( &(*it) );
	}
	return containers.Size() > 0;
}

