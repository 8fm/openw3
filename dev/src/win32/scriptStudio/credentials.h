/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __SCRIPT_STUDIO_CREDENTIALS_H__
#define __SCRIPT_STUDIO_CREDENTIALS_H__

#include "../../common/redThreads/redThreadsAtomic.h"
#include "../../common/redNetwork/channel.h"

class CCredentialsChangedEvent : public wxEvent
{
public:
	CCredentialsChangedEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCredentialsChangedEvent( const map< wxString, wxString >& credentials );
	CCredentialsChangedEvent( const CCredentialsChangedEvent* other );

	inline virtual ~CCredentialsChangedEvent();

	inline const map< wxString, wxString >& GetCredentials() const { return *m_credentials; }

private:
	virtual wxEvent* Clone() const override final { return new CCredentialsChangedEvent( this ); }

private:
	const map< wxString, wxString >* m_credentials;
	Red::Threads::CAtomic< Red::System::Uint32 >* m_referenceCount;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCredentialsChangedEvent );
};

wxDECLARE_EVENT( ssEVT_CREDENTIALS_CHANGED_EVENT, CCredentialsChangedEvent );

//////////////////////////////////////////////////////////////////////////

class CSSVersionControlCredentialsDialog : public wxDialog 
{
	wxDECLARE_CLASS( CSSVersionControlCredentialsDialog );

public:
	CSSVersionControlCredentialsDialog( wxWindow* parent );
	virtual ~CSSVersionControlCredentialsDialog() override;

private:
	void OnCredentialsChanged( wxCommandEvent& event );
	void OnOK( wxCommandEvent& event );

private:
	enum EColumns
	{
		Col_Label = 0,
		Col_TextField,
		
		Col_Max
	};

	map< int, wxString > m_wxIdToCredentialKey;
	map< wxString, wxString > m_keyToValue;
};

#endif // __SCRIPT_STUDIO_CREDENTIALS_H__
