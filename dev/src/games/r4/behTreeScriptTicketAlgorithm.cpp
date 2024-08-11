#include "build.h"
#include "behTreeScriptTicketAlgorithm.h"

#include "behTreeTicketData.h"
#include "r4BehTreeInstance.h"

IMPLEMENT_ENGINE_CLASS( ITicketAlgorithmScriptDefinition );
IMPLEMENT_ENGINE_CLASS( ITicketAlgorithmScript );
IMPLEMENT_ENGINE_CLASS( CBehTreeScriptTicketAlgorithmDefinition );


////////////////////////////////////////////////////////////////////////////
// ITicketAlgorithmScriptDefinition
////////////////////////////////////////////////////////////////////////////
THandle< ITicketAlgorithmScript > ITicketAlgorithmScriptDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	THandle< IScriptable > script = SpawnInstanceBase( context );

	ITicketAlgorithmScript* algorithm = Cast< ITicketAlgorithmScript > ( script.Get() );

	if ( !algorithm )
	{
		AI_LOG( TXT("Problem while spawning ITicketAlgorithmScript instance!") );
		return nullptr;
	}

	THandle< ITicketAlgorithmScript > algHandle( algorithm );
	return algHandle;
}

////////////////////////////////////////////////////////////////////////////
// ITicketAlgorithmScript
////////////////////////////////////////////////////////////////////////////
Float ITicketAlgorithmScript::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	m_node = node;
	m_ticket = &ticket;
	m_overrideTicketsCount = count;
	Float importance = 0.f;
	CallFunctionRet< Float >( this, CNAME( CalculateTicketImportance ), importance );
	count = Uint16(m_overrideTicketsCount);
	m_node = NULL;
	m_ticket = NULL;
	return importance;
}

void ITicketAlgorithmScript::funcGetActor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CActor > actor = m_owner->GetActor();
	RETURN_HANDLE( CActor, actor );
}
void ITicketAlgorithmScript::funcGetNPC( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CNewNPC > npc = m_owner->GetNPC();
	RETURN_HANDLE( CNewNPC, npc );
}
void ITicketAlgorithmScript::funcGetLocalTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float f = m_owner->GetLocalTime();
	RETURN_FLOAT( f );
}
void ITicketAlgorithmScript::funcGetActionTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CNode > node = m_owner->GetActionTarget();
	RETURN_HANDLE( CNode, node );
}
void ITicketAlgorithmScript::funcGetCombatTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CActor > actor = m_owner->GetCombatTarget();
	RETURN_HANDLE( CActor, actor );
}
void ITicketAlgorithmScript::funcGetTimeSinceMyAcquisition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float t = m_ticket->GetTimeSinceMyLastAquisition();
	RETURN_FLOAT( t );
}

void ITicketAlgorithmScript::funcGetTimeSinceAnyAcquisition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Float t = m_ticket->GetTimeSinceAnyoneLastAquisition();
	RETURN_FLOAT( t );
}

void ITicketAlgorithmScript::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool b = m_node->IsActive();
	RETURN_BOOL( b );
	
}
void ITicketAlgorithmScript::funcHasTicket(CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool b = m_ticket->HasTicket();
	RETURN_BOOL( b );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeScriptTicketAlgorithmDefinition
////////////////////////////////////////////////////////////////////////////
CBehTreeScriptTicketAlgorithmDefinition::CBehTreeScriptTicketAlgorithmDefinition()
	: m_scriptDef( NULL )
	, m_importanceMultiplier( 1.f )
{}
IBehTreeTicketAlgorithmInstance* CBehTreeScriptTicketAlgorithmDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeScriptTicketAlgorithmInstance( *this, owner, context );
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeScriptTicketAlgorithmInstance
////////////////////////////////////////////////////////////////////////////
CBehTreeScriptTicketAlgorithmInstance::CBehTreeScriptTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmInstance( definition, owner, context )
	, m_importanceMultiplier( definition.m_importanceMultiplier.GetVal( context ) )
{
	m_script = ( definition.m_scriptDef != nullptr ) ? definition.m_scriptDef->SpawnInstance( owner, context ) : nullptr;
	if ( m_script )
	{
		m_script->SetOwner( owner );
	}
}


CBehTreeScriptTicketAlgorithmInstance::~CBehTreeScriptTicketAlgorithmInstance()
{
}

Float CBehTreeScriptTicketAlgorithmInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	if ( m_script )
	{
		return m_script->CalculateTicketImportance( node, ticket, count ) * m_importanceMultiplier;
	}
	return 0.f;
}
