#pragma once
#include "../../common/game/actionPointSelectors.h"

//////////////////////////////////////////////////////////////////////////
// CHorseParkingActionPointSelector
///////////////////////////////////////////////////////////////////////////
class CHorseParkingActionPointSelectorInstance;
class CHorseParkingActionPointSelector : public CActionPointSelector
{
	friend class CHorseParkingActionPointSelectorInstance;
	DECLARE_RTTI_SIMPLE_CLASS( CHorseParkingActionPointSelector );

public:
	CHorseParkingActionPointSelector();
	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
protected:
	TagList				m_apTags;
	Float				m_radius;
};

BEGIN_CLASS_RTTI( CHorseParkingActionPointSelector );
	PARENT_CLASS( CActionPointSelector );
	PROPERTY_EDIT( m_apTags, TXT( "Optional list of required AP tags" ) );
	PROPERTY_EDIT( m_radius, TXT( "Radius that limits search for action points" ) );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////////////////////////
// CHorseParkingActionPointSelectorInstance
////////////////////////////////////////////////////////////////////////////////////////
class CHorseParkingActionPointSelectorInstance : public CActionPointSelectorInstance
{
	typedef CActionPointSelectorInstance Super;
public:
	CHorseParkingActionPointSelectorInstance( const CHorseParkingActionPointSelector& def );

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) override;
protected:
	TagList				m_apTags;
};