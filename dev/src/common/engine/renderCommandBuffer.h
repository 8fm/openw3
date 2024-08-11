/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Command buffer access classes
class CCommandBufferWriter;
class CCommandBufferReader;

#ifdef RED_PLATFORM_ORBIS
typedef Red::MemoryFramework::CAdaptiveMutexBase< Red::MemoryFramework::OSAPI::CAdaptiveMutexImpl > CAdaptiveMutex;
#endif

/// Command buffer for rendering thread
class CRenderCommandBuffer
{
	friend class CCommandBufferWriter;
	friend class CCommandBufferReader;

public:
	CRenderCommandBuffer();
	~CRenderCommandBuffer();

	//! Returns true if there are data in the buffer
	Bool HasData();

	//! Allocate command data, will block if there is no space
	void* Alloc( Uint32 size );

	//! Commit command that was just written
	void Commit( void* mem );

	//! Read block of data, will block if there is no data
	void* ReadData();

private:
	const static Uint32		MARGIN; // command buffer margin

	const static Uint32		CMD_JTS; // wait
	const static Uint32		CMD_RESET; // page switch
	const static Uint32		CMD_EXECUTE; // command follows

	Uint32							m_maxSize;				//!< Maximum allowed size of data in the command buffer
	Red::Threads::CAtomic< Int32 >	m_dataSize;				//!< Size of data in the command buffer
	Red::Threads::CAtomic< Int32 >	m_dataCount;			//!< Number of blocks in the command buffer

	Uint8*							m_buffer;				//!< Memory buffer

#ifdef RED_PLATFORM_ORBIS
	CAdaptiveMutex					m_writeLock;			//!< Writing lock, use adaptive mutex on PS4
#else
#endif

	Red::Threads::CAtomic< Int32 >	m_writePos;				//!< Write pointer
	Int32							m_writeEnd;				//!< End of the allowed write region (wraps)

	const Uint8*					m_readPtr;				//!< Read pointer

	void EnsureSpace( const Uint32 size );
};
