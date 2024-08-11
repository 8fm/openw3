#pragma once
#include "entity.h"

enum ESkyTransformType : CEnum::TValueType;

class CSkyTransformEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CSkyTransformEntity, CEntity, 0 );

protected:
	ESkyTransformType	m_transformType;
	Bool				m_alignToPlayer;
	Bool				m_onlyYaw;

public:
	CSkyTransformEntity();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// On tick
	virtual void OnTick( Float time );
};

BEGIN_CLASS_RTTI( CSkyTransformEntity );
PARENT_CLASS( CEntity );
PROPERTY_EDIT(m_transformType,	TXT("Transformation type"));
PROPERTY_EDIT(m_alignToPlayer,	TXT("Align to player"));
PROPERTY_EDIT(m_onlyYaw,		TXT("Only yaw"));
END_CLASS_RTTI();
