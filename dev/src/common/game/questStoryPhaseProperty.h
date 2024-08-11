/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class IQuestSpawnsetAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IQuestSpawnsetAction, CObject )

public:
	virtual ~IQuestSpawnsetAction() {}

	virtual void CollectContent( class IQuestContentCollector& collector ) const = 0;
	virtual void Perform() const {}
	virtual void ResetCachedData() const {}
};
BEGIN_ABSTRACT_CLASS_RTTI( IQuestSpawnsetAction )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CActivateStoryPhase : public IQuestSpawnsetAction, public ISpawnsetPhaseNamesGetter
{
	DECLARE_ENGINE_CLASS( CActivateStoryPhase, IQuestSpawnsetAction, 0 )

private:
	TSoftHandle< CCommunity >						m_spawnset;
	CName											m_phase;
	String											m_streamingPartition;

public:
	CActivateStoryPhase();

protected:
	virtual void CollectContent( class IQuestContentCollector& collector ) const override;
	virtual void Perform() const override;
	virtual void ResetCachedData() const override;
	void GetSpawnsetPhaseNames( IProperty *property, THashSet<CName> &outNames );
};

BEGIN_CLASS_RTTI( CActivateStoryPhase )
	PARENT_CLASS( IQuestSpawnsetAction )
	PROPERTY_EDIT( m_spawnset, TXT("Spawnset") )
	PROPERTY_CUSTOM_EDIT( m_phase, TXT( "Story phase" ), TXT( "SpawnsetPhasesEditor" ) )
	PROPERTY_CUSTOM_EDIT( m_streamingPartition, TXT( "Streaming partition" ), TXT("PartitionTree") );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CDeactivateSpawnset : public IQuestSpawnsetAction
{
	DECLARE_ENGINE_CLASS( CDeactivateSpawnset, IQuestSpawnsetAction, 0 )

private:
	THandle< CCommunity >	m_spawnset;

public:
	CDeactivateSpawnset();

	virtual void CollectContent( class IQuestContentCollector& collector ) const override;
	virtual void Perform() const override;
};
BEGIN_CLASS_RTTI( CDeactivateSpawnset )
	PARENT_CLASS( IQuestSpawnsetAction )
	PROPERTY_EDIT( m_spawnset, TXT("Spawnset") )
END_CLASS_RTTI()
