#pragma once

#include "../redio/redIO.h"

#include "staticarray.h"
#include "ioTags.h"

/// Resolver for IO tags
class CFileSystemPriorityResovler
{
public:
	CFileSystemPriorityResovler();

	// Initialize priority tables, can fail
	Bool InitializePriorityTables();

	// Switch IO context
	void SwitchConext( const EIOContext context );

	// Resolve tag into actual priority
	const Red::IO::EAsyncPriority Resolve( const EIOTag tag ) const;

private:
	// tables
	Uint8		m_tables[ eIOContext_MAX ][ eIOTag_MAX ];

	// current context
	volatile EIOContext	m_context;

	// internal flag
	volatile Bool m_initialized;
};

/// Priority table system
extern CFileSystemPriorityResovler GFileSysPriorityResovler;