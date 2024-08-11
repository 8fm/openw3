/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"

class CEdBehaviorPreviewPanel;
class CVariantArray;

class CEdBehaviorEditorProperties : public CEdPropertiesPage
								  , public CEdBehaviorEditorPanel
{
	DECLARE_EVENT_TABLE()

public:
	CEdBehaviorEditorProperties( CEdBehaviorEditor* editor, const PropertiesPageSettings& settings, CEdUndoManager* undoManager );

	static wxString		GetPanelNameStatic()	{ return wxT("Properties"); }

	virtual wxWindow*	GetPanelWindow()		{ return this; }
	virtual wxString	GetPanelName() const	{ return wxT("Properties"); }
	virtual wxString	GetPanelCaption() const { return wxT("Static properties"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnDebug( Bool flag );
	virtual void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	virtual void OnNodesDeselect();

public:
	//! Query interface
	virtual CEdBehaviorEditorProperties* QueryBehaviorEditorProperties() { return this; }

protected:
	virtual void PropertyPreChange( IProperty* property, STypedObject object );
	virtual void PropertyPostChange( IProperty* property, STypedObject object );
};

//////////////////////////////////////////////////////////////////////////

class CEdInstancePropertiesPage : public wxGrid
{
	DECLARE_EVENT_TABLE()
	
public:
	CEdInstancePropertiesPage( wxWindow* parent );

	void ClearPage();
	void FillPage( CInstancePropertiesBuilder& properties );
	void RefreshPage( CInstancePropertiesBuilder& properties );

protected:
	Bool IsClass( const CVariant& variant ) const;
	Bool IsArray( const CVariant& variant ) const;
	Bool ParseVariant( const CVariant& variant, String& out ) const;
	Bool ParseVariant( const CVariantArray& variant, String& out ) const;
};

class CEdBehaviorEditorRuntimeProperties : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	Bool						m_connected;
	CEdInstancePropertiesPage*	m_properties;
	CBehaviorGraphNode*			m_selectedNode;

public:
	CEdBehaviorEditorRuntimeProperties( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("PropertiesRuntime"); }
	virtual wxString	GetPanelCaption() const { return wxT("Runtime properties"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	virtual void OnNodesDeselect();
	virtual void OnTick( Float dt );

protected:
	void OnConnect( wxCommandEvent& event );

protected:
	void FillProperties();
	void RefreshProperties();
};
