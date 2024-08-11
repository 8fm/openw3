/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "wayPointComponent.h"

/// Spawn point component
class CSpawnPointComponent : public CWayPointComponent
{
	DECLARE_ENGINE_CLASS( CSpawnPointComponent, CWayPointComponent, 0 )

protected:
	virtual void WaypointGenerateEditorFragments( CRenderFrame* frame ) override;

public:
	CSpawnPointComponent()
		: m_radius( 0.f )
#ifndef RED_FINAL_BUILD
		, m_notUsedByCommunity( false )
#endif
	{}
	Bool GetSpawnPoint( Bool isAppear, Vector &spawnPoint /* out */ );

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	virtual Uint32 GetMinimumStreamingDistance() const;

	// Debug methods
	virtual Color CalcSpriteColor() const override;
	virtual CBitmapTexture* GetSpriteIcon() const override;

#ifndef NO_EDITOR
	virtual Bool RemoveOnCookedBuild() override;
#endif

protected:
	Float				m_radius;
#ifndef RED_FINAL_BUILD
	Bool				m_notUsedByCommunity;
#endif

// methods exported to scripts
private:
	void funcGetSpawnPoint( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CSpawnPointComponent );
	PARENT_CLASS( CWayPointComponent );
	PROPERTY_EDIT( m_radius, TXT("Spawn radius") );
#ifndef RED_FINAL_BUILD
	PROPERTY_EDIT_NOT_COOKED( m_notUsedByCommunity, TXT("Is NOT used by community system. If its only used by encounters, mark it as 'true' for optimization purposes.") )
#endif
	NATIVE_FUNCTION( "GetSpawnPoint", funcGetSpawnPoint );
END_CLASS_RTTI();

