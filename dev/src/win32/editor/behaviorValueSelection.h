/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"

class CBehaviorParentInputSelection : public ISelectionEditor									
{
public:
	CBehaviorParentInputSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CBehaviorParentValueInputSelection : public ISelectionEditor
{
public:
	CBehaviorParentValueInputSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CBehaviorParentVectorValueInputSelection : public ISelectionEditor
{
public:
	CBehaviorParentVectorValueInputSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CBehaviorMimicParentInputSelection : public ISelectionEditor
{
public:
	CBehaviorMimicParentInputSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CBehaviorParentValueInputTransitionConditionSelection : public ISelectionEditor
{
public:
	CBehaviorParentValueInputTransitionConditionSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CBehaviorVariableSelection : public ISelectionEditor
{
public:
	CBehaviorVariableSelection( CPropertyItem* item, bool forInternalVariable ) : ISelectionEditor( item ), m_forInternalVariable( forInternalVariable ) {};
protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );
private:
	Bool m_forInternalVariable;
};

class CBehaviorVectorVariableSelection : public ISelectionEditor
{
public:
	CBehaviorVectorVariableSelection( CPropertyItem* item, bool forInternalVariable ) : ISelectionEditor( item ), m_forInternalVariable( forInternalVariable ) {};
protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );
private:
	Bool m_forInternalVariable;
};

class CBehaviorEventSelection : public ISelectionEditor
{
private:
	Bool		m_withNew;

public:
	CBehaviorEventSelection( CPropertyItem* item, Bool withNew ) : ISelectionEditor( item ), m_withNew( withNew ) {};
protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );
};

class CBehaviorNotificationSelection : public ISelectionEditor
{
public:
	CBehaviorNotificationSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );
};

class CBehaviorTrackSelection : public ISelectionEditor
{
public:
	CBehaviorTrackSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

