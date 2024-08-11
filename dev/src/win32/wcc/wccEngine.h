/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/engine/platformViewport.h"

// This is a temporary interface and will be removed when the new platform projects come online
class CWccPlatformViewport : public IPlatformViewport
{
public:
	Bool PumpMessages()
	{
		return SPumpMessages();
	}
};

class CWccEngine : public CBaseEngine//, public IOutputDevice
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	Int32					m_numErrors;		// Number of errors encountered
	Int32					m_numWarnings;		// Number of warnings encountered
	HANDLE					m_stdOut;			// Std output
	TDynArray< String >		m_errors;			// Errors, each is reported once
	bool					m_rememberErrors;   // Do we have to remember passed errors
	CWccPlatformViewport	m_renderViewport;	// Platform -> Render bridge

public:
	//! Do we remember the errors
	RED_INLINE bool GetRememberErrors() const { return m_rememberErrors; };

	//! Toggle error history
	RED_INLINE void SetRememberErrors( bool rememberErrors ) { m_rememberErrors = rememberErrors; };

public:
	CWccEngine();
	~CWccEngine();

	// Cooker interface
	Bool Main( TDynArray< String > args );

	// IOutputDevice::Write
	//virtual void Write( const CName& type, const Char* str, const Uint64& tick, const EngineTime& engineTime );

	virtual Bool IsFPSDisplayEnabled() const override { return false; }

protected:
	
	// Enums names of all registered commandlets
	void EnumCommandletNames( TDynArray< CName > &commandletNames ) const;

	// Enums all commandlets as ICommandlet* TDynArray
	void EnumCommandlets( TDynArray< ICommandlet* > &commandlets ) const;

	// Prints out all commandlets with their oneliners
	void PrintCommandletsWithOneliners( ) const;

	// Returns registeres commandlet with desired name - NULL if it doesn't exist
	ICommandlet* GetCommandlet( const CName &name ) const;

	Bool IsPostInitialized() { return m_postInitialized; }

public:

	IPlatformViewport* GetPlatformViewport()
	{
		return &m_renderViewport;
	}
};


void SetGlobalProgress( Int32 current, Int32 max );
void SetGlobalStatus( const TCHAR* format, ... );
