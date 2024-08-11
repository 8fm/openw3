/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __PROFILER_HELPER_H__
#define __PROFILER_HELPER_H__

#include "../../common/redThreads/redThreadsAtomic.h"
#include "../../common/redNetwork/channel.h"

#define RED_NET_CHANNEL_SCRIPT_PROFILER "ScriptProfiler"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
struct SProfileData;

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////
class CProfilerStartedEvent : public wxEvent
{
public:
	CProfilerStartedEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CProfilerStartedEvent( Red::System::Bool continuous );
	virtual ~CProfilerStartedEvent();

	inline Red::System::Bool IsContinuous() const { return m_continuous; }

private:
	virtual wxEvent* Clone() const override final { return new CProfilerStartedEvent( m_continuous ); }

private:
	Red::System::Bool m_continuous;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CProfilerStartedEvent );
};

//////////////////////////////////////////////////////////////////////////

class CProfilerReportEvent : public wxEvent
{
public:
	CProfilerReportEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CProfilerReportEvent( SProfileData* data, Red::System::Uint32 size );
	CProfilerReportEvent( const CProfilerReportEvent& other );

	virtual ~CProfilerReportEvent();

	inline Red::System::Uint32 GetSize() const { return m_size; }
	inline SProfileData* GetReport() { return m_data; }
	inline const SProfileData* GetReport() const { return m_data; }

private:
	virtual wxEvent* Clone() const override final { return new CProfilerReportEvent( *this ); }

private:
	SProfileData* m_data;
	Red::System::Uint32 m_size;

	Red::Threads::CAtomic< Red::System::Uint32 >* m_referenceCount;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CProfilerReportEvent );
};

wxDECLARE_EVENT( ssEVT_PROFILER_STARTED_EVENT, CProfilerStartedEvent );
wxDECLARE_EVENT( ssEVT_PROFILER_REPORT_EVENT, CProfilerReportEvent );

//////////////////////////////////////////////////////////////////////////
// Connection Helper
//////////////////////////////////////////////////////////////////////////
class CProfilerHelper : public wxEvtHandler, public Red::Network::ChannelListener
{
	wxDECLARE_CLASS( CProfilerHelper );

public:
	CProfilerHelper();
	virtual ~CProfilerHelper();

	Red::System::Bool Initialize( const wxChar* ip );

	void StartProfiling( Red::System::Uint32 framesToProfile );

	void StartContinuousProfiling();
	void StopContinuousProfiling();

private:
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override final;
	
	void ProfileReportHeaderReceived( Red::Network::IncomingPacket& packet );
	void ProfileReportBodyReceived( Red::Network::IncomingPacket& packet );

	SProfileData* m_profileReport;
	Red::System::Uint32 m_profileReportSize;
	Red::System::Uint32 m_profileReportPosition;
	Red::System::Uint32 m_profileReportId;
};

#endif //__PROFILER_HELPER_H__
