/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __EDITOR_COMPILATION_HELPER_H__
#define __EDITOR_COMPILATION_HELPER_H__

#define RED_NET_CHANNEL_SCRIPT_COMPILER "ScriptCompiler"

#include "../../common/redNetwork/manager.h"

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////
class CCompilationStartedEvent : public wxEvent
{
public:
	CCompilationStartedEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCompilationStartedEvent( Red::System::Bool strictMode, Red::System::Bool finalBuild );
	virtual ~CCompilationStartedEvent();

	inline Red::System::Bool IsStrictModeEnabled() const { return m_strictMode; }
	inline Red::System::Bool IsFinalBuild() const { return m_finalBuild; }

private:
	virtual wxEvent* Clone() const override final { return new CCompilationStartedEvent( m_strictMode, m_finalBuild ); }

private:
	Red::System::Bool m_strictMode;
	Red::System::Bool m_finalBuild;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCompilationStartedEvent );
};

//////////////////////////////////////////////////////////////////////////

class CCompilationEndedEvent : public wxEvent
{
public:
	CCompilationEndedEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCompilationEndedEvent( Red::System::Bool success );
	virtual ~CCompilationEndedEvent();

	inline Red::System::Bool WasBuildSuccessful() const { return m_success; }

private:
	virtual wxEvent* Clone() const override final { return new CCompilationEndedEvent( m_success ); }

private:
	Red::System::Bool m_success;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCompilationEndedEvent );
};

//////////////////////////////////////////////////////////////////////////

class CCompilationLogEvent : public wxEvent
{
public:
	CCompilationLogEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCompilationLogEvent( const wxString& message );
	CCompilationLogEvent( const CCompilationLogEvent& event );

	virtual ~CCompilationLogEvent();

	inline const wxString& GetMessage() const { return m_message; }

private:
	virtual wxEvent* Clone() const override final { return new CCompilationLogEvent( *this ); }

private:
	wxString m_message;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCompilationLogEvent );
};

//////////////////////////////////////////////////////////////////////////

class CCompilationErrorEvent : public wxEvent
{
public:
	enum ESeverity
	{
		Severity_Warning = 0,
		Severity_Error
	};

public:
	CCompilationErrorEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCompilationErrorEvent( ESeverity severity, Red::System::Int32 line, const wxString& file, const wxString& message );
	CCompilationErrorEvent( const CCompilationErrorEvent& event );

	virtual ~CCompilationErrorEvent();

	inline ESeverity GetSeverity() const { return m_severity; }
	inline const wxString& GetMessage() const { return m_message; }
	inline const wxString& GetFile() const { return m_file; }
	inline Red::System::Int32 GetLine() const { return m_line; }

private:
	virtual wxEvent* Clone() const override final { return new CCompilationErrorEvent( *this ); }

private:
	ESeverity m_severity;
	Red::System::Int32 m_line;
	wxString m_message;
	wxString m_file;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCompilationErrorEvent );
};

//////////////////////////////////////////////////////////////////////////

class CPathConfirmationEvent : public wxEvent
{
public:
	CPathConfirmationEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CPathConfirmationEvent( const wxString& path );
	CPathConfirmationEvent( const CPathConfirmationEvent& event );

	virtual ~CPathConfirmationEvent();

	inline const wxString& GetPath() const { return m_path; }

private:
	virtual wxEvent* Clone() const override final { return new CPathConfirmationEvent( *this ); }

private:
	wxString m_path;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CPathConfirmationEvent );
};

//////////////////////////////////////////////////////////////////////////

class CPackageSyncListingEvent : public wxEvent
{
public:
	CPackageSyncListingEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CPackageSyncListingEvent( const map< wxString, wxString >& packages );
	CPackageSyncListingEvent( map< wxString, wxString >&& packages );

	virtual ~CPackageSyncListingEvent();

	inline const map< wxString, wxString >& GetPath() const { return m_map; }

private:
	virtual wxEvent* Clone() const override final { return new CPackageSyncListingEvent( m_map ); }

private:
	map< wxString, wxString > m_map;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CPackageSyncListingEvent );
};

//////////////////////////////////////////////////////////////////////////

wxDECLARE_EVENT( ssEVT_COMPILATION_STARTED_EVENT, CCompilationStartedEvent );
wxDECLARE_EVENT( ssEVT_COMPILATION_ENDED_EVENT, CCompilationEndedEvent );
wxDECLARE_EVENT( ssEVT_COMPILATION_LOG_EVENT, CCompilationLogEvent );
wxDECLARE_EVENT( ssEVT_COMPILATION_ERROR_EVENT, CCompilationErrorEvent );
wxDECLARE_EVENT( ssEVT_PATH_CONFIRMATION_EVENT, CPathConfirmationEvent );
wxDECLARE_EVENT( ssEVT_PACKAGE_SYNC_LISTING_EVENT, CPackageSyncListingEvent );

//////////////////////////////////////////////////////////////////////////
// Connection Helper
//////////////////////////////////////////////////////////////////////////
class CEditorCompilationHelper : public wxEvtHandler, public Red::Network::ChannelListener
{
	wxDECLARE_CLASS( CEditorCompilationHelper );

public:
	CEditorCompilationHelper();
	virtual ~CEditorCompilationHelper();

	Red::System::Bool Initialize( const wxChar* ip );

private:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;
};

#endif __EDITOR_COMPILATION_HELPER_H__
