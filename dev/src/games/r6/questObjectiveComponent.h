/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once
#include "followerPOIComponent.h"

class CQuestObjectiveComponent : public CSelfUpdatingComponent
{
	DECLARE_ENGINE_CLASS( CQuestObjectiveComponent, CSelfUpdatingComponent, 0 );

public:
	void SortPOIByPriority( TDynArray< THandle <CFollowerPOIComponent> >& POIToSort );

private:
	void funcSortPOIByPriority ( CScriptStackFrame& stack, void* result );

};

BEGIN_CLASS_RTTI( CQuestObjectiveComponent );
	PARENT_CLASS( CSelfUpdatingComponent );
	NATIVE_FUNCTION( "I_SortPOIByPriority", funcSortPOIByPriority );	
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
// This struct is used to sort POI
//------------------------------------------------------------------------------------------------------------------
struct SPOISortingPredicate
{
	Bool operator()( const THandle <CFollowerPOIComponent>& POI_1, const THandle <CFollowerPOIComponent>& POI_2 ) const
	{
		Int32	l_P1	=	POI_1.Get()->GetPriority();
		Int32	l_P2	=	POI_2.Get()->GetPriority();
		
		return 	l_P1 > l_P2;
	}
};