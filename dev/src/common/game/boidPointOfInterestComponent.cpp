#include "build.h"
#include "boidPointOfInterestComponent.h"
#include "boidLairEntity.h"
#include "boidManager.h"
#include "../engine/renderFrame.h"

IMPLEMENT_RTTI_ENUM( EZoneAcceptor );

/////////////////////////////////////////////////////////////////////////
// CBoidPointOfInterestComponent
/////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBoidPointOfInterestComponent );

CBoidPointOfInterestComponent::CBoidPointOfInterestComponent() 
	: m_params( )
	, m_acceptor( ZA_LairAreaOnly )
	, m_crawlingSwarmDebug( false )
{

}
void CBoidPointOfInterestComponent::AddToLair( IBoidLairEntity* lair )
{
	m_lairs.PushBackUnique( lair );
}
void CBoidPointOfInterestComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CBoidPointOfInterestComponent_OnAttached );

	const Bool testBothZones  = m_acceptor == ZA_LairAreaOnly ? false : true;
	if ( GetEntity()->IsInGame() )
	{
		auto functor =
			[ this ] ( IBoidLairEntity* lair )
		{
			lair->OnStaticPointOfInterestAdded( this );
		};

		CBoidManager::GetInstance()->IterateLairs( GetWorldPosition(), testBothZones, functor );
	}
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Swarms );
}
void CBoidPointOfInterestComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	ForEach( m_lairs,
		[ this ] ( THandle< IBoidLairEntity >& lairHandle )
	{
		IBoidLairEntity* lair = lairHandle.Get();
		if ( lair )
		{
			lair->OnStaticPointOfInterestRemoved( this );
		}
	} );
	m_lairs.ClearFast();
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Swarms );
}

void CBoidPointOfInterestComponent::OnUsed( Uint32 count, Float deltaTime )
{
}

void CBoidPointOfInterestComponent::funcDisable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, disable, true );
	FINISH_PARAMETERS;
	m_params.m_enabled = disable == false;
}

#ifndef NO_EDITOR_FRAGMENTS
// Generate editor rendering fragments
void CBoidPointOfInterestComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	CComponent::OnGenerateEditorFragments( frame, flag );
	if ( flag == SHOW_Swarms )
	{
		Box box( Vector( -0.5f, -0.5f, -0.5f ), Vector( 0.5f, 0.5f, 0.5f ) );
		// selection code
	#ifndef NO_COMPONENT_GRAPH
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
			frame->AddDebugSolidBox( box, m_localToWorld, GetHitProxyID().GetColor() );
			return;
		}
	#endif
		frame->AddDebugSolidBox( box, m_localToWorld, m_lairs.Size() == 0 ? Color::RED : Color::BLUE );

		if ( m_crawlingSwarmDebug )
		{
			Vector3 lineStart			= GetWorldPositionRef() + Vector3( 0.0f, 0.0f, -SWARM_FLOOD_FILL_SP_RANGE * 0.5f );
			Vector3 lineEnd				= GetWorldPositionRef() + Vector3( 0.0f, 0.0f, SWARM_FLOOD_FILL_SP_RANGE * 0.5f );
			frame->AddDebugLine( lineStart, lineEnd, Color::BLUE, false );

			const Float size = 0.1f;
			const Float thickness = 0.025f;
			frame->AddDebugSolidBox( Box( lineStart - Vector3( size, size, thickness ), lineStart + Vector3( size, size, thickness ) ), Matrix::IDENTITY, Color::RED );
			frame->AddDebugSolidBox( Box( lineEnd - Vector3( size, size, thickness ), lineEnd + Vector3( size, size, thickness ) ), Matrix::IDENTITY, Color::RED );
		}
	}
}
#endif

