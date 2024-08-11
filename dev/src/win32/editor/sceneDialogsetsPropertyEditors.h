/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "selectionEditor.h"
#include "numberRangePropertyEditor.h"

class CEdDialogsetCameraShotNamePropertyEditor : public ISelectionEditor
{
public:
	CEdDialogsetCameraShotNamePropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogsetCameraShotNamePropertyEditor();

protected:
	virtual void FillChoices();
};

class CEdDialogsetCameraShotAnimationPropertyEditor : public ISelectionEditor
{
public:
	CEdDialogsetCameraShotAnimationPropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogsetCameraShotAnimationPropertyEditor();

protected:
	virtual void FillChoices();
};

class CEdDialogsetCameraNumberPropertyEditor : public INumberRangePropertyEditor
{

public:
	CEdDialogsetCameraNumberPropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogsetCameraNumberPropertyEditor();

protected:
	virtual void ApplyRange();
	virtual void OnSpinChanged( wxCommandEvent& event );
};

class CEdDialogsetCharacterNumberPropertyEditor : public INumberRangePropertyEditor
{
public:
	CEdDialogsetCharacterNumberPropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogsetCharacterNumberPropertyEditor();

protected:
	virtual void ApplyRange();
};




class CEdActionCategoryPropEditor : public ISelectionEditor
{
public:
	CEdActionCategoryPropEditor( CPropertyItem* item ) : ISelectionEditor( item )
	{}

protected:
	virtual void FillChoices();
};