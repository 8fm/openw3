/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CWetnessSupplier
{
private:
	THandle< CWetnessComponent > 		m_wetnessComponent;
	Uint32								m_boneCount;
	TDynArray< Float >					m_wetnessDataFromOcean;
	TDynArray< Float >					m_wetnessData;

public:
	CWetnessSupplier( const CWetnessComponent* wc, Uint32 numBones );
	~CWetnessSupplier();

	void				CalcWetness( const TDynArray< Matrix >& bonesWS );
	RED_INLINE Float	GetWetnessDataFromBone( Int32 boneIndex ) const { return m_wetnessData[ boneIndex ]; }
};

class CWetnessComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CWetnessComponent, CComponent, 0 );

public:
	/////////////// out params from UpdateValues
	struct SWetnessParams
	{
		Float	m_blendInFromOceanSpeed;
		Float	m_blendInFromRainSpeed;
		Float	m_blendOutFromOceanSpeed;
		Float	m_blendOutFromRainSpeed;
		Float	m_waterLevelZ;
		Float	m_interior;
	};

public:
	CWetnessComponent();

	virtual void	OnAttached( CWorld* world ) override;
	virtual void	OnDetached( CWorld* world ) override;
	virtual Bool	UsesAutoUpdateTransform() override { return false; }
	virtual void	OnCinematicStorySceneStarted() override;
	virtual void	OnCinematicStorySceneEnded() override;
	RED_INLINE Bool	ShouldUpdateWetnessFromOcean() { return m_shouldUpdateFromOcean; }

public:
	void			UpdateValues( SWetnessParams& params );

private:
	class CWeatherManager* m_weatherManager;

	Float			m_blendInFromOcean;
	Float			m_blendInFromRain;
	Float			m_blendOutFromOcean;
	Float			m_blendOutFromRain;

	SWetnessParams	m_wetnessParams;
	Uint64			m_tickTimeMarker;
	Bool			m_shouldUpdateFromOcean;	// hack that prevents npcs from getting wet when they are teleported underground in scenes
};

BEGIN_CLASS_RTTI( CWetnessComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_blendInFromOcean, TXT("Time to blend in from ocean") )
	PROPERTY_EDIT( m_blendInFromRain, TXT("Time to blend in from rain") )
	PROPERTY_EDIT( m_blendOutFromOcean, TXT("Time to blend out when leaving water") )
	PROPERTY_EDIT( m_blendOutFromRain, TXT("Time to blend out when leaving rain") )
END_CLASS_RTTI();