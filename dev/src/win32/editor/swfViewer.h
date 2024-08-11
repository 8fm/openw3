/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/bitmapTexture.h"

class CEdSwfImagePanel;
class CSwfResource;
class CEdColorPicker;

class CEdSwfViewer	: public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

private:
	struct STextureData 
	{
		CBitmapTexture::MipMap	m_mipCopy;
		Uint32					m_format;

		STextureData();
		~STextureData();
	};

private:
	CSwfResource*							m_swfResource;
	CEdPropertiesPage*						m_properties;
	CEdSwfImagePanel*						m_preview;
	wxListCtrl*								m_thumbnailList;
	CEdColorPicker*							m_colorPicker;

public:
	CEdSwfViewer( wxWindow* parent, CSwfResource* swfResource );
	~CEdSwfViewer();

	virtual wxString GetShortTitle();

	// Save / Load options from config file
	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;
	virtual void SaveSession( CConfigurationManager& config ) override;
	virtual void RestoreSession( CConfigurationManager& config ) override;

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnPropertySelected( wxCommandEvent& event );
	void OnThumbnailSelected( wxCommandEvent& event );

	void OnSave( wxCommandEvent& event );
	void OnBackgroundColor( wxCommandEvent& event );
	void OnColorPicked( wxCommandEvent& event );

	void OnRestore( wxCommandEvent& event );

	Bool CheckIfSourceExists();

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	Bool CreateImage( const STextureData& textureData, wxImage& outImage );
	void SetCurrentImage( const STextureData& textureData );
	//void ClearCurrentImage();

private:
	Bool GetTextureData( const CSwfTexture* swfTexture, STextureData& outTextureData );

private:
	void DumpTexturesToFile();
};
