/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once


///////////////////////////////////////////////////////////////////////////////

enum EAgentLOD
{
	ALOD_Invisible,
	ALOD_CloseDistance
};

///////////////////////////////////////////////////////////////////////////////

enum ELODWeight : CEnum::TValueType
{
	LW_HIGHEST,
	LW_HIGH,
	LW_MEDIUM,
	LW_LOW,
	LW_LOWEST
};

///////////////////////////////////////////////////////////////////////////////

class CSpawnManager;
class CAgentsWorld;
class ISpawnStrategy;

///////////////////////////////////////////////////////////////////////////////

/**
 * A common agent interface.
 */
class IAgent
{
private:
	EAgentLOD			m_newLod;
	Float				m_lodWeight;

public:
	virtual ~IAgent() {}

	//! Returns the current world position of the agent.
	virtual const Vector& GetWorldPositionRef() const = 0;

	//! Sets the LOD weight for the agent
	RED_INLINE void SetLODWeight( Float val ) { m_lodWeight = val; }

	//! Returns the LOD weight for the agent
	RED_INLINE Float GetLODWeigth() const { return m_lodWeight; }

	//! Request a LOD change on the agent.
	RED_INLINE void SetLOD( EAgentLOD lod ) { m_newLod = lod; }

	//! Returns the current lod of the agent.
	virtual EAgentLOD GetLOD() const = 0;

protected:
	IAgent();

	friend class CAgentsWorld;
	friend class SpawnedAgentsIterator;

	//! Processes the LOD change request.
	Bool ProcessLODChange( Float deltaTime );

	//! Recalculates the LOD value
	void CalculateLOD( const CAgentsWorld& world );

	//!Notification about an LOD being changed.
	virtual Bool OnLODChanged( EAgentLOD newLod ) { return true; }

	//! Returns the current lod of the agent.
	RED_INLINE EAgentLOD GetDesiredLOD() const { return m_newLod; }

	//! Returns a spawn strategy according to which the agent should adjust its LOD
	virtual const ISpawnStrategy& GetSpawnStrategy() const = 0;

	//! Reduces the LOD of the agent to minimum
	RED_INLINE void SetBudgetLOD() { m_newLod = ALOD_Invisible; }
};

///////////////////////////////////////////////////////////////////////////////
