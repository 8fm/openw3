/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiParamInjectHandler.h"

#include "aiParametersSpawnList.h"

void CAiParamInjectHandler::InjectAIParams( SAIParametersSpawnList& aiList )
{
	ASSERT( aiList.m_list.Size() < aiList.m_list.Capacity(), TXT("Ride SAIParametersSpawnList::m_list capacity limit!") );
	IAIParameters* ai =  m_injectedParams.Get();
	if( ai && aiList.m_list.Size() < aiList.m_list.Capacity() )
	{
		aiList.m_list.PushBack( ai );
	}
}
void CAiSpawnSystemParamInjectHandler::InjectAIParams( SAIParametersSpawnList& aiList )
{
	aiList.m_spawnSystemParameters = m_injectedParams;
}