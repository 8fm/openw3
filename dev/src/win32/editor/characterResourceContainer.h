#pragma once


class CCharacter;

// Stores all character resources
class CEdCharacterResourceContainer : public CObject
{
	DECLARE_ENGINE_CLASS( CEdCharacterResourceContainer, CObject, 0 );

	typedef TDynArray< THandle< CCharacterResource > > TResources;
	TResources m_resources;

public:
	CEdCharacterResourceContainer();
	~CEdCharacterResourceContainer();

	CCharacterResource* CreateResouce();
	void RemoveResource( CCharacterResource* characterResource );
	TDynArray< THandle< CCharacterResource > >& GetResources() { return  m_resources; }
	void AddResource( CCharacterResource* characterResource );
	CCharacter* FindCharacterByGUID( CGUID guid );
	CCharacter* FindFirstCharacterWithName( CName name );
};
BEGIN_CLASS_RTTI( CEdCharacterResourceContainer )
	PARENT_CLASS( CObject )
	PROPERTY( m_resources )
END_CLASS_RTTI()



