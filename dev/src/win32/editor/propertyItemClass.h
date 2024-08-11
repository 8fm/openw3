/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemClass : public CPropertyItem
{
	Bool			m_dividePropertiesPerClass;
	CPropertyDataBuffer m_writePropBuffer;

public:
	CPropertyItemClass( CEdPropertiesPage* page, CBasePropItem* parent );
	~CPropertyItemClass();

	virtual void Expand() override;
	virtual STypedObject GetParentObject( Int32 objectIndex ) const override;

public:
	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 ) override;

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
};