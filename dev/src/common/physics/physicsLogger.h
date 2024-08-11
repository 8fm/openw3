#pragma once

#ifndef RED_PLATFORM_ORBIS
class PcScopePhysicsLogger
{
protected:
	double m_time;
	const char* m_name;
public:
	PcScopePhysicsLogger( const char* name );
	~PcScopePhysicsLogger();
};
#endif

