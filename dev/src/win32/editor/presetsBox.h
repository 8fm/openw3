/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "presets.h"

class CEdPresetsBox;

//! Hook for CEdPresetsBox events
class IEdPresetsBoxHook
{
public:
	virtual ~IEdPresetsBoxHook(){}

	//! Called to confirm that a preset can be applied
	virtual Bool OnCanApplyPreset( CEdPresetsBox* /* source */, const String& /* presetName */ ) { return true; }

	//! Called to apply a preset
	virtual void OnApplyPreset( CEdPresetsBox* /* source */, const String& /* presetName */ ){}

	//! Called to save a preset
	virtual void OnSavePreset( CEdPresetsBox* /* source */, const String& /* presetName */ ){}

	//! Called after a preset has been deleted
	virtual void OnPresetDeleted( CEdPresetsBox* /* source */, const String& /* presetName */ ){}
};


//! A wxWidgets panel that provides functionality for editing presets.
//!
//! Note: this will install a custom hook to the presets object you pass
//! which will call any original hook - make sure you set up that hook
//! before passing it to this object.
class CEdPresetsBox : public wxPanel
{
	friend class CEdPresetsBoxPresetsHook;
	
	wxComboBox*							m_presetName;
	wxBitmapButton*						m_saveButton;
	wxBitmapButton*						m_deleteButton;
	wxBitmapButton*						m_menuButton;
	CEdPresets*							m_presets;
	IEdPresetsBoxHook*					m_hook;
	IEdPresetsHook*						m_originalHook;
	class CEdPresetsBoxPresetsHook*		m_boxHook;
	String								m_lastKnownName;

	void OnPresetNameSelected( wxCommandEvent& event );
	void OnSavePresetClick( wxCommandEvent& event );
	void OnDeletePresetClick( wxCommandEvent& event );
	void OnMenuClick( wxCommandEvent& event );

public:
	//! Constructs a presets box
	CEdPresetsBox( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL ); 
	~CEdPresetsBox();

	//! Set the presets object to use
	void SetPresets( CEdPresets* presets );

	//! Returns the current presets object
	RED_INLINE CEdPresets* GetPresets() const { return m_presets; }

	//! Updates the box with the data from the presets
	//! (call this if you modify the presets object outside of a hook event)
	void UpdatePresetsUI();

	//! Sets the hook for this box to be notified when events happen
	void SetPresetsBoxHook( IEdPresetsBoxHook* hook );

	//! Returns the current presets box hook
	RED_INLINE IEdPresetsBoxHook* GetPresetsBoxHook() const { return m_hook; }
};
