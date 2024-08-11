/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


/// Group property for material instance object
class CMaterialInstanceGroupItem : public IReferencable, public CBaseGroupItem 
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialInstanceGroupItem )

protected:
	CMaterialInstance*		m_materialInstance;		//!< Material
	CMaterialGroupItem*		m_parametersGroupItem;	//!< Parameters group item
	CPropertyItem*			m_enableMaskItem;

protected:
	void RemoveSpecialItems();
	void RecreateSpecialItems();

public:
	CMaterialInstanceGroupItem();
	CMaterialInstanceGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CMaterialInstance* materialInstance );

	virtual void Expand() override;
	virtual void Collapse() override;
	virtual String GetCaption() const override;
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex ) override;

private:
	void operator = ( const CMaterialInstanceGroupItem& );
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialInstanceGroupItem, IReferencable );
