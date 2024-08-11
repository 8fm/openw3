/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once


///////////////////////////////////////////////////////////////////////////////

class CAgentsWorld;
class IAgent;

///////////////////////////////////////////////////////////////////////////////

/**
 * A strategy that tells who can be spawned and who should be unspawned.
 */
class ISpawnStrategy
{
public:
	virtual ~ISpawnStrategy() {}

	virtual void SetLOD( const CAgentsWorld& world, IAgent& agent ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * A strategy that tells who can be spawned and who should be unspawned.
 */
class IEngineSpawnStrategy : public CObject, public ISpawnStrategy
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEngineSpawnStrategy, CObject );

public:
	virtual ~IEngineSpawnStrategy() {}

	// ------------------------------------------------------------------------
	// ISpawnStrategy implementation
	// ------------------------------------------------------------------------
	virtual void SetLOD( const CAgentsWorld& world, IAgent& agent ) const {}
};
BEGIN_ABSTRACT_CLASS_RTTI( IEngineSpawnStrategy )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////