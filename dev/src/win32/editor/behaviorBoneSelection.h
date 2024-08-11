/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class ISkeletonBoneSelection : public wxEvtHandler, public ICustomPropertyEditor
{
	String		m_boneChosen;
	wxBitmap	m_icon;

public:
	ISkeletonBoneSelection( CPropertyItem* propertyItem );

	void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	void CloseControls() override;

	Bool GrabValue( String& displayData ) override;

	Bool SaveValue() override;

protected:
	void OnSelectionDialog( wxCommandEvent &event );

	void ReadStringFromProperty( String& readValue );
	void WriteStringToProperty( String& valueToWrite );

protected:
	virtual CSkeleton* GetSkeleton() = 0;
};

//////////////////////////////////////////////////////////////////////////

class CBehaviorBoneSelection : public ISkeletonBoneSelection
{
public:
	CBehaviorBoneSelection( CPropertyItem* propertyItem );

protected:
	virtual CSkeleton* GetSkeleton();
};

//////////////////////////////////////////////////////////////////////////

class CSkeletonBoneSelection : public ISkeletonBoneSelection
{
public:
	CSkeletonBoneSelection( CPropertyItem* propertyItem );

protected:
	virtual CSkeleton* GetSkeleton();
};
