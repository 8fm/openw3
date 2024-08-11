#include "build.h"

#include "..\..\common\game\selfUpdatingComponent.h"
#include "questObjectiveComponent.h"
#include "followerPOIComponent.h"

IMPLEMENT_ENGINE_CLASS( CQuestObjectiveComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CQuestObjectiveComponent::SortPOIByPriority( TDynArray< THandle <CFollowerPOIComponent> >& POIToSort )
{
	SPOISortingPredicate	predicate;
	Sort( POIToSort.Begin(), POIToSort.End(), predicate );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CQuestObjectiveComponent::funcSortPOIByPriority( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle <CFollowerPOIComponent> >, POIToSort, TDynArray< THandle< CFollowerPOIComponent > > () );
	FINISH_PARAMETERS;	

	SortPOIByPriority( POIToSort );
}