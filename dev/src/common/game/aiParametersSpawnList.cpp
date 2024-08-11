/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiParametersSpawnList.h"


void SAIParametersSpawnList::Clear()
{
	m_spawnSystemParameters = nullptr;
	m_list.Clear();
}