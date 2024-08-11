
#pragma once

#include "poppedUp.h"

class IDialogBodyPartOwner;

class CDialogBodyPartSelection_GroupName	: public wxEvtHandler
											, public ICustomPropertyEditor
{
	String		m_value;

	wxBitmap	m_iconSelect;
	wxBitmap	m_iconSave;
	wxBitmap	m_iconClear;

public:
	CDialogBodyPartSelection_GroupName( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayData ) override;

	virtual Bool SaveValue() override;

protected:
	void OnButtonSelect( wxCommandEvent &event );
	void OnButtonSave( wxCommandEvent &event );
	void OnButtonClear( wxCommandEvent &event );

private:
	IDialogBodyPartOwner* GetOwner();
	const CSkeleton* GetSkeleton();

	void SelectPreset( const String& p );
	Bool SavePreset( const String& presetName );
	Bool RemovePreset( const String& presetName );
	void CollectRecords( CEdPoppedUp< wxListBox >& ed ) const;
	Bool CollectRecords( const CSkeleton* skeleton, TDynArray<SBehaviorGraphBoneInfo>& arr ) const;
};

//////////////////////////////////////////////////////////////////////////

class CDialogBodyPartSelection_Bones	: public wxEvtHandler
										, public ICustomPropertyEditor
{
	wxBitmap	m_iconBone;

public:
	CDialogBodyPartSelection_Bones( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayData ) override;

	virtual Bool SaveValue() override;

protected:
	void OnSelectionDialog( wxCommandEvent &event );
};
