#include "build.h"
#include "humbleCritterUpdateJob.h"
#include "humbleCritterAlgorithmData.h"


void CHumbleCrittersUpdateJob::UpdateSwarm()
{
	CHumbleCrittersAlgorithmData* worldData = static_cast< CHumbleCrittersAlgorithmData* >( m_swarmAlgorithmData );
	worldData->Update( m_currentState, m_targetState );
}