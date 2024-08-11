/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemArray : public CPropertyItem
{
public:
	CPropertyItemArray( CEdPropertiesPage* page, CBasePropItem* parent );

	virtual void Expand() override;
	virtual void CreateMainControl() override;

public:
	void OnArrayClear( wxCommandEvent& event );
	void OnArrayAdd( wxCommandEvent& event );
	void OnArrayDeleteItem( Uint32 arrayIndex );
	void OnArrayInsertItem( Uint32 arrayIndex );

	void* GetArrayElement( Uint32 arrayIndex, Int32 objectIndex );

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
    virtual Bool SerializeXML( IXMLFile& file ) override;
};

class CPropertyItemStaticArray : public CPropertyItem
{
public:
	CPropertyItemStaticArray( CEdPropertiesPage* page, CBasePropItem* parent );

	virtual void Expand() override;
	virtual void CreateMainControl() override;

public:
	void* GetArrayElement( Uint32 arrayIndex, Int32 objectIndex );

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
    virtual Bool SerializeXML( IXMLFile& file ) override;
};