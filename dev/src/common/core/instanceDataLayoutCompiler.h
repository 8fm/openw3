/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "pair.h"

class InstanceVar;
class InstanceDataLayout;

// Array of instanced variables with their offsets
typedef TDynArray< TPair< InstanceVar*, Uint32 > > TInstanceVarList;

/// Compiles the layout of data buffer
class InstanceDataLayoutCompiler
{
protected:
	Uint32				m_currentSize;		//!< Current size
	TInstanceVarList	m_variables;		//!< Allocated variables with their offsets
	InstanceDataLayout* m_dataLayout;		//<! Used OMLY in game for direct building of variables list

public:
	//! Get the size so far
	RED_INLINE Uint32 GetCurrentSize() const { return m_currentSize; }

	//! Get the variable map
	RED_INLINE const TInstanceVarList& GetVariables() const { return m_variables; }

public:
	InstanceDataLayoutCompiler( InstanceDataLayout& dataLayout );

	//! Add the variable to the compiled layout
	InstanceDataLayoutCompiler& operator<<( InstanceVar& var );
};
