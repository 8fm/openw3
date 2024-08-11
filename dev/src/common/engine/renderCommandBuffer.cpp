/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCommandBuffer.h"
#include "../core/configVar.h"

const Uint32 CRenderCommandBuffer::MARGIN = 16000;

const Uint32 CRenderCommandBuffer::CMD_JTS = 'JTS0';
const Uint32 CRenderCommandBuffer::CMD_RESET = 'REST';
const Uint32 CRenderCommandBuffer::CMD_EXECUTE = 'EXEC';

namespace Config
{
	TConfigVar< Int32, Validation::IntRange<32,INT_MAX> >		cvCommandBufferSize( "Rendering", "CommandBufferSizeKB", 0, eConsoleVarFlag_ReadOnly ); //real value read from ini file
}

namespace Access
{
	static const Uint32 ReadData( const void* base, const Uint32 offset )
	{
		void* ptr = OffsetPtr( (void*)base, offset );
		RED_THREADS_MEMORY_BARRIER();
		Uint32 ret = Red::Threads::AtomicOps::Or32( (Red::Threads::AtomicOps::TAtomic32*)ptr, 0 );
		RED_THREADS_MEMORY_BARRIER();
		return ret;
	}

	static void WriteData( void* base, const Uint32 offset, const Uint32 data )
	{
		void* ptr = OffsetPtr( base, offset );
		RED_THREADS_MEMORY_BARRIER();
		Red::Threads::AtomicOps::Exchange32( (Red::Threads::AtomicOps::TAtomic32*)ptr, (Red::Threads::AtomicOps::TAtomic32)data );
		RED_THREADS_MEMORY_BARRIER();
	}
}

CRenderCommandBuffer::CRenderCommandBuffer()
	: m_writePos( 0 )
	, m_dataSize( 0 )
	, m_dataCount( 0 )
{
	// allocate memory for command buffer
	const Uint32 memorySize = Config::cvCommandBufferSize.Get() * 1024;
	m_buffer = (Uint8*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_RenderCommandBuffer, memorySize, 16 );

#ifndef RED_FINAL_BUILD
	Red::MemorySet( m_buffer, memorySize, 0xAB ); // security check
#endif

	// we are allowed to write a little bit less than the size of the memory
	m_maxSize = memorySize - 6*MARGIN; // each writing threads needs it's own margin
	m_writeEnd = memorySize - MARGIN; // actual wrapping needs only small margin

	// write the initial JTS that will act as the guard
	*(volatile Uint32*) m_buffer = CMD_JTS;
	m_readPtr = m_buffer;
}

CRenderCommandBuffer::~CRenderCommandBuffer()
{
	// Free memory
	if ( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_RenderCommandBuffer, m_buffer );
		m_buffer = nullptr;
	}
}

Bool CRenderCommandBuffer::HasData()
{
	RED_FATAL_ASSERT( ((Uint64)m_readPtr & 0xF) == 0, "Read address is not aligned" );

	// make sure command is written
	RED_THREADS_MEMORY_BARRIER();
	const Uint32 cmd = Access::ReadData( m_readPtr, 0 );

	// jump to new page
	if ( cmd == CMD_RESET )
	{
		m_readPtr = m_buffer;
		return false; // Reset does not count as valid command, wait for the proper CMD_EXECUTE
	}

	// is this an execution command ? (we can only directly execute those)
	RED_FATAL_ASSERT( cmd == CMD_JTS || cmd == CMD_EXECUTE, "Invalid command buffer token in the render command buffer" );
	return cmd == CMD_EXECUTE;
}

void CRenderCommandBuffer::EnsureSpace( const Uint32 size )
{
	// Protect from running out of space
	const Int32 sizeAllocated = size + 16;
	RED_THREADS_MEMORY_BARRIER();
	const Int32 sizeAfterCommand = m_dataSize.ExchangeAdd( sizeAllocated );
	RED_THREADS_MEMORY_BARRIER();
	if ( sizeAfterCommand >= (Int32) m_maxSize )
	{
		Uint32 iter = 0;
		while ( 1 )
		{
			ERR_ENGINE( TXT("!!! RENDER COMMAND BUFFER CONTENTION !!! Cmds=%d, Size=%d, Iter=%d, Thread=%d"), 
				m_dataCount.GetValue(), sizeAfterCommand, iter++, 
				Red::System::Internal::ThreadId::CurrentThread().AsNumber() );

			// release allocation
			RED_THREADS_MEMORY_BARRIER();
			m_dataSize.ExchangeAdd( -sizeAllocated );
			RED_THREADS_MEMORY_BARRIER();
			Red::Threads::SleepOnCurrentThread( 10 );

			// retry
			RED_THREADS_MEMORY_BARRIER();
			const Int32 sizeAfterCommandSafe = m_dataSize.ExchangeAdd( sizeAllocated );
			RED_THREADS_MEMORY_BARRIER();
			if ( sizeAfterCommandSafe < (Int32) m_maxSize )
				break;
		}
	}

	// Count commands
	RED_THREADS_MEMORY_BARRIER();
	const Int32 countAfterCommand = m_dataCount.Increment();
	RED_THREADS_MEMORY_BARRIER();
	RED_FATAL_ASSERT( countAfterCommand >= 1, "Invalid commandline count" );
}

void* CRenderCommandBuffer::Alloc( Uint32 unalignedSize )
{
	RED_FATAL_ASSERT( unalignedSize < MARGIN, "Render command size (%d) is larger than the preallocated margin (%d). This is not safe, increase the margin.", unalignedSize, MARGIN );

	// Align size
	const Uint32 size = (Uint32) AlignOffset( unalignedSize, 16 );

	// Ensure that we have enough space in the command buffer
	EnsureSpace( size );

	// Allocate writing pos
	Uint32 unwrappedWritePos;
	Uint32 writePos;
	{
		while (1)
		{
			RED_THREADS_MEMORY_BARRIER();
			unwrappedWritePos = writePos = m_writePos.GetValue();
			if ( writePos + size >= (Uint32)m_writeEnd )
				writePos = 0;

			// advance pos
			const Uint32 newPos = writePos + size + 16;
			RED_THREADS_MEMORY_BARRIER();
			if ( unwrappedWritePos == m_writePos.CompareExchange( newPos, unwrappedWritePos ) )
				break;
			RED_THREADS_MEMORY_BARRIER();
		}

		// Add the guard byte at the end (unlock other threads)
		Access::WriteData( m_buffer, writePos + 16 + size, CMD_JTS );

		// Make sure we still have our guard byte before continuing - this ensured order of operation in the writing list
		Uint32 spinCount = 0;
		while ( 1 )
		{
			const Uint32 jts = Access::ReadData( m_buffer, unwrappedWritePos );
			if ( jts == CMD_JTS )
			{
				break;
			}
			else
			{
				// W3: anti lock hack
				// We have a case when StreamingThread (aff: b110000) prio: 710 is preemptied
				// by the job threads (aff: b100000, prio: 680) and we have basically a deadlock
				// In such conditions this hack is unlocking the deadlock by temporarly sleeping the thread that is waiting
				if ( spinCount++ > 100000 )
				{
#ifdef RED_PLATFORM_ORBIS
	#ifdef RED_FINAL_BUILD
					Red::Threads::SleepOnCurrentThread(1);
	#else
					int prio = 0;
					scePthreadGetprio( scePthreadSelf(), &prio );

					SceKernelCpumask aff = 0;
					scePthreadGetaffinity( scePthreadSelf(), &aff );

					CTimeCounter timer;
					Red::Threads::SleepOnCurrentThread(1);

					LOG_ENGINE( TXT("SHIT! TID: %d, PRIO: %d, AFF: 0x%llX, RES: %1.2fms"), 
						Red::System::Internal::ThreadId::CurrentThread().AsNumber(),
						prio, aff, timer.GetTimePeriodMS() );
	#endif
#else
					Red::Threads::SleepOnCurrentThread(1);
#endif
				}
			}
		}
	}

	// We wrapped the buffer, reset the reader
	RED_FATAL_ASSERT( (writePos & 0xF) == 0, "Write position should be aligned" );
	if ( writePos == 0 && unwrappedWritePos != 0 )
	{
		// write the JTS in a NEW place
		Access::WriteData( m_buffer, writePos, CMD_JTS );

		// write the reset command to rewind the buffer to the beginning
		Access::WriteData( m_buffer, unwrappedWritePos, CMD_RESET );
	}

	// Make sure we have the Write temporary JTS - guard command
	Access::WriteData( m_buffer, writePos + 4, size ); // size of the data written (needed for CMD_EXECUTE)

	// Remember where's the place to write the render command
	void* mem = (Uint8*)(m_buffer + writePos + 16);

	// Use the allocated command space, the guard will be cleared in the following Commit
	return mem;
}

void CRenderCommandBuffer::Commit( void* mem )
{
	// find the command guard byte
	Uint8* guardMem = (Uint8*) OffsetPtr( mem, -16 );
	RED_FATAL_ASSERT( ((Uint64)guardMem & 0xF) == 0, "Write address is not aligned" );
	const Uint32 cmdSize = Access::ReadData( guardMem, 4 );

	// clear the guard (JTS) and convert it to EXECUTE - we can run our command now
	RED_FATAL_ASSERT( Access::ReadData( guardMem, 0 ) == CMD_JTS, "Command data guard corrupted on writing" );
	Access::WriteData( guardMem, 0, CMD_EXECUTE );
}

void* CRenderCommandBuffer::ReadData()
{
	RED_FATAL_ASSERT( ((Uint64)m_readPtr & 0xF) == 0, "Read address is not aligned" );
	RED_FATAL_ASSERT( Access::ReadData( m_readPtr, 0 ) == CMD_EXECUTE, "BeingRead called on something that is not a valid command to execute" );

	// Get size of the data in this command
	const Uint32 size = Access::ReadData( m_readPtr, 4 );
	RED_FATAL_ASSERT( (size & 0xF) == 0, "Writen size should be aligned" );

	// The moment command is accepted clear the marker from it
	Access::WriteData( (void*)m_readPtr, 0, '----' );
	Access::WriteData( (void*)m_readPtr, 4, '----' );
	Access::WriteData( (void*)m_readPtr, 8, '----' );
	Access::WriteData( (void*)m_readPtr, 12, '----' );

	// Advance read pointer past the command
	void* readData = (void*)( m_readPtr + 16 );
	m_readPtr = m_readPtr + 16 + size;

	// Validate stuff after movement
	const Uint32 cmd2 = Access::ReadData( m_readPtr, 0 );
	RED_FATAL_ASSERT( cmd2 == CMD_JTS || cmd2 == CMD_EXECUTE || cmd2 == CMD_RESET, "Invalid command buffer state after reading" );

	// Uncount size
	const Int32 sizeAllocataed = size + 16;
	RED_THREADS_MEMORY_BARRIER();
	const Int32 orgSize = m_dataSize.ExchangeAdd( -sizeAllocataed );
	RED_THREADS_MEMORY_BARRIER();
	RED_FATAL_ASSERT( orgSize >= 0, "Invalid command buffer size" );

	// Uncount commands
	RED_THREADS_MEMORY_BARRIER();
	const Int32 orgCount = m_dataCount.Decrement();
	RED_THREADS_MEMORY_BARRIER();
	RED_FATAL_ASSERT( orgCount >= 0, "Invalid command buffer count" );
	
	// Return pointer to command data
	return readData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

