#pragma once

#include "../../common/game/swarmSound.h"
class CFlyingSwarmGroup;

/// This filter uses the boid state from the critter ai to do its thing
class CGroupStateSwarmSoundFilter : public CBaseSwarmSoundFilter
{
public :
	CGroupStateSwarmSoundFilter();

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const override;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const override;
	Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const override;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )override;
private:
	CName			m_groupState;

public:
	enum ETypes
	{
		TYPE_GROUP_STATE_SWARM_SOUND_FILTER			= FLAG( 5 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_GROUP_STATE_SWARM_SOUND_FILTER, 
	};
};

class CR4SwarmSoundConfig : public CSwarmSoundConfig
{
public :
	CR4SwarmSoundConfig( ){}

	Bool FilterGroup( const CFlyingSwarmGroup & group )const;
	CBaseSwarmSoundFilter*const CreateXmlFromXmlAtt( const SCustomNodeAttribute & att )override;
};
