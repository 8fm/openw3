/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if !defined( NO_DEBUG_SERVER ) 

#include "questDebuggerPlugin.h"
#include "questDebuggerCommands.h"

#include "../engine/debugServerManager.h"
#include "../engine/debugServerHelpers.h"
#include "../engine/graphConnection.h"
#include "../core/diskFile.h"

#include "commonGame.h"
#include "questThread.h"
#include "quest.h"
#include "questGraphSocket.h"
#include "questInteractionDialogBlock.h"
#include "questExternalScenePlayer.h"

class CQuestLogSender
{
	enum LogMessageType
	{
		LMT_INFO,
		LMT_WARNING,
		LMT_ERROR
	};

public:
	static void WriteQuestStartedLog( Red::Network::ChannelPacket& packet, CQuest& quest )
	{
		WriteLog( packet, LMT_INFO, TXT( "Quest ") + quest.GetFileName() + TXT( " started" ), nullptr );
	}

	static void WriteQuestStoppedLog( Red::Network::ChannelPacket& packet, CQuestThread* thread )
	{
		WriteLog( packet, LMT_INFO, TXT( "Quest ") + ( thread ? thread->GetName() : TXT( "'NULL'" ) ) + TXT( " stopped" ), nullptr );
	}

	static void WriteSystemPausedLog( Red::Network::ChannelPacket& packet, Bool paused )
	{
		const String desc = ( paused ? TXT( "paused" ) : TXT( "resumed" ) );
		WriteLog( packet, LMT_INFO, TXT( "Quest ") + desc, nullptr );
	}

	static void WriteThreadPausedLog( Red::Network::ChannelPacket& packet, CQuestThread* thread, Bool paused )
	{
		const String desc = TXT( "[" ) + thread->GetName() + TXT( "]" ) + (paused ? TXT( " thread paused ") : TXT( " thread unpaused "));
		LogMessageType type = LMT_INFO;
		WriteLog( packet, type, desc, nullptr );
	}

	static void WriteAddThreadLog( Red::Network::ChannelPacket& packet, CQuestThread* parentThread, CQuestThread* thread )
	{
		const String desc = GetThreadDescription( parentThread, thread ) + TXT( " thread started" );
		LogMessageType type = LMT_INFO;
		if ( !thread || !parentThread )
		{
			type = LMT_ERROR;
		}
		WriteLog( packet, type, desc, nullptr );
	}

	static void WriteRemoveThreadLog( Red::Network::ChannelPacket& packet, CQuestThread* parentThread, CQuestThread* thread )
	{
		const String desc = GetThreadDescription( parentThread, thread ) + TXT( " thread killed" );
		LogMessageType type = LMT_INFO;
		if ( !thread || !parentThread )
		{
			type = LMT_ERROR;
		}
		WriteLog( packet, type, desc, nullptr );
	}

	static void WriteAddBlockLog( Red::Network::ChannelPacket& packet, CQuestThread* thread, const CQuestGraphBlock* block )
	{
		const String desc = GetBlockDescription( thread, block ) + TXT( " started in thread " );
		LogMessageType type = LMT_INFO;
		if ( !thread || !block )
		{
			type = LMT_ERROR;
		}
		WriteLog( packet, type, desc, block );
	}

	static void WriteRemoveBlockLog( Red::Network::ChannelPacket& packet, CQuestThread* thread, const CQuestGraphBlock* block )
	{
		// check what outputs were activated
		InstanceBuffer& data = thread->GetInstanceData();
		if ( block->WasOutputActivated( data ) )
		{
			// block's output was activated

			const CName outputName = block->GetActivatedOutputName( data );
			const CQuestGraphSocket* socket = block->FindSocket< CQuestGraphSocket >( outputName );

			if ( !socket )
			{
				// non-existing output was activated!!!
				WriteLog( packet, LMT_ERROR, GetBlockDescription( thread, block ) + TXT( " exited using an invalid output" ), block );
			}
#if !defined( NO_EDITOR_GRAPH_SUPPORT )
			else if ( socket->GetDirection() != LSD_Output )
			{
				// this is not an output socket
				WriteLog( packet, LMT_ERROR, GetBlockDescription( thread, block ) + TXT( " exited with a non-output socket [" ) + outputName.AsString() + TXT( "]" ), block );
			}
#endif
			else if ( !block->IsBlockEnabled( data ) )
			{
				// the block is inactive - this should NEVER happen
				WriteLog( packet, LMT_ERROR, GetBlockDescription( thread, block ) + TXT( " has been deactivated while exiting" ), block );
			}
			else
			{
				// everything seems ok, just make sure the output the block activated
				// is connected to something
				if ( socket->GetConnections().Empty() )
				{
					WriteLog( packet, LMT_WARNING, GetBlockDescription( thread, block ) + TXT( " exited with [" ) + outputName.AsString() + TXT( "], which is NOT CONNECTED to anything" ), block );
				}
				else
				{
					WriteLog( packet, LMT_WARNING, GetBlockDescription( thread, block ) + TXT( " correctly exited with [" ) + outputName.AsString() + TXT( "]" ), block );
				}
			}
		}
		else
		{
			// block's output WASN'T activated

			// check if any errors are set
			if ( !block->GetErrorMsg( data ).Empty() )
			{
				WriteLog( packet, LMT_ERROR, GetBlockDescription( thread, block ) + TXT( " has thrown an exception: " ) + block->GetErrorMsg( data ), block );
			}
			else if ( !block->IsBlockEnabled( data ) )
			{
				// the block was cut controlled
				WriteLog( packet, LMT_INFO, GetBlockDescription( thread, block ) + TXT( " was CUT-CONTROLLED" ), block );
			}
			else
			{
				// an output wasn't cut controlled, this indicates that the thread the block
				// was in died - it probably was cut controlled
				WriteLog( packet, LMT_INFO, GetBlockDescription( thread, block ) + TXT( " died, because the phase it was in was CUT_CONTROLLED" ), block );
			}
		}
	}

	static void WriteBlockInputActivatedLog( Red::Network::ChannelPacket& packet, CQuestThread* thread, const CQuestGraphBlock* block )
	{
		const String desc = GetBlockDescription( thread, block ) + TXT( " input activated" );
		LogMessageType type = LMT_INFO;
		if ( !thread || !block )
		{
			type = LMT_ERROR;
		}
		WriteLog( packet, type, desc, block );
	}

private:
	static void WriteLog( Red::Network::ChannelPacket& packet, LogMessageType type, const String& text, const CQuestGraphBlock* block )
	{
		const Uint64 blockPtr = reinterpret_cast<Uint64>( block );

		packet.WriteString( UNICODE_TO_ANSI( text.AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( (Int32)type ).AsChar() ) );
		packet.WriteString( UNICODE_TO_ANSI( ToString( (Float)GGame->GetEngineTime() ).AsChar() ) );
	}

	static String GetThreadDescription( CQuestThread* parentThread, CQuestThread* thread )
	{
		const String parentThreadName = parentThread ? parentThread->GetName() : TXT( "'NULL'" );
		const String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
		return TXT( "[" ) + parentThreadName + TXT( ":" ) + threadName + TXT( "]" );
	}

	static String GetBlockDescription( CQuestThread* thread, const CQuestGraphBlock* block )
	{
		const String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
		const String blockName = block ? block->GetCaption() : TXT( "'NULL'" );
		return TXT( "[" ) + threadName + TXT( "->" ) + blockName + TXT( "]" );
	}
};

Bool CQuestDebuggerPlugin::Init()
{	
	#ifndef RED_PROFILE_BUILD
	m_questSystem = GCommonGame->GetSystem< CQuestsSystem >();
	m_requestBreakpoint = false;
	DBGSRV_REG_COMMAND( this, "getQuestLayout", CDebugServerCommandGetQuestLayout );
	DBGSRV_REG_COMMAND( this, "getQuestData", CDebugServerCommandGetQuestData );
	DBGSRV_REG_COMMAND( this, "toggleConnection", CDebugServerCommandToggleConnection );
	DBGSRV_REG_COMMAND( this, "getQuestBlockProperties", CDebugServerCommandGetQuestBlockProperties );	
	DBGSRV_REG_COMMAND( this, "getCallstack", CDebugServerCommandGetCallstack );
	DBGSRV_REG_COMMAND( this, "toggleBreakpoint", CDebugServerCommandToggleBreakpoint );
	DBGSRV_REG_COMMAND( this, "continueFromBreakpoint", CDebugServerCommandContinueFromBreakpoint );
	DBGSRV_REG_COMMAND( this, "continueFromPin", CDebugServerCommandContinueFromPin );
	DBGSRV_REG_COMMAND( this, "startInteractionDialog", CDebugServerCommandStartInteractionDialog );	
	DBGSRV_REG_COMMAND( this, "killSignal", CDebugServerCommandKillSignal );	
	return true;
	#else
	return false;
	#endif
}

Bool CQuestDebuggerPlugin::ShutDown()
{
	#ifndef RED_PROFILE_BUILD
	m_questSystem->DetachListener( *this );
	m_questSystem = nullptr;
	#endif
	return true;
}

void CQuestDebuggerPlugin::GameStarted()
{
}

void CQuestDebuggerPlugin::GameStopped()
{
	m_breakpoints.Clear();
}

void CQuestDebuggerPlugin::AttachToWorld()
{
}

void CQuestDebuggerPlugin::DetachFromWorld()
{
}

void CQuestDebuggerPlugin::Tick()
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	if ( !m_requestBreakpoint )
		return;

	GGame->Pause( TXT( "Breakpoint in QuestDebugger" ) );
	GGame->Stop();
#ifdef RED_PLATFORM_WINPC
	::ReleaseCapture();
	::ClipCursor( nullptr );
	GGame->GetViewport()->SetCursorVisibility( true );
#endif
	m_requestBreakpoint = false;
	#endif
}

void CQuestDebuggerPlugin::SendCallstack()
{
	#ifndef RED_PROFILE_BUILD
	m_questSystem->DetachListener( *this );
	m_questSystem->AttachListener( *this );
	#endif
}

void CQuestDebuggerPlugin::ToggleBreakpoint( Uint64 pointer, CGUID guid )
{	
	Bool shouldAddNewBreakpoint = !m_breakpoints.KeyExist( guid );
	if ( shouldAddNewBreakpoint )
	{
		const CQuestGraphBlock* questBlockPointer = reinterpret_cast<const CQuestGraphBlock*>( pointer );
		m_breakpoints.Insert( guid, questBlockPointer );
	}
	else
	{
		m_breakpoints.Erase( guid );
	}

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "toggleBreakpoint" );

	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( pointer ).AsChar() ) );	
	packet.WriteString( UNICODE_TO_ANSI( ToString( guid ).AsChar() ) );

	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
}

void CQuestDebuggerPlugin::StartInteractionDialog( Uint64 pointer )
{
	#ifndef RED_PROFILE_BUILD
	const CQuestInteractionDialogBlock* interactionDialogBlock = reinterpret_cast<const CQuestInteractionDialogBlock*>( pointer );
	CQuestsSystem* questsSystem = GCommonGame->GetSystem< CQuestsSystem >();
	const TDynArray< CName >& tags = interactionDialogBlock->GetActorTags().GetTags();

	Bool started = false;
	const Uint32 count = tags.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		started = m_questSystem->GetInteractionDialogPlayer()->StartDialog( tags[ i ] );
		if ( started )
			break;
	}	
	#endif
}

void CQuestDebuggerPlugin::ContinueFromBreakpoint()
{
	Continue();
}

void CQuestDebuggerPlugin::ContinueFromPin( CQuestThread* thread, CQuestGraphBlock* block, const String& socketName, const String& socketDirection )
{	
	#ifndef RED_PROFILE_BUILD
	Continue();
	typedef TDynArray< SBlockDesc >::iterator SBlockDescIterator;	

	if( socketDirection == TXT( "in" ) )
	{
		CheckBreakpoints( block );
		CName socketCName( socketName );
		thread->ActivateBlock( *block, socketCName );
	}
	else if( socketDirection == TXT( "out" ) )
	{
		TDynArray< SBlockDesc > blocksToActivate;
		block->GetConnectedBlocks( CName( socketName ), blocksToActivate );

		SBlockDescIterator blocksToActivateEnd = blocksToActivate.End();
		for ( SBlockDescIterator it = blocksToActivate.Begin(); it != blocksToActivateEnd; ++it )
		{
			thread->ActivateBlock( *it->block, it->inputName );
		}
	}
	#endif
}

void CQuestDebuggerPlugin::KillSignal( CQuestThread* thread, CQuestGraphBlock* block )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	thread->DeactivateBlock( block );
	#endif
}

void CQuestDebuggerPlugin::OnQuestStarted( CQuestThread* thread, CQuest& quest )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnQuestStarted" );

	CQuestLogSender::WriteQuestStartedLog( packet, quest );

	const Uint64 threadPtr = reinterpret_cast<Uint64>( thread );
	const Uint64 questPtr = reinterpret_cast<Uint64>( &quest );
	const String& filePath = quest.GetFile()->GetDepotPath();
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( questPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( threadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( thread->GetName().AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( filePath.AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnQuestStopped( CQuestThread* thread )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnQuestStopped" );
	CQuestLogSender::WriteQuestStoppedLog( packet, thread );
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnSystemPaused( Bool paused )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnSystemPaused" );
	CQuestLogSender::WriteSystemPausedLog( packet, paused );
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnThreadPaused( CQuestThread* thread, Bool paused )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnThreadPaused" );
	CQuestLogSender::WriteThreadPausedLog( packet, thread, paused );
	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnAddThread" );

	CQuestLogSender::WriteAddThreadLog( packet, parentThread, thread );

	const Uint64 parentThreadPtr = reinterpret_cast<Uint64>( parentThread );
	const Uint64 threadPtr = reinterpret_cast<Uint64>( thread );
	const Uint64 parentBlockPtr = reinterpret_cast<Uint64>( thread->GetParentBlock() );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentBlockPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentThreadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( threadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( thread->GetName().AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( ToString( thread->GetParentBlock()->GetGUID() ).AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnRemoveThread" );

	CQuestLogSender::WriteRemoveThreadLog( packet, parentThread, thread );

	const Uint64 parentThreadPtr = reinterpret_cast<Uint64>( parentThread );
	const Uint64 threadPtr = reinterpret_cast<Uint64>( thread );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentThreadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( threadPtr ).AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnAddBlock" );

	CQuestLogSender::WriteAddBlockLog( packet, thread, block );

	const Uint64 parentThreadPtr = reinterpret_cast<Uint64>( thread );
	const Uint64 blockPtr = reinterpret_cast<Uint64>( block );
	const String callstackDisplayName = CreateBlockName( thread, block );
	const String name( block->GetCaption().AsChar() );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentThreadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( blockPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( callstackDisplayName.AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( name.AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( ToString( block->GetGUID() ).AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );

	UpdateBreakpointData( block );
	#endif
}

void CQuestDebuggerPlugin::OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnRemoveBlock" );

	CQuestLogSender::WriteRemoveBlockLog( packet, thread, block );

	const Uint64 parentThreadPtr = reinterpret_cast<Uint64>( thread );
	const Uint64 blockPtr = reinterpret_cast<Uint64>( block );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentThreadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( blockPtr ).AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

void CQuestDebuggerPlugin::OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block )
{
	#ifndef RED_PROFILE_BUILD
	if ( !DBGSRV_CALL( IsConnected() ) )
		return;

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_DEBUG_SERVER );
	packet.WriteString( "OnBlockInputActivated" );

	CQuestLogSender::WriteBlockInputActivatedLog( packet, thread, block );

	const Uint64 parentThreadPtr = reinterpret_cast<Uint64>( thread );
	const Uint64 blockPtr = reinterpret_cast<Uint64>( block );
	const String name = CreateBlockName( thread, block );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( parentThreadPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( CDebugServerHelpers::GetStringFromUid( blockPtr ).AsChar() ) );
	packet.WriteString( UNICODE_TO_ANSI( name.AsChar() ) );

	DBGSRV().Send( RED_NET_CHANNEL_DEBUG_SERVER, packet );
	#endif
}

String CQuestDebuggerPlugin::CreateBlockName( CQuestThread* thread, const CQuestGraphBlock* block ) const
{
	if ( !thread || !block )
		return String::EMPTY;

	String name( block->GetCaption().AsChar() );

	// append the names of active inputs
	const TDynArray< CName >* activeInputs = thread->GetActivatedInputs( block );
	if ( activeInputs != nullptr )
	{
		name += TXT( "[" );
		const Uint32 count = activeInputs->Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( i > 0 )
			{
				name += TXT( ", " );
			}
			name += (*activeInputs)[ i ].AsString().AsChar();
		}
		name += TXT( "]" );
	}

	return name;
}

void CQuestDebuggerPlugin::UpdateBreakpointData( const CQuestGraphBlock* block )
{
	const CGUID& guid = block->GetGUID();
	if ( m_breakpoints.KeyExist( guid ) )
	{
		if ( m_breakpoints[ guid ] == nullptr )
			m_breakpoints[ guid ] = block;
	}
}

void CQuestDebuggerPlugin::CheckBreakpoints( const CQuestGraphBlock* block )
{
	const CGUID& guid = block->GetGUID();
	if ( m_breakpoints.KeyExist( guid ) )
	{
		m_requestBreakpoint = true;
		PauseQuestSystem();
	}
}

void CQuestDebuggerPlugin::PauseQuestSystem()
{
	if ( !m_questSystem->IsPaused() )
		m_questSystem->Pause( true );
}

void CQuestDebuggerPlugin::UnpauseQuestSystem()
{
	if ( m_questSystem->IsPaused() )
		m_questSystem->Pause( false );
}

void CQuestDebuggerPlugin::Continue()
{
	UnpauseQuestSystem();	
	GGame->Unpause( TXT( "Breakpoint in QuestDebugger" ) );
	GGame->Unstop();
}

#endif
