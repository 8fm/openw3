/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef IO_BUFFER_ACQUIRE_CONTEXT_H_
#define IO_BUFFER_ACQUIRE_CONTEXT_H_
#include "bufferSpan.h"

typedef TDynArray< CBufferSpan > BufferSpanCollection;

class CIOBufferAcquireContext : public Red::System::NonCopyable
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Constructor
	//////////////////////////////////////////////////////////////////////////
	CIOBufferAcquireContext( const Uint32 bufferSize, const Uint32 flags, const Uint32 spanCount = 0 );

	//////////////////////////////////////////////////////////////////////////
	// Destructor
	//////////////////////////////////////////////////////////////////////////
	~CIOBufferAcquireContext();

	//////////////////////////////////////////////////////////////////////////
	// Public Methods.
	//////////////////////////////////////////////////////////////////////////
	void AddBufferSpan( const Uint32 start, const Uint32 end );
	const Uint32 GetBufferSpanCount() const;
	const Uint32 GetSize() const;
	const Uint32 GetFlags() const;
	const BufferSpanCollection& GetBufferSpans() const;

private:
	const Uint32 m_requiredSize;
	const Uint32 m_flags;
	BufferSpanCollection m_bufferSpans;
};

#endif // IO_BUFFER_ACQUIRE_CONTEXT_H_