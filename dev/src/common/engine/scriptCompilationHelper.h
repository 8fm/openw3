/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_COMPILATION_HELPER_H__
#define __SCRIPT_COMPILATION_HELPER_H__

// Channel Interface
#include "../redNetwork/channel.h"

// Script Log Interface
#include "../core/scriptCompiler.h"

enum ECompileScriptsReturnValue
{
	CSRV_Recompile = 0,
	CSRV_Skip,
	CSRV_Quit,

	CSRV_Max
};

//////////////////////////////////////////////////////////////////////////

class CScriptCompilationMessages : public IScriptLogInterface
{
public:
	struct SContext
	{
		int line;
		String file;
		String text;
	};

	TDynArray< SContext > m_errors;
	TDynArray< SContext > m_warnings;

public:
	CScriptCompilationMessages()
	{
	}

	virtual void Log( const String& /*text*/ )
	{

	}

	virtual void Warn( const CScriptFileContext& context, const String& text )
	{
		SContext newContext;

		newContext.line = context.m_line;
		newContext.file = context.m_file;
		newContext.text = text;

		m_warnings.PushBack( newContext );
	}

	virtual void Error( const CScriptFileContext& context, const String& text )
	{
		SContext newContext;

		newContext.line = context.m_line;
		newContext.file = context.m_file;
		newContext.text = text;

		m_errors.PushBack( newContext );
	}
};

//////////////////////////////////////////////////////////////////////////

class IScriptCompilationFeedback
{
public:
	IScriptCompilationFeedback() {}
	virtual ~IScriptCompilationFeedback() {}

	virtual ECompileScriptsReturnValue OnCompilationFailed( const CScriptCompilationMessages& ) = 0;
};

#ifdef RED_NETWORK_ENABLED

//////////////////////////////////////////////////////////////////////////

class CScriptNetworkHandler : Red::Network::ChannelListener
{
public:
	CScriptNetworkHandler();
	~CScriptNetworkHandler();

	void Initialize();

	RED_INLINE void RequestReload( Bool requested = true ) { m_reloadRequested = requested; }
	RED_INLINE Bool ReloadRequested() const { return m_reloadRequested; }

	void Update();

private:
	virtual void OnPacketReceived( const AnsiChar* channelName, Red::Network::IncomingPacket& packet );

	void StartProfiling( Uint32 numberOfFramesToProfile );
	void EndProfiling();
	void WriteProfilingReport( TDynArray< FuncPerfData* > perfData );

	static Bool AddProfileDataToPacket( Red::Network::ChannelPacket* packet, FuncPerfData* data, Uint32 totalTicks, THashMap< CFunction*, Int32 >& functionIndices );

private:
	String m_overriddenScriptPath;
	Bool m_overrideScriptPath;
	Bool m_reloadRequested;
	Red::Threads::CAtomic< Uint32 > m_profileFrameCount;
	Red::Threads::CAtomic< Bool > m_continuousProfiling;
	
	Red::Threads::CAtomic< Bool > m_profilingRequested;

	TDynArray< Red::Network::ChannelPacket* > m_profilingPackets;
	Uint32 m_profileReportCount;
	Red::Threads::CAtomic< Bool > m_sendingProfilingPackets;
	Red::Threads::CAtomic< Uint32 > m_profilingPacketSent;
};

#endif // RED_NETWORK_ENABLED

#endif //__SCRIPT_COMPILATION_HELPER_H__
