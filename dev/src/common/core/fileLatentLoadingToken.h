/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// A token that is capable for resuming a load of a resource from a particular place
class IFileLatentLoadingToken
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
protected:
	Uint64 m_offset;

public:
	IFileLatentLoadingToken( Uint64 offset ) : m_offset( offset ) {}
	virtual ~IFileLatentLoadingToken() {};

	//! Resume loading, returns valid IFile that can be used to continue file loading
	virtual IFile* Resume( Uint64 relativeOffset )=0;

	//! Clone token. Required as a token can be passed between threads.
	virtual IFileLatentLoadingToken* Clone() const = 0;

	RED_INLINE Uint64 GetOffset() const { return m_offset; }

	//! Describe loading token
	virtual String GetDebugInfo() const=0;
};
