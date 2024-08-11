#pragma once

// class that contains properties to customize character
class CCharacter : public CObject
{
	DECLARE_ENGINE_CLASS( CCharacter, CObject, 0 );

private:
	CGUID m_guid;
	CName m_name;

#ifndef FINAL
	// for editor use only - character inherits value properties from parent
	CCharacter* m_parentCharacter;
#endif

	CName m_voiceTag;
	TagList m_tags;
	TSoftHandle< CEntityTemplate >  m_entityTemplate;
	// items
	// ai params

public:
	CCharacter();
	RED_INLINE  CName GetName() const { return m_name; }
	RED_INLINE  CGUID GetGUID() const { return m_guid; }
	RED_INLINE  void SetName( CName name ) { m_name = name; }

#ifndef FINAL
	RED_INLINE const CCharacter* GetParentCharacter() const { return m_parentCharacter; }
	void SetParentCharacter( CCharacter* newParent );

	// checks if value of the property is the same in character and its parent
	Bool IsInherited( const CProperty* prop );

	// sets property value from parent
	void UpdateInheritedProperty( const CProperty* prop );
#endif
};

BEGIN_CLASS_RTTI( CCharacter )
	PARENT_CLASS( CObject )
	PROPERTY_RO( m_name, TXT( "Name" ) );
	PROPERTY( m_guid )

#ifndef FINAL
	PROPERTY_RO_NOT_COOKED( m_parentCharacter, TXT("i_parentCharacter") );
#endif

	PROPERTY_CUSTOM_EDIT_NAME( m_voiceTag, TXT( "i_voiceTag" ), TXT( "Voice Tag" ), TXT("EntityVoiceTagSelect") );
	PROPERTY_EDIT( m_tags, TXT("Character tag") );
	PROPERTY_EDIT( m_entityTemplate, TXT("Entity tamplate") );
END_CLASS_RTTI();

