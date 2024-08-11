#pragma once
class CLayersEntityCompatibilityChecker
{
public:
	CLayersEntityCompatibilityChecker() {}
	~CLayersEntityCompatibilityChecker() {}

	static Bool CheckLayersEntitiesCompatibility( const TDynArray< CLayerInfo* >& layerInfos, TDynArray< String >& compatibilityErrors, Bool onlyModified = true );
	static Bool CheckLayersEntitiesCompatibility( const CLayerInfo* layerInfo, TDynArray< String >& compatibilityErrors );
	static Bool IsEntityCompatible( const CEntity* entity, const CLayerInfo* layerInfo, String& outInfo );

};

