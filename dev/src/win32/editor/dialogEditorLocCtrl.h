
#pragma once

class CEdSceneEditor;

class CEdSceneLocalizationCtrl
{
private:
	struct LocLineDataElem
	{
		String						m_lang;
		String						m_text;
		Float						m_percent;
		Float						m_duration;
		Float						m_fakeDuration;

		LocLineDataElem() : m_percent( 0.f ), m_duration( 0.f ), m_fakeDuration( 0.f ) {}
	};

	struct LocLineData
	{
		Uint32						m_id;
		String						m_stringId;
		TDynArray< LocLineDataElem > m_data;

		Uint32						m_maxDiffId;
		Float						m_maxDiffValTime;
		Float						m_maxValPercent;

		THandle< CStorySceneSection >	m_section;
		THandle< CStorySceneLine >		m_line;

		LocLineData() : m_id( 0 ), m_maxDiffId( 0 ), m_maxDiffValTime( 0.f ), m_maxValPercent( 0.f ) {}
	};

	struct LocSectionData
	{
		String			m_name;
		String			m_sectionMaxDiffName;
		Float			m_sectionRefDuration;
		Float			m_sectionMaxDiffSec;
		Float			m_sectionMaxDiffPercent;
		Float			m_sectionMaxDuration;
		Float			m_sectionMaxDurationAll;
		Float			m_sectionMaxDuration_loc;
		Float			m_sectionMaxDurationAll_loc;

		LocSectionData() : m_sectionRefDuration( 0.f ), m_sectionMaxDiffSec( 0.f ), m_sectionMaxDiffPercent( 0.f ), m_sectionMaxDuration( 0.f ), m_sectionMaxDurationAll( 0.f ) {}
	};

private:
	Bool						m_enabled;
	wxPanel*					m_window;

	const CStorySceneSection*	m_section;
	TDynArray< LocLineData >	m_sectionLinesData;
	TDynArray< LocSectionData >	m_sectionsData;

	TDynArray< String >			m_langs;
	TDynArray< Bool >			m_langsFlag;

	const Float					m_percentThr1;
	const Float					m_percentThr2;

public:
	CEdSceneLocalizationCtrl();
	~CEdSceneLocalizationCtrl();

	void Enable( Bool flag );
	Bool IsEnabled() const;

	void BindToWindow( CEdSceneEditor* ed, wxPanel* window );
	Bool IsBindedToWindow() const;

	void ParseScene( const CStoryScene* scene, IFeedbackSystem* f = nullptr );
	Bool ParseSection( const CStorySceneSection* section, IFeedbackSystem* f = nullptr );
	void ReleaseAll();

	void RefreshWindow();

public:
	wxColor FindColorForLocString( Uint32 stringId ) const;
	void CollectUsedLangs( TDynArray< String >& langs ) const;

private:
	Uint32 GetRefLangId() const;
	Uint32 GetCurrLangId() const;
	Uint32 GetLangId( const String& lang ) const;
	void DoParse( IFeedbackSystem* f );
	Bool HasLocData( Uint32 stringId ) const;
	Bool HasLocForLang( const String& lang ) const;
	Bool HasLocDataForSection( const String& sectionName ) const;

public:
	void OnLinkClicked( wxHtmlLinkEvent& event );
	void OnLinkHover( wxHtmlCellEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class wxDialogScreenshotActorClientData : public wxClientData
{
public:
	THandle< CEntity > m_data;

public:
	wxDialogScreenshotActorClientData( CEntity* e  )
		: m_data( e )
	{}
};

class CEdSceneScreenshotCtrl : public wxEvtHandler
{
private:
	CEdSceneEditor*	m_mediator;
	Bool			m_enabled;
	wxPanel*		m_window;

	TDynArray< THandle< CEntity > > m_entities;

private:
	wxStaticText*	m_textFrame;
	wxTextCtrl*		m_textTag;
	wxTextCtrl*		m_textRange;
	wxCheckListBox*	m_actorList;

public:
	CEdSceneScreenshotCtrl();
	~CEdSceneScreenshotCtrl();

	void Enable( Bool flag );
	Bool IsEnabled() const;

	void BindToWindow( CEdSceneEditor* ed, wxPanel* window );
	Bool IsBindedToWindow() const;

public:
	void AddExtraActorsToScene( TDynArray< THandle< CEntity > >& actors, TDynArray< THandle< CEntity > >& props ) const;

private:
	void AddEntity( CEntity* e );
	void RemoveEntity( const CEntity* e );
	void OnAddEntityToScene( CEntity* e );

	void AddEntityToActorList( CEntity* e );

private:
	void OnRec( wxCommandEvent& event );
	void OnPrevFrame( wxCommandEvent& event );
	void OnNextFrame( wxCommandEvent& event );
	void OnAddTag( wxCommandEvent& event );
	void OnAddRange( wxCommandEvent& event );
	void OnActorSelected( wxCommandEvent& event );
};
