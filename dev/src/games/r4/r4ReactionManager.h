/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/game/behTreeReactionData.h"
#include "../../common/game/behTreeReactionManager.h"
#include "../../common/game/actorsManager.h"
///////////////////////////////////////////////////////////////////////////////

class CWeatherManager;
class CCommonMapManager;

struct RainNotyfier
{
	RainNotyfier( CBehTreeReactionEventData* rainEventData, const Vector3& center, Float distance,  CCommonMapManager* mapManager )
		: m_rainEventData( rainEventData )
		, m_center( center )
		, m_distanceSqr( distance * distance )
		, m_mapManager( mapManager )
	{}


	Bool Notify( CNewNPC* npc );

	THandle< CBehTreeReactionEventData >	m_rainEventData;
	Vector3									m_center;
	Float									m_distanceSqr;
	CCommonMapManager*						m_mapManager;
};

class CR4ReactionManager : public CBehTreeReactionManager
{
	static const Int32 MAX_INTERIORS_SEARCH = 5;
	const static Float RAIN_REACTION_TREESHOLD;
	const static Float RAIN_REACTION_DISTANCE;

	DECLARE_ENGINE_CLASS( CR4ReactionManager, CBehTreeReactionManager, 0 );

private:
	Bool									m_rainReactionsEnabled;	
	THandle< CBehTreeReactionEventData >	m_rainEventParams;	

public:
	CR4ReactionManager()
		: m_rainReactionsEnabled( true )		
	{
	}
	
	virtual void Update() override;
protected:
	void FindActorsToBroadecast( TDynArray< TPointerWrapper< CActor > >& actors, CBehTreeReactionEventData& data ) override;
	void UpdateRaining();
private:
	CWeatherManager* GetWeatherManager(); 	
};

BEGIN_CLASS_RTTI( CR4ReactionManager );
	PARENT_CLASS( CBehTreeReactionManager );
	PROPERTY( m_rainReactionsEnabled );	
	PROPERTY( m_rainEventParams );
END_CLASS_RTTI();
