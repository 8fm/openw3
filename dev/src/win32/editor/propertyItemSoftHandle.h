/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPropertyItemSoftHandle : public CPropertyItem
{
public:
	CPropertyItemSoftHandle( CEdPropertiesPage* page, CBasePropItem* parent );

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

private:
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override; 

private:
	Bool ShouldLinkIconBeDisplayed();
};