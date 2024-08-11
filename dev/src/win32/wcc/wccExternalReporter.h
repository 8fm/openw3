/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CookerExternalReporter //: public IOutputDevice
{
protected:
	String				m_resource;			//!< Current resource

public:
	CookerExternalReporter();
	~CookerExternalReporter();

	//! Set current resource being cooked
	void SetCookedResource( const String& resource );

	//! Send status update
	void SendStatus( const TCHAR* txt, ... );

	//! Send progress update
	void SendProgress( Uint32 cur, Uint32 total );

	//! Send error message
	void SendError( const TCHAR* txt, ... );

	//! Send raw error message
	void SendErrorRaw( const TCHAR* txt );

	//! Send warning message
	void SendWarning( const TCHAR* txt, ... );

	//! Send raw warning message
	void SendWarningRaw( const TCHAR* txt );

protected:
	void Reconnect();

protected:
	//virtual void Write( const CName& type, const Char* str, const Uint64& tick, const EngineTime& engineTime );
};

extern CookerExternalReporter* GExternalReporter;
