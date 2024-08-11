#include "build.h"
#include "poiJobData.h"
#include "boidLairEntity.h"

CPoiJobData::CPoiJobData( CBoidPointOfInterestComponent *const poi, Boids::PointOfInterestId id )
		: m_uid( id )
		, m_position( 0.0f, 0.0f, 0.0f )
		, m_positionOffset( 0.0f, 0.0f, 0.0f )
		, m_orientation( EulerAngles::ZEROS )
		, m_forwardVect( 1.0f, 0.0f, 0.0f )
		, m_rightVect( 0.0f, 1.0f, 0.0f )
		, m_useCounter( 0 )
		, m_reachCounter( 0 )
		, m_entityHandle()
		, m_poiCpnt( poi )
		, m_cpntParams()
		, m_applyEffector( true )
		, m_tanHalfMinConeOpeningAngle( 0.0f )
		, m_cosHalfMinConeOpeningAngle( 0.0f )
		, m_tanHalfMaxConeOpeningAngle( 0.0f )
		, m_cosHalfMaxConeOpeningAngle( 0.0f )
		, m_tanHalfEffectorConeOpeningAngle( 0.0f )
		, m_cosHalfEffectorConeOpeningAngle( 0.0f )
{
	// catching scripters potential errors :
	if ( m_cpntParams.m_gravityRangeMin > m_cpntParams.m_gravityRangeMax )
	{
		m_cpntParams.m_gravityRangeMin = m_cpntParams.m_gravityRangeMax;
	}

	if ( poi )
	{
		m_entityHandle	= poi->GetEntity();
		m_cpntParams	= poi->GetParameters();
		m_uid			= id;
		m_position		= poi->GetWorldPosition();
		m_orientation	= poi->GetWorldRotation();
	}
	PrecomputeValues();
}

Bool CPoiJobData::PreUpdateSynchronization( IBoidLairEntity*const lair, Float updateTime )
{
	CEntity*const entity								= m_entityHandle.Get();
	if( entity == NULL )
	{
		return false;
	}
	else
	{
		// entity not beeing NULL garanties m_poiCpnt pointer is valid
		if ( m_poiCpnt )
		{
			m_position				= m_poiCpnt->GetWorldPosition();
			m_orientation			= m_poiCpnt->GetWorldRotation();
			m_cpntParams.m_enabled	= m_poiCpnt->GetParameters().m_enabled;
		}
		else // if not component then the poiData was generated for player
		{
			m_position		= entity->GetWorldPosition();
			m_orientation	= entity->GetWorldRotation();
		}
		// Precomputed stuff
		m_orientation.ToAngleVectors( &m_forwardVect, NULL, NULL );
	}
	return true;
}
void CPoiJobData::PostUpdateSynchronization( IBoidLairEntity *const lair, Float updateTime )
{
	CEntity*const entity								= m_entityHandle.Get();
	if ( entity )
	{
		if ( m_useCounter != 0 )
		{
			// entity not beeing NULL garanties m_poiCpnt pointer is valid
			if ( m_poiCpnt && m_useCounter != 0)
			{
				// script calls:
				m_poiCpnt->OnUsed( m_useCounter,  updateTime );
			}
		}
		if ( m_cpntParams.m_useReachCallBack && m_reachCounter != 0)
		{
			lair->OnBoidPointOfInterestReached( m_reachCounter, entity, updateTime );
		}
		m_useCounter	= 0;
		m_reachCounter	= 0;
	}
}