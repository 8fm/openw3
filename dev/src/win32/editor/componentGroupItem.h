/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CComponentGroupItem: public CBaseGroupItem
{
protected:
	CClass*						m_componentClass;
	TDynArray< CComponent* >	m_components;

public:
	CComponentGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CClass *componentClass );

	void SetObjects( const TDynArray< CComponent* > &objects  );

	virtual String GetCaption() const;
	
	void Expand();

public:
	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );

public:
	STypedObject GetParentObject( Int32 objectIndex ) const override;
};
