#include "build.h"
#include "gameFactEditor.h"


BEGIN_EVENT_TABLE( CEdGameFact, wxDialog )
END_EVENT_TABLE()

CEdGameFact::CEdGameFact( wxWindow* parent )
: m_fact(NULL)
, m_edit(false)
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "GameFactEditor" ) );

	m_value = XRCCTRL( *this, "value", wxSpinCtrl);
	m_value->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdGameFact::OnValueChanged ), NULL, this );

	m_second = XRCCTRL( *this, "timeSecond", wxSpinCtrl);
	m_second->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdGameFact::OnTimeChanged ), NULL, this );

	m_expireNeverFlag = XRCCTRL( *this, "expireNever", wxRadioButton);
	m_expireNeverFlag->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CEdGameFact::OnExpTimeChanged ), NULL, this );

	m_expireWhenActEnds = XRCCTRL( *this, "expireWhenActEnds", wxRadioButton);
	m_expireWhenActEnds->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CEdGameFact::OnExpTimeChanged ), NULL, this );

	m_expireAfterTime = XRCCTRL( *this, "expireAfterTime", wxRadioButton);
	m_expireAfterTime->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CEdGameFact::OnExpTimeChanged ), NULL, this );

	m_expirationTimeValue = XRCCTRL( *this, "expirationTimeValue", wxSpinCtrl);
	m_expirationTimeValue->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdGameFact::OnExpTimeChanged ), NULL, this );

	m_okButton = XRCCTRL( *this, "okButton", wxButton);
	m_okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdGameFact::OnOK ), NULL, this );

	m_cancelButton = XRCCTRL( *this, "cancelButton", wxButton);
	m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdGameFact::OnCancel ), NULL, this );
}

void CEdGameFact::OnOK ( wxCommandEvent& event )
{
	EndDialog( wxID_OK ); 
}

void CEdGameFact::OnCancel ( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL ); 
}

void CEdGameFact::OnValueChanged ( wxCommandEvent& event )
{
	if (m_fact == NULL)
	{
		return;
	}
	m_fact->m_value = m_value->GetValue();
}

void CEdGameFact::OnTimeChanged ( wxCommandEvent& event )
{
	if (m_fact == NULL)
	{
		return;
	}

	Int32 second = m_second->GetValue();

	m_fact->m_time = (Float) second;
}

void CEdGameFact::OnExpTimeChanged ( wxCommandEvent& event )
{
	if (m_fact == NULL)
	{
		return;
	}

	if ( m_expireNeverFlag->GetValue() == true )
	{
		m_fact->m_expiryTime = CFactsDB::EXP_NEVER;
		
	}
	else if ( m_expireWhenActEnds->GetValue() == true )
	{
		m_fact->m_expiryTime = CFactsDB::EXP_ACT_END;
		
	}
	else if ( m_expireAfterTime->GetValue() == true )
	{
		m_fact->m_expiryTime = (Double)m_expirationTimeValue->GetValue();
	}

	EnableExpTimeValue();
}

void CEdGameFact::Attach( CFactsDB::Fact& fact )
{
	m_fact = &fact;
	m_edit = true;

	RefreshViews();
}

void CEdGameFact::Attach( const CFactsDB::Fact& fact )
{
	m_fact = const_cast< CFactsDB::Fact* > ( &fact );
	m_edit = false;

	RefreshViews();
}

void CEdGameFact::RefreshViews()
{
	if (m_fact == NULL)
	{
		m_value->SetValue( 0 );
		m_second->SetValue( 0 );
		m_expireNeverFlag->SetValue( true );
		m_expirationTimeValue->SetValue( 0 );
	}
	else
	{
		m_value->SetValue( m_fact->m_value );

		Int32 time = (Float) m_fact->m_time;
		m_second->SetValue( time );

		if (m_fact->m_expiryTime == CFactsDB::EXP_NEVER)
		{
			m_expireNeverFlag->SetValue( true );
			m_expirationTimeValue->SetValue( 0 );
		}
		else if (m_fact->m_expiryTime == CFactsDB::EXP_ACT_END)
		{
			m_expireWhenActEnds->SetValue( true );
			m_expirationTimeValue->SetValue( 0 );
		}
		else
		{
			m_expireAfterTime->SetValue( true );

			Int32 expiryTime = (Float) ( m_fact->m_expiryTime - m_fact->m_time );

			m_expirationTimeValue->SetValue( expiryTime );
		}
		EnableExpTimeValue();
	}

	// enable / disable edition
	m_value->Enable( m_edit );
	m_second->Enable( m_edit );
	m_expireNeverFlag->Enable( m_edit );
	m_expireWhenActEnds->Enable( m_edit );
	m_expireAfterTime->Enable( m_edit );
	EnableExpTimeValue();

	m_cancelButton->Show( m_edit );
}

void CEdGameFact::EnableExpTimeValue()
{
	if (m_expireAfterTime->GetValue() == true)
	{
		m_expirationTimeValue->Enable(m_edit);
	}
	else
	{
		m_expirationTimeValue->Enable(false);
	}
}
