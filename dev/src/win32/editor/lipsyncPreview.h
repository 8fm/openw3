
#pragma once

#include "animBrowser.h"

class CEdLipsyncPreviewPlaylistItem : public wxTreeItemData
{
public:
	String		m_text;
	String		m_actor;

	CEdLipsyncPreviewPlaylistItem( const String& actor, const String& text ) : m_actor( actor ), m_text( text ) {}
};

class CEdLipsyncPreviewPlaylist
{
	wxTreeCtrl*			m_tree;
	wxListBox*			m_list;

	Bool				m_paused;

	wxColor				SEL_COLOR;
	wxColor				DEFAULT_COLOR;

	enum LatentAction
	{
		LA_None,
		LA_Next,
		LA_Prev,
	};

	LatentAction		m_latentAction;

	wxTreeItemId		m_currItem;

public:
	CEdLipsyncPreviewPlaylist( wxTreeCtrl* tree, wxListBox* list );
	~CEdLipsyncPreviewPlaylist();

	void Fill( CActor* actor, const TDynArray< String >& fileList );
	void Clear();

	void Update( CActor* actor );

public:
	void Play( CActor* actor );
	void Pause( CActor* actor );
	Bool IsPaused() const;

	void Reset( CActor* actor );
	void Next();
	void Prev();

	void Select();

protected:
	wxTreeItemId FindTreeItemByActor( const String& actor ) const;

	void PlayItem( CActor* actor );

	Bool FindNextItem();
	Bool FindPrevItem();

	void SelectCurrItem();
	void DeselectCurrItem();
};

class CEdLipsyncPreview : public wxFrame, public IAnimBrowserPreviewListener
{
	DECLARE_EVENT_TABLE();

	CEdLipsyncPreviewPlaylist*	m_list;
	CEdAnimBrowserPreview*		m_preview;
	CEntity*					m_previewEntity;

public:
	CEdLipsyncPreview( wxWindow* parent );
	virtual ~CEdLipsyncPreview();

public: //IAnimBrowserPreviewListener
	virtual void Tick( Float timeDelta );

protected:
	void LoadEntity( const String &entName );
	void UnloadEntity();

	void ClearList();
	void FillListManual();
	void FillListCsv();

protected:
	void OnLoadSelEntity( wxCommandEvent& event );
	void OnLoadEntity( wxCommandEvent& event );
	void OnManualSelect( wxCommandEvent& event );
};
