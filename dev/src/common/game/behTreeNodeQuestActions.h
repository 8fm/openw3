#pragma once

#include "behTreeDecorator.h"
#include "behTreeMetanode.h"

class CBehTreeNodeQuestActionsCompletedInstance;
class CBehTreeNodeExternalListenerInstance;

////////////////////////////////////////////////////////////////////////
// External tree listener
////////////////////////////////////////////////////////////////////////
class CBehTreeExternalListener
{
private:
	CBehTreeNodeExternalListenerInstance*		m_node;

public:
	CBehTreeExternalListener()
		: m_node( NULL )												{}
	virtual ~CBehTreeExternalListener();

	// goal lifetime tracking - more methods can be added with ease
	virtual void OnBehaviorCompletion( Bool success );
	virtual void OnBehaviorDestruction();
	virtual void OnBehaviorEvent( CBehTreeEvent& e );

	void Unregister();
	void Register( CBehTreeNodeExternalListenerInstance* node );
};

struct SBehTreeExternalListenerPtr
{
	DECLARE_RTTI_STRUCT( SBehTreeExternalListenerPtr )

	CBehTreeExternalListener*					m_ptr;
};

BEGIN_NODEFAULT_CLASS_RTTI( SBehTreeExternalListenerPtr )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// Special internal action that notifies quest action block of
// scripted actions completion.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeExternalListenerDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeExternalListenerDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeExternalListenerInstance, ExternalListener );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	//static Bool IsInternalNode()										{ return true; }
};
BEGIN_CLASS_RTTI( CBehTreeNodeExternalListenerDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();


class CBehTreeNodeExternalListenerInstance : public IBehTreeNodeDecoratorInstance
{
	friend class CBehTreeExternalListener;
	typedef IBehTreeNodeDecoratorInstance Super;
	
protected:
	CBehTreeExternalListener*			m_listener;

public:
	typedef CBehTreeNodeExternalListenerDefinition Definition;

	CBehTreeNodeExternalListenerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool OnEvent( CBehTreeEvent& e ) override;
	void Complete( eTaskOutcome outcome ) override;
	void OnDestruction() override;
};


class CBehTreeNodeScriptedActionsListReaderDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeScriptedActionsListReaderDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, ScriptedActionsListReader );
protected:
	CName								m_actionListVar;
public:
	//static Bool IsInternalNode()										{ return true; }

	void SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const override;
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

	Bool OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const;
	//Bool IsMyInstance( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const;

};

BEGIN_CLASS_RTTI( CBehTreeNodeScriptedActionsListReaderDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_EDIT( m_actionListVar, TXT("Action list parameters variable name") );
END_CLASS_RTTI();