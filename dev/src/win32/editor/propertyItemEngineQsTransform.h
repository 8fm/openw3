/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Property item handing edition of EngineQsTransform type
class CPropertyItemEngineQsTransform : public CPropertyItem
{
protected:
	static CName propTranslationX;
	static CName propTranslationY;
	static CName propTranslationZ;
	static CName propRotationX;
	static CName propRotationY;
	static CName propRotationZ;
	static CName propRotationW;
	static CName propScaleX;
	static CName propScaleY;
	static CName propScaleZ;

protected:
	TDynArray< CProperty* >		m_dynamicProperties;

public:
	CPropertyItemEngineQsTransform( CEdPropertiesPage* page, CBasePropItem* parent );
	~CPropertyItemEngineQsTransform();

	virtual void Expand();
	virtual void Collapse();
	virtual void CreateControls();

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool SerializeXML( IXMLFile& file );
	virtual Bool IsReadOnly() const;

private:
	CProperty* CreateFloatProperty( CName propName, const Char* info );

private:
	void OnResetToIdentity( wxCommandEvent& event );
};
