#include "build.h"

#include "rainActionPointSelector.h"

IMPLEMENT_ENGINE_CLASS( CRainActionPointSelector )

RED_DEFINE_STATIC_NAME( rain_shelter_man )
RED_DEFINE_STATIC_NAME( rain_shelter_woman )
RED_DEFINE_STATIC_NAME( rain_shelter_child )
RED_DEFINE_STATIC_NAME( rain_shelter_dwarf )



CActionPointSelectorInstance* CRainActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CRainActionPointSelectorInstance( *this, context );
}


CRainActionPointSelectorInstance::CRainActionPointSelectorInstance( CRainActionPointSelector& def, CBehTreeSpawnContext& context )
	: CWanderActionPointSelectorInstance( def, context )
	, m_categoryInitialized( false )
{	
	m_reset				= true;	
	m_ignoreDirection	= true;
	m_chooseClosestAP	= true;
	m_radius			= 30;
}

Bool CRainActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{	
	InitializeCategoty( behTreeInstance );

	return Super::SelectActionPoint(behTreeInstance, workDataPtr, actionPointId, lastActionPointId, actionPointCategory, loopInAP, forceRadius );
}

void CRainActionPointSelectorInstance::InitializeCategoty( const CBehTreeInstance* behTreeInstance )
{
	if( m_categoryInitialized )
		return;

	CActor* actor = behTreeInstance->GetActor();
	if( !actor )
		return;

	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if( !mac )
		return;

	const String& macName = mac->GetName();

	CName category( CNAME( rain_shelter_man ) );

	if( macName == String( TXT( "woman_base" ) ) )
	{
		category = CNAME( rain_shelter_woman );
	}
	else if( macName == String( TXT( "child_base" ) ) )
	{
		category = CNAME( rain_shelter_child );
	}
	else if( macName == String( TXT( "dwarf_base" ) ) )
	{
		category = CNAME( rain_shelter_dwarf );
	}	

	SEncounterActionPointSelectorPair cat;
	cat.m_chance	= 100;
	cat.m_name		= category;
	m_categories.PushBack( cat );

	m_apTags.AddTag( category );

	m_categoryInitialized = true;
}