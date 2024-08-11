#pragma once

#include "behTreeNodeSpecial.h"
#include "behTreeNodeArbitrator.h"

class CBehTreeNodePlaySceneInstance;

struct SPlaySceneRequestData
{
	DECLARE_RTTI_STRUCT( SPlaySceneRequestData );
public:
	SPlaySceneRequestData( Bool enablePlayScene = true, IBehTreeNodeDefinition::Priority priority = 0, Bool gameplayScene = false )
		: m_enablePlayScene( enablePlayScene )
		, m_scenePriority( priority )
		, m_isGameplayScene( gameplayScene )
		, m_handled( false )											{}
	Bool										m_enablePlayScene;
	IBehTreeNodeDefinition::Priority			m_scenePriority;
	Bool										m_isGameplayScene;

	Bool										m_handled;

	static const CName& EVENT_ID;

};

BEGIN_CLASS_RTTI( SPlaySceneRequestData );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlaySceneDefinition : public IBehTreeNodeSpecialDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlaySceneDefinition, IBehTreeNodeSpecialDefinition, CBehTreeNodePlaySceneInstance, PlayScene );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePlaySceneDefinition );
	PARENT_CLASS( IBehTreeNodeSpecialDefinition );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlaySceneInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;
protected:
	Bool				m_isGameplayScene;
public:
	typedef CBehTreeNodePlaySceneDefinition Definition;

	CBehTreeNodePlaySceneInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	
	Bool Activate() override;
	void Deactivate() override;
	////////////////////////////////////////////////////////////////////
	Bool IsAvailable() override;
	Int32 Evaluate() override;
	////////////////////////////////////////////////////////////////////
	Bool OnListenedEvent( CBehTreeEvent& e );
	Bool OnEvent( CBehTreeEvent& e ) override;
	void OnDestruction() override;
};