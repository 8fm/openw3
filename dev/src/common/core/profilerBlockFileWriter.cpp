/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "profilerFileWriter.h"
#include "profilerBlockFileWriter.h"
#include "configVar.h"

CProfilerBlockFileWriter::CProfilerBlockFileWriter()
	: m_writer( nullptr )
{
	Red::MemoryZero( m_blocks, sizeof(m_blocks) );
	Red::MemoryZero( m_signals, sizeof(m_signals) );
	Red::MemoryZero( m_counter, sizeof(m_counter) );
}

CProfilerBlockFileWriter::~CProfilerBlockFileWriter()
{
}

Bool CProfilerBlockFileWriter::Start( const String& path )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// stop current profiling
	if ( IsProfiling() )
		Stop();

	// cleanup string tables
	m_strings.ClearFast();

	// create writer
	m_writer = CProfilerFileWriter::Open( path );
	if ( m_writer )
	{
		// write initial stuff
		{
			CProfilerMessage< Header > msg( m_writer );
			msg->m_magic = 'RDIO';
			msg->m_version = 1;
			msg->m_mainThread = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
			Red::Clock::GetInstance().GetTimer().GetTicks( msg->m_tickBase );
			Red::Clock::GetInstance().GetTimer().GetFrequency( msg->m_tickFreq );
		}

		// flush block defines
		for ( Uint32 i=0; i<MAX_DEFS; ++i )
			if ( m_blocks[i].m_defined )
				WriteBlockDefine( (TBlockID)i, m_blocks[i] );

		// flush counter defines
		for ( Uint32 i=0; i<MAX_DEFS; ++i )
			if ( m_counter[i].m_defined )
				WriteCounterDefine( (TCounterID)i, m_counter[i] );

		// flush signal defines
		for ( Uint32 i=0; i<MAX_DEFS; ++i )
			if ( m_signals[i].m_defined )
				WriteSignalDefine( (TSignalID)i, m_signals[i] );

		// flush the header into the file
		m_writer->Flush();
		return true;
	}
	else
	{
		ERR_CORE( TXT("Unable to create output file '%ls'"), path.AsChar() );
		return false;
	}
}

void CProfilerBlockFileWriter::Stop()
{
	// close file
	if ( m_writer )
	{
		m_writer->Flush();
		delete m_writer;
	}
}

const Bool CProfilerBlockFileWriter::IsProfiling() const
{
	return (m_writer != nullptr);
}

void CProfilerBlockFileWriter::Flush()
{
	if ( m_writer )
	{
		m_writer->Flush();
	}
}

//----

void CProfilerBlockFileWriter::RegisterBlockType( const TBlockID id, const AnsiChar* name, const Uint32 numStartParams, const Uint32 numEndParams )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// ID already registered
	if ( m_blocks[id].m_defined )
	{
		WARN_CORE( TXT("Invalid ID%d for profiling block %s or ID already used"), id, name );
		return;
	}

	// Define the crap
	m_blocks[id].m_defined = true;
	m_blocks[id].m_name = name;
	m_blocks[id].m_numStartParams = numStartParams;
	m_blocks[id].m_numEndParams = numEndParams;

	// Send the definition info
	if ( m_writer )
	{
		WriteBlockDefine( id, m_blocks[id] );
	}
}

void CProfilerBlockFileWriter::RegisterSignalType( const TSignalID id, const AnsiChar* name, const Uint32 numParams )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// ID already registered
	if ( m_signals[id].m_defined )
	{
		WARN_CORE( TXT("Invalid ID%d for profiling signal %s or ID already used"), id, name );
		return;
	}

	// Define the crap
	m_signals[id].m_defined = true;
	m_signals[id].m_name = name;
	m_signals[id].m_numParams = numParams;

	// Send the definition info
	if ( m_writer )
	{
		WriteSignalDefine( id, m_signals[id] );
	}
}

void CProfilerBlockFileWriter::RegisterCounterType( const TCounterID id, const AnsiChar* name )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// ID already registered
	if ( m_counter[id].m_defined )
	{
		WARN_CORE( TXT("Invalid ID%d for profiling counter %s or ID already used"), id, name );
		return;
	}

	// Define the crap
	m_counter[id].m_defined = true;
	m_counter[id].m_name = name;

	// Send the definition info
	if ( m_writer )
	{
		WriteCounterDefine( id, m_counter[id] );
	}
}

const Uint32 CProfilerBlockFileWriter::MapString( const String& path )
{
	return MapString( path.AsChar() );
}

const Uint32 CProfilerBlockFileWriter::MapString( const StringAnsi& path )
{
	return MapString( path.AsChar() );
}

const Uint32 CProfilerBlockFileWriter::MapString( const Char* path )
{
	return MapString( UNICODE_TO_ANSI( path ) );
}

const Uint32 CProfilerBlockFileWriter::MapString( const AnsiChar* path )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// no paths
	if ( !m_writer || !path || !path[0] )
		return 0;

	// calculate path hash
	const Uint32 stringHash = Red::CalculateAnsiHash32LowerCase( path );
	if ( !m_strings.Exist( stringHash ) )
	{
		// prepare a message specifying path name details
		const Uint32 length = (Uint32) Red::StringLength( path );
		const Uint32 size = sizeof(Uint32)*3 + length;
		Uint8* raw = m_writer->AllocMessage( size );

		// message header
		*(Uint16*)(raw+0) = eEvent_DefString;
		*(Uint16*)(raw+2) = 0; // ID
		*(Uint32*)(raw+4) = stringHash;
		*(Uint32*)(raw+8) = length;

		// copy string
		Red::MemoryCopy( raw+12, path, length );

		// finish message
		m_writer->FinishMessage();
	}

	// return string HASH - to be used instead of costly string
	return stringHash;
}

void CProfilerBlockFileWriter::SetThreadName( const AnsiChar* threadName )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// get internal thread ID
	const Uint32 threadID = Red::System::Internal::ThreadId::CurrentThread().AsNumber();

	// set thread name
	Bool added = false;
	for ( Uint32 i=0; i<m_threads.Size(); ++i )
	{
		if ( m_threads[i].m_id == threadID )
		{
			m_threads[i].m_name = threadName;
			added = true;
			break;
		}
	}

	// add new thread entry
	if ( !added )
	{
		ThreadInfo info;
		info.m_id = threadID;
		info.m_name = threadName;
		m_threads.PushBack( info );
	}

	// no name
	if ( !m_writer || !threadName || !threadName[0] )
		return;

	// prepare a message specifying path name details
	const Uint32 length = (Uint32) Red::StringLength( threadName );
	const Uint32 size = sizeof(Uint32)*3 + length;
	Uint8* raw = m_writer->AllocMessage( size );

	// message header
	*(Uint32*)(raw+0) = eEvent_ThreadName;
	*(Uint32*)(raw+4) = threadID;
	*(Uint32*)(raw+8) = length;

	// copy string
	Red::MemoryCopy( raw+12, threadName, length );

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::Start( const TBlockID blockType, const Uint8 numParams, const Uint32 paramA/*=0*/, const Uint32 paramB/*=0*/, const Uint32 paramC/*=0*/, const Uint32 paramD/*=0*/, const Uint32 paramE/*=0*/ )
{
	// check definition
	RED_ASSERT( m_blocks[blockType].m_defined, TXT("Using undefined block %d"), blockType );
	if ( !m_blocks[blockType].m_defined )
		return;

	// check parameter count
	RED_ASSERT( m_blocks[blockType].m_numStartParams == numParams, TXT("Parameter count mismatch for start of block %ls, expected %d, provided %d"), 
		ANSI_TO_UNICODE( m_blocks[blockType].m_name.AsChar() ), m_blocks[blockType].m_numStartParams, numParams );
	if ( m_blocks[blockType].m_numStartParams != numParams )
		return;

	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 messageSize = (sizeof(Message)-sizeof(Uint32)) + sizeof(Uint32) * numParams;
	Message* message = (Message*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_StartBlock;
	message->m_id = blockType;
	message->m_threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	message->m_timestamp = Red::Clock::GetInstance().GetTimer().GetTicks();

	// copy data
	if ( numParams >= 1 ) message->m_data[0] = paramA;
	if ( numParams >= 2 ) message->m_data[1] = paramB;
	if ( numParams >= 3 ) message->m_data[2] = paramC;
	if ( numParams >= 4 ) message->m_data[3] = paramD;
	if ( numParams >= 5 ) message->m_data[4] = paramE;

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::End( const TBlockID blockType, const Uint8 numParams, const Uint32 paramA/*=0*/, const Uint32 paramB/*=0*/, const Uint32 paramC/*=0*/, const Uint32 paramD/*=0*/, const Uint32 paramE/*=0*/ )
{
	// check definition
	RED_ASSERT( m_blocks[blockType].m_defined, TXT("Using undefined block %d"), blockType );
	if ( !m_blocks[blockType].m_defined )
		return;

	// check parameter count
	RED_ASSERT( m_blocks[blockType].m_numEndParams == numParams, TXT("Parameter count mismatch for end of block %ls, expected %d, provided %d"), 
		ANSI_TO_UNICODE( m_blocks[blockType].m_name.AsChar() ), m_blocks[blockType].m_numEndParams, numParams );
	if ( m_blocks[blockType].m_numEndParams != numParams )
		return;

	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 messageSize = (sizeof(Message)-sizeof(Uint32)) + sizeof(Uint32) * numParams;
	Message* message = (Message*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_EndBlock;
	message->m_id = blockType;
	message->m_threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	message->m_timestamp = Red::Clock::GetInstance().GetTimer().GetTicks();

	// copy data
	if ( numParams >= 1 ) message->m_data[0] = paramA;
	if ( numParams >= 2 ) message->m_data[1] = paramB;
	if ( numParams >= 3 ) message->m_data[2] = paramC;
	if ( numParams >= 4 ) message->m_data[3] = paramD;
	if ( numParams >= 5 ) message->m_data[4] = paramE;

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::Signal( const TSignalID signalType, const Uint8 numParams, const Uint32 paramA/*=0*/, const Uint32 paramB/*=0*/, const Uint32 paramC/*=0*/, const Uint32 paramD/*=0*/, const Uint32 paramE/*=0*/, const Uint32 paramF/*=0*/  )
{
	// check definition
	RED_ASSERT( m_signals[signalType].m_defined, TXT("Using undefined signal %d"), signalType );
	if ( !m_signals[signalType].m_defined )
		return;

	// check parameter count
	RED_ASSERT( m_signals[signalType].m_numParams == numParams, TXT("Parameter count mismatch for signal %ls, expected %d, provided %d"), 
		ANSI_TO_UNICODE( m_signals[signalType].m_name.AsChar() ), m_signals[signalType].m_numParams, numParams );
	if ( m_signals[signalType].m_numParams != numParams )
		return;

	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 messageSize = (sizeof(Message)-sizeof(Uint32)) + sizeof(Uint32) * numParams;
	Message* message = (Message*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_Signal;
	message->m_id = signalType;
	message->m_threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	message->m_timestamp = Red::Clock::GetInstance().GetTimer().GetTicks();

	// copy data
	if ( numParams >= 1 ) message->m_data[0] = paramA;
	if ( numParams >= 2 ) message->m_data[1] = paramB;
	if ( numParams >= 3 ) message->m_data[2] = paramC;
	if ( numParams >= 4 ) message->m_data[3] = paramD;
	if ( numParams >= 5 ) message->m_data[4] = paramE;
	if ( numParams >= 6 ) message->m_data[5] = paramF;

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::CounterIncrement( const TCounterID counterType, const Uint32 value )
{
	// check definition
	RED_ASSERT( m_counter[counterType].m_defined, TXT("Using undefined counter %d"), counterType );
	if ( !m_counter[counterType].m_defined )
		return;

	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 messageSize = sizeof(Message);// + sizeof(Uint32);
	Message* message = (Message*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_IncrementCounter;
	message->m_id = counterType;
	message->m_threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	message->m_timestamp = Red::Clock::GetInstance().GetTimer().GetTicks();
	message->m_data[0] = value;

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::CounterDecrement( const TCounterID counterType, const Uint32 value )
{
	// check definition
	RED_ASSERT( m_counter[counterType].m_defined, TXT("Using undefined counter %d"), counterType );
	if ( !m_counter[counterType].m_defined )
		return;

	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 messageSize = sizeof(Message);// + sizeof(Uint32);
	Message* message = (Message*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_DecrementCounter;
	message->m_id = counterType;
	message->m_threadId = Red::System::Internal::ThreadId::CurrentThread().AsNumber();
	message->m_timestamp = Red::Clock::GetInstance().GetTimer().GetTicks();
	message->m_data[0] = value;

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::WriteBlockDefine( const TBlockID id, const BlockInfo& info )
{
	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 nameLength = info.m_name.GetLength();
	const Uint32 messageSize = sizeof(Entry) + nameLength;
	Entry* message = (Entry*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_DefBlock;
	message->m_id = id;
	message->m_numParams1 = (Uint8)info.m_numStartParams;
	message->m_numParams2 = (Uint8)info.m_numEndParams;
	message->m_nameLength = nameLength;

	// name
	Red::MemoryCopy( (Uint8*) message + sizeof(Entry), info.m_name.AsChar(), nameLength );

	// finish message
	m_writer->FinishMessage();	
}

void CProfilerBlockFileWriter::WriteSignalDefine( const TSignalID id, const SignalInfo& info )
{
	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 nameLength = info.m_name.GetLength();
	const Uint32 messageSize = sizeof(Entry) + nameLength;
	Entry* message = (Entry*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_DefSignal;
	message->m_id = id;
	message->m_numParams1 = (Uint8)info.m_numParams;
	message->m_numParams2 = 0;
	message->m_nameLength = nameLength;

	// name
	Red::MemoryCopy( (Uint8*) message + sizeof(Entry), info.m_name.AsChar(), nameLength );

	// finish message
	m_writer->FinishMessage();
}

void CProfilerBlockFileWriter::WriteCounterDefine( const TCounterID id, const CounterInfo& info )
{
	// no writer
	if ( !m_writer )
		return;

	// prepare message
	const Uint32 nameLength = info.m_name.GetLength();
	const Uint32 messageSize = sizeof(Entry) + nameLength;
	Entry* message = (Entry*) m_writer->AllocMessage( messageSize );

	// message header
	message->m_type = eEvent_DefCounter;
	message->m_id = id;
	message->m_numParams1 = 1;
	message->m_numParams2 = 0;
	message->m_nameLength = nameLength;

	// name
	Red::MemoryCopy( (Uint8*) message + sizeof(Entry), info.m_name.AsChar(), nameLength );

	// finish message
	m_writer->FinishMessage();
}