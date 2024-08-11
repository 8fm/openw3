/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "deniedAreaSaveable.h"

#include "../engine/deniedAreaComponent.h"
#include "questGraphSocket.h"

#include "gameWorld.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CDeniedAreaSaveable )

CDeniedAreaSaveable::CDeniedAreaSaveable()
	: CGameplayEntity()
	, m_isEnabled( true )
{
}

CDeniedAreaSaveable::~CDeniedAreaSaveable()
{

}

void CDeniedAreaSaveable::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	SetEnabled( m_isEnabled );
}

void CDeniedAreaSaveable::SetEnabled( Bool enabled )
{
	if ( m_isEnabled != enabled )
	{
		SetShouldSave( true );
	}
	
	m_isEnabled = enabled;

	for ( ComponentIterator< CDeniedAreaComponent > it( this ); it; ++it )
	{
		(*it)->SetEnabled( enabled );
	}
}

void CDeniedAreaSaveable::OnPostRestoreState()
{
	SetEnabled( m_isEnabled );
}

///////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CDeniedAreaBlock )

CDeniedAreaBlock::CDeniedAreaBlock()
	: CQuestGraphBlock()
	, m_entityTag( CName::NONE )
	, m_enabled( true )
{
	m_name = TXT( "Denied area" ); 
}

void CDeniedAreaBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// Activate output either way
	ActivateOutput( data, CNAME( Out ) );

	if( GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		ASSERT( ! "No world loaded" );
		return;
	}

	CGameWorld* witcherWorld = Cast< CGameWorld >( GGame->GetActiveWorld() );
	if ( witcherWorld )
	{	
		CEnableDeniedAreaRequest* request = ::CreateObject< CEnableDeniedAreaRequest >( witcherWorld );
		request->SetEnabled( m_enabled );
		witcherWorld->RegisterStateChangeRequest( m_entityTag, request );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CDeniedAreaBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create input
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );

	// Create output
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEnableDeniedAreaRequest );
CEnableDeniedAreaRequest::CEnableDeniedAreaRequest( Bool enable )
: m_enable( enable )
{
}

void CEnableDeniedAreaRequest::Execute( CGameplayEntity* entity )
{
	ComponentIterator< CDeniedAreaComponent > it( entity );
	while( it )
	{
		(*it)->SetEnabled( m_enable );

		++it;
	}
}

String CEnableDeniedAreaRequest::OnStateChangeRequestsDebugPage() const
{	
	return String::Printf( TXT( "CEnableDeniedAreaRequest - enable( %s )" ), ToString( m_enable ).AsChar() );
}

///////////////////////////////////////////////////////////////////////////////
