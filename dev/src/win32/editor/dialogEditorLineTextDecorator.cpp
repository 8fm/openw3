#include "build.h"
#include "dialogEditorLineTextDecorator.h"

CEdStorySceneLineTextDecorator::CEdStorySceneLineTextDecorator( wxTextCtrl* textField )
	: m_textField( textField )
{
	m_textField->Connect( wxEVT_COMMAND_TEXT_UPDATED, 
		wxCommandEventHandler( CEdStorySceneLineTextDecorator::OnValueChanged ), NULL, this );
	m_textField->Connect( wxEVT_SET_FOCUS, 
		wxFocusEventHandler( CEdStorySceneLineTextDecorator::OnFocus ), NULL, this );
}

CEdStorySceneLineTextDecorator::~CEdStorySceneLineTextDecorator()
{
	m_textField->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, 
		wxCommandEventHandler( CEdStorySceneLineTextDecorator::OnValueChanged ), NULL, this );
	m_textField->Disconnect( wxEVT_SET_FOCUS, 
		wxFocusEventHandler( CEdStorySceneLineTextDecorator::OnFocus ), NULL, this );
}

void CEdStorySceneLineTextDecorator::ChangeValue( wxString text )
{
	long caretPosition = m_textField->GetInsertionPoint();

	Bool isBracketAppended = false;

	if ( m_leftBracket.IsEmpty() == false && text.StartsWith( m_leftBracket ) == false )
	{
		text = m_leftBracket + text;
		caretPosition += 1;
		isBracketAppended = true;
	}

	if ( m_rightBracket.IsEmpty() == false && text.EndsWith( m_rightBracket ) == false )
	{
		text = text + m_rightBracket;
		isBracketAppended = true;
	}

	if ( isBracketAppended == true )
	{
		m_textField->ChangeValue( text );
		m_textField->SetInsertionPoint( caretPosition );
	}
}

wxString CEdStorySceneLineTextDecorator::GetValue()
{
	wxString text = m_textField->GetValue();
	if ( text.StartsWith( m_leftBracket ) == true && text.EndsWith( m_rightBracket ) == true )
	{
		text = text.Mid( m_leftBracket.Length(), 
			text.Length() - m_leftBracket.Length() - m_rightBracket.Length() );
	}
	return text;
}

void CEdStorySceneLineTextDecorator::SetBrackets( const wxString leftBracket, const wxString rightBracket )
{
	m_leftBracket = leftBracket;
	m_rightBracket = rightBracket;
}

void CEdStorySceneLineTextDecorator::OnValueChanged( wxCommandEvent& event )
{
	ChangeValue( m_textField->GetValue() );
}

void CEdStorySceneLineTextDecorator::OnFocus( wxFocusEvent& event )
{
	if ( m_textField->GetValue().EndsWith( m_rightBracket ) )
	{
		m_textField->SetInsertionPoint( m_textField->GetLastPosition() - m_rightBracket.Length() );
	}
}