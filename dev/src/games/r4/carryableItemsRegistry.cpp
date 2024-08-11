#include "build.h"

#include "carryableItemsRegistry.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"

#include "../../common/engine/idTagManager.h"
#include "../../common/engine/dynamicLayer.h"


IMPLEMENT_RTTI_ENUM( EExplorationHeldItems );
IMPLEMENT_ENGINE_CLASS( CCarryableItemsRegistry );

CGatheredResource itemsDefinitionRes( TXT("gameplay\\globals\\carryable_items.csv"), 0 );

void CCarryableItemsRegistry::Initialize()
{
	C2dArray* itemsDefinitionArray = itemsDefinitionRes.LoadAndGet< C2dArray >();

	if( !itemsDefinitionArray ) return;
	m_itemsNames			= itemsDefinitionArray->GetColumn< String >( TXT("name")		);
	m_itemsResourcesPaths	= itemsDefinitionArray->GetColumn< String >( TXT("resource")	);
	m_pickZOffsets			= itemsDefinitionArray->GetColumn< Float >( TXT("pickZOffset")	);
	m_spawnDistances		= itemsDefinitionArray->GetColumn< Float >( TXT("spawnDistance"));	
	m_itemTypes				= itemsDefinitionArray->GetColumn< EExplorationHeldItems >( TXT("itemType") );
}

TDynArray< String >& CCarryableItemsRegistry::GetItemsNames()
{
	return m_itemsNames;
}

Float CCarryableItemsRegistry::GetPickOffset( String& itemName )
{
	for( Int32 i=0; i<m_itemsNames.SizeInt(); ++i )
	{
		if( m_itemsNames[ i ] == itemName )
		{
			return m_pickZOffsets[ i ];
		}
	}
	return 0;
}

Float CCarryableItemsRegistry::GetSpawnDistance( String& itemName )
{
	for( Int32 i=0; i<m_itemsNames.SizeInt(); ++i )
	{
		if( m_itemsNames[ i ] == itemName )
		{
			return m_spawnDistances[ i ];
		}
	}
	return 0;
}

EExplorationHeldItems CCarryableItemsRegistry::GetItemType( String& itemName )
{
	for ( Int32 i = 0; i < m_itemsNames.SizeInt(); ++i )
	{
		if ( m_itemsNames[ i ] == itemName )
		{
			return m_itemTypes[ i ];
		}
	}
	return EEHI_None;
}

THandle< CEntity >	CCarryableItemsRegistry::CreateItem( String itemName, const Vector &spawnPosition, EulerAngles spawnRotation )
{
	for( Int32 i=0; i<m_itemsNames.SizeInt(); ++i )
	{
		if( m_itemsNames[ i ] == itemName )
		{
			return CreateEntity( m_itemsResourcesPaths[ i ], spawnPosition, spawnRotation );
		}
	}

	return nullptr;
}

THandle< CEntity > CCarryableItemsRegistry::CreateEntity( String& resourcePath, const Vector &spawnPosition, EulerAngles spawnRotation )
{
	THandle< CResource > res;
	res = GDepot->LoadResource( resourcePath );
	CEntityTemplate *pEntityTemplate = Cast< CEntityTemplate >( res.Get() );

	if ( pEntityTemplate == NULL || !GGame->GetActiveWorld() )
	{				
		return nullptr;
	}

	EntitySpawnInfo einfo;
	einfo.m_template		= pEntityTemplate;
	einfo.m_spawnPosition	= spawnPosition;
	einfo.m_spawnRotation	= spawnRotation;
	einfo.m_entityFlags		= EF_ManagedEntity;
	einfo.m_idTag			= GGame->GetIdTagManager()->Allocate();
	return GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
}