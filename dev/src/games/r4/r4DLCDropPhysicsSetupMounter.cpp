#include "build.h"
#include "r4DLCDropPhysicsSetupMounter.h"
#include "../../common/core/depot.h"
#include "../../common/engine/dropPhysicsComponent.h"

IMPLEMENT_ENGINE_CLASS( CR4DropPhysicsSetupDLCMounter );

CR4DropPhysicsSetupDLCMounter::CR4DropPhysicsSetupDLCMounter() : m_mounterStarted( false )
{
}

CR4DropPhysicsSetupDLCMounter::~CR4DropPhysicsSetupDLCMounter()
{
}


void CR4DropPhysicsSetupDLCMounter::AddDropPhysicsSetups( SDropPhysicsComponentEntry& entry )
{
	CDropPhysicsComponent* dropPhysicsComponent = entry.m_handle.Get();
	if ( dropPhysicsComponent != nullptr )
	{
		TDropSetups::const_iterator endDropSetups = m_dropSetups.End();
		for ( TDropSetups::iterator iterDropSetups = m_dropSetups.Begin(); iterDropSetups != endDropSetups; ++iterDropSetups )
		{
			CDropPhysicsSetup *dropPhysicsSetup = iterDropSetups->Get();

			CDropPhysicsSetup *dropPhysicsSetupNewInstance = Cast< CDropPhysicsSetup >( dropPhysicsSetup->Clone( dropPhysicsComponent ) );

			entry.m_setups.PushBackUnique( dropPhysicsSetupNewInstance );	

			dropPhysicsComponent->AddDropPhysicsSetup( dropPhysicsSetupNewInstance );
		}
	}
}

void CR4DropPhysicsSetupDLCMounter::RemoveDropPhysicsSetups( SDropPhysicsComponentEntry& entry )
{
	CDropPhysicsComponent* dropPhysicsComponent = entry.m_handle.Get();
	if ( dropPhysicsComponent != nullptr )
	{
		TDynArray<CDropPhysicsSetup*>::const_iterator endDropSetups = entry.m_setups.End();
		for ( TDynArray<CDropPhysicsSetup*>::iterator iterDropSetups = entry.m_setups.Begin(); iterDropSetups != endDropSetups; ++iterDropSetups )
		{
			CDropPhysicsSetup *dropPhysicsSetup = *iterDropSetups;
			dropPhysicsComponent->RemoveDropPhysicsSetup( dropPhysicsSetup->m_name );
		}
	}
	entry.m_setups.Clear();
}

bool CR4DropPhysicsSetupDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4DropPhysicsSetupDLCMounter::OnGameStarting()
{
	Activate();
}

void CR4DropPhysicsSetupDLCMounter::OnGameEnding()
{
	Deactivate();
}

void CR4DropPhysicsSetupDLCMounter::DropPhysicsComponentOnInitialized( CDropPhysicsComponent* dropPhysicsComponent )
{
	m_dropPhysicsComponentEntries.Set( dropPhysicsComponent, SDropPhysicsComponentEntry( dropPhysicsComponent ) );

	if ( m_mounterStarted )
	{
		AddDropPhysicsSetups( m_dropPhysicsComponentEntries[ dropPhysicsComponent ] );
	}	
}

void CR4DropPhysicsSetupDLCMounter::DropPhysicsComponentOnFinalize( CDropPhysicsComponent* dropPhysicsComponent )
{
	SDropPhysicsComponentEntry* dropPhysicsComponentEntry = m_dropPhysicsComponentEntries.FindPtr( dropPhysicsComponent );
	if ( dropPhysicsComponentEntry != nullptr )
	{
		if ( m_mounterStarted )
		{
			RemoveDropPhysicsSetups( *dropPhysicsComponentEntry );
		}	
		m_dropPhysicsComponentEntries.Erase( dropPhysicsComponent );
	}
}

#ifndef NO_EDITOR

void CR4DropPhysicsSetupDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4DropPhysicsSetupDLCMounter::OnEditorStopped()
{
	Deactivate();
}

Bool CR4DropPhysicsSetupDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	return true;
}

#endif // !NO_EDITOR

void CR4DropPhysicsSetupDLCMounter::Activate()
{
	for ( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SDropPhysicsComponentModifierManager::GetInstance().RegisterModifier( entityTemplatePath, this );
	}

	m_mounterStarted = true;
}

void CR4DropPhysicsSetupDLCMounter::Deactivate()
{
	m_mounterStarted = false;

	for ( TDropPhysicsComponentEntries::iterator itEntry = m_dropPhysicsComponentEntries.Begin(), endEntries = m_dropPhysicsComponentEntries.End(); itEntry != endEntries; ++itEntry )
	{
		RemoveDropPhysicsSetups( itEntry->m_second );
	}
	m_dropPhysicsComponentEntries.Clear();

	for ( auto& entityTemplatePath : m_entityTemplatePaths )
	{
		SDropPhysicsComponentModifierManager::GetInstance().UnregisterModifier( entityTemplatePath, this );
	}
}
