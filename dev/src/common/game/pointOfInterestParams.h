#pragma once

class CPointOfInterestParams
{
public:
	/// Describes the effect the poi has : Fire, Food etc ... 
	Boids::PointOfInterestType				m_type;
	/// if false the poi will be ignored by the swarm
	Bool									m_enabled;
	/// Scale, multiplies the parameters found in POIConfig
	Float									m_scale;
	/// The range at which the gravity is equal to gravity
	Float									m_gravityRangeMin;
	/// the range at which the gravity starts to be applied
	Float									m_gravityRangeMax;
	Float									m_effectorRadius;

	/// Type of shape for the poi Circle and Cone are supported
	CName									m_shapeType;
	/// If true LairEntity::OnBoidPointOfInterestReached script will be called for the POI
	Bool									m_useReachCallBack;
	/// Amongst all POIs with this value set to true, only the closest will affect a given boid
	Bool									m_closestOnly;
	/// Angle for the cone, ignored for circle
	Float									m_coneMinOpeningAngle;
	Float									m_coneMaxOpeningAngle;
	Float									m_coneEffectorOpeningAngle;

	CPointOfInterestParams()
		: m_enabled( true )
		, m_type( CName::NONE )
		, m_scale( 1.0f )
		, m_gravityRangeMin( 0.5f )
		, m_gravityRangeMax( 3.0f )
		, m_effectorRadius( 0.5f )
		, m_shapeType( CNAME( Circle ) )
		, m_useReachCallBack( false )
		, m_closestOnly( false )
		, m_coneMinOpeningAngle( 20.0f )
		, m_coneMaxOpeningAngle(30.0f)
		, m_coneEffectorOpeningAngle( 15.0f )
	{
	}
};