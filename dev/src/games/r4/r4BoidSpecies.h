#pragma once
#include "../../common/game/swarmLairEntity.h"
#include "r4SwarmUtils.h"

class CGroupState;
class CFlyingPoiConfig;
class CPoiConfigByGroup;

typedef TDynArray<CGroupState*>			CGroupState_Array;
typedef TDynArray<CFlyingPoiConfig*>	CFlyingPoiConfig_Array;

class CR4BoidSpecies : public CBoidSpecies
{
public:
	CR4BoidSpecies();
	virtual ~CR4BoidSpecies();

	virtual CBoidLairParams* NewBoidSpeciesParams( CName className );
	void	AddGroupState( CGroupState *const groupState ){ m_groupStateArray.PushBack( groupState );}
	void	AddFlyingPoiConfig( CFlyingPoiConfig *const flyingPoiConfig ){ m_flyingPoiConfigArray.PushBack( flyingPoiConfig ); }
	void	AddPoiConfigByGroup( CPoiConfigByGroup *const poiConfigByGroup ){ m_poiConfigByGroupArray.PushBack( poiConfigByGroup ); }
private :
	/// Book keeping array for deleting properly
	CGroupState_Array				m_groupStateArray;
	CFlyingPoiConfig_Array			m_flyingPoiConfigArray;
	CPoiConfigByGroup_CPointerArray	m_poiConfigByGroupArray;

};