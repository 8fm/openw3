/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Group of dynamic object properties
class CDynamicPropGroupItem : public CBaseGroupItem, public IEdEventListener
{
protected:
	IDynamicPropertiesSupplier*		m_supplier;
	TDynArray< CProperty* >			m_fakeProperties;

public:
	CDynamicPropGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, IDynamicPropertiesSupplier* supplier );
	~CDynamicPropGroupItem();

	virtual String GetCaption() const;
	virtual void Collapse();
	virtual void Expand();

public:
	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );

private:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
};
