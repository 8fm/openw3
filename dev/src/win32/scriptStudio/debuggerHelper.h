/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __DEBUGGER_HELPER_H__
#define __DEBUGGER_HELPER_H__

#include "../../common/redThreads/redThreadsAtomic.h"
#include "../../common/redNetwork/channel.h"

#define RED_NET_CHANNEL_SCRIPT_DEBUGGER "ScriptDebugger"
#define RED_NET_CHANNEL_SCRIPT_SYSTEM	"ScriptSystem"

struct SProperty
{
	wxString name;
	wxString value;
};

struct SFrame
{
	wxString file;
	Red::System::Uint32 line;
	wxString function;

	Red::System::Uint32 numParameters;
	SProperty* parameters;

	SFrame()
		:	numParameters( 0 )
		,	parameters( nullptr )
	{
	}

	~SFrame()
	{
		delete [] parameters;
	}
};

struct SNativeFrame
{
	wxString location;
	wxString symbol;
};

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////
class CBreakpointToggleConfirmationEvent : public wxEvent
{
public:
	CBreakpointToggleConfirmationEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CBreakpointToggleConfirmationEvent( const wxString& file, Red::System::Int32 line, Red::System::Bool success );
	virtual ~CBreakpointToggleConfirmationEvent() {}

	inline const wxString& GetFile() const { return m_file; }
	inline Red::System::Int32 GetLine() const { return m_line; }
	inline Red::System::Bool ToggleConfirmed() const { return m_success; }

private:
	virtual wxEvent* Clone() const override final { return new CBreakpointToggleConfirmationEvent( m_file, m_line, m_success ); }

private:
	wxString m_file;
	Red::System::Int32 m_line;
	Red::System::Bool m_success;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CBreakpointToggleConfirmationEvent );
};

//////////////////////////////////////////////////////////////////////////

class CBreakpointHitEvent : public wxEvent
{
public:
	CBreakpointHitEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CBreakpointHitEvent( const wxString& file, Red::System::Int32 line );
	virtual ~CBreakpointHitEvent();

	inline const wxString& GetFile() const { return m_file; }
	inline Red::System::Int32 GetLine() const { return m_line; }

private:
	virtual wxEvent* Clone() const override final { return new CBreakpointHitEvent( m_file, m_line ); }

private:
	wxString m_file;
	Red::System::Int32 m_line;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CBreakpointHitEvent );
};

//////////////////////////////////////////////////////////////////////////

class CCallstackUpdateEvent : public wxEvent
{
public:
	CCallstackUpdateEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CCallstackUpdateEvent( Red::System::Uint32 numberOfFrames, SFrame* frames, Red::System::Uint32 numberOfNativeFrames, SNativeFrame* nativeFrames );
	CCallstackUpdateEvent( const CCallstackUpdateEvent& other );

	virtual ~CCallstackUpdateEvent();

	inline Red::System::Uint32 GetNumberOfFrames() const { return m_numberOfFrames; }
	inline SFrame* GetFrames() { return m_frames; }
	inline const SFrame* GetFrames() const { return m_frames; }

	inline Red::System::Uint32 GetNumberOfNativeFrames() const { return m_numNativeFrames; }
	inline SNativeFrame* GetNativeFrames() { return m_nativeFrames; }
	inline const SNativeFrame* GetNativeFrames() const { return m_nativeFrames; }

private:
	virtual wxEvent* Clone() const override final { return new CCallstackUpdateEvent( *this ); }

private:
	SFrame* m_frames;
	SNativeFrame* m_nativeFrames;

	Red::System::Uint32 m_numberOfFrames;
	Red::System::Uint32 m_numNativeFrames;

	Red::Threads::CAtomic< Red::System::Uint32 >* m_referenceCount;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCallstackUpdateEvent );
};

wxDECLARE_EVENT( ssEVT_BREAKPOINT_TOGGLE_CONFIRMATION_EVENT, CBreakpointToggleConfirmationEvent );
wxDECLARE_EVENT( ssEVT_BREAKPOINT_HIT_EVENT, CBreakpointHitEvent );
wxDECLARE_EVENT( ssEVT_CALLSTACK_EVENT, CCallstackUpdateEvent );

//////////////////////////////////////////////////////////////////////////
// Connection Helper
//////////////////////////////////////////////////////////////////////////
class CDebuggerHelper : public wxEvtHandler, public Red::Network::ChannelListener
{
	wxDECLARE_CLASS( CDebuggerHelper );

public:
	CDebuggerHelper();
	virtual ~CDebuggerHelper();

	Red::System::Bool Initialize( const wxChar* ip );

	// Control
	Red::System::Bool DebugContinue();
	Red::System::Bool DebugStepOver();
	Red::System::Bool DebugStepOut();
	Red::System::Bool DebugStepInto();
	Red::System::Bool RequestCallstack();

	Red::System::Bool SetLocalsSorted( Red::System::Bool enabled );
	Red::System::Bool SetLocalsUnfiltered( Red::System::Bool enabled );

	// Configuration
	Red::System::Bool ToggleBreakpoint( const wxString& filepath, Red::System::Int32 line, Red::System::Bool enabled );
	Red::System::Bool DisableAllBreakpoints();

private:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;

	void OnCallstackReceived( Red::Network::IncomingPacket& packet );
	void OnOpcodesReceived( Red::Network::IncomingPacket& packet );
};

#endif __DEBUGGER_HELPER_H__
