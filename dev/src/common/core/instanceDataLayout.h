/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"

// Instanced variable
class InstanceVar;
class InstanceBuffer;
class InstanceDataLayoutCompiler;

/// Data layout compiler
class InstanceDataLayout
{
	friend class InstanceBuffer;
	friend class InstanceDataLayoutCompiler;

	typedef TDynArray< const InstanceVar*, MC_InstanceBuffer >	TInstanceConstVarArray;
	typedef TDynArray< InstanceBuffer*, MC_InstanceBuffer >		TInstanceBufferArray;

protected:
	Uint32									m_size;				//!< Current size
	TInstanceConstVarArray					m_vars;				//!< Instance variables
	TInstanceConstVarArray					m_destructVars;		//!< Instance variables that need destructors to be called

#if !defined(NO_EDITOR)
	mutable TInstanceBufferArray			m_buffers;			//!< Buffers created from this data layout
	mutable Red::Threads::CMutex			m_bufferMutex;		//!< Mutex for operation on buffer array
#endif

public:
	//! Get size of compiled data
	RED_INLINE Uint32 GetSize() const { return m_size; }

public:
	InstanceDataLayout();
	~InstanceDataLayout();

	//! Create initialized instance buffer
	InstanceBuffer* CreateBuffer( CObject* owner, const String& info ) const;

	//! Change layout of data
	void ChangeLayout( const InstanceDataLayoutCompiler& newLayout );

protected:
	//! Clear data in given buffer
	void ClearBuffer( InstanceBuffer& buffer ) const;

	//! Copy data from one buffer to another
	void CopyBuffer( InstanceBuffer& dest, const InstanceBuffer& source ) const;

	//! Release data from given buffer
	void ReleaseBuffer( InstanceBuffer& buffer ) const;

	//! Serialize data from given buffer
	void SerializeBuffer( IFile& file, InstanceBuffer& buffer ) const;

	//! Validate variable; assert on failure
	void ValidateVariable( const InstanceVar* var ) const;
};