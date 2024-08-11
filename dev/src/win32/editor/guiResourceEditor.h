/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class IGuiResource;

class CEdGuiResourceEditor
	: public wxFrame
	, public GraphEditorHook
	, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

private:
	IGuiResource*							m_guiResource;
	CEdPropertiesPage*						m_nodeProperties;
	class CEdGuiResourceGraphEditor*		m_graphEditor;

protected:
	//! Events
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );

public:
	CEdGuiResourceEditor( wxWindow* parent, class IGuiResource* guiResource = nullptr );
	~CEdGuiResourceEditor();

	IGuiResource* GetGuiResource() const;

public:
	void SetPropertyGraphBaseItem( ISerializable* item );
	void RefreshPreview();

public:
	//! GraphEditorHook interface
	virtual void OnGraphStructureModified( IGraphContainer* graph );
	virtual void OnGraphSelectionChanged();

public:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
};

