/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

class CEdDownscaleBiasDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdDownscaleBiasDlg( wxWindow* parent, CContextMenuDir* contextMenuDir );

private:
	wxChoice*						m_pcChoice;
	wxChoice*						m_xboneChoice;
	wxChoice*						m_ps4Choice;
	CContextMenuDir 				m_contextMenuDir;

	void OnBatch( wxCommandEvent& event );
	void Downscale( CBitmapTexture* atexture );
};