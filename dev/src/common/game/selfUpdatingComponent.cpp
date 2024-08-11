#include "build.h"

#include "selfUpdatingComponent.h"
#include "../engine/tickManager.h"

IMPLEMENT_ENGINE_CLASS( CSelfUpdatingComponent );

CSelfUpdatingComponent::CSelfUpdatingComponent()
{
	m_useUpdateTransform = false;
}

void CSelfUpdatingComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CSelfUpdatingComponent_OnAttached );

	DecideIfItHasToTickScript();

	m_tickedNow	= false;
	if(m_tickedByDefault)
	{
		AddToTickGroups( world );
	}
}

void CSelfUpdatingComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	if(m_tickedNow)
	{
		RemoveFromTickGroups( world );
	}
}

void CSelfUpdatingComponent::DecideIfItHasToTickScript()
{
	// If we find the function, it means there is a script deriving from thins and we need to tick it later
	const CFunction* function	= NULL;
	IScriptable* context = this;
	m_scriptNeedsTicking = ::FindFunction( context, CNAME( OnComponentTick ), function );
}

void CSelfUpdatingComponent::AddToTickGroups( CWorld* world )
{
	if(!m_tickedNow)
	{
		world->GetTickManager()->AddToGroup( this, m_tickGroup );
		m_tickedNow	= true;
	}
}

void CSelfUpdatingComponent::RemoveFromTickGroups( CWorld* world )
{
	if(m_tickedNow)
	{
		world->GetTickManager()->RemoveFromGroup( this, m_tickGroup );
		m_tickedNow	= false;
	}
}

void CSelfUpdatingComponent::OnTickPrePhysics( Float timeDelta )
{
	TBaseClass::OnTickPrePhysics( timeDelta );
	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::OnTickPrePhysicsPost( Float timeDelta )
{
	TBaseClass::OnTickPrePhysicsPost( timeDelta );
	
	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::OnTickPostPhysics( Float timeDelta )
{
	TBaseClass::OnTickPostPhysics( timeDelta );

	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::OnTickPostPhysicsPost( Float timeDelta )
{
	TBaseClass::OnTickPostPhysicsPost( timeDelta );

	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::OnTickPostUpdateTransform( Float timeDelta )
{
	PC_SCOPE_PIX( CSelfUpdatingComponent_OnTickPostUpdateTransform );

	TBaseClass::OnTickPostUpdateTransform( timeDelta );

	CustomTick( timeDelta );
}

void CSelfUpdatingComponent::CustomTick( Float timeDelta ) // virtual
{
	TickScript( timeDelta );
}


void CSelfUpdatingComponent::TickScript( Float timeDelta )
{
	if( m_scriptNeedsTicking )
	{
		if( GGame->IsActive() )
		{
			CallEvent( CNAME( OnComponentTick ), timeDelta );
		}
	}
}

void CSelfUpdatingComponent::StartTicking()
{
	AddToTickGroups( GetLayer()->GetWorld() );
}


void CSelfUpdatingComponent::StopTicking()
{
	RemoveFromTickGroups( GetLayer()->GetWorld() );
}


void CSelfUpdatingComponent::funcStartTicking( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	StartTicking();
}

void CSelfUpdatingComponent::funcStopTicking( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	StopTicking();
}

void CSelfUpdatingComponent::funcGetIsTicking( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( GetIsTicking() );
}