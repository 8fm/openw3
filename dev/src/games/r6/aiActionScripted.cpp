/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiActionScripted.h"

#if 0 // see .h for explaination

IMPLEMENT_ENGINE_CLASS( CAIActionScripted )

CAIActionScripted::CAIActionScripted()
	: m_ticked( false )
{	
}

Bool CAIActionScripted::CanBeStartedOn( CComponent* component ) const
{
	Bool result( false );
	
	// i REALLY don't like the const_cast<> here, and REALLY think that scripts should support const.
	if ( false == CallFunctionRet< Bool, THandle< CComponent > > ( const_cast< CAIActionScripted* > ( this ), CNAME( CanBeStartedOn ), component, result ) )
	{
		return false;
	}

	return result;
}

EAIActionStatus CAIActionScripted::StartOn( CComponent* component )
{
	EAIActionStatus result( ACTION_InProgress );	
	if ( false == CallFunctionRet< EAIActionStatus, THandle< CComponent > > ( this, CNAME( OnStart ), component, result ) )
	{
		return Stop( ACTION_Failed );
	}

	if ( result != ACTION_InProgress )
	{
		m_status = result;
		return result;
	}

	return TBaseClass::StartOn( component );
}


EAIActionStatus CAIActionScripted::Tick( Float timeDelta )
{
	EAIActionStatus result( m_status );	
	CallFunctionRet< EAIActionStatus, Float > ( this, CNAME( OnTick ), timeDelta, result );

	if ( result != ACTION_InProgress )
	{
		return result;
	}

	return TBaseClass::Tick( timeDelta );
}

EAIActionStatus CAIActionScripted::Stop( EAIActionStatus newStatus )
{
	TBaseClass::Stop( newStatus );

	EAIActionStatus result( m_status );	
	CallFunctionRet< EAIActionStatus > ( this, CNAME( OnStop ), result );

	return result;  
}

EAIActionStatus CAIActionScripted::Reset()
{
	CallFunction( this, CNAME( OnReset ) );
	return TBaseClass::Reset();
}

void CAIActionScripted::funcStop( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EAIActionStatus, newStatus, ACTION_Failed ); 
	FINISH_PARAMETERS;
	RETURN_ENUM( Stop( newStatus ) );
}

void CAIActionScripted::funcGetStatus( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_ENUM( GetStatus() );
}

#endif