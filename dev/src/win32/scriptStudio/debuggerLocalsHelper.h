/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __DEBUGGER_LOCALS_HELPER_H__
#define __DEBUGGER_LOCALS_HELPER_H__

#include "../../common/redSystem/hash.h"
#include "../../common/redThreads/redThreadsAtomic.h"
#include "../../common/redNetwork/channel.h"

//////////////////////////////////////////////////////////////////////////
// Structures
////////////////////////////////////////////////////////////////////////// 
struct SDebugValue
{
	wxString name;
	wxString value;
	Red::System::Uint8 icon;
	Red::System::Bool isModifiable;
};

struct SDebugValueChild : public SDebugValue
{
	Red::System::Bool isExpandable;
};

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////

class CLocalsEvent : public wxEvent
{
public:
	CLocalsEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CLocalsEvent( SDebugValue* property, SDebugValueChild* children, Red::System::Uint32 numChildren, Red::System::Uint32 stackFrame, Red::System::Uint32 stamp, const wxString& path, Red::System::Uint32 itemId );
	CLocalsEvent( const CLocalsEvent& other );

	virtual ~CLocalsEvent();

	const SDebugValue* GetProperty() const { return m_property; }
	const SDebugValueChild* GetChildren() const { return m_children; }
	Red::System::Uint32 GetNumberOfChildren() const { return m_numChildren; }
	Red::System::Uint32 GetStackFrameIndex() const { return m_stackFrame; }
	Red::System::Uint32 GetStamp() const { return m_stamp; }
	const wxString& GetPath() const { return m_path; }
	Red::System::Uint32 GetItemId() const { return m_itemId; }

private:
	virtual wxEvent* Clone() const override final { return new CLocalsEvent( *this ); }

private:
	SDebugValue* m_property;
	SDebugValueChild* m_children;
	Red::System::Uint32 m_numChildren;
	
	Red::System::Uint32 m_stackFrame;
	Red::System::Uint32 m_stamp;

	wxString m_path;
	Red::System::Uint32 m_itemId;


	Red::Threads::CAtomic< Red::System::Uint32 >* m_referenceCount;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CLocalsEvent );
};

wxDECLARE_EVENT( ssEVT_LOCALS_EVENT, CLocalsEvent );

class CLocalsModificationAckEvent : public wxEvent
{
public:
	CLocalsModificationAckEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CLocalsModificationAckEvent( Red::System::Bool success );
	virtual ~CLocalsModificationAckEvent();

	RED_INLINE Red::System::Bool GetSuccess() const { return m_success; }

private:
	virtual wxEvent* Clone() const override final { return new CLocalsModificationAckEvent( m_success ); }

	Red::System::Bool m_success;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CLocalsModificationAckEvent );
};

wxDECLARE_EVENT( ssEVT_LOCALS_MODIFY_ACK_EVENT, CLocalsModificationAckEvent );

//////////////////////////////////////////////////////////////////////////
// Connection Helper
//////////////////////////////////////////////////////////////////////////
class CDebuggerLocalsHelper : public wxEvtHandler, public Red::Network::ChannelListener
{
	wxDECLARE_CLASS( CDebuggerLocalsHelper );

public:
	CDebuggerLocalsHelper();
	virtual ~CDebuggerLocalsHelper();

	Red::System::Bool Initialize( const wxChar* ip );

	// Control
	void RequestLocals( Red::System::Uint32 stackFrameIndex, Red::System::Uint32 stamp, const Red::System::Char* path, Red::System::Uint32 id );
	void SetLocalsValue( Red::System::Uint32 stackFrameIndex, const wxString& path, const wxString& value );

private:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;

	void OnCallstackReceived( Red::Network::IncomingPacket& packet );

	void ProcessLocalsHeader( Red::Network::IncomingPacket& packet );
	void ProcessLocalsBody( Red::Network::IncomingPacket& packet );
	void ProcessLocalsEvent();

private:
	wxString m_path;
	SDebugValue* m_property;
	SDebugValueChild* m_children;
	Red::System::THash32 m_hash;
	Red::System::Uint32 m_stamp;
	Red::System::Uint32 m_stackFrame;
	Red::System::Uint32 m_id;
	Red::System::Uint32 m_numChildren;

	Red::System::Uint32 m_numChildrenReceived;
};

#endif __DEBUGGER_LOCALS_HELPER_H__
