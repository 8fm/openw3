/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemBitField : public CPropertyItem
{
protected:
	TDynArray< CProperty* >		m_dynamicProperties;

public:
	CPropertyItemBitField( CEdPropertiesPage* page, CBasePropItem* parent );
	~CPropertyItemBitField();

	virtual void Expand();
	virtual void Collapse();

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool SerializeXML( IXMLFile& file );
};