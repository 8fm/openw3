/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnTreeNode.h"


class ISpawnTreeScriptedDecorator : public ISpawnTreeDecorator
{
	DECLARE_ENGINE_CLASS( ISpawnTreeScriptedDecorator, ISpawnTreeDecorator, 0 )

protected:
	enum EScriptFlags
	{
		SF_DEFAULTS								= 0,
		SF_HAS_OnActivate						= FLAG( 0 ),
		SF_HAS_OnDeactivate						= FLAG( 1 ),
		SF_HAS_GetFriendlyName					= FLAG( 2 ),
		SF_HAS_Main								= FLAG( 3 ),
		SF_HAS_OnFullRespawn					= FLAG( 4 ),
		SF_NOT_INITIALIZED						= 0xffffffff
	};

	typedef TInstanceVar< THandle< IScriptable > > TUserDataHandle;

	mutable Uint32						m_scriptFlags;	// lazy initialization
	CScriptThread*						m_thread;		// script thread for latent main function
	Bool								m_mainTerminated;
	TUserDataHandle						i_userData;

	// script functions
	void						Script_GetFriendlyName( String& outName );
	void						Script_Activate( CSpawnTreeInstance& instance );
	void						Script_Deactivate( CSpawnTreeInstance& instance );
	void						Script_OnFullRespawn( CSpawnTreeInstance& instance ) const;

	void						InitializeScriptFunctions() const;

public:
	ISpawnTreeScriptedDecorator()
		: m_scriptFlags( Uint32( SF_NOT_INITIALIZED ) )	
		, m_thread( nullptr )
		, m_mainTerminated( false ){}

	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;
	void						OnFullRespawn( CSpawnTreeInstance& instance ) const override;

	// Node interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	// Editor interface
	Bool						IsSpawnableByDefault() const override;

	// Editor interface// IEdSpawnTreeNode interface
	String						GetEditorFriendlyName() const override;
	String						GetBlockCaption() const override;

	////////////////////////////////////////////////////////////////////
	// CScriptThread::IListener interface
	void OnScriptThreadKilled( CScriptThread * thread, Bool finished ) override;
	String GetDebugName() const override;
};

BEGIN_CLASS_RTTI( ISpawnTreeScriptedDecorator );
	PARENT_CLASS( ISpawnTreeDecorator );
END_CLASS_RTTI();
