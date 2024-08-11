/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/pathlibWalkableSpotQueryRequest.h"
#include "../engine/areaConvex.h"

class CQueryReacheableSpotInAreaRequest : public PathLib::CWalkableSpotQueryRequest
{
	typedef PathLib::CWalkableSpotQueryRequest Super;
protected:
	THandle< CAreaComponent >						m_areaHandle;				// needed to check if request is still valid
	CAreaShapePtr									m_areaShape;
	Matrix											m_areaShapeTransform;
	Float											m_tolerance;
	Bool											m_useTolerance;
public:
	typedef TRefCountPointer< CQueryReacheableSpotInAreaRequest > Ptr;

	CQueryReacheableSpotInAreaRequest();

	Bool											RefreshSetup( CWorld* world, CActor* actor, const CAreaComponent* area, const Vector& referencePos, Float maxDist = 1024.f, Float tolerance = 0.f );

	Bool											AcceptPosition( const Vector3& nodePos ) override;
	void											CompletionCallback() override;
};
