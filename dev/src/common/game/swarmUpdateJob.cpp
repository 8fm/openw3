#include "Build.h"
#include "swarmUpdateJob.h"


CSwarmUpdateJob::CSwarmUpdateJob( volatile CSwarmLairEntity* entity, const TSwarmStatesList currentState, TSwarmStatesList targetState, volatile CSwarmAlgorithmData* data )
	: m_lair( entity )
	, m_currentState( currentState )
	, m_targetState( targetState )
	, m_swarmAlgorithmData( const_cast< CSwarmAlgorithmData* >( data ) )
{

}

void CSwarmUpdateJob::Run()
{
	PC_SCOPE_PIX( CSwarmUpdateJob );

	UpdateSwarm();
	m_lair->OnJobFinished( this );
}

#ifndef NO_DEBUG_PAGES

const Char* CSwarmUpdateJob::GetDebugName() const
{
	return TXT("Swarm update");
}
Uint32 CSwarmUpdateJob::GetDebugColor() const
{
	return COLOR_UINT32( 0xff, 0xff, 0x00 );
}

#endif