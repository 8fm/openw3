#pragma once

#include "../../common/game/actionPointSelectors.h"

class CRainActionPointSelectorInstance;

class CRainActionPointSelector : public CWanderActionPointSelector
{
	friend class CRainActionPointSelectorInstance;
	DECLARE_RTTI_SIMPLE_CLASS( CRainActionPointSelector );

public:
	CRainActionPointSelector(){}
	CActionPointSelectorInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CRainActionPointSelector )
	PARENT_CLASS( CWanderActionPointSelector )	
END_CLASS_RTTI()

class CRainActionPointSelectorInstance : public CWanderActionPointSelectorInstance 
{
	typedef CWanderActionPointSelectorInstance Super;

private:
	Bool m_categoryInitialized;

	void InitializeCategoty( const CBehTreeInstance* behTreeInstance );

public:	
	CRainActionPointSelectorInstance( CRainActionPointSelector& def, CBehTreeSpawnContext& context );

	Bool	SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius = -1.0f ) override;
};