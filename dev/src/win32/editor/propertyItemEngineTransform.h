/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Property item handing edition of EngineTransform type
class CPropertyItemEngineTransform : public CPropertyItem
{
protected:
	static CName propTranslationX;
	static CName propTranslationY;
	static CName propTranslationZ;
	static CName propRotationPitch;
	static CName propRotationRoll;
	static CName propRotationYaw;
	static CName propScaleX;
	static CName propScaleY;
	static CName propScaleZ;

protected:
	TDynArray< CProperty* >		m_dynamicProperties;

public:
	CPropertyItemEngineTransform( CEdPropertiesPage* page, CBasePropItem* parent );
	~CPropertyItemEngineTransform();

	virtual void Expand();
	virtual void Collapse();
	virtual void CreateControls();

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool SerializeXML( IXMLFile& file );

private:
	CProperty* CreateFloatProperty( CName propName, const Char* info );

private:
	void OnResetToIdentity( wxCommandEvent& event );
};