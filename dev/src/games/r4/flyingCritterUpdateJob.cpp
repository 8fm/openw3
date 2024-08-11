#include "build.h"
#include "flyingCritterUpdateJob.h"
#include "flyingCritterAlgorithmData.h"


void CFlyingCrittersUpdateJob::UpdateSwarm()
{
	CFlyingCrittersAlgorithmData* worldData = static_cast< CFlyingCrittersAlgorithmData* >( m_swarmAlgorithmData );
	worldData->Update( m_currentState, m_targetState );
}