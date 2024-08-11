/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/core/uniquePtr.h"

class IEntitiesData
{
public:
	virtual ~IEntitiesData() {}
	virtual void Initialize( Bool onlySelected ) = 0;
	virtual Uint32 Size() const = 0;
	virtual const TagList& GetTagList( Uint32 i ) const = 0;
	virtual String GetShortInfo( Uint32 i ) const = 0;
	virtual Bool IsValid( Uint32 i ) const = 0;
	virtual void ClearSelection() const = 0;
	virtual void Select( Uint32 i ) const = 0;
	virtual void Goto( Uint32 i ) const = 0;
	virtual Bool OnObjectInspector( wxWindow* parent, Uint32 i ) const = 0;

	virtual String GetName( Uint32 i ) const = 0;
	virtual String GetType( Uint32 i ) const = 0;
	virtual String GetPath( Uint32 i ) const = 0;
	virtual String GetTags( Uint32 i ) const = 0;
};

class CEntitiesBrowser : public wxPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:
	CEntitiesBrowser( wxWindow* parent );
	~CEntitiesBrowser();

private:
	void InitializeAll();

	// High level GUI methods
	void ClearGUI();
	void RefreshGUI();
	void ApplyFilter();

	// GUI events handlers
	void OnRefresh( wxCommandEvent &event );
	void OnFind( wxCommandEvent &event );
	void OnGotoEntity( wxGridEvent &event );
	void OnLeftClick( wxGridEvent &event );
	void OnCopyToClipboard( wxCommandEvent &event );
	void OnObjectInspector( wxCommandEvent &event );
	void OnSelectAll( wxCommandEvent& event );
	void OnModeChange( wxCommandEvent& event );
	void OnShow( wxShowEvent& event );

	void UpdateWorldSelection();
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	Bool CompareTexts( const String& text, const String& searchText, Uint32 dataIndex );
 
private:
	// Data
	Red::TUniquePtr< IEntitiesData >	m_entitiesData;

	// Connection Data <-> Gui
	TDynArray< Uint32 >	m_filteredToEntitiesIdx;
	TDynArray< CName >	m_currentTagList;

	// GUI elements
	wxGrid*			m_entitiesGrid;
	wxMenuBar*		m_menuBar;
	wxTextCtrl*		m_textCtrlFind;
	wxCheckBox*		m_checkBoxNameFind;
	wxCheckBox*		m_checkBoxTypeFind;
	wxCheckBox*		m_checkBoxPathFind;
	wxCheckBox*		m_checkBoxTagFind;
	wxCheckBox*		m_matchWholeWord;
	wxCheckBox*		m_matchCase;
	wxCheckBox*		m_inverseSearch;
	wxCheckBox*		m_showSelected;

	wxRadioButton*	m_searchEntities;
	wxRadioButton*	m_searchEntityTemplates;
};

class CEntitiesDataEntity : public IEntitiesData
{
public:
	virtual void Initialize( Bool onlySelected );
	virtual Uint32 Size() const;
	virtual const TagList& GetTagList( Uint32 i ) const;
	virtual String GetShortInfo( Uint32 i ) const;
	virtual Bool IsValid( Uint32 i ) const;
	virtual void ClearSelection() const;
	virtual void Select( Uint32 i ) const;
	virtual void Goto( Uint32 i ) const;
	virtual Bool OnObjectInspector( wxWindow* parent, Uint32 i ) const;

	virtual String GetName( Uint32 i ) const;
	virtual String GetType( Uint32 i ) const;
	virtual String GetPath( Uint32 i ) const;
	virtual String GetTags( Uint32 i ) const;

private:
	TDynArray< THandle< CEntity > > m_worldEntities; // all world attached entities
};

class CEntitiesDataEntityTemplate : public IEntitiesData
{
public:
	virtual void Initialize( Bool onlySelected );
	virtual Uint32 Size() const;
	virtual const TagList& GetTagList( Uint32 i ) const;
	virtual String GetShortInfo( Uint32 i ) const;
	virtual Bool IsValid( Uint32 i ) const;
	virtual void ClearSelection() const;
	virtual void Select( Uint32 i ) const;
	virtual void Goto( Uint32 i ) const;
	virtual Bool OnObjectInspector( wxWindow* parent, Uint32 i ) const { return false; }

	virtual String GetName( Uint32 i ) const;
	virtual String GetType( Uint32 i ) const;
	virtual String GetPath( Uint32 i ) const;
	virtual String GetTags( Uint32 i ) const;

private:
	TDynArray< THandle< CEntityTemplate > > m_worldEntityTemplates; // all world attached entities
};
