#include "build.h"
#include "performableAction.h"
#include "../core/scriptStackFrame.h"
#include "entity.h"
#include "node.h"


IMPLEMENT_ENGINE_CLASS( IPerformableAction );
IMPLEMENT_ENGINE_CLASS( CScriptedAction );

RED_DEFINE_STATIC_NAME( PerformArgNode )
RED_DEFINE_STATIC_NAME( PerformArgFloat )

void IPerformableAction::PerformAll( const TDynArray< IPerformableAction* >& actionsToPerform, CEntity* parent )
{
	for( Uint32 i=0; i<actionsToPerform.Size(); ++i )
	{
		if( actionsToPerform[ i ] )
		{
			actionsToPerform[ i ]->Perform( parent );
		}
	}
}

void IPerformableAction::Perform( CEntity* parent )
{
	ASSERT( false, TXT("NOT IMPLEMENTED") );
	ASSUME( false );
}

void IPerformableAction::Perform( CEntity* parent, CNode* node )
{
	Perform( parent );
}

void IPerformableAction::Perform( CEntity* parent, float value )
{
	Perform( parent );
}

void IPerformableAction::funcTrigger( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, parent, NULL );
	FINISH_PARAMETERS;

	Perform( parent.Get() );
}

void IPerformableAction::funcTriggerArgNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, parent, NULL );
	GET_PARAMETER( THandle< CNode >, node, NULL );
	FINISH_PARAMETERS;

	Perform( parent.Get(), node.Get() );
}

void IPerformableAction::funcTriggerArgFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, parent, NULL );
	GET_PARAMETER( float, value, 0.0f );
	FINISH_PARAMETERS;

	Perform( parent.Get(), value );
}

/////////////////////////////////////////////////////////////////

void CScriptedAction::Perform( CEntity* parent )
{
	CallFunction( this, CNAME( Perform ), THandle< CEntity >( parent ) );
}

void CScriptedAction::Perform( CEntity* parent, CNode* node )
{
	CallFunction( this, CNAME( PerformArgNode ), THandle< CEntity >( parent ), THandle< CNode >( node ) );
}

void CScriptedAction::Perform( CEntity* parent, float value )
{
	CallFunction( this, CNAME( PerformArgFloat ), THandle< CEntity >( parent ), value );
}
