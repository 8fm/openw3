#pragma once

#include "boidPointOfInterestComponent.h"


class CBoidLairParams;
class CSwarmSoundConfig;
typedef TDynArray< CBoidLairParams * > CBoidLairParamsArray;

/// Container for swarm parameters
class CBoidSpecies
{
public:
	CBoidSpecies();
	virtual ~CBoidSpecies();

	void PreloadTemplates();
	void UnloadTemplates();
	void InitParams(CDefinitionsManager *const definitionManager);
	
	void GetBoidTemplatePaths( TDynArray<String>& output );

	void							AddParams( CBoidLairParams *const params );
	const CBoidLairParams *const	GetParamsByName( CName name )const;
	void							AddBaseBoidNode( CBaseBoidNode *const baseBoidNode ){ m_baseBoidNodeArray.PushBack(baseBoidNode); }
	void							AddPointOfInterestConfig( CPointOfInterestSpeciesConfig *const pointOfInterestConfig ){ m_pointOfInterestConfigArray.PushBack(pointOfInterestConfig); }
	void							AddSoundConfig( const CSwarmSoundConfig *const swarmSoundConfig ){ m_soundConfigArray.PushBack( swarmSoundConfig ); }
private:
	CBoidLairParamsArray				m_boidLairParamsArray;
	/// All boid nodes are stored here for proper destruction
	CBaseBoidNode_Array					m_baseBoidNodeArray;
	/// All POI config are stored here for proper destruction
	CPointOfInterestSpeciesConfig_Array	m_pointOfInterestConfigArray;
	/// All sound config stored here for easy deletion 
	CSwarmSoundConfig_CPointerArray		m_soundConfigArray;

	virtual CBoidLairParams* NewBoidSpeciesParams( CName className ) = 0;
	void ParseSpecies(const SCustomNode & defNode, const CBoidLairParams *const parentParams = NULL);

};