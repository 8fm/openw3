/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEntityTemplate;

/// Entity template parameter
class CEntityTemplateParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CEntityTemplateParam, CObject );

protected:
	Bool		m_wasIncluded;

public:
	//! Was this entity paramter included ?
	RED_INLINE Bool WasIncluded() const { return m_wasIncluded; }
	 
public:
	CEntityTemplateParam();

	//! Mark as included
	void SetWasIncluded();

#ifndef NO_RESOURCE_COOKING
	virtual Bool IsCooked();
#endif
};

BEGIN_CLASS_RTTI( CEntityTemplateParam )
	PARENT_CLASS( CObject )
	PROPERTY( m_wasIncluded );
END_CLASS_RTTI()

class CGameplayEntityParam  : public CEntityTemplateParam
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CGameplayEntityParam, CEntityTemplateParam );

	Bool								m_overrideInherited;
	String								m_name;

public:
	RED_INLINE void				SetName( const String& name )	{ m_name = name; }
	RED_INLINE const String&	GetName() const		{ return m_name; }
	RED_INLINE Bool				OverrideInherited(){ return m_overrideInherited; }

	CGameplayEntityParam( const String& name = TXT("none"), Bool overrideInherited = false ) 
		: m_overrideInherited( overrideInherited )
		, m_name( name )						{}
	virtual Bool OnPropModified( CName fieldName )	{ return false; }
};
BEGIN_CLASS_RTTI( CGameplayEntityParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_name, TXT("Name") );
	PROPERTY_EDIT( m_overrideInherited , TXT("Ignore params of this type from included components") );
END_CLASS_RTTI();


class CTemplateListParam  : public CGameplayEntityParam
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CTemplateListParam, CGameplayEntityParam );

public:
#ifndef RED_FINAL_BUILD
	typedef TDynArray< THandle< CEntityTemplate > > TTemplateList;

	TTemplateList		m_templateList;
#endif

public:
	CTemplateListParam();

#ifndef NO_RESOURCE_COOKING
	virtual Bool IsCooked() override;
#endif
	
};
BEGIN_CLASS_RTTI( CTemplateListParam );
	PARENT_CLASS( CGameplayEntityParam );
#ifndef RED_FINAL_BUILD
	PROPERTY_NOT_COOKED( m_templateList );
#endif
END_CLASS_RTTI();
