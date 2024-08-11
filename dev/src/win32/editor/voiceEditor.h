
#pragma once

#include "voice.h"

class CEdVoiceDialog: public wxDialog
{
	enum EVDState
	{
		S_Waiting,
		S_Recording,
		S_CreatingLipsync
	};

	DECLARE_EVENT_TABLE();

	String			m_stringId;
	String			m_langId;
	String			m_text;

	EVDState		m_state;
	wxTimer			m_timer;
	Float			m_timeAcc;

	wxStaticText*	m_waitText;
	wxStaticText*	m_recText;
	wxStaticText*	m_commentText;

	VoiceRecording*	m_voice;

	static const Float WAITING_DURATION;
	static const Float REC_MAX_TIME;

public:
	CEdVoiceDialog( wxWindow* parent, const String& stringId, const String& langId, const String& text );
	~CEdVoiceDialog();

	void DoModal();

protected:
	void OnKeyDown( wxKeyEvent& event );
	void OnInternalTimer( wxTimerEvent& event );

private:
	void UpdateWaitText( Float time );
	void UpdateRecText( Float time );
	void UpdateCommentText( EVDState state );

	void StartRec();
	void StopRec();

	void CreateLipsync();
};
