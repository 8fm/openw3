/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"
#include "../../common/engine/dropPhysicsComponentModifier.h"

class CR4DropPhysicsSetupDLCMounter : public IGameplayDLCMounter, public IDropPhysicsComponentModifier
{
	DECLARE_ENGINE_CLASS( CR4DropPhysicsSetupDLCMounter, IGameplayDLCMounter, 0 );

	struct SDropPhysicsComponentEntry
	{
		THandle< CDropPhysicsComponent >	m_handle;
		TDynArray< CDropPhysicsSetup* >		m_setups;

		SDropPhysicsComponentEntry()
		{}

		SDropPhysicsComponentEntry( CDropPhysicsComponent* c )
			: m_handle( c )
		{}

		RED_INLINE Bool IsValid() const { return m_handle.IsValid(); }
	};

public:
	CR4DropPhysicsSetupDLCMounter();
	~CR4DropPhysicsSetupDLCMounter();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

	//! IDropPhysicsComponentModifier
	virtual void DropPhysicsComponentOnInitialized( CDropPhysicsComponent* dropPhysicsComponent ) override;
	virtual void DropPhysicsComponentOnFinalize( CDropPhysicsComponent* dropPhysicsComponent ) override;

#ifndef NO_EDITOR

	//! IGameplayDLCMounter
	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;
	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

private:
	void AddDropPhysicsSetups( SDropPhysicsComponentEntry& entry  );
	void RemoveDropPhysicsSetups( SDropPhysicsComponentEntry& entry );

private:
	void Activate();
	void Deactivate();

	typedef TDynArray< String > TEntityTemplatePaths;
	TEntityTemplatePaths m_entityTemplatePaths;

	typedef TDynArray< THandle< CDropPhysicsSetup > > TDropSetups;
	TDropSetups	m_dropSetups;

	Bool m_mounterStarted;

	typedef THashMap< CDropPhysicsComponent*, SDropPhysicsComponentEntry > TDropPhysicsComponentEntries;
	TDropPhysicsComponentEntries m_dropPhysicsComponentEntries;
};

BEGIN_CLASS_RTTI( CR4DropPhysicsSetupDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
	PROPERTY_EDIT_ARRAY( m_entityTemplatePaths, TXT("Paths to entity templates") );
	PROPERTY_INLINED( m_dropSetups, TXT( "Setups" ) );
END_CLASS_RTTI();
