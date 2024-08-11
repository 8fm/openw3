#include "build.h"
#include "r4ActionPointSelector.h"
#include "../../common/game/communitySystem.h"

IMPLEMENT_ENGINE_CLASS( CHorseParkingActionPointSelector )

/////////////////////////////////////////////////////////////////////////////////////////////
// CHorseParkingActionPointSelector
////////////////////////////////////////////////////////////////////////////////////////
CHorseParkingActionPointSelector::CHorseParkingActionPointSelector()
	: m_apTags( )
	, m_radius( 20.0f )
{
	m_apTags.AddTag( CNAME( tag_horse_parking ) );
}
CActionPointSelectorInstance* CHorseParkingActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CHorseParkingActionPointSelectorInstance( *this );
}
/////////////////////////////////////////////////////////////////////////////////////////////
// CHorseParkingActionPointSelectorInstance
////////////////////////////////////////////////////////////////////////////////////////
CHorseParkingActionPointSelectorInstance::CHorseParkingActionPointSelectorInstance( const CHorseParkingActionPointSelector& def )
		: CActionPointSelectorInstance()
		, m_apTags( def.m_apTags )
{
}

Bool CHorseParkingActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{
	SActionPointFilter apFilter;

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == nullptr )
	{
		return false;
	}

	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
	if ( actionPointManager == nullptr )
	{
		return false;
	}

	const CNewNPC *const npc		= behTreeInstance->GetNPC();
	const Vector & horsePosition	= npc->GetWorldPositionRef();

	apFilter.m_category			= CNAME( horse );
	apFilter.m_onlyFree			= true;
	apFilter.m_actionPointTags	= m_apTags;
	apFilter.m_matchAll			= false;

	Float radius = 30.0f;
	apFilter.m_sphere = Sphere( horsePosition, radius );

	CActionPointManager::APFilteredIterator it( *actionPointManager, apFilter );
	Float shortestSquareDist = NumericLimits< Float >::Max();
	
	while ( it )
	{
		const CActionPointComponent* ap	= *it;
		const Float squareDist = ( ap->GetGoToPosition() - horsePosition ).SquareMag3();

		if ( squareDist < shortestSquareDist )
		{
			shortestSquareDist = squareDist;
			actionPointId		= ap->GetID();
			actionPointCategory = CNAME( horse );
		}
		++it;
	}
	if ( shortestSquareDist != NumericLimits< Float >::Max() )
	{
		return true;
	}
	return false;
}
