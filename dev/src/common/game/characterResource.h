#pragma once

class CCharacter;

// Resource which stores list of characters
class CCharacterResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCharacterResource, CResource, "cdb", "Character Database" );

public:
	CCharacterResource();
	~CCharacterResource();

	RED_INLINE TDynArray< CCharacter* >& GetCharactersData() { return m_charactersData; }
	CCharacter* FindCharacterByGUID( CGUID guid );
	CCharacter* FindFirstCharacterWithName( CName name );
	

private:

	TDynArray< CCharacter* > m_charactersData;
};

BEGIN_CLASS_RTTI( CCharacterResource )
	PARENT_CLASS( CResource )
	PROPERTY_RO( m_charactersData, TXT( "Characters" ) )
END_CLASS_RTTI()