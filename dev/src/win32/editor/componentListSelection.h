/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

enum ComponentListFlags
{
	CLF_Components = FLAG(0),
	CLF_Slots      = FLAG(1),
	CLF_NoClear    = FLAG(2),
};

class CComponentListSelectionBase : public CListSelection
{
protected:
	THandle< CEntity >	m_entity;
	CObject*			m_parentObject;
	Int32				m_flags;

public:
	CComponentListSelectionBase( CPropertyItem* item, Int32 flags );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

protected:
	virtual Bool SaveValue() override;

	virtual Bool IsComponentApplicable( CComponent* component ) = 0;

	void AddEntityComponents( CEntity* entity );

private:
	Bool FindEntity();
};

// Components with
class CComponentListSelectionEffectParameters : public CComponentListSelectionBase
{
public:
	CComponentListSelectionEffectParameters( CPropertyItem* item, Int32 flags )
		: CComponentListSelectionBase( item, flags ) 
	{};

protected:
	virtual Bool IsComponentApplicable( CComponent* component ) override;
};

// All components
class CComponentListSelectionAll : public CComponentListSelectionBase
{
public:
	CComponentListSelectionAll( CPropertyItem* item, Int32 flags )
		: CComponentListSelectionBase( item, flags )
	{};

protected:
	virtual Bool IsComponentApplicable( CComponent* component ) override;
};

template < class T >
class CComponentListSelectionComponentType : public CComponentListSelectionBase
{
public:
	CComponentListSelectionComponentType( CPropertyItem* item, Int32 flags )
		: CComponentListSelectionBase( item, flags )
	{};

protected:
	virtual Bool IsComponentApplicable( CComponent* component ) override
	{
		return component->IsA< T >();
	}
};
