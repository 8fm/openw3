/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "gameplayEntity.h"

#include "actionPointDataDef.h"
#include "entityActionsRouter.h"

/// Action point entity
class CActionPointComponent; 

#define ACTION_POINT_CATEGORY_TABLE TXT("gameplay\\globals\\action_categories.csv")

class CActionPointCategoriesResourcesManager: public C2dArraysResourcesManager
{
public:
	CActionPointCategoriesResourcesManager();
};

typedef TSingleton< CActionPointCategoriesResourcesManager, TDefaultLifetime, TCreateUsingNew > SActionPointResourcesManager;

class CActionPoint : public CGameplayEntity, public IEntityActionsRouter
{
	DECLARE_ENGINE_CLASS( CActionPoint, CGameplayEntity, 0 );

	friend CActionPointComponent;

private:
	Bool m_actionBreakable;
	Bool m_overrideActionBreakableInComponent;

public:
	Bool IsSupportingCategory( const CName& category ) const;

public:
	CActionPoint();	

	//! After the entity's been pasted onto a layer
	virtual void OnPasted( CLayer* layer );

	//! Is this ok to extract components from this entity during sector data building ?
	virtual Bool CanExtractComponents( const Bool isOnStaticLayer ) const;

	IEntityActionsRouter*  AsEntityActionsRouter() override { return static_cast< IEntityActionsRouter* >( this ); }

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

	virtual Bool CheckShouldSave() const override { return false; }
};

BEGIN_CLASS_RTTI( CActionPoint );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_EDIT( m_events, TXT("Events") )
	PROPERTY_EDIT( m_actionBreakable					, TXT("Is action breakable") );
	PROPERTY_EDIT( m_overrideActionBreakableInComponent	, TXT("Is overriding 'action breakable' in component") );
END_CLASS_RTTI();
