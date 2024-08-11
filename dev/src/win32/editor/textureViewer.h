/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/core/diskFile.h"

class CEdTexturePreviewPanel;

/// Material editor
class CEdTextureViewer	: public wxSmartLayoutPanel
						, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CResource*								m_texture; // only bitmaps derive from ITexture so this can only be a CResource to handle CubeTextures
	Float									m_lodBias;
	CEdPropertiesBrowserWithStatusbar*		m_properties;
	CEdTexturePreviewPanel*					m_preview;
	CBitmapTexture*							m_bitmapTexture;	
	wxChoice*								m_pcSizeList;
	wxChoice*								m_xboneSizeList;
	wxChoice*								m_ps4SizeList;

public:
	CEdTextureViewer( wxWindow* parent, CResource* texture );
	~CEdTextureViewer();

	virtual wxString GetShortTitle() { return m_texture->GetFile()->GetFileName().AsChar() + wxString(TXT(" - TextureViewer")); }
	CResource* GetTexture() const { return m_texture; }
	Float GetLODBias() const { return m_lodBias; }

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );

	void OnMipChange( wxCommandEvent& event );
	void OnUpdateMipMapSlider();

	void OnDownsize( wxCommandEvent& event );

	void OnRestore( wxCommandEvent& event );
	Bool CheckIfSourceExists();
	void UpdateDownsizeLists();

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnUpdateUI( wxUpdateUIEvent& event );
};
