/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SAIParametersSpawnList
{
	THandle< IAIParameters >							m_spawnSystemParameters;
	TStaticArray< THandle< IAIParameters >, 16 >		m_list;

	SAIParametersSpawnList()															{}
	~SAIParametersSpawnList()															{}

	void Clear();
};