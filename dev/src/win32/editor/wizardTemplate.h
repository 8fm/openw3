/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CWizardQuestionNode;

class CEdWizardTemplate : public wxWizard
{
	static const wxString TEXT_CTRL_STR;
	static const wxString LIST_BOX_CTRL_STR;
	static const wxString EDITABLE_TEXT_STR;

protected:
	struct SWizardPage
	{
		SWizardPage()
			: m_page( NULL )
			, m_node( NULL )
			, m_wasAnswered( false )
		{}

		SWizardPage( wxWizardPageSimple* page, CWizardQuestionNode* node)
			: m_page( page )
			, m_node( node )
			, m_wasAnswered( false )
		{}

		wxWizardPageSimple*		m_page;
		CWizardQuestionNode*	m_node;
		Bool					m_wasAnswered;
	};

	typedef TDynArray< SWizardPage > TPageList;

	DECLARE_EVENT_TABLE()
	DECLARE_CLASS( CEdWizardTemplate );

	CWizardDefinition*				m_definition;
	TPageList						m_pages;
	TPageList						m_progress;
	Bool							m_loaded;
	Bool							m_pageChanging;
	Bool							m_finished;
	Bool							m_wasCanceled;
	CObject*						m_argument;
	CEdWizardSavedAnswers *			m_wizardSavedAnswers;

	void Initialize( );

	void GenerateWizardControls( CWizardBaseNode* node );
	template< class T >
	T* GetPageElement( wxWizardPageSimple* page, wxString name );
	Bool AddPage( wxWizardPageSimple* page, CWizardQuestionNode* node );

	void CommitPage( SWizardPage* wizardPage );

	SWizardPage* GetNextPageFromOption( CWizardOption* option );
	CWizardQuestionNode* GetNextNodeFromOption( CWizardOption* option );
	SWizardPage* FindWizardPage( wxWizardPageSimple* wxPage );
	SWizardPage* FindWizardPage( CWizardQuestionNode* node );
	CWizardOption* GetSelectedOption( CWizardQuestionNode* node );

	wxString GetSelectedValue( CWizardQuestionNode* node);

	void OnCancel( wxWizardEvent& event );
	void OnFinished( wxWizardEvent& event );
	void OnPageChanging( wxWizardEvent& event );
	void OnPageChanged( wxWizardEvent& event );

public:
	CEdWizardTemplate( wxWindow* parent, wxString templateName, CWizardDefinition* definition, CObject* arg = nullptr, CEdWizardSavedAnswers *const wizardSavedAnswers = nullptr );
	
	void Commit();

	Bool IsLoaded() const { return m_loaded; }
	wxWizardPageSimple* GetFirstPage() const;
	CObject* GetArgument() const { return m_argument; }
	Bool HasNextPage( wxWizardPage* page ) override;

	Bool IsFinished() const { return m_finished; }
};
