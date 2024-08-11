/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "entity.h"

class CEntityGroup : public CEntity
{
	DECLARE_ENGINE_CLASS( CEntityGroup, CEntity, 0 )

public:
	CEntityGroup();
	virtual ~CEntityGroup();

	const TDynArray< CEntity* >& GetEntities() const;

	void AddEntity( CEntity* entity );
	void AddEntities( const TDynArray< CEntity* >& entities );

	// Removes entity from this group or lower groups
	Bool DeleteEntity( const CEntity* entity );

	// Removes all entities from this group
	void DeleteAllEntities();

	// Update bounding box
	void OnPostUpdateTransform();

	virtual void OnPostLoad() override;
	virtual void OnDestroyed( CLayer* layer ) override;

	RED_INLINE void Unlock() { m_locked = false; }
	RED_INLINE void Lock() { m_locked = true; }
	RED_INLINE Bool IsLocked() const { return m_locked; }
	RED_INLINE Bool IsEmpty() const { return m_entities.Empty(); }

	RED_MOCKABLE Box CalcBoundingBox();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;

protected:
	TDynArray< CEntity* >	m_entities;
	Box						m_boundingBox;
	Bool					m_locked;
};

BEGIN_CLASS_RTTI( CEntityGroup )
	PARENT_CLASS( CEntity )
	PROPERTY( m_entities )
	PROPERTY( m_locked )
END_CLASS_RTTI()
