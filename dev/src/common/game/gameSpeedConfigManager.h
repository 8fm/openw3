#pragma once

#include "../engine/speedConfig.h"
#include "definitionsManagerListener.h"
#include "definitionsManager.h"

class CGameSpeedConfigManager : public CSpeedConfigManager, public CDefinitionsManagerListener
{
public:
	// Engine interface

	virtual ~CGameSpeedConfigManager(){}

	/// Call this each time you want to refresh parameters
	void InitParams( );

	/// CDefinitionsManagerListener interface
	void OnDefinitionsReloaded() override;

	/// Called for each speedConfig node in XML
	Bool ParseSpeedConfig( const SCustomNode & speedConfigNode );
};