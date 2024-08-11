#pragma once

#include "pathlibComponent.h"
#include "pathlib.h"
#include "areaComponent.h"

namespace PathLib
{
	class IComponent;

}

struct SDeniedAreaSpawnData
{
	EPathLibCollision	m_collisionType;
	RED_FORCE_INLINE SDeniedAreaSpawnData()
		: m_collisionType( PLC_Static )
	{}
};

/// Denied area for path finding
class CDeniedAreaComponent : public CAreaComponent, public PathLib::IObstacleComponent
{
	DECLARE_ENGINE_CLASS( CDeniedAreaComponent, CAreaComponent, 0 );
protected:
	EPathLibCollision					m_collisionType;
	Bool								m_isEnabled;
	Bool								m_canBeDisabled;

	void								SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup ) override;
	EPathLibCollision					GetPathLibCollisionGroup() const override;			// PathLib::IObstacleComponent interface
	CComponent*							AsEngineComponent() override;						// PathLib::IComponent interface
	PathLib::IComponent*				AsPathLibComponent() override;						// PathLib::IComponent interface

	Bool								IsLayerBasedGrouping() const override;
	Bool								IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const override;
public:
	CDeniedAreaComponent()
		: m_collisionType( PLC_Static )
		, m_isEnabled( true )
		, m_canBeDisabled( true )											{}
	~CDeniedAreaComponent()													{}

	void								SetPathLibCollisionGroup( EPathLibCollision collisionType ) { m_collisionType = collisionType; }
	EPathLibCollision					GetCollisionType() const			{ return m_collisionType; }
	Bool								IsEnabled() const					{ return m_isEnabled; }

	//! Attached/Detached to world
	virtual void						OnAttached( CWorld* world ) override;
	virtual void						OnDetached( CWorld* world ) override;
	virtual void						OnSpawned( const SComponentSpawnInfo& spawnInfo ) override;

	//! Editor only stuff for pathlib system notifications
	// End vertex edit mode
	virtual void						OnEditorEndVertexEdit() override;
#ifndef NO_EDITOR
	// Position is  changed externally
	virtual void						EditorOnTransformChanged() override;
	// Called pre deletion from editor
	virtual void						EditorPreDeletion() override;

	// Logic for removing denied area components on cooked builds
	virtual Bool						RemoveOnCookedBuild() override;
#endif
	//! Internal property was changed in editor
	virtual void						OnPropertyPostChange( IProperty* property ) override;

	//! Visualize denied area state
	virtual Color						CalcLineColor() const override;

	//! Enable obstacle On/Off
	virtual void						SetEnabled( Bool enabled ) override;

protected:
	void								UpdateObstacle();
};

BEGIN_CLASS_RTTI( CDeniedAreaComponent );
	PARENT_CLASS( CAreaComponent );
	PROPERTY_EDIT( m_collisionType, TXT("Static obstacles are burned into NavMesh, dynamic may be enabled/disabled in runtime") );
	PROPERTY_EDIT_SAVED( m_isEnabled, TXT("Is Enabled") );
	PROPERTY_EDIT( m_canBeDisabled, TXT("Set to 'true' if denied area can be enabled/disabled in runtime (only available for PLC_Dynamic areas). Otherwise keep it 'off' as 'falst' boost performance & navigation quality.") );
END_CLASS_RTTI();

