#include "build.h"
#include "physicsLogger.h"

#ifndef RED_PLATFORM_ORBIS
void PhysicsLogger( const char* format, ... )
{
	char buffer[ 1024 * 80 ];
	Red::System::MemoryZero( buffer, 1024 * 80 );
	va_list arglist;
	va_start( arglist, format );
	Red::System::VSNPrintF( buffer, 1024 * 80, format, arglist );
	va_end( arglist );
	OutputDebugStringA( buffer );
}

PcScopePhysicsLogger::PcScopePhysicsLogger( const char* name ) : m_name( name )
{

}

PcScopePhysicsLogger::~PcScopePhysicsLogger()
{
	double endTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	static float amount = 0.002f;
	if( ( float )endTime - m_time > amount)
	{
		PhysicsLogger( " %s %f threadid: %i \0", m_name, (float)(endTime - m_time) * 1000, Red::System::Internal::ThreadId::CurrentThread().id );
		PhysicsLogger( "\n" );
	}
}
#endif