#pragma once

#include "behTreeTicketAlgorithm.h"

class ITicketAlgorithmScript;

////////////////////////////////////////////////////////////////////////////
// This classes are meant to expose ticket interface to scripts.
////////////////////////////////////////////////////////////////////////////

// Script level definition
class ITicketAlgorithmScriptDefinition : public IBehTreeObjectDefinition
{
	DECLARE_RTTI_SIMPLE_CLASS( ITicketAlgorithmScriptDefinition );
public:
	THandle< ITicketAlgorithmScript > SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const;
};

BEGIN_CLASS_RTTI( ITicketAlgorithmScriptDefinition );
	PARENT_CLASS( IBehTreeObjectDefinition );
END_CLASS_RTTI();

// Script level instance
class ITicketAlgorithmScript : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( ITicketAlgorithmScript );
protected:
	CBehTreeInstance*			m_owner;
	IBehTreeNodeInstance*		m_node;
	const CBehTreeTicketData*	m_ticket;
	Int32						m_overrideTicketsCount;
public:
	ITicketAlgorithmScript()
		: m_owner( nullptr )
		, m_overrideTicketsCount( 0 )												{ EnableReferenceCounting( true ); }

	void				SetOwner( CBehTreeInstance* owner )							{ m_owner = owner; }
	CBehTreeInstance*	GetOwner() const											{ return m_owner; }
	Float				CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count );

private:
	void funcGetActor( CScriptStackFrame& stack, void* result );
	void funcGetNPC( CScriptStackFrame& stack, void* result );
	void funcGetLocalTime( CScriptStackFrame& stack, void* result );
	void funcGetActionTarget( CScriptStackFrame& stack, void* result );
	void funcGetCombatTarget( CScriptStackFrame& stack, void* result );
	void funcGetTimeSinceMyAcquisition( CScriptStackFrame& stack, void* result );
	void funcGetTimeSinceAnyAcquisition( CScriptStackFrame& stack, void* result );
	void funcIsActive( CScriptStackFrame& stack, void* result );
	void funcHasTicket(CScriptStackFrame& stack, void* result );
};
BEGIN_CLASS_RTTI( ITicketAlgorithmScript );
	PARENT_CLASS( IScriptable );
	PROPERTY_NOSERIALIZE( m_overrideTicketsCount );
	NATIVE_FUNCTION( "GetActor", funcGetActor );
	NATIVE_FUNCTION( "GetNPC", funcGetNPC );
	NATIVE_FUNCTION( "GetLocalTime", funcGetLocalTime );
	NATIVE_FUNCTION( "GetActionTarget", funcGetActionTarget );
	NATIVE_FUNCTION( "GetCombatTarget", funcGetCombatTarget );
	NATIVE_FUNCTION( "GetTimeSinceMyAcquisition", funcGetTimeSinceMyAcquisition );
	NATIVE_FUNCTION( "GetTimeSinceAnyAcquisition", funcGetTimeSinceAnyAcquisition );
	NATIVE_FUNCTION( "IsActive", funcIsActive );
	NATIVE_FUNCTION( "HasTicket", funcHasTicket );
END_CLASS_RTTI();

// Code level definition
class CBehTreeScriptTicketAlgorithmDefinition : public IBehTreeTicketAlgorithmDefinition
{
	friend class CBehTreeScriptTicketAlgorithmInstance;

	DECLARE_ENGINE_CLASS( CBehTreeScriptTicketAlgorithmDefinition, IBehTreeTicketAlgorithmDefinition, 0 );
protected:
	THandle< ITicketAlgorithmScriptDefinition >	m_scriptDef;
	CBehTreeValFloat							m_importanceMultiplier;
public:
	CBehTreeScriptTicketAlgorithmDefinition();

	IBehTreeTicketAlgorithmInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeScriptTicketAlgorithmDefinition );
	PARENT_CLASS( IBehTreeTicketAlgorithmDefinition );
	PROPERTY_INLINED( m_scriptDef, TXT( "Script object definition" ) );
	PROPERTY_EDIT( m_importanceMultiplier, TXT( "Importance multiplier" ) );
END_CLASS_RTTI();

// Code level instance
class CBehTreeScriptTicketAlgorithmInstance : public IBehTreeTicketAlgorithmInstance
{
protected:
	THandle< ITicketAlgorithmScript >	m_script;
	Float								m_importanceMultiplier;
public:
	typedef CBehTreeScriptTicketAlgorithmDefinition Definition;

	CBehTreeScriptTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context );
	~CBehTreeScriptTicketAlgorithmInstance();

	Float CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count ) override;
};



