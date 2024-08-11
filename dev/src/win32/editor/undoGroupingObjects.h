/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "undoStep.h"
#include "../../common/engine/node.h"

//////////////////////////////////////////////////////////////////////////
class CUndoGroupObjects: public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoGroupObjects, IUndoStep, 0 );

	struct SUndoEntityGroupInfo
	{
		String					m_name;
		TDynArray< CEntity* >	m_entities;
		THandle< CLayer >		m_parentLayer;

		SUndoEntityGroupInfo() { /* intentionally empty */ }
	};

public:
	static void CreateGroupStep( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups );
	static void CreateUngroupStep( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups );

private:
	CUndoGroupObjects() { /* intentionally empty */ }
	CUndoGroupObjects( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups, Bool grouping );

	void DoStep( Bool create );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

private:
	Bool								m_grouping;
	TDynArray< SUndoEntityGroupInfo >	m_entitiesInGroups;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoGroupObjects, IUndoStep );

//////////////////////////////////////////////////////////////////////////
class CUndoLockGroupObjects : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoLockGroupObjects, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CEntityGroup* group, Bool locking );

private:
	CUndoLockGroupObjects() { /* intentionally empty */ }
	CUndoLockGroupObjects( CEdUndoManager& undoManager, CEntityGroup* group, Bool locking );

	void DoStep( Bool locking );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

private:
	CEntityGroup*	m_group;
	Bool			m_locking;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoLockGroupObjects, IUndoStep );

//////////////////////////////////////////////////////////////////////////
class CUndoRemoveObjectFromGroup : public IUndoStep
{
	DECLARE_ENGINE_CLASS( CUndoRemoveObjectFromGroup, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, const TDynArray< CEntity* >& entities, Bool removing );

private:
	CUndoRemoveObjectFromGroup() { /* intentionally empty */ }
	CUndoRemoveObjectFromGroup( CEdUndoManager& undoManager, const TDynArray< CEntity* >& entities, Bool removing );

	void DoStep( Bool removing );
	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

private:
	THashMap< CEntityGroup*, TDynArray< CEntity* > >	m_info;
	Bool												m_removing;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoRemoveObjectFromGroup, IUndoStep );
