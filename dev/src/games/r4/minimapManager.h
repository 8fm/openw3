#pragma once

#include "../../common/engine/flashValue.h"
#include "commonMapStructs.h"

class CMinimapManager
{
private:
	Bool							m_initialized;

	CFlashValue						m_addMapPinHook;
	CFlashValue						m_moveMapPinHook;
	CFlashValue						m_highlightMapPinHook;
	CFlashValue						m_deleteMapPinHook;
	CFlashValue						m_updateDistanceToHighlightedMapPinHook;
	CFlashValue						m_addWaypointHook;
	CFlashValue						m_clearWaypointsHook;

	Float							m_currMinimapRadius;

public:
	CMinimapManager();
	virtual ~CMinimapManager();

	RED_FORCE_INLINE Bool IsMinimapAvailable()
	{
		return m_initialized;
	}

	void Initialize( const THandle< CR4HudModule >& minimapModule );

	void AddMapPin( const SCommonMapPinInstance& pin );
	void MoveMapPin( const SCommonMapPinInstance& pin );
	void HighlightMapPin( const SCommonMapPinInstance& pin );
	void UpdateMapPin( const SCommonMapPinInstance& pin );
	void DeleteMapPin( const SCommonMapPinInstance& pin );
	void UpdateDistanceToHighlightedMapPin( Float questDistance, Float userDistance );

	void AddWaypoints( const TDynArray< Vector >& waypoints );
	void DeleteWaypoints( Uint32 startingFrom = 0 );

	void OnChangedMinimapRadius( Float radius );
	Bool ShouldMapPinBeVisibleOnMinimap( const SCommonMapPinInstance& pin, const Vector& playerPos );
	Float GetCurrentMinimapRadius() const { return m_currMinimapRadius; }

private:
	static CFlashValue GetStringFlashValue( const String& stringArg, CFlashValue& functionFlashValue );
	static String GetPinTypePostfix( const SCommonMapPinInstance& pin );

private:
	typedef CFlashValue CMinimapManager::*					TFlashFunctionImport;

	struct SFlashFuncImportDesc
	{
		const Char*					m_memberName;
		TFlashFunctionImport		m_flashFuncImport;	
	};

	static SFlashFuncImportDesc		sm_flashFunctionImportTable[];
};

