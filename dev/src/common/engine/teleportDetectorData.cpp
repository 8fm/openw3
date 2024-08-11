#include "build.h"
#include "teleportDetectorData.h"

IMPLEMENT_ENGINE_CLASS( CTeleportDetectorData );
IMPLEMENT_ENGINE_CLASS( STeleportBone );

CTeleportDetectorData::CTeleportDetectorData()
	: m_angleDif( 15.f )
	, m_pelvisPositionThreshold( 1.f )
{
}