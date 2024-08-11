/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "wizardTemplate.h"
#include "editorWizardDefinition.h"

const wxString CEdWizardTemplate::TEXT_CTRL_STR		= wxT("text" );
const wxString CEdWizardTemplate::LIST_BOX_CTRL_STR = wxT("listBox" );
const wxString CEdWizardTemplate::EDITABLE_TEXT_STR	= wxT("editableText");

BEGIN_EVENT_TABLE( CEdWizardTemplate, wxWizard )
	EVT_WIZARD_FINISHED( XRCID("OnWizardFinished"), CEdWizardTemplate::OnFinished )
	EVT_WIZARD_CANCEL( XRCID("OnWizardCancel"), CEdWizardTemplate::OnCancel )
	EVT_WIZARD_PAGE_CHANGING( XRCID("OnWizardPageChanging"), CEdWizardTemplate::OnPageChanging )
	EVT_WIZARD_PAGE_CHANGED( XRCID("OnWizardPageChanged"), CEdWizardTemplate::OnPageChanged )
	END_EVENT_TABLE()

	IMPLEMENT_CLASS( CEdWizardTemplate, wxWizard );

CEdWizardTemplate::CEdWizardTemplate( wxWindow* parent, wxString templateName, CWizardDefinition* definition, CObject* arg, CEdWizardSavedAnswers *const wizardSavedAnswers )
	: m_definition( definition )	
	, m_loaded( false )
	, m_pageChanging( false )
	, m_finished( false )
	, m_wasCanceled( false )
	, m_argument( arg )
	, m_wizardSavedAnswers( wizardSavedAnswers )
{
	m_loaded = wxXmlResource::Get()->LoadObject( this, parent, templateName, "wxWizard" );
	if( m_loaded )
	{
		this->Connect( wxEVT_WIZARD_FINISHED, wxWizardEventHandler( CEdWizardTemplate::OnFinished ), NULL, this );
		this->Connect( wxEVT_WIZARD_CANCEL, wxWizardEventHandler( CEdWizardTemplate::OnCancel ), NULL, this );
		this->Connect( wxEVT_WIZARD_PAGE_CHANGING, wxWizardEventHandler( CEdWizardTemplate::OnPageChanging ), NULL, this );
		this->Connect( wxEVT_WIZARD_PAGE_CHANGED, wxWizardEventHandler( CEdWizardTemplate::OnPageChanged ), NULL, this );

		Initialize( );
	}
}

wxWizardPageSimple* CEdWizardTemplate::GetFirstPage() const
{
	if( m_pages.Size() > 0 )
	{
		return m_pages.Begin()->m_page;
	}
	
	return NULL;
}

template< class T >
T* CEdWizardTemplate::GetPageElement( wxWizardPageSimple* page, wxString name )
{
	if( page )
	{
		wxWindow* element = page->FindWindowByName( name, page );
		if( element )
		{
			T* convElement = static_cast< T* >( element );
			if( convElement)
			{
				return convElement;
			}
		}
	}

	return NULL;
}

void CEdWizardTemplate::GenerateWizardControls( CWizardBaseNode* node )
{
	if( node )
	{
		Bool progress = true;
		if( node->IsA< CWizardQuestionNode >() )
		{
			CWizardQuestionNode* questionNode	= static_cast< CWizardQuestionNode* >( node );	
			wxString name						= questionNode->GetTemplateName().AsChar();
			wxWizardPageSimple* templatePage	= XRCCTRL( *this, name, wxWizardPageSimple );

			wxWizardPageSimple* newPage = new wxWizardPageSimple( this );
			if( templatePage )
			{
				newPage->SetSize( templatePage->GetSize() );
				newPage->SetPosition( templatePage->GetPosition() );
			}

			wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
			
			if( node->IsA< CWizardNode >() )
			{
				// Create wizard page
				CWizardNode* pageNode = static_cast< CWizardNode* >( node );	
				wxStaticText* templateText = NULL;
				wxListBox* templateListBox = NULL;
			
				if( templatePage )
				{
					templateText = GetPageElement<wxStaticText>( templatePage, TEXT_CTRL_STR );
					templateListBox = GetPageElement<wxListBox>( templatePage, LIST_BOX_CTRL_STR );
				}

				sizer->Add( new wxStaticText( newPage, wxID_ANY, pageNode->GetText().AsChar(), templateText ? templateText->GetPosition() : wxDefaultPosition, wxDefaultSize, wxALL, TEXT_CTRL_STR ) );
				sizer->Add( new wxStaticLine( newPage )  );
			
				wxListBox* listBox = new wxListBox(newPage, wxID_ANY, templateListBox ? templateListBox->GetPosition() : wxDefaultPosition, templateListBox ? templateListBox->GetSize() : wxDefaultSize );
				listBox->SetName( LIST_BOX_CTRL_STR);
				Int32 selectIndex = 0;
				for( Uint32 i = 0; i < node->GetNumChildren(); ++i )
				{
					CWizardOption* option = static_cast< CWizardOption* >( node->GetChild( i ) );
					if( option )
					{
						Int32 index = listBox->Append( option->GetValueName().AsChar() );
						if ( m_wizardSavedAnswers )
						{
							String savedAnswer;
							if ( m_wizardSavedAnswers->FindAnswer( questionNode->GetUniqueName(), savedAnswer ) )
							{	
								if ( savedAnswer == option->GetValueName() )
								{
									selectIndex = index;
								}
							}
						}
					}
				}
				listBox->Select( selectIndex );
				sizer->Add( listBox, wxALL );
			}
			else if( node->IsA< CWizardCNameQuestionNode >() )
			{
				// Create wizard page
				CWizardCNameQuestionNode* pageNode = static_cast< CWizardCNameQuestionNode* >( node );	
				wxStaticText* templateText			= NULL;
				wxTextCtrl* templateEditableText	= NULL;
			
				if( templatePage )
				{
					templateText			= GetPageElement< wxStaticText >( templatePage, TEXT_CTRL_STR );
					templateEditableText	= GetPageElement <wxTextCtrl >( templatePage, EDITABLE_TEXT_STR );
				}
	
				sizer->Add( new wxStaticText( newPage, wxID_ANY, pageNode->GetText().AsChar(), templateText ? templateText->GetPosition() : wxDefaultPosition, wxDefaultSize, wxALL, TEXT_CTRL_STR ) );
				sizer->Add( new wxStaticLine( newPage )  );
			
				wxTextCtrl* editableText = new wxTextCtrl( newPage, wxID_ANY, wxEmptyString, templateEditableText ? templateEditableText->GetPosition() : wxDefaultPosition, templateEditableText ? templateEditableText->GetSize() : wxDefaultSize );
				editableText->SetName( EDITABLE_TEXT_STR);

				String defaultAnswer( TXT("generic_horse") );
				String savedAnswer( TXT("generic_horse") );
				if ( m_wizardSavedAnswers->FindAnswer( questionNode->GetUniqueName(), savedAnswer ) )
				{	
					defaultAnswer = savedAnswer;
				}

				editableText->SetValue( defaultAnswer.AsChar() );
				sizer->Add( editableText, wxALL );
			}
			newPage->SetAutoLayout(true);
			newPage->SetSizerAndFit( sizer );
	
			sizer->Fit( this );
			sizer->SetSizeHints( this );
			newPage->Layout();
			progress &= AddPage( newPage, questionNode ) != NULL;
		}

		if( progress )
		{
			// Iterate over children
			for( Uint32 i = 0; i < node->GetNumChildren(); ++i )
			{
				GenerateWizardControls( node->GetChild( i ) );
			}
		}
	}
}

Bool CEdWizardTemplate::AddPage( wxWizardPageSimple* page, CWizardQuestionNode* node )
{
	if( page && node )
	{
		m_pages.PushBack( SWizardPage( page, node) );
		return true;
	}

	return false;
}

void CEdWizardTemplate::Initialize( )
{
	ASSERT( m_definition );
	if( m_definition )
	{
		m_finished		= false;
		m_wasCanceled	= false;

		// Create page instances and controls based on wizard definition
		for( Uint32 i = 0; i < m_definition->GetNodes().Size(); ++i )
		{
			GenerateWizardControls( m_definition->GetNodes()[i] );
		}

		// Chain created pages
		if(m_pages.Size() > 0 )
		{
			for( Uint32 i = 1; i < m_pages.Size(); ++i )
			{
				wxWizardPageSimple::Chain( m_pages[i-1].m_page, m_pages[i].m_page );
			}
		}

		Layout();
	}
}

void CEdWizardTemplate::Commit( )
{
	if ( m_wizardSavedAnswers )
	{
		m_wizardSavedAnswers->PostCommit();
	}
	for( Uint32 i = 0; i < m_pages.Size(); ++i )
	{
		CommitPage( &m_pages[i] );
	}
	if ( m_wizardSavedAnswers )
	{
		m_wizardSavedAnswers->Prune();
	}
}

void CEdWizardTemplate::CommitPage( SWizardPage* wizardPage )
{
	CWizardQuestionNode* node  = wizardPage->m_node;
	if( node )
	{
		// Establish selection
		String answer		= GetSelectedValue( node );
		CName &questionName = node->GetUniqueName();
		if( answer.Empty() == false )
		{
			// Commit if page was answered
			if ( wizardPage->m_wasAnswered )
			{
				if ( node->IsA< CWizardNode >() )
				{
					for( Uint32 i = 0; i < node->GetNumChildren(); ++i )
					{
						// Call commit on it
						CWizardOption* option = static_cast< CWizardOption* >( node->GetChild( i ) );
						if( answer == option->GetValueName() )
						{
							CWizardOptionData* data = option->GetValue();
							if( data ) 
							{
								data->CommitData( m_argument );
							}
							break;
						}
					}
				}
				else if ( node->IsA< CWizardCNameQuestionNode >() )
				{
					CWizardCNameQuestionNode *const nameQuestionNode	= static_cast< CWizardCNameQuestionNode* >( node );
					CCustomParamWizardData *const customParamWizardData = nameQuestionNode->GetValue();
					if ( customParamWizardData )
					{
						customParamWizardData->SetValue( answer );
						customParamWizardData->CommitData( m_argument );
					}
				}
			}
			
			if ( wizardPage->m_wasAnswered || m_wasCanceled )
			{
				m_wizardSavedAnswers->AddAnswer( questionName, answer );
			}
		}
	}
}

CEdWizardTemplate::SWizardPage* CEdWizardTemplate::FindWizardPage( wxWizardPageSimple* wxPage )
{
	for( Uint32 i = 0; i < m_pages.Size(); ++i )
	{
		if( m_pages[i].m_page == wxPage )
		{
			return &m_pages[i];
		}
	}

	return NULL;
}

CEdWizardTemplate::SWizardPage* CEdWizardTemplate::FindWizardPage( CWizardQuestionNode* node )
{
	for( Uint32 i = 0; i < m_pages.Size(); ++i )
	{
		if( m_pages[i].m_node == node )
		{
			return &m_pages[i];
		}
	}

	return NULL;
}


wxString CEdWizardTemplate::GetSelectedValue( CWizardQuestionNode* node)
{
	SWizardPage* page = FindWizardPage( node );
	if( page )
	{
		if ( node->IsA< CWizardNode >() )
		{
			wxListBox* listBox = GetPageElement<wxListBox>( page->m_page, LIST_BOX_CTRL_STR );
			if( listBox )
			{
				Uint32 i = listBox->GetSelection();
				if( i >= 0 && i < listBox->GetCount() )
				{
					return listBox->GetString( listBox->GetSelection() );
				}
			}
		}
		else if ( node->IsA< CWizardCNameQuestionNode >() )
		{
			wxTextCtrl* textCtrl = GetPageElement< wxTextCtrl >( page->m_page, EDITABLE_TEXT_STR );
			if( textCtrl )
			{
				return textCtrl->GetLineText( 0 );
			}
		}
	}

	return wxEmptyString;
}

void CEdWizardTemplate::OnPageChanged( wxWizardEvent& event )
{
	m_pageChanging = false;
}

CWizardQuestionNode* CEdWizardTemplate::GetNextNodeFromOption( CWizardOption* option )
{
	CWizardQuestionNode* nextNode = NULL;
	if( option && option->GetNumChildren() > 0 )
	{	
		for( Uint32 i = 0; i < option->GetNumChildren() && !nextNode; ++i )
		{
			nextNode = static_cast< CWizardQuestionNode* >( option->GetChild( i ) );
			for( Int32 j = m_progress.Size()-1; j >= 0; --j )
			{
				// Already used it, try next one
				if( m_progress[j].m_node == nextNode )
				{
					nextNode = NULL;
					break;
				}
			}
		}	
	}

	return nextNode;
}

CEdWizardTemplate::SWizardPage* CEdWizardTemplate::GetNextPageFromOption( CWizardOption* option )
{
	if( option )
	{
		CWizardQuestionNode* nextNode = NULL;

		// Subnodes
		if( option->GetNumChildren() > 0 )
		{		
			nextNode = GetNextNodeFromOption( option );		
			if( nextNode )
			{
				return FindWizardPage( nextNode ); 
			}
		}

		// End of node chain, track back progress
		if( !nextNode && m_progress.Size() > 0 )
		{
			for( Int32 j = m_progress.Size()-1; j >= 0 && !nextNode; --j )
			{
				CWizardOption* option = GetSelectedOption( m_progress[j].m_node );
				if( option )
				{
					nextNode = GetNextNodeFromOption( option );
				}
			}

			return FindWizardPage( nextNode );
		}
	}

	return NULL;
}

CWizardOption* CEdWizardTemplate::GetSelectedOption( CWizardQuestionNode* node )
{
	if( node )
	{
		wxString selectedValue = GetSelectedValue( node );
		for( Uint32 i = 0; i < node->GetNumChildren(); ++i )
		{
			CWizardOption* option = static_cast< CWizardOption* > ( node->GetChild( i ) );
			if( selectedValue.CompareTo( option->GetValueName().AsChar() ) == 0)
			{
				return option;
			}
		}
	}

	return NULL;
}

Bool CEdWizardTemplate::HasNextPage( wxWizardPage* page )
{
	SWizardPage* currPage = FindWizardPage( static_cast< wxWizardPageSimple* >( page ) );
	if( currPage )
	{
		// Last top node?
		if( currPage->m_node == m_definition->GetNodes()[m_definition->GetNodes().Size()-1] )
		{
			return false;
		}

		// End node?
		return !currPage->m_node->IsEndNode();
	}

	return false;
}

void CEdWizardTemplate::OnPageChanging( wxWizardEvent& event )
{
	if( m_pageChanging)
	{
		return;
	}

	wxWizardPageSimple* page	= static_cast< wxWizardPageSimple* >( GetCurrentPage() );
	SWizardPage* wizPage		= FindWizardPage( page );
	CWizardQuestionNode* node	= wizPage->m_node;	

	// First validate page as answered
	wizPage->m_wasAnswered = true;

	// Next page choice (based on node structuring)
	if( event.GetDirection() )
	{
		// Next

		// Skipping check
		if( node && page )
		{
			if( !node->IsOptional() )
			{
				wxString selectedValue = GetSelectedValue( node );
				if ( selectedValue.IsEmpty() )
				{
					wxMessageBox( wxT("This choice is required for the AI wizard to proceed."), wxT("Error") );
					event.Veto();
					return;
				}
			}
		}

		// Ending check
		if( node->IsEndNode() )
		{
			event.Veto();
			m_pageChanging = true;
			wxWizardEvent endEvent(wxEVT_CLOSE_WINDOW, wxID_ANY, false, GetCurrentPage());
			OnFinished( endEvent );
			Close();
			return;
		}

		// Continue to selected option page
		SWizardPage* nextPage = nullptr;
		if ( node->IsA< CWizardNode >() )
		{
			CWizardOption* option = GetSelectedOption( node );
			if( option == nullptr )
			{
				return;
			}
			m_progress.PushBack( *wizPage );
			nextPage = GetNextPageFromOption( option );	
		}
		else if ( node->IsA< CWizardCNameQuestionNode >() )
		{
			m_progress.PushBack( *wizPage );
		}
	
		
		// Last resort, use top level node list
		if( !nextPage )
		{
			for( Uint32 i = 0; i < m_definition->GetNodes().Size(); ++i )
			{
				Bool alreadyUsed = false;
				for( Uint32 j = 0; j < m_progress.Size(); ++j )
				{
					if( m_progress[j].m_node == m_definition->GetNodes()[i] )
					{
						alreadyUsed = true;
						break;
					}
				}

				if( !alreadyUsed )
				{
					nextPage = FindWizardPage( m_definition->GetNodes()[i] );
					break;
				}
			}
		}

		if( nextPage )
		{
			event.Veto();
			m_pageChanging = true;
			ShowPage( nextPage->m_page );
		}
	}
	else
	{
		// Previous button was hit, go back to parent node
		event.Veto();
		m_pageChanging = true;
		ShowPage( m_progress[m_progress.Size()-1].m_page, false );
		m_progress.RemoveAt( m_progress.Size()-1 );
	}
}
	


void CEdWizardTemplate::OnFinished( wxWizardEvent& event )
{
	m_finished = true;
}

void CEdWizardTemplate::OnCancel( wxWizardEvent& event )
{
	if ( m_finished == false ) // hack because sometimes canceled is called when finish is click ( wxwidget bug ? )
	{
		// Invalidating all nodes
		for( Uint32 i = 0; i < m_pages.Size(); ++i )
		{
			m_pages[i].m_wasAnswered = false;
		}

		m_wasCanceled = true;
	}
}