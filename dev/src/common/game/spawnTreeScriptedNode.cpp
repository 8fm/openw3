/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeScriptedNode.h"

#include "encounter.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/scriptingSystem.h"

IMPLEMENT_ENGINE_CLASS( ISpawnTreeScriptedDecorator )

inline void ISpawnTreeScriptedDecorator::Script_GetFriendlyName( String& outName )
{
	InitializeScriptFunctions();
	if ( (m_scriptFlags & SF_HAS_GetFriendlyName) == 0 )
	{
		outName = GetClass()->GetName().AsString();
		return;
	}
	CallFunctionRet< String >( const_cast< ISpawnTreeScriptedDecorator* >( this ), CNAME( GetFriendlyName ), outName );
}
inline void ISpawnTreeScriptedDecorator::Script_Activate( CSpawnTreeInstance& instance )
{
	InitializeScriptFunctions();
	if ( (m_scriptFlags & SF_HAS_OnActivate) == 0 )
	{
		return;
	}
	THandle< CEncounter > encounter = instance.GetEncounter();
	THandle< IScriptable > userDataObject;
	CallFunctionRet< THandle< IScriptable > >( const_cast< ISpawnTreeScriptedDecorator* >( this ), CNAME( OnActivate ), encounter, userDataObject );
	instance[ i_userData ]	= userDataObject;
	m_mainTerminated		= false;

	if ( (m_scriptFlags & SF_HAS_Main) == 0 )
	{
		return;
	}

	if ( m_mainTerminated == false && m_thread == nullptr )
	{
		IScriptable* context	= this;
		const CFunction* fun	= this->FindFunction( context, CNAME( Main ) );
		const Uint32 stackSize	= fun->GetStackSize();
		void* stackData			= RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ScriptObject, stackSize );
		Red::System::MemorySet( stackData, 0, stackSize );
		
		const THandle< IScriptable > & userData = instance[ i_userData ];

		// Find matching function parameter
		const Uint32 numFunctionParams = static_cast< Uint32 >( fun->GetNumParameters() );
	
		if ( numFunctionParams == 0 )
		{
			return;
		}

		CProperty* funcParam = fun->GetParameter(0);
		// Copy value
		if ( funcParam->GetType()->GetType() != RT_Handle )
		{
			return;
		}

		void* destData = funcParam->GetOffsetPtr( stackData );
		THandle< IScriptable >& handleDest	= * ( THandle< IScriptable >* )( destData );
		handleDest						= userData;
		
		CScriptStackFrame* frame = new CScriptStackFrame( NULL, context, fun, stackData, stackData );
		m_thread = GScriptingSystem->CreateThreadUseGivenFrame( context, fun, *frame );
		m_thread->SetListener( this );
	}
}

inline void ISpawnTreeScriptedDecorator::Script_Deactivate( CSpawnTreeInstance& instance )
{
	InitializeScriptFunctions();
	if ( (m_scriptFlags & SF_HAS_OnDeactivate) == 0 )
	{
		return;
	}
	THandle< CEncounter > encounter = instance.GetEncounter();
	CallFunction( const_cast< ISpawnTreeScriptedDecorator* >( this ), CNAME( OnDeactivate ), encounter );

	if ( m_thread )
	{
		m_thread->SetListener( nullptr );

		if ( m_mainTerminated == false )
		{
			m_mainTerminated = true;
			m_thread->ForceKill();
		}

		m_thread = nullptr;
	}
}

void ISpawnTreeScriptedDecorator::Script_OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	InitializeScriptFunctions();
	if ( (m_scriptFlags & SF_HAS_OnFullRespawn) == 0 )
	{
		return;
	}
	THandle< CEncounter > encounter = instance.GetEncounter();
	CallFunction( const_cast< ISpawnTreeScriptedDecorator* >( this ), CNAME( OnFullRespawn ), encounter );
}

void ISpawnTreeScriptedDecorator::InitializeScriptFunctions() const
{
	if ( m_scriptFlags != SF_NOT_INITIALIZED )
	{
		return;
	}

	auto funCheck =
		[ this ]( const CName& c ) -> Bool
	{
		IScriptable* context = const_cast< ISpawnTreeScriptedDecorator* >( this );
		const CFunction* fun = FindFunction( context, c );
		return fun != NULL;
	};


	m_scriptFlags = SF_DEFAULTS;

	if ( funCheck( CNAME( OnActivate ) ) )
	{
		m_scriptFlags |= SF_HAS_OnActivate;
	}

	if ( funCheck( CNAME( Main ) ) )
	{
		m_scriptFlags |= SF_HAS_Main;
	}

	if ( funCheck( CNAME( OnDeactivate ) ) )
	{
		m_scriptFlags |= SF_HAS_OnDeactivate;
	}

	if ( funCheck( CNAME( GetFriendlyName ) ) )
	{
		m_scriptFlags |= SF_HAS_GetFriendlyName;
	}

	if ( funCheck( CNAME( OnFullRespawn ) ) )
	{
		m_scriptFlags |= SF_HAS_OnFullRespawn;
	}
}


void ISpawnTreeScriptedDecorator::OnScriptThreadKilled( CScriptThread* thread, Bool finished )
{
	m_thread			= nullptr;
	m_mainTerminated	= true;
}

String ISpawnTreeScriptedDecorator::GetDebugName() const
{
	return String(TXT("ISpawnTreeScriptedDecorator"));
}

void ISpawnTreeScriptedDecorator::Activate( CSpawnTreeInstance& instance )
{
	TBaseClass::Activate( instance );

	Script_Activate( instance );
}

void ISpawnTreeScriptedDecorator::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_userData;
}
void ISpawnTreeScriptedDecorator::Deactivate( CSpawnTreeInstance& instance )
{
	TBaseClass::Deactivate( instance );

	Script_Deactivate( instance );
}

void ISpawnTreeScriptedDecorator::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	Script_OnFullRespawn( instance );

	TBaseClass::OnFullRespawn( instance );
}

Bool ISpawnTreeScriptedDecorator::IsSpawnableByDefault() const
{
	return GetClass() != GetStaticClass();
}
String ISpawnTreeScriptedDecorator::GetEditorFriendlyName() const
{
	String friendlyName;
	const_cast< ISpawnTreeScriptedDecorator* >( this )->Script_GetFriendlyName( friendlyName );
	return String::Printf( TXT("Script %s"), friendlyName.AsChar() );
}
String ISpawnTreeScriptedDecorator::GetBlockCaption() const
{
	String friendlyName;
	const_cast< ISpawnTreeScriptedDecorator* >( this )->Script_GetFriendlyName( friendlyName );
	return friendlyName;
}

