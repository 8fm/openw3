#pragma once

class CFactsDB;

enum EQueryFact
{
	QF_Sum,
	QF_SumSince,
	QF_LatestValue,
	QF_DoesExist,
};

BEGIN_ENUM_RTTI( EQueryFact );
	ENUM_OPTION( QF_Sum );
	ENUM_OPTION( QF_SumSince );
	ENUM_OPTION( QF_LatestValue );
	ENUM_OPTION( QF_DoesExist );
END_ENUM_RTTI();

// A tool providing an easy way for all editor-related features to
// query the facts db using a unified interface
class CFactsDBEditorQuery
{
public:
	// Checks if the specified fact exists and matches the specific criteria
	static Bool Evaluate( CFactsDB& factsDB, EQueryFact queryFact, const String& factId, Int32 value = 0, ECompareFunc compareFunc = CF_Equal, const EngineTime& sinceTime = EngineTime() );

	// Checks if the specified facts exist and match the specific criteria
	static Bool Evaluate( CFactsDB& factsDB, EQueryFact queryFact, const String& factId1, const String& factId2, ECompareFunc compareFunc = CF_Equal, const EngineTime& sinceTime = EngineTime() );

	// Evaluate numerical value of query
	static Bool EvaluateNumericalQuery( CFactsDB& factsDB, EQueryFact queryFact, const String& factId, Int32 value, ECompareFunc compareFunc, const EngineTime& sinceTime );

	// Evaluate numerical value of query
	static Bool EvaluateNumericalQuery( CFactsDB& factsDB, EQueryFact queryFact, const String& factId1, const String& factId2, ECompareFunc compareFunc, const EngineTime& sinceTime );
};
