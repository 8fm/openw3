/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/pathlibSearchData.h"

enum EAsyncTestResult
{
	EAsyncTastResult_Failure,
	EAsyncTastResult_Success,
	EAsyncTastResult_Pending,
	EAsyncTastResult_Invalidated,
};

BEGIN_ENUM_RTTI( EAsyncTestResult );
	ENUM_OPTION( EAsyncTastResult_Failure );
	ENUM_OPTION( EAsyncTastResult_Success );
	ENUM_OPTION( EAsyncTastResult_Pending );
	ENUM_OPTION( EAsyncTastResult_Invalidated );
END_ENUM_RTTI();

enum ENavigationReachabilityTestType
{
	ENavigationReachability_All = PathLib::CMultiReachabilityData::REACH_ALL,
	ENavigationReachability_Any = PathLib::CMultiReachabilityData::REACH_ANY,
	ENavigationReachability_FullTest = PathLib::CMultiReachabilityData::REACH_FULL
};

BEGIN_ENUM_RTTI( ENavigationReachabilityTestType );
	ENUM_OPTION( ENavigationReachability_All );
	ENUM_OPTION( ENavigationReachability_Any );
	ENUM_OPTION( ENavigationReachability_FullTest );
END_ENUM_RTTI();


class CNavigationReachabilityQueryInterface : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CNavigationReachabilityQueryInterface )

protected:
	typedef PathLib::CMultiReachabilityData CMultiReachabilityData;

	CMultiReachabilityData::Ptr			m_reachabilityDataPtr;

	EngineTime							m_lastTest;
public:
	CNavigationReachabilityQueryInterface();
	~CNavigationReachabilityQueryInterface();

private:
	void funcGetLastOutput( CScriptStackFrame& stack, void* result );
	void funcGetOutputClosestDistance( CScriptStackFrame& stack, void* result );
	void funcGetOutputClosestEntity( CScriptStackFrame& stack, void* result );
	void funcTestActorsList( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CNavigationReachabilityQueryInterface )
	PARENT_CLASS( IScriptable )
	NATIVE_FUNCTION( "GetLastOutput", funcGetLastOutput );
	NATIVE_FUNCTION( "GetOutputClosestDistance", funcGetOutputClosestDistance );
	NATIVE_FUNCTION( "GetOutputClosestEntity", funcGetOutputClosestEntity );
	NATIVE_FUNCTION( "TestActorsList", funcTestActorsList );
END_CLASS_RTTI()

