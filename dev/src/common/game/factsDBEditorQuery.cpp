#include "build.h"
#include "factsDBEditorQuery.h"
#include "factsDB.h"


IMPLEMENT_RTTI_ENUM( EQueryFact );

Bool CFactsDBEditorQuery::Evaluate( CFactsDB& factsDB, 
									EQueryFact queryFact, 
								    const String& factId, 
									Int32 value, 
									ECompareFunc compareFunc,
									const EngineTime& sinceTime )
{
	if (queryFact == QF_DoesExist)
	{
		return factsDB.DoesExist( factId );
	}
	else 
	{
		return EvaluateNumericalQuery( factsDB, queryFact, factId, value, compareFunc, sinceTime );
	}
}

Bool CFactsDBEditorQuery::Evaluate( CFactsDB& factsDB, 
									EQueryFact queryFact, 
								    const String& factId1, 
								    const String& factId2, 
									ECompareFunc compareFunc,
									const EngineTime& sinceTime )
{
	if (queryFact == QF_DoesExist)
	{
		return factsDB.DoesExist( factId1 ) && factsDB.DoesExist( factId2 );
	}
	else 
	{
		return EvaluateNumericalQuery( factsDB, queryFact, factId1, factId2, compareFunc, sinceTime );
	}
}

Bool CFactsDBEditorQuery::EvaluateNumericalQuery( CFactsDB& factsDB, 
												 EQueryFact queryFact, 
												 const String& factId, 
												 Int32 value, 
												 ECompareFunc compareFunc,
												 const EngineTime& sinceTime )
{
	Int32 queryValue;
	if (queryFact == QF_Sum)
	{
		queryValue = factsDB.QuerySum( factId );
	}
	else if (queryFact == QF_SumSince)
	{
		queryValue = factsDB.QuerySumSince( factId, sinceTime );
	}
	else if (queryFact == QF_LatestValue)
	{
		queryValue = factsDB.QueryLatestValue( factId );
	}
	else
	{
		ERR_GAME( TXT( "Unknown fact query." ) );
		return false;
	}

	if (compareFunc == CF_Equal)
	{
		return queryValue == value;
	}
	else if (compareFunc == CF_NotEqual)
	{
		return queryValue != value;
	}
	else if (compareFunc == CF_Less)
	{
		return queryValue < value;
	}
	else if (compareFunc == CF_LessEqual)
	{
		return queryValue <= value;
	}
	else if (compareFunc == CF_Greater)
	{
		return queryValue > value;
	}
	else if (compareFunc == CF_GreaterEqual)
	{
		return queryValue >= value;
	}

	return false;
}

Bool CFactsDBEditorQuery::EvaluateNumericalQuery( CFactsDB& factsDB,
												 EQueryFact queryFact,
												 const String& factId1,
												 const String& factId2,
												 ECompareFunc compareFunc,
												 const EngineTime& sinceTime )
{
	Int32 queryValue1, queryValue2;
	if (queryFact == QF_Sum)
	{
		queryValue1 = factsDB.QuerySum( factId1 );
		queryValue2 = factsDB.QuerySum( factId2 );
	}
	else if (queryFact == QF_SumSince)
	{
		queryValue1 = factsDB.QuerySumSince( factId1, sinceTime );
		queryValue2 = factsDB.QuerySumSince( factId2, sinceTime );
	}
	else if (queryFact == QF_LatestValue)
	{
		queryValue1 = factsDB.QueryLatestValue( factId1 );
		queryValue2 = factsDB.QueryLatestValue( factId2 );
	}
	else
	{
		ERR_GAME( TXT( "Unknown fact query." ) );
		return false;
	}

	if (compareFunc == CF_Equal)
	{
		return queryValue1 == queryValue2;
	}
	else if (compareFunc == CF_NotEqual)
	{
		return queryValue1 != queryValue2;
	}
	else if (compareFunc == CF_Less)
	{
		return queryValue1 < queryValue2;
	}
	else if (compareFunc == CF_LessEqual)
	{
		return queryValue1 <= queryValue2;
	}
	else if (compareFunc == CF_Greater)
	{
		return queryValue1 > queryValue2;
	}
	else if (compareFunc == CF_GreaterEqual)
	{
		return queryValue1 >= queryValue2;
	}

	return false;
}