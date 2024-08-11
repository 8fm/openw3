#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodePokeDecoratorInstance;


////////////////////////////////////////////////////////////////////////
// CBehTreeNodePokeDecoratorDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePokeDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePokeDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodePokeDecoratorInstance, PokeScriptedAction );
	CBehTreeValCName	m_pokeEvent;
	CBehTreeValBool		m_acceptPokeOnlyIfActive;
protected:

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodePokeDecoratorDefinition()
		: m_pokeEvent()
		, m_acceptPokeOnlyIfActive( false )								{}

};


BEGIN_CLASS_RTTI( CBehTreeNodePokeDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_pokeEvent, TXT( "event that will interrupt the scripted action" ) );
	PROPERTY_EDIT( m_acceptPokeOnlyIfActive, TXT( "Accept poke event only if node is active") );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////////////////
// CBehTreeNodePokeDecoratorInstance
/////////////////////////////////////////////////////////////////////////////////
class CBehTreeNodePokeDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName m_pokeEvent;
	Bool m_acceptPokeOnlyIfActive;
	Bool m_poked;
public:
	typedef CBehTreeNodePokeDecoratorDefinition Definition;

	CBehTreeNodePokeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;

	void Deactivate() override;
	void Update() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	// state saving
	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;
};


