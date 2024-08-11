/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "task.h"
#include "math.h"

class IRedTelemetryServiceInterface;

class CRedTelemetryServiceUpdateJob : public CTask
{
public:
	CRedTelemetryServiceUpdateJob( IRedTelemetryServiceInterface* redTelemetryServiceInterface )
		: m_redTelemetryServiceInterface( redTelemetryServiceInterface )
	{
		RED_ASSERT( m_redTelemetryServiceInterface, TXT( "m_redTelemetryServiceInterface == NULL" ) );
	}

protected:
	//! Process the job, is called from job thread
	void Run();

public:
	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT( "RedTelemetryServiceUpdate" ); };

	//! Get debug color
	virtual Uint32 GetDebugColor() const { return Color::LIGHT_YELLOW.ToUint32(); };

private:

	IRedTelemetryServiceInterface*	m_redTelemetryServiceInterface;
};