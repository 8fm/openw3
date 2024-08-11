
#pragma once
#include "..\..\common\game\storySceneItems.h"
#include "..\..\common\game\storySceneVoicetagMapping.h"

class CEdSceneEditor;

class CEdSceneActorsProvider : public CObject , public IControllerSetupActorsProvider
{
	DECLARE_ENGINE_CLASS( CEdSceneActorsProvider, CObject, 0 );

	const CEdSceneEditor*				m_editor;

	Bool								m_syncMode;

	// TODO: entities, props, effects, lights etc. really should be combined into 1 array.
	// The IDs of these should be made unique etc. as well...
	TDynArray< THandle< CEntity > >		m_entities;
	TDynArray< THandle< CEntity > >		m_props;
	TDynArray< THandle< CEntity > >		m_effects;
	TDynArray< THandle< CEntity > >		m_lights;

	TDynArray< CJobSpawnEntity* >		m_entitiesJob;
	TDynArray< TPair< CName, THandle< CEntityTemplate > > > m_entitiesJobData;

	TDynArray< THandle< CEntity > >		m_externalEntities;
	TDynArray< THandle< CEntity > >		m_externalProps;

public:
	CEdSceneActorsProvider();

	void Init( const CEdSceneEditor* editor, const CStoryScene* scene );

	Bool AreAllActorsReady() const;
	void ProcessSpawningActors();

	void Rebuild( const CStoryScene* s );
	void Refresh( const CStoryScene* s );

	Bool RebuildLights( const CStoryScene* s );
	Bool RefreshLights( const CStoryScene* s );

	void ResetSceneContextActors();
	const TDynArray< THandle< CEntity > >& GetActorsForEditor();
	const TDynArray< THandle< CEntity > >& GetEffectsForEditor()		{ return m_effects; }
	const TDynArray< THandle< CEntity > >& GetLightsForEditor()			{ return m_lights; }

	//IControllerSetupActorsProvider
	virtual const TDynArray< THandle<CEntity> >* GetActors() override { return &m_entities; }
	virtual const TDynArray< THandle<CEntity> >* GetLights() override { return &m_lights; }
	virtual const TDynArray< THandle<CEntity> >* GetProps() override  { return &m_props; }
	virtual const TDynArray< THandle<CEntity> >* GetEffects() override{ return &m_effects; }

	void ClearAllSceneDataForActors();

	Bool IsSomethingFromActors( const CComponent* c ) const;

private:
	void AddExtraActors();
	Bool HasActor( const CName& vt ) const;
	Bool HasEntity( const CName& vt ) const;
	Bool RefreshLight( const String& name, ELightType type ) const;
	void SpawnEntity( CWorld* world, EntitySpawnInfo&& info, const CName& vt );
	void OverrideSpawnClass( EntitySpawnInfo& info );

	void InitializeActorsItems();
	void WaitForAllActorsItems();

	CEntity* FindActorById( CName id );
	CEntity* FindPropById( CName id );
	CEntity* FindEffectById( CName id );
	CEntity* FindLightById( CName id );
	CEntity* FindAnyEntityByIdforLight( CName id );
};

BEGIN_CLASS_RTTI( CEdSceneActorsProvider );
	PARENT_CLASS( CObject );
	PROPERTY_NOSERIALIZE( m_entities );
END_CLASS_RTTI();
