#pragma once

#include "swarmUtils.h"
#include "boidPointOfInterestComponent.h"
#include "pointOfInterestParams.h"

class IBoidLairEntity;

// simplified data for job update
// no pointer are stored as they can't be accessed because we are on a separate thread
class CPoiJobData
{
public:
	Boids::PointOfInterestId				m_uid;
	// needs to be private and accessed through because of positionOffset
	Vector									m_position;
	Vector									m_positionOffset;
	EulerAngles								m_orientation;
	Vector									m_forwardVect;
	Vector									m_rightVect;
	/// How many boids are using the POI at this frame
	Uint32									m_useCounter;
	/// How many boids have reached the POI this frame
	Uint32									m_reachCounter;
	/// Used only on sync overwise it will crash !!
	THandle< CEntity >						m_entityHandle;
	CBoidPointOfInterestComponent *			m_poiCpnt;

	CPointOfInterestParams					m_cpntParams;
	/// Used for timing effector aplication
	Bool									m_applyEffector;

	/// precomputed stuff :
	Float									m_tanHalfMinConeOpeningAngle;
	Float									m_cosHalfMinConeOpeningAngle;
	Float									m_tanHalfMaxConeOpeningAngle;
	Float									m_cosHalfMaxConeOpeningAngle;
	Float									m_tanHalfEffectorConeOpeningAngle;
	Float									m_cosHalfEffectorConeOpeningAngle;

public:
	CPoiJobData( CBoidPointOfInterestComponent *const poi = NULL, Boids::PointOfInterestId id = (Uint32)-1 );

	void PrecomputeValues()
	{
		m_orientation.ToAngleVectors( &m_forwardVect, &m_rightVect, NULL );

		m_tanHalfMinConeOpeningAngle		= tanf( DEG2RAD( m_cpntParams.m_coneMinOpeningAngle * 0.5f ) );
		m_cosHalfMinConeOpeningAngle		= cosf( DEG2RAD( m_cpntParams.m_coneMinOpeningAngle * 0.5f ) );
		m_tanHalfMaxConeOpeningAngle		= tanf( DEG2RAD( m_cpntParams.m_coneMaxOpeningAngle * 0.5f ) );
		m_cosHalfMaxConeOpeningAngle		= cosf( DEG2RAD( m_cpntParams.m_coneMaxOpeningAngle * 0.5f ) );
		m_tanHalfEffectorConeOpeningAngle	= tanf( DEG2RAD( m_cpntParams.m_coneEffectorOpeningAngle * 0.5f ) );
		m_cosHalfEffectorConeOpeningAngle	= cosf( DEG2RAD( m_cpntParams.m_coneEffectorOpeningAngle * 0.5f ) );
	}

	void			PostUpdateSynchronization( IBoidLairEntity*const lair, Float updateTime );
	Bool			PreUpdateSynchronization( IBoidLairEntity*const lair, Float updateTime );
	Vector 			GetPositionWithOffset()const{ return m_position + m_positionOffset; }
};

typedef TDynArray< CPoiJobData * >						CPoiJobData_PointerArray;
typedef TDynArray< CPoiJobData >						CPoiJobData_Array;
typedef TArrayMap< CName, CPoiJobData_Array >			CPoiJobData_Map;


enum EPOIUsage
{
	POI_USAGE_NONE,
	POI_USAGE_ACTION,
	POI_USAGE_ACTION_WANDER,
};

class CPoiDataByType
{
public:
	EPOIUsage					m_usage;
	Float						m_wanderTimeOut;

	CPoiDataByType()
		: m_usage( POI_USAGE_NONE ) 
		, m_wanderTimeOut( NumericLimits< Float >::Max() )
	{
	}
};

typedef TArrayMap< CName, CPoiDataByType > CPoiDataByType_Map;

class CPoiJobData;
class CPoiConfigByGroup;

class CPoiConfigAndList
{
public:
	CPoiJobData_PointerArray		m_poiJobDataArray;
	const CPoiConfigByGroup *		m_poiConfig;

	CPoiConfigAndList( const CPoiConfigByGroup *const poiConfig = NULL )
		: m_poiConfig( poiConfig )
	{

	}
};
typedef TArrayMap< CName, CPoiConfigAndList >		CFlyingCrittersPoi_Map;
