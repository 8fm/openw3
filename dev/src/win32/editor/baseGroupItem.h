/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CBaseGroupItem : public CBasePropItem
{
public:
	CBaseGroupItem( CEdPropertiesPage* page, CBasePropItem* parent );

public:
	virtual void Expand()=0;
	virtual Int32 GetHeight() const;
	virtual Int32 GetLocalIndent() const;
	virtual void DrawLayout( wxDC& dc );
	virtual void UpdateLayout( Int32& yOffset, Int32 x, Int32 width );
    virtual Bool SerializeXML( IXMLFile& file );
};
