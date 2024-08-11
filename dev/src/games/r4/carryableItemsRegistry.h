#pragma once

enum EExplorationHeldItems
{
	EEHI_None,
	EEHI_Box,
	EEHI_Barrel
};

BEGIN_ENUM_RTTI( EExplorationHeldItems )
	ENUM_OPTION( EEHI_None );
	ENUM_OPTION( EEHI_Box );
	ENUM_OPTION( EEHI_Barrel );
END_ENUM_RTTI()

template<> RED_INLINE Bool FromString( const String& text, EExplorationHeldItems& value )
{
	String textTrimmed = text.TrimCopy();
	if ( textTrimmed == TXT("EEHI_Box") )
	{
		value = EEHI_Box;
		return true;
	}
	else if ( textTrimmed == TXT("EEHI_Barrel") )
	{
		value = EEHI_Barrel;
		return true;
	}
	else
	{
		ASSERT( !TXT("FromString(): Unknown string EExplorationHeldItems value."));
		value = EEHI_None;
		return false;
	}
}

class CCarryableItemsRegistry : public CObject
{
	DECLARE_ENGINE_CLASS( CCarryableItemsRegistry ,CObject, 0 );

private:
	TDynArray< String >	m_itemsNames;
	TDynArray< String >	m_itemsResourcesPaths;
	TDynArray< Float  > m_pickZOffsets;	
	TDynArray< Float  > m_spawnDistances;
	TDynArray< EExplorationHeldItems > m_itemTypes;

public:
	void Initialize();
	TDynArray< String >& GetItemsNames();
	THandle< CEntity >	CreateItem( String itemName, const Vector &spawnPosition, EulerAngles spawnRotation );
	Float GetPickOffset( String& itemName );
	Float GetSpawnDistance( String& itemName );
	EExplorationHeldItems GetItemType( String& itemName );

private:
	THandle< CEntity > CreateEntity( String& resourcePath, const Vector &spawnPosition, EulerAngles spawnRotation );
};

BEGIN_CLASS_RTTI( CCarryableItemsRegistry );	
	PARENT_CLASS( CObject );
END_CLASS_RTTI();