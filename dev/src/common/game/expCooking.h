/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "expManager.h"

// initial version of exploration cooked data
#define EXPLORATION_DATA_VERSION_INITIAL	1

// mediator data saved properly
#define EXPLORATION_DATA_VERSION_MEDIATOR	2

// current version of exploration cooked data
#define	EXPLORATION_DATA_VERSION_CURRENT	EXPLORATION_DATA_VERSION_MEDIATOR

// it's magic!
#define EXPLORATION_DATA_MAGIC 'LPXE' /* EXPL */

#ifndef NO_EDITOR
#include "../../common/engine/pathlibCookerData.h"

class CExplorationCookingContext : public INavigationCookingSystem
{
	ExpManager							m_manager;
	CDirectory*							m_outputDirectory;
	String								m_worldFileName;
	CCookedExplorations*				m_resource;
	TDynArray< CCookedExploration* >	m_exps;
	ExpManager::TExpLayerMap			m_layerMap;

public:
	static const Uint32 COOKING_SYSTEM_ID = 0xf00d;

	struct SStats
	{
		Uint32 m_numExps;
		Uint32 m_numDynamic;
		Uint32 m_treeMem;
	};

	CExplorationCookingContext( const CWorld& world, CDirectory* directory );
	~CExplorationCookingContext();

	void OnExploration( CExplorationComponent* e );

	void GetStats( SStats& stats ) const;
	DataBuffer* CookToDataBuffer();

	void CookToFile( IFile& file );

	virtual Bool CommitOutput() override;

private:
	Uint32 ComputeCookedDataSize() const;
	Uint32 ComputeLayerDataSize() const;
	void BindExpWithLayer( Int32 expIndex, const CLayer* layer );
};
#endif // NO_EDITOR
