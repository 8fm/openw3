#include "build.h"
#include "dialogEditorInjectedChoiceLine.h"
#include "dialogEditor.h"
#include "dialogEditorActions.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneSection.h"
#include "assetBrowser.h"
#include "textControl.h"
#include "../../common/core/diskFile.h"

wxIMPLEMENT_CLASS( CEdStorySceneInjectedChoiceLinePanel, wxPanel );

CEdStorySceneInjectedChoiceLinePanel::CEdStorySceneInjectedChoiceLinePanel
( 
	CEdStorySceneChoicePanel* choicePanel, 
	Uint32 lineIndex,
	const String& choiceLine,
	CStoryScene& hostScene
)
: m_lineIndex( lineIndex )
, m_choiceLine( choiceLine )
, m_choicePanel( choicePanel )
, m_choiceContentField( NULL )
, m_storySceneEditor( NULL )
, m_hostScene( hostScene )
{
	Create( choicePanel );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );

	wxBoxSizer* bSizer283;
	bSizer283 = new wxBoxSizer( wxHORIZONTAL );

	m_indexLabel = new wxStaticText( this, wxID_ANY, wxT("1)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_indexLabel->Wrap( -1 );
	m_indexLabel->SetFont( wxFont( 12, 70, 90, 90, false, wxT("Courier New") ) );

	bSizer283->Add( m_indexLabel, 0, wxLEFT, 5 );

	wxPanel* choiceTextPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer284;
	bSizer284 = new wxBoxSizer( wxVERTICAL );

	m_choiceFileField = new CEdTextControl( choiceTextPanel, wxID_ANY, wxT("(Comment)"), wxDefaultPosition, wxDefaultSize, 0|wxNO_BORDER );
	m_choiceFileField->SetFont( wxFont( 10, 70, 93, 90, false, wxT("Courier New") ) );
	m_choiceFileField->Hide();

	bSizer284->Add( m_choiceFileField, 0, wxEXPAND, 5 );

	m_choiceContentField = new CEdTextControl( choiceTextPanel, wxID_ANY, wxT("[ enter text here ]"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER|wxTE_RICH|wxNO_BORDER );
	m_choiceContentField->SetFont( wxFont( 10, 70, 90, 90, false, wxT("Courier New") ) );

	bSizer284->Add( m_choiceContentField, 0, wxEXPAND, 5 );

	wxPanel* m_postLinkPanel = new wxPanel( choiceTextPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_postLinkPanel->SetBackgroundColour( wxColour( 255, 255, 255 ) );

	wxBoxSizer* bSizer244;
	bSizer244 = new wxBoxSizer( wxVERTICAL );

	m_postLinkPanel->SetSizer( bSizer244 );
	m_postLinkPanel->Layout();
	bSizer244->Fit( m_postLinkPanel );
	bSizer284->Add( m_postLinkPanel, 0, wxEXPAND, 5 );

	choiceTextPanel->SetSizer( bSizer284 );
	choiceTextPanel->Layout();
	bSizer284->Fit( choiceTextPanel );
	bSizer283->Add( choiceTextPanel, 1, wxEXPAND|wxRIGHT, 5 );

	this->SetSizer( bSizer283 );
	this->Layout();
	bSizer283->Fit( this );

	//////////////////////////////////////////////////////////////////////////

	//wxXmlResource::Get()->LoadPanel( this, choicePanel, TEXT( "DialogEditorChoiceLine" ) );

	//m_indexLabel = XRCCTRL( *this, "LineNumberLabel", wxStaticText );
	//m_choiceFileField = XRCCTRL( *this, "ChoiceCommentField", wxTextCtrl );
	//m_choiceContentField = XRCCTRL( *this, "ChoiceContentField", wxTextCtrl );
	m_choiceFileField->Show();

	m_storySceneEditor = m_choicePanel->GetSectionPanel()->GetStorySceneEditor();

	CEdStorySceneHandlerFactory* handlerFactory = m_storySceneEditor->GetHandlerFactory();

	handlerFactory->CreateCaretOnFocusHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateArrowTraverseHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateScrollOnFocusHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateAutoExpandHandler()->ConnectTo( m_choiceContentField );
	handlerFactory->CreateManualMouseScrollHandler()->ConnectTo( m_choiceContentField );

	m_choiceFileField->Connect( wxEVT_LEFT_DCLICK, 
		wxMouseEventHandler( CEdStorySceneInjectedChoiceLinePanel::OnDoubleClick ), NULL, this );
	m_choiceContentField->Connect( wxEVT_LEFT_DCLICK, 
		wxMouseEventHandler( CEdStorySceneInjectedChoiceLinePanel::OnDoubleClick ), NULL, this );

	ChangeFontSize( m_storySceneEditor->GetFontSize() );
	m_choiceContentField->SetEditable( false );
	m_choiceFileField->SetEditable( false );
	RefreshData();
	SetBackgroundColour( wxColour( 255, 182, 219 ) );
}

CEdStorySceneInjectedChoiceLinePanel::~CEdStorySceneInjectedChoiceLinePanel()
{
}

void CEdStorySceneInjectedChoiceLinePanel::RefreshData()
{
	m_indexLabel->SetLabel( wxString::Format( wxT( "%d)" ), m_lineIndex ) );
	m_choiceFileField->SetLabel( m_hostScene.GetFile()->GetDepotPath().AsChar() );
	m_choiceContentField->ChangeValue( m_choiceLine.AsChar() );
}

void CEdStorySceneInjectedChoiceLinePanel::ChangeFontSize( Int32 sizeChange )
{
	m_storySceneEditor->ChangeWindowFontSize( m_choiceContentField, sizeChange );
	m_storySceneEditor->ChangeWindowFontSize( m_choiceFileField, sizeChange );
	m_storySceneEditor->ChangeWindowFontSize( m_indexLabel, sizeChange );
}

bool CEdStorySceneInjectedChoiceLinePanel::SetBackgroundColour( const wxColour& colour )
{
	if ( m_indexLabel )
	{
		m_indexLabel->SetBackgroundColour( colour );
	}

	if ( m_choiceFileField )
	{
		m_choiceFileField->SetForegroundColour( wxColour( 145, 104, 125 ) );
		m_choiceFileField->SetBackgroundColour( colour );
	}

	if ( m_choiceContentField )
	{
		m_choiceContentField->SetBackgroundColour( colour );
	}

	return wxPanel::SetBackgroundColour( colour );
}

void CEdStorySceneInjectedChoiceLinePanel::OnDoubleClick( wxMouseEvent& event )
{
	wxTheFrame->GetAssetBrowser()->OpenFile( m_hostScene.GetFile()->GetDepotPath() );
}

