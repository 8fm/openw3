/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Wind parameters
struct CWindParameters
{
	DECLARE_RTTI_STRUCT( CWindParameters );

public:	
	Float						m_environmentWindStrengthOverride;
	
	Float						m_accumTimeSinceGameStart;

	Bool						m_speedTreeEnabled;		
	Float						m_targetDirection;

private:
	Float						m_windScale;	
	Float						m_windDirection;						//!< [-PI , PI]
	Vector						m_cachedWindDirection;
	Vector						m_accumCloudOffset;
	Float						m_sourceWindDirection;
	Float						m_windDirectionAdaptationSpeed;
	Float						m_frac;

public:
	CWindParameters( );	

	void						SetWindScale( Float scale );	
	RED_INLINE Float			GetWindScale() const { return m_windScale*m_environmentWindStrengthOverride; };
	
	RED_INLINE Vector			GetCloudsShadowsOffset() const { return Vector( m_accumCloudOffset.X, m_accumCloudOffset.Y, m_accumTimeSinceGameStart, 0.0f ); };

	RED_INLINE Vector			GetWindDirection() const { return m_cachedWindDirection; };
	RED_INLINE Float			GetWindRotationZ() const { return m_windDirection; };

	void						NotifyConditionsChanged();
	void						ForceWindParameters( Float windScale, Bool forceDirectionStabilize, Float environmentWindStrengthOverride);
	void						TickWindParameters( Float timeDelta );
};

BEGIN_CLASS_RTTI( CWindParameters );
PROPERTY_EDIT( m_speedTreeEnabled,			TXT("m_speedTreeEnabled") );
END_CLASS_RTTI();