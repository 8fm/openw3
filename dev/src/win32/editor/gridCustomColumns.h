/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gridColumnDesc.h"

class CGridChoiceColumnDesc : public IGridColumnDesc
{
private:
	wxArrayString		m_choices;
	wxArrayString		m_values;

	CGridChoiceColumnDesc( const wxArrayString & values, const wxArrayString * choices = NULL );
	
public:
	wxGridCellEditor*	GetCellEditor() const;
	wxGridCellRenderer*	GetCellRenderer() const;

	const wxArrayString & GetChoices() const { return m_choices; }
	const wxArrayString & GetValues()  const { return m_values; }

	virtual Bool AllowAutoSize() const { return false; }

public:
	static IGridColumnDesc* CreateFromStrings( const wxArrayString & values, const wxArrayString * choices = NULL );
	static IGridColumnDesc* CreateFrom2da( const C2dArray* valueTable, Int32 valueColumn, Int32 textColumn = -1 );
};

class CGridReadOnlyColumnDesc : public IGridColumnDesc
{
public:
	wxGridCellEditor*	GetCellEditor() const;
	wxGridCellRenderer*	GetCellRenderer() const;
};

class CGridVoicetagColumnDesc : public IGridColumnDesc
{
public:
	CGridVoicetagColumnDesc( Int32 columnNum ) : m_sourceColumnNum( columnNum ) {}

	virtual wxGridCellEditor*	GetCellEditor() const;
	virtual wxGridCellRenderer*	GetCellRenderer() const;

	// Extra info
	virtual Bool DoesRequireExtraInfo() { return true; }
	virtual Int32 GetInterestColumnNumber() { return m_sourceColumnNum; }
	virtual void SetExtraInfo( const IRTTIType *type, void *data );

private:
	const CRTTISoftHandleType *m_extraInfoType;
	void                      *m_data;
	Int32                        m_sourceColumnNum; // grid column number, that we are interested in
};

class CGridAppearanceColumnDesc : public IGridColumnDesc
{
public:
	CGridAppearanceColumnDesc( Int32 columnNum ) : m_sourceColumnNum( columnNum ) {}

	virtual wxGridCellEditor*	GetCellEditor() const;
	virtual wxGridCellRenderer*	GetCellRenderer() const;

	// Extra info
	virtual Bool DoesRequireExtraInfo() { return true; }
	virtual Int32 GetInterestColumnNumber() { return m_sourceColumnNum; }
	virtual void SetExtraInfo( const IRTTIType *type, void *data );

private:
	const CRTTISoftHandleType *m_extraInfoType;
	void                      *m_data;
	Int32                        m_sourceColumnNum; // grid column number, that we are interested in
};

// This event handler's task is only to skip killing focus of the inventory item grid field
// That allows safely writing new value when choose item dialog is closing
class CGridCellChooseItemEditorEventHandler : public wxEvtHandler
{
	friend class CGridCellInventoryItemEditor;
public:
	CGridCellInventoryItemEditor * m_owner;

public:
	void OnKillFocus( wxFocusEvent & event );

public:
	CGridCellChooseItemEditorEventHandler( CGridCellInventoryItemEditor * owner );

	DECLARE_EVENT_TABLE()
	DECLARE_DYNAMIC_CLASS(CGridCellChooseItemEditorEventHandler)
	DECLARE_NO_COPY_CLASS(CGridCellChooseItemEditorEventHandler)
};

class CGridInventoryItemColumnDesc : public IGridColumnDesc
{
public:
	CGridInventoryItemColumnDesc() {}

	virtual wxGridCellEditor*	GetCellEditor() const;
	virtual wxGridCellRenderer*	GetCellRenderer() const;
};