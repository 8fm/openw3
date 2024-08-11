/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _FILESYS_PROFILER_WRAPPER_INL_H__
#define _FILESYS_PROFILER_WRAPPER_INL_H__

//----

void CFileManagerProfiler::Start( const EBlockType type )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 0 );
}

void CFileManagerProfiler::Start( const EBlockType type, const Uint32 paramA )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 1, paramA );
}

void CFileManagerProfiler::Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 2, paramA, paramB );
}

void CFileManagerProfiler::Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 3, paramA, paramB, paramC );
}

void CFileManagerProfiler::Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 4, paramA, paramB, paramC, paramD );
}

void CFileManagerProfiler::Start( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE )
{
	m_writer.Start( (CProfilerBlockFileWriter::TBlockID)type, 5, paramA, paramB, paramC, paramD, paramE );
}

//----

void CFileManagerProfiler::End( const EBlockType type )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 0 );
}

void CFileManagerProfiler::End( const EBlockType type, const Uint32 paramA )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 1, paramA );
}

void CFileManagerProfiler::End( const EBlockType type, const Uint32 paramA, const Uint32 paramB )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 2, paramA, paramB );
}

void CFileManagerProfiler::End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 3, paramA, paramB, paramC );
}

void CFileManagerProfiler::End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 4, paramA, paramB, paramC, paramD );
}

void CFileManagerProfiler::End( const EBlockType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE )
{
	m_writer.End( (CProfilerBlockFileWriter::TBlockID)type, 5, paramA, paramB, paramC, paramD, paramE );
}

//----

void CFileManagerProfiler::Signal( const ESignalType type )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 0 );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 1, paramA );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 2, paramA, paramB );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 3, paramA, paramB, paramC );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 4, paramA, paramB, paramC, paramD );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 5, paramA, paramB, paramC, paramD, paramE );
}

void CFileManagerProfiler::Signal( const ESignalType type, const Uint32 paramA, const Uint32 paramB, const Uint32 paramC, const Uint32 paramD, const Uint32 paramE, const Uint32 paramF )
{
	m_writer.Signal( (CProfilerBlockFileWriter::TSignalID)type, 6, paramA, paramB, paramC, paramD, paramE, paramF );
}

//---

void CFileManagerProfiler::CounterIncrement( const ECounterType counter, const Uint32 value )
{
	m_writer.CounterIncrement( (CProfilerBlockFileWriter::TCounterID) counter, value );
}

void CFileManagerProfiler::CounterDecrement( const ECounterType counter, const Uint32 value )
{
	m_writer.CounterDecrement( (CProfilerBlockFileWriter::TCounterID) counter, value );
}

//----

#endif