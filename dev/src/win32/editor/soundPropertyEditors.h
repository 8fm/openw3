/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "selectionEditor.h"

class CAudioElementBrowserPropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	CAudioElementBrowserPropertyEditor( CPropertyItem * );

	// Property item got selected
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;

	Bool GrabValue( String& displayValue ) override;

private:
	void OnSpawnAudioEventBrowser( wxCommandEvent &event );
	void OnRemoveAudioEvent( wxCommandEvent &event );
	void OnEditTextEnter( wxCommandEvent& event );

	void SetValue( const String& value );

	virtual TDynArray< String > GetElements() = 0;
	wxBitmap            m_iconAudioEventBrowser;
	wxBitmap            m_iconRemoveAudioEvent;
	wxTextCtrlEx*		m_textBox;
};

class CAudioEventBrowserPropertyEditor : public CAudioElementBrowserPropertyEditor
{
public:
	CAudioEventBrowserPropertyEditor( CPropertyItem * );
	virtual TDynArray< String > GetElements();

};

class CAudioSwitchBrowserPropertyEditor : public CAudioElementBrowserPropertyEditor
{
public:
	CAudioSwitchBrowserPropertyEditor( CPropertyItem * );

	virtual TDynArray< String > GetElements();

};

class CSoundReverbPropertyEditor : public CAudioElementBrowserPropertyEditor
{
public:
	CSoundReverbPropertyEditor( CPropertyItem* item );
	virtual ~CSoundReverbPropertyEditor();

private:
	virtual TDynArray< String > GetElements();
};

class CSoundBankBrowserPropertyEditor : public CAudioElementBrowserPropertyEditor
{
public:
	CSoundBankBrowserPropertyEditor( CPropertyItem * );

	virtual TDynArray< String > GetElements();
};

class CSoundGameParamterEditor : public CAudioElementBrowserPropertyEditor
{
public:
	CSoundGameParamterEditor( CPropertyItem * );

	virtual TDynArray< String > GetElements();
};