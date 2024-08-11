/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include <tracerecording.h>

// Trace dump code scope 
class CXenonScopeTracer
{
private:
	const char*		m_baseName;
	Bool			m_wasStarted;
	static Int		st_count;

public:
	CXenonScopeTracer( const char* name )
		: m_baseName( name )
		, m_wasStarted( false )
	{
	}

	~CXenonScopeTracer()
	{
		if ( m_wasStarted )
		{
			XTraceStopRecording();
		}
	}

	void Start()
	{
		if ( !m_wasStarted )
		{
			// Remember
			m_wasStarted = true;

			// Format grab file name
			char formatedCrap[ 256 ];
			sprintf( formatedCrap, "e:\\%s%i.pix2", m_baseName, st_count );
			++st_count;

			// Start grabbing
			XTraceStartRecording( formatedCrap );
		}
	}
};
