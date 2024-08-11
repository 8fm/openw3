/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Tag definitions
class CEntityTagDefinitions
{
public:
	struct TagInfo
	{
		CName	m_name;		//!< Display name
		CName	m_class;	//!< Required class
		CName	m_tag;		//!< tag that is set/unset
	};

	CEntityTagDefinitions();
	~CEntityTagDefinitions();

	// get tags matching given entity class
	void GetTags( const CClass* entityClass, TDynArray< TagInfo >& outTags ) const;

private:
	TDynArray< TagInfo >		m_tags;
};

/// entity tags
typedef TSingleton< CEntityTagDefinitions >  SEntityTagDefinitions;

/// 
class CEntityTagGroupItem : public CBaseGroupItem
{
public:
	CEntityTagGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, const CClass* entityClass );

	virtual String GetCaption() const override;
	virtual void Expand() override;
	virtual void Collapse() override;

	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override;

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;

	struct FakeProp
	{
		CProperty*			m_prop;
		CName				m_tagName;
	};

	TDynArray< FakeProp	>			m_fakeProperties;
	const CClass*					m_entityClass;

	CName GetTagName( const CProperty* prop ) const;
};


