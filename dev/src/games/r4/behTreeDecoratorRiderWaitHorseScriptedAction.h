#pragma once

#include "../../common/game/behTreeNodeQuestActions.h" 
#include "../../common/game/questScriptedActionsAIParameters.h"

#include "ridingAiStorage.h"

class CBehTreeNodeRiderWaitHorseScriptedActionInstance;

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderWaitHorseScriptedActionDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeRiderWaitHorseScriptedActionInstance;
class CBehTreeNodeRiderWaitHorseScriptedActionDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRiderWaitHorseScriptedActionDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeRiderWaitHorseScriptedActionInstance, Rider_WaitHorseScriptedAction );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
};
BEGIN_CLASS_RTTI( CBehTreeNodeRiderWaitHorseScriptedActionDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeRiderWaitHorseScriptedActionInstance : public IBehTreeNodeDecoratorInstance, public CBehTreeExternalListener
{
	friend class CBehTreeExternalListener;
	typedef IBehTreeNodeDecoratorInstance Super;

protected:
	Bool									m_initSucceded;
	THandle<CAIQuestScriptedActionsTree>	m_horseDecoratorTree;

	CAIStorageRiderData::CStoragePtr		m_riderData;
	Int16									m_scriptedActionId;
	Bool									m_horseActionFinished;
	Bool									m_horseArrivedAtPath;
public:
	typedef CBehTreeNodeRiderWaitHorseScriptedActionDefinition Definition;

	CBehTreeNodeRiderWaitHorseScriptedActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void OnBehaviorCompletion( Bool success ) override;
	void OnBehaviorEvent( CBehTreeEvent& e ) override;
	IAIActionTree * GetAITree()const;

	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;

	Bool Activate() override;
	void Deactivate() override;
	void Update() override;
};


