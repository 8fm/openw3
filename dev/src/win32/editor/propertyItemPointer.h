/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemPointer : public CPropertyItem
{
public:
	CPropertyItemPointer( CEdPropertiesPage* page, CBasePropItem* parent );

	virtual void Init( CProperty *prop, Int32 arrayIndex = -1 ) override;
	virtual void Init( IRTTIType *type, Int32 arrayIndex = -1 ) override;

	virtual void Expand() override;
	virtual void CreateMainControl() override;

	virtual STypedObject GetParentObject( Int32 objectIndex ) const override;

	virtual void UseSelectedResource() override { wxCommandEvent fakeEvent; OnObjectUse(fakeEvent); }

public:
	void OnObjectClear( wxCommandEvent& event );
	void OnObjectUse( wxCommandEvent& event );
	void OnObjectBrowser( wxCommandEvent& event );
	void OnObjectInlineDelete( wxCommandEvent& event );
	void OnObjectInlineNewInternal( CClass* objectClass );
	void OnObjectInlineNew( wxCommandEvent& event );

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
    virtual Bool SerializeXML( IXMLFile& file ) override;

private:
	Bool ReadObj( void* buffer, Int32 objectIndex = 0 ) const;
	void WriteObj( void* buffer, Int32 objectIndex = 0 ) const;

private:
	Bool ShouldLinkIconBeDisplayed();
};