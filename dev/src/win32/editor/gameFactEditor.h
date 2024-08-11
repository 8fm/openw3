/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/factsDB.h"


class CEdGameFact : public wxDialog
{
	DECLARE_EVENT_TABLE();

private:
	enum
	{
		EXPFLAG_NEVER = 0,
		EXPFLAG_ACT_END,
		EXPFLAG_TIME
	};

private:
	CFactsDB::Fact* m_fact;
	Bool m_edit;

	wxSpinCtrl* m_value;
	wxSpinCtrl* m_second;
	wxRadioButton* m_expireNeverFlag;
	wxRadioButton* m_expireWhenActEnds;
	wxRadioButton* m_expireAfterTime;
	wxSpinCtrl* m_expirationTimeValue;
	wxButton* m_okButton;
	wxButton* m_cancelButton;

public:
	CEdGameFact( wxWindow* parent );

	void Attach( CFactsDB::Fact& fact );
	void Attach( const CFactsDB::Fact& fact );

protected:
	// ------------------------------------------------------------------------
	// window events handlers
	// ------------------------------------------------------------------------
	void OnValueChanged ( wxCommandEvent& event );
	void OnTimeChanged ( wxCommandEvent& event );
	void OnExpTimeChanged ( wxCommandEvent& event );
	void OnOK ( wxCommandEvent& event );
	void OnCancel ( wxCommandEvent& event );

private:
	void RefreshViews();
	void EnableExpTimeValue();
};
