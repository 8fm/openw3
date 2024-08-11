#include "build.h"
#include "boidManager.h"


void CBoidManager::AddLair( IBoidLairEntity* lair )
{
	m_lairs.PushBackUnique( lair );
}
void CBoidManager::RemoveLair( IBoidLairEntity* lair )
{
	m_lairs.Remove( lair );
}


CBoidManager g_boidManager;

CBoidManager* CBoidManager::GetInstance()
{
	return &g_boidManager;
}
