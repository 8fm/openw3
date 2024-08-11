#include "build.h"

#ifdef RED_NETWORK_ENABLED

#include "remoteConnection.h"
#include "scriptInvoker.h"

#include "../redNetwork/manager.h"

CRemoteConnection* GRemoteConnection = nullptr;

const AnsiChar* CRemoteConnection::CHANNEL_NAME = "Remote";
const Uint32 CRemoteConnection::CHANNEL_MAGIC = 0x12345678;

CRemoteConnection::CRemoteConnection()
	: m_lastReceivedTransaction( 0 )
{
	Red::Network::Manager::GetInstance()->RegisterListener( CHANNEL_NAME, this );
}

CRemoteConnection::~CRemoteConnection()
{
	Red::Network::Manager::GetInstance()->UnregisterListener( CHANNEL_NAME, this );
}

void CRemoteConnection::Flush()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Flushing of remote connection should be done only from main thread" );

	// Get messages to process
	TDelayedMessages messages;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		messages = Move( m_messages );
	}

	// execute the commands
	for ( const DelayedMessage& msg : messages )
	{
		StringAnsi result;
		if ( ProcessSync( msg.m_command, result ) )
		{
			// respond right now
			if ( result.Empty() )
			{
				Respond( msg.m_transaction, "Spam: Command executed without errors" );
			}
			else
			{
				Respond( msg.m_transaction, result );
			}
		}
		else
		{
			Respond( msg.m_transaction, "Warn: Failed to process command" );
		}
	}
}

void CRemoteConnection::OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// validate channel name
	if ( Red::StringCompare( channelName, CHANNEL_NAME) != 0 )
		return;

	// parse transaction magic to confirm that it's the proper source
	Uint32 magic = 0;
	if ( !packet.Read<Uint32>( magic ) || magic != CHANNEL_MAGIC )
		return;

	// parse transaction ID
	Uint32 transaction = 0;
	if ( !packet.Read<Uint32>( transaction) )
		return;

	// special transaction, reset internal counters
	if ( transaction == 0xFFFFFFFF )
	{
		m_lastReceivedTransaction = 0;
		return;
	}

	// out of order - do not process
	if ( transaction < m_lastReceivedTransaction )
		return;

	// remember
	m_lastReceivedTransaction = transaction;

	// parse string to execute
	AnsiChar buffer[2048];
	if ( !packet.ReadString<AnsiChar>(&buffer[0], ARRAY_COUNT_U32(buffer)) )
		return;

	// process the command
	StringAnsi result;
	if ( ProcessAsync( buffer, result ) )
	{
		// respond right now
		if ( result.Empty() )
		{
			Respond( transaction, "Spam: Command executed without errors" );
		}
		else
		{
			Respond( transaction, result );
		}
	}
	else
	{
		// add to pending list
		DelayedMessage delayed;
		delayed.m_transaction = transaction;
		delayed.m_command = buffer;
		m_messages.PushBack( delayed );
	}
}

void CRemoteConnection::Respond( const Uint32 transactionID, const StringAnsi& txt )
{
	// format message
	Red::Network::OutgoingPacket packet;
	packet.Write<Uint32>( CHANNEL_MAGIC );
	packet.Write<Uint32>( transactionID );
	packet.WriteString<AnsiChar>( txt.AsChar() );

	// send it
	Red::Network::Manager::GetInstance()->Send( CHANNEL_NAME, packet );
}

/// Helper class used to capture stuff outputed by whatever we call
class CScopedLogCapturer : public Red::System::Log::OutputDevice
{
public:
	CScopedLogCapturer()
		: m_truncated(false)
	{
		m_parentThreadID = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	}

	~CScopedLogCapturer()
	{
	}

	virtual void Write( const Red::System::Log::Message& message )
	{
		// ignore stuff from different threads
		const Uint32 thread = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
		if ( thread != m_parentThreadID )
			return;

		const Uint32 length = (const Uint32) Red::StringLength( message.text );
		const Uint32 size = m_capturedLog.GetLength() + length;
		if ( size < MAX_SIZE )
		{
			m_capturedLog += UNICODE_TO_ANSI( message.text );
			m_capturedLog += "\n";
		}
		else
		{
			if ( !m_truncated )
			{
				m_capturedLog += "......(truncated).......\n";
				m_truncated = true;
			}
		}
	}

	RED_INLINE const StringAnsi& GetResult() const
	{
		return m_capturedLog;
	}

private:
	const static Uint32 MAX_SIZE = 2000;

	Uint32			m_parentThreadID;
	StringAnsi		m_capturedLog;
	Bool			m_truncated;
};

const Bool CRemoteConnection::ProcessAsync( const StringAnsi& command, StringAnsi& result ) const
{
	return false;
}

#ifdef RED_MOD_SUPPORT
namespace ModHelpers
{
	typedef void (*LOG_FUNC)(CName, const String&);
	extern LOG_FUNC LogFn;

	CScopedLogCapturer* logCapture;
	LOG_FUNC original;

	void ModLogCapture( CName channel, const String& text )
	{
		if( logCapture )
		{
			Red::System::Log::Message message;

			message.channelHash = channel.GetSerializationHash();
			message.channelText = channel.AsChar();
			Red::System::Clock::GetInstance().GetLocalTime( message.dateTime );
			message.text = text.AsChar();

			logCapture->Write( message );
		}

		original( channel, text );
	}
}
#endif

const Bool CRemoteConnection::ProcessSync( const StringAnsi& command, StringAnsi& result ) const
{
	Bool returnVal = false;

	CScopedLogCapturer logCapture;

#ifdef RED_MOD_SUPPORT
	ModHelpers::logCapture = &logCapture;
	ModHelpers::original = ModHelpers::LogFn;
	ModHelpers::LogFn = &ModHelpers::ModLogCapture;
#endif

	// try to invoke script function
	if ( CScriptInvoker::Parse( ANSI_TO_UNICODE( command.AsChar() ) ) )
	{
		result = logCapture.GetResult();
		returnVal = true;
	}

#ifdef RED_MOD_SUPPORT
	ModHelpers::LogFn = ModHelpers::original;
	ModHelpers::logCapture = nullptr;
#endif

	return returnVal;
}

#endif // RED_NETWORK_ENABLED
