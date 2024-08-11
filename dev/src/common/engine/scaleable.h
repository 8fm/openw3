/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "budgetedContainer.h"
#include "gameplayConfig.h"

class CLODableManager;
class CTickManager;

class ILODable
{
DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( ILODable, lodable )

public:
	enum LOD
	{
		LOD_0 = 0,
		LOD_1,
		LOD_2,

		LOD_COUNT
	};

protected:
	LOD m_currentLOD;

public:
	RED_FORCE_INLINE ILODable() : m_currentLOD( LOD_0 ) {}
	virtual ~ILODable(){}
	RED_FORCE_INLINE void SetCurrentLOD( ILODable::LOD lod ) { m_currentLOD = lod; }
	RED_FORCE_INLINE LOD GetCurrentLOD() const { return m_currentLOD; }
	virtual LOD ComputeLOD( CLODableManager* manager ) const = 0;
	virtual void UpdateLOD( LOD lod, CLODableManager* manager ) = 0;
};

class CLODableManager : public Red::NonCopyable
{
protected:
	typedef TBudgetedContainer< ILODable, CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( ILODable, lodable ) > LODablesContainer;

	CTickManager*		m_tickManager;

	LODablesContainer	m_lodables;
	Bool				m_forceUpdateAll;

	Float				m_budgetableDistance;
	Float				m_budgetableDistanceSqr;
	Float				m_disableDistance;
	Float				m_disableDistanceSqr;

	Vector				m_position;
	Bool				m_positionValid;

public:
	CLODableManager();

	void SetTickManager( CTickManager* tickManager ) { m_tickManager = tickManager; }
	RED_FORCE_INLINE CTickManager* GetTickManager() const { return m_tickManager; }

	// setup reference position for streaming and other calculations
	void SetReferencePosition( const Vector& position );

	void SetMaxUpdateTime( Float maxUpdateTime );
	void SetDistances( Float budgetableDistance, Float disableDistance );

	RED_FORCE_INLINE void Register( ILODable* lodable, Bool budgeted = true ) { m_lodables.Add( lodable, budgeted ); }
	RED_FORCE_INLINE void Unregister( ILODable* lodable, Bool budgeted = true ) { m_lodables.Remove( lodable, budgeted ); }

	RED_FORCE_INLINE const Vector& GetPosition() const { return m_position; }
	RED_FORCE_INLINE Float GetBudgetableDistanceSqr() const { return m_budgetableDistanceSqr; }
	RED_FORCE_INLINE Float GetDisableDistanceSqr() const { return m_disableDistanceSqr; }

	void ForceUpdateAll() { m_forceUpdateAll = true; }
	void UpdateLODs();
};

