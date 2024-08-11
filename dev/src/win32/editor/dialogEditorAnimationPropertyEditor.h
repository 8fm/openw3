/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"
#include "animationSelectionEditor.h"

class CSkeletalAnimationSetEntry;

class IAnimationSelectionByNameEditor : public ISelectionEditor, public IAnimationComponentAwareCustomPropertyEditor
{
public:
	IAnimationSelectionByNameEditor( CPropertyItem* propertyItem );
	virtual void FillChoices() override;
	virtual Bool SaveValue() override;

protected:
	class CAnimationEntryDataItem : public wxClientData
	{
	public:
		CSkeletalAnimationSetEntry*	m_animationEntry;
		CAnimationEntryDataItem( CSkeletalAnimationSetEntry* animationEntry ) : m_animationEntry( animationEntry ) {}
	};

	virtual CAnimatedComponent* RetrieveAnimationComponent() const;

	virtual void OnAnimationSelected( const CSkeletalAnimationSetEntry* animationEntry ) = 0;
	virtual Int32 AppendChoiceIfAnimationIsValid( CSkeletalAnimationSetEntry* animationEntry ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogAnimationSelection : public IAnimationSelectionEditor
{
public:
	CEdDialogAnimationSelection( CPropertyItem* propertyItem );

protected:
	virtual CAnimatedComponent* RetrieveAnimationComponent() const override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogMimicAnimationSelection : public IAnimationSelectionEditor
{
public:
	CEdDialogMimicAnimationSelection( CPropertyItem* propertyItem );

protected:
	virtual CAnimatedComponent* RetrieveAnimationComponent() const override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogCameraAnimationSelection : public IAnimationSelectionEditor
{
public:
	CEdDialogCameraAnimationSelection( CPropertyItem* propertyItem );

protected:
	virtual CAnimatedComponent* RetrieveAnimationComponent() const override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogBodyAnimationFilter : public ISelectionEditor
{
protected:
	Uint32	m_categoryLevel;

public:
	CEdDialogBodyAnimationFilter( CPropertyItem* propertyItem, Uint32 categoryLevel );

protected:
	virtual void FillChoices() override;

	virtual void OnChoiceChanged( wxCommandEvent &event ) override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogMimicsAnimationFilter : public ISelectionEditor
{
protected:
	Uint32	m_categoryLevel;

public:
	CEdDialogMimicsAnimationFilter( CPropertyItem* propertyItem, Uint32 categoryLevel );

protected:
	virtual void FillChoices() override;

	virtual void OnChoiceChanged( wxCommandEvent &event ) override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogExitAnimationSelection : public ISelectionEditor
{
public:
	CEdDialogExitAnimationSelection( CPropertyItem* propertyItem );

protected:
	virtual void FillChoices() override;
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogEnterAnimationSelection : public ISelectionEditor
{
public:
	CEdDialogEnterAnimationSelection( CPropertyItem* propertyItem );

protected:
	virtual void FillChoices() override;

};