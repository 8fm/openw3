
#include "build.h"
#include "voiceEditor.h"

BEGIN_EVENT_TABLE( CEdVoiceDialog, wxDialog )
	EVT_CHAR_HOOK( CEdVoiceDialog::OnKeyDown )
END_EVENT_TABLE()

const Float CEdVoiceDialog::WAITING_DURATION = 3.f;
const Float CEdVoiceDialog::REC_MAX_TIME = 60.f;

CEdVoiceDialog::CEdVoiceDialog( wxWindow* parent, const String& stringId, const String& langId, const String& text )
	: m_timer( this, wxID_ANY )
	, m_state( S_Waiting )
	, m_timeAcc( 0.f )
	, m_voice( NULL )
	, m_stringId( stringId )
	, m_langId( langId )
	, m_text( text )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("VoiceDialog") );

	m_waitText = XRCCTRL( *this, "startText", wxStaticText );
	m_recText = XRCCTRL( *this, "recText", wxStaticText );
	m_commentText = XRCCTRL( *this, "commentText", wxStaticText );

	Connect( m_timer.GetId(), wxEVT_TIMER, wxTimerEventHandler( CEdVoiceDialog::OnInternalTimer ) );

	m_voice = new VoiceRecording();

	if ( !m_voice->PrepareBuffer( REC_MAX_TIME + 10.f ) )
	{
		wxMessageBox( wxT("PrepareBuffer"), wxT("Error") );
		EndDialog( 0 );
	}

	if ( !m_voice->Open() )
	{
		wxMessageBox( wxT("Open"), wxT("Error") );
		EndDialog( 0 );
	}

	m_timer.Start( 100, false );

	UpdateWaitText( 0.f );
	UpdateRecText( 0.f );
}

CEdVoiceDialog::~CEdVoiceDialog()
{
	if ( m_voice )
	{
		delete m_voice;
		m_voice = NULL;
	}
}

void CEdVoiceDialog::DoModal()
{
	
}

void CEdVoiceDialog::OnKeyDown( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		EndDialog( 0 );
	}
	
	if ( m_state == S_Recording )
	{
		StopRec();
	}
}

void CEdVoiceDialog::OnInternalTimer( wxTimerEvent& event )
{
	Int32 interval = event.GetInterval();
	m_timeAcc += interval / 1000.f;

	if ( m_state == S_Waiting )
	{
		UpdateWaitText( m_timeAcc );

		if ( m_timeAcc > WAITING_DURATION )
		{
			StartRec();
		}
	}
	else if ( m_state == S_Recording )
	{
		UpdateRecText( m_timeAcc );

		if ( m_timeAcc > REC_MAX_TIME )
		{
			StopRec();
		}
	}
}

void CEdVoiceDialog::StartRec()
{
	ASSERT( m_state == S_Waiting );

	m_timeAcc = 0.f;

	if ( !m_voice->Record() )
	{
		wxMessageBox( wxT("Record"), wxT("Error") );
		EndDialog( 0 );
	}

	m_state = S_Recording;

	UpdateCommentText( m_state );
}

void CEdVoiceDialog::StopRec()
{
	ASSERT( m_state == S_Recording );
	ASSERT( m_voice );

	m_timer.Stop();

	m_voice->Stop();

	String path = SEdLipsyncCreator::GetInstance().GetWavPath( m_stringId, m_langId );
	if ( path.Empty() )
	{
		wxMessageBox( wxT("Lipsync dll"), wxT("Error") );
		EndDialog( 0 );
	}

	if ( !m_voice->Save( path ) )
	{
		wxMessageBox( wxT("Save"), wxT("Error") );
		EndDialog( 0 );
	}

	CreateLipsync();
}

void CEdVoiceDialog::CreateLipsync()
{
	ASSERT( m_state == S_Recording );

	m_state = S_CreatingLipsync;

	Bool ret = SEdLipsyncCreator::GetInstance().CreateLipsync( m_stringId, m_langId, m_text );

	if ( ret )
	{
		EndDialog( 1 );
	}
	else
	{
		EndDialog( 0 );
	}

	UpdateCommentText( m_state );
}

void CEdVoiceDialog::UpdateWaitText( Float time )
{
	time = Max( WAITING_DURATION - time, 0.f );
	String str = String::Printf( TXT("Start: %2.3f sec"), time );
	m_waitText->SetLabelText( str.AsChar() );
}

void CEdVoiceDialog::UpdateRecText( Float time )
{
	String str = String::Printf( TXT("Rec time: %2.3f sec"), time );
	m_recText->SetLabelText( str.AsChar() );
}

void CEdVoiceDialog::UpdateCommentText( EVDState state )
{
	wxString text;

	if ( m_state == S_Waiting )
	{
		text = wxT("   Waiting...");
	}
	else if ( m_state == S_Recording )
	{
		text = wxT("   Recording...");
	}
	else if ( m_state == S_CreatingLipsync )
	{
		text = wxT("   Creating lipsync...");
	}

	m_commentText->SetLabelText( text );
}
