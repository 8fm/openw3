/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "windParameters.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CWindParameters );

CWindParameters::CWindParameters()	
	: m_speedTreeEnabled( true )	
	, m_windScale( 0.1f )
	, m_windDirection( 0.0f )
	, m_environmentWindStrengthOverride( 1.0f )
	, m_accumTimeSinceGameStart( 0.0f )
	, m_targetDirection( 0.0f )
	, m_windDirectionAdaptationSpeed( 0.002f )
	, m_frac( 0.0f )
	, m_cachedWindDirection(1.0f, 0.0f, 0.0f)
	, m_accumCloudOffset( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_sourceWindDirection( 0.0f )
{	
	
}

void CWindParameters::SetWindScale( Float scale )
{
	m_windScale = Clamp<Float>( scale, 0.0f, 1.0f );
}

void CWindParameters::NotifyConditionsChanged()
{
	// randomize wind direction now
	Float rndZ = GEngine->GetRandomNumberGenerator().Get< Float >( -M_PI, M_PI );
	// make sure the turn is substantial
	while( abs(rndZ) < 0.3f*M_PI  )
	{
		rndZ = GEngine->GetRandomNumberGenerator().Get< Float >( -M_PI, M_PI );
	}

	m_targetDirection = m_windDirection + rndZ;

	if( m_targetDirection > M_PI ) m_targetDirection = -M_PI + ( m_targetDirection - MFloor( m_targetDirection / M_PI ) );
	else if( m_targetDirection < -M_PI ) m_targetDirection = M_PI + ( MFloor( m_targetDirection / M_PI ) - m_targetDirection );

	m_frac = 0.0f;
	m_sourceWindDirection = m_windDirection;
}

void CWindParameters::TickWindParameters( Float timeDelta )
{	
	if( abs(m_windDirection - m_targetDirection) > 0.01f )
	{		
		m_frac = Clamp<Float>( m_frac + timeDelta*m_windDirectionAdaptationSpeed, 0.0f, 1.0f );		
		m_windDirection = Lerp<Float>( m_frac, m_sourceWindDirection, m_targetDirection );
		
		m_cachedWindDirection.X = cos( m_windDirection );
		m_cachedWindDirection.Y = sin( m_windDirection );
	}

	m_accumCloudOffset += -m_cachedWindDirection * timeDelta * m_windScale;
	m_accumTimeSinceGameStart += timeDelta;

	//RED_LOG( RED_LOG_CHANNEL(GpuApi), TXT("%s"), ( ToString( m_cachedWindDirection ) ).AsChar() );	
}

void CWindParameters::ForceWindParameters( Float windScale, Bool forceDirectionStabilize, Float environmentWindStrengthOverride )
{
	m_windScale = windScale;
	m_environmentWindStrengthOverride = environmentWindStrengthOverride;

	if( forceDirectionStabilize )
	{
		m_windDirection = m_targetDirection;
		m_frac = 0.0f;
	}
}
