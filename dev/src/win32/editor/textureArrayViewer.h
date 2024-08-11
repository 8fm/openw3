#pragma once

#include "textureArrayGrid.h"

class CEdTextureArrayViewer :  public wxSmartLayoutFrame, public CEdTextureArrayGridHook
{
private:
	//////////////////////////////////////////////////////////////////////////
	// wxWidgets stuff
	wxButton*							m_searchButton;
	wxButton*							m_replaceButton;
	wxButton*							m_deleteButton;
	wxButton*							m_shiftButton;
	wxPanel*							m_gridPanel;
	wxPanel*							m_TAVPanel;
	wxStaticText*						m_slotTxt;
	CEdTextureArrayGrid*				m_grid;
	wxChoice*							m_pcSizeList;
	wxChoice*							m_xboneSizeList;
	wxChoice*							m_ps4SizeList;
	//////////////////////////////////////////////////////////////////////////

	CTextureArray*						m_textureArray;
	
	//////////////////////////////////////////////////////////////////////////
	// backup stuff
	// when user modifies something in the texture array, keep track of modifications on the backup
	// of textures. This way, if user decides not to save the array on closing, the original array remains untouched
	TDynArray< CBitmapTexture* >		m_backupTextures;
	TDynArray< Int32 >					m_selectedTextureIndices;
	Bool								m_backupValid;
	//////////////////////////////////////////////////////////////////////////

	void CreateBackup();

	void UpdateThumbnails();
	void RefreshWindow( Bool updateThumbnails = true );

protected:
	virtual void OnTextureArrayGridModified( class CEdTextureArrayGrid* grid );
	virtual void OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary );

public:
	CEdTextureArrayViewer( wxWindow* parent, CTextureArray* textureArray );
	~CEdTextureArrayViewer();

	void OnAddTexture(wxCommandEvent& event);
	void OnRemoveTexture(wxCommandEvent& event);
	void OnReplaceTexture(wxCommandEvent& event);
	void OnSearchTexture(wxCommandEvent& event);
	void OnShiftTextures(wxCommandEvent& event);
	void OnDoubleClick( wxMouseEvent & event );

	void OnClose(wxCloseEvent& event);
	void OnCloseAndSave(wxCommandEvent& event);
	void OnCloseSavePrompt(wxCommandEvent& event);
	void UpdateDownsizeLists();
	void OnChoiceListChanged( wxCommandEvent& event );
};

