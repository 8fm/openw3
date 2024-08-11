/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CCommunityAreaType : public CObject
{
	DECLARE_ENGINE_CLASS( CCommunityAreaType, CObject, 0 );

public:
	virtual void OnEnter( CTriggerAreaComponent *triggerArea ){}
	virtual void OnExit(){}
};

BEGIN_CLASS_RTTI( CCommunityAreaType );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCommunityAreaTypeDefault : public CCommunityAreaType
{
	DECLARE_ENGINE_CLASS( CCommunityAreaTypeDefault, CCommunityAreaType, 0 );

public:
	CCommunityAreaTypeDefault() : m_areaDespawnRadius( 0.0f ) {}

	virtual void OnEnter( CTriggerAreaComponent *triggerArea );
	virtual void OnExit();

private:
	Float m_areaSpawnRadius;
	Float m_areaDespawnRadius;

	Float m_spawnRadius;
	Float m_despawnRadius;
	Bool  m_dontRestore; // if true than spawn/despawn radius will not be restored on exit

	Float m_prevSpawnRadius;
	Float m_prevDespawnRadius;

// debug
public:
	void GetAreaData( Float &areaSpawnRadius, Float &areaDespawnRadius, Float &spawnRadius, Float &despawnRadius, Bool &dontRestore, Float &prevSpawnRadius, Float &prevDespawnRadius );
};

BEGIN_CLASS_RTTI( CCommunityAreaTypeDefault );
	PARENT_CLASS( CCommunityAreaType );
	PROPERTY_EDIT( m_areaSpawnRadius, TXT("Area spawn radius") );
	PROPERTY_EDIT( m_areaDespawnRadius, TXT("Area despawn radius") );
	PROPERTY_EDIT( m_spawnRadius, TXT("Visibility spawn radius (>0 - active)") );
	PROPERTY_EDIT( m_despawnRadius, TXT("Visibility despawn radius (>0 - active)") );
	PROPERTY_EDIT( m_dontRestore, TXT("Don't restore spawn/despawn radius on area exit.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCommunityAreaTypeSpawnRadius : public CCommunityAreaType
{
	DECLARE_ENGINE_CLASS( CCommunityAreaTypeSpawnRadius, CCommunityAreaType, 0 );

public:
	CCommunityAreaTypeSpawnRadius() : 
	  m_spawnRadius( 0 ), m_despawnRadius( 0 ), m_prevSpawnRadius( 0 ), m_prevDespawnRadius( 0 ), m_dontRestore( false ) {}

	virtual void OnEnter( CTriggerAreaComponent *triggerArea );
	virtual void OnExit();

private:
	Float m_spawnRadius;
	Float m_despawnRadius;
	Bool  m_dontRestore; // if true than spawn/despawn radius will not be restored on exit
	
	Float m_prevSpawnRadius;
	Float m_prevDespawnRadius;

// debug
public:
	void GetAreaData( Float &spawnRadius, Float &despawnRadius, Bool &dontRestore, Float &prevSpawnRadius, Float &prevDespawnRadius );
};

BEGIN_CLASS_RTTI( CCommunityAreaTypeSpawnRadius );
	PARENT_CLASS( CCommunityAreaType );
	PROPERTY_EDIT( m_spawnRadius, TXT("Visibility spawn radius") );
	PROPERTY_EDIT( m_despawnRadius, TXT("Visibility despawn radius") );
	PROPERTY_EDIT( m_dontRestore, TXT("Don't restore spawn/despawn radius on area exit.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CCommunityArea : public CEntity
{
	DECLARE_ENGINE_CLASS( CCommunityArea, CEntity, 0 );

public:
	CCommunityArea();

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	// Something has entered trigger area owned by this entity
	virtual void OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator );

	// Something has exited trigger area owned by this entity
	virtual void OnAreaExit( CTriggerAreaComponent* area, CComponent* activator );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

// for debug
public:
	CCommunityAreaType* GetAreaType() const { return m_communityAreaType; }

private:
	CTriggerAreaComponent*	m_triggerArea;

	CCommunityAreaType	*m_communityAreaType;
};

BEGIN_CLASS_RTTI( CCommunityArea );
	PARENT_CLASS( CEntity );
	PROPERTY_INLINED( m_communityAreaType, TXT("Area type") );
END_CLASS_RTTI();
