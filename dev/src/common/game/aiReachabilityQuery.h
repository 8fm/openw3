/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/pathlibSearchData.h"

class CAIReachabilityQuery : public PathLib::CReachabilityData
{
	typedef PathLib::CReachabilityData Super;
protected:
	CActor*										m_owner;
	Bool										m_cachedOutputIsSuccessful;
	Float										m_chechedOutputTimeout;
public:
	typedef			TRefCountPointer< CAIReachabilityQuery >				Ptr;

										CAIReachabilityQuery( CActor* owner );

 	void								OnCombatTargetChanged()								{ m_chechedOutputTimeout = 0.f; }

	Bool								QueryReachable( const Vector3& destinationPos, Float reachabilityTolerance, Float localTime );
};