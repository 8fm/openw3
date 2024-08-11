#pragma once

#include "../core/intsBitField.h"

#include "edSpawnTreeNode.h"

class ISpawnTreeBaseNode;

//////////////////////////////////////////////////////////////////////////
class CEncounterCreatureDefinition : public CObject
#ifndef NO_EDITOR
	, public IEntityTemplatePropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CEncounterCreatureDefinition, CObject, 0 );

public:
	struct CreatureDefinitionOrder
	{
		static Bool Less( CEncounterCreatureDefinition* def1, CEncounterCreatureDefinition* def2 ) 
		{
			return def1->m_definitionName < def2->m_definitionName;
		}
		static Bool Less( CName name, CEncounterCreatureDefinition* def2 ) 
		{
			return name < def2->m_definitionName;
		}
		static Bool Less( CEncounterCreatureDefinition* def1, CName name ) 
		{
			return def1->m_definitionName < name;
		}
	};

	typedef TSortedArray< CEncounterCreatureDefinition*, CreatureDefinitionOrder > CompiledCreatureList;

	CEncounterCreatureDefinition()
		: m_totalSpawnLimit( 0 )
		, m_maxSpawnedAtOnce( 0 )
		, m_totalSpawned( 0 )
		, m_bodyCount( 0 )
		, m_override( false )
		, m_usedAppearancesCount( -1 )									{}

	CName		SelectUniqueAppearance( CEntityTemplate* entityTemplate );
	Bool		MarkAppearanceAsUsed( CEntityTemplate* entityTemplate, CName appearance );
	CName		GetDefinitionName() const								{ return m_definitionName; }

	void		OnCreatureSpawned()										{ ++m_totalSpawned; }
	void		OnCreatureDespawned()									{ --m_totalSpawned; }

	void		OnCreatureDead()										{ ++m_bodyCount; }
	void		FullRespawn()											{ m_bodyCount = 0; }

	void		SetOverride( Bool b )									{ m_override = b; }
	Bool		Override() const										{ return m_override; }
	const TagList& GetTags() const										{ return m_tags; }

	Bool		ShouldSave() const										{ return m_bodyCount > 0; }
	void		Save( IGameSaver* saver );
	void		Load( IGameLoader* loader );

	RED_INLINE const String& GetEntityTemplatePath() const				{ return m_entityTemplate.GetPath(); }
	RED_INLINE const TSoftHandle< CEntityTemplate >& GetEntityTemplate() { return m_entityTemplate; }

	static Bool	CheckEntityTemplate( CEntityTemplate* entityTemplate );
	
#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	RED_INLINE Bool WasSpawnLimitReached() const
	{
		if ( m_maxSpawnedAtOnce && m_totalSpawned >= m_maxSpawnedAtOnce )
		{
			return true;
		}
		if ( m_totalSpawnLimit && m_totalSpawned + m_bodyCount >= m_totalSpawnLimit )
		{
			return true;
		}
		return false;
	}

	RED_INLINE Bool WasSpawnLimitReached( Uint32 numToSpawn ) const
	{
		if ( m_maxSpawnedAtOnce && m_totalSpawned + numToSpawn > m_maxSpawnedAtOnce )
		{
			return true;
		}
		if ( m_totalSpawnLimit && m_totalSpawned + m_bodyCount + numToSpawn > m_totalSpawnLimit )
		{
			return true;
		}
		return false;
	}

	CName									m_definitionName;
	TSoftHandle< CEntityTemplate >			m_entityTemplate;

#ifndef NO_EDITOR
	// IEntityTemplatePropertyOwner interface
	CEntityTemplate* Editor_GetEntityTemplate()	override;
#endif

protected:
	Uint16									m_totalSpawnLimit;
	Uint16									m_maxSpawnedAtOnce;
	Int16									m_totalSpawned;
	Uint16									m_bodyCount;
	Bool									m_override;
	Int16									m_usedAppearancesCount;
	CName									m_forcedAppearance;
	TIntsBitField< 0 >						m_usedAppearances;
	TagList									m_tags;
};

BEGIN_CLASS_RTTI( CEncounterCreatureDefinition );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_definitionName, TXT("Unique creature definition name") );
	PROPERTY_EDIT( m_entityTemplate, TXT("Creature entity template") );
	PROPERTY_EDIT( m_totalSpawnLimit, TXT("Total number of creatures to be spawned (0 is infinite)") );
	PROPERTY_EDIT( m_maxSpawnedAtOnce, TXT("Maximum number of creatures spawned at once (0 is infinite)") );
	PROPERTY_EDIT( m_override, TXT("Override creature definitions from included templates") );
	PROPERTY_CUSTOM_EDIT( m_forcedAppearance, TXT("Forced appearance"), TXT("EntityAppearanceSelect") );
	PROPERTY_EDIT( m_tags, TXT("Creature extra tags") );
END_CLASS_RTTI();



class ICreatureDefinitionContainer : public IEdSpawnTreeNode
{
protected:
	ESpawnTreeType													m_spawnTreeType;

	virtual TDynArray< CEncounterCreatureDefinition* >& InternalGetCreatureDefinitions() = 0;
	virtual ISpawnTreeBaseNode*				InternalGetRootTreeNode() const = 0;
public:
	ICreatureDefinitionContainer()
		: m_spawnTreeType( STT_default )		{}

	ESpawnTreeType							GetSpawnTreeType()const { return m_spawnTreeType; }
	virtual CEncounterCreatureDefinition*	AddCreatureDefinition() = 0;
	virtual void							RemoveCreatureDefinition( CEncounterCreatureDefinition* ) =0;
	virtual CEncounterCreatureDefinition*	GetCreatureDefinition( CName name ) =0;
	void									GetCreatureDefinitions( TDynArray< CEncounterCreatureDefinition* >& outCreatureDefs ) { outCreatureDefs = InternalGetCreatureDefinitions(); }

	void									CompileCreatureDefinitions( CEncounterCreatureDefinition::CompiledCreatureList& defs );

	void									FillUpCreatureDefinition();
	void									ClearCreatureDefinitions();

	void									OnEditorOpened() override;
};
