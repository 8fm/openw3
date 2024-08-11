#include "build.h"
#include "teleporter.h"
#include "actor.h"
#include "../engine/tagManager.h"
#include "../engine/renderFrame.h"


IMPLEMENT_ENGINE_CLASS( CTeleporter );
RED_DEFINE_STATIC_NAME( OnTeleported );

void CTeleporter::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Paths );

}

void CTeleporter::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Paths );
}

void CTeleporter::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	CActor* actor = Cast< CActor >( activator->GetEntity() );
	if ( !actor || m_teleportedActors.Find( actor ) != m_teleportedActors.End() )
	{
		return;
	}

	const TagList& entityTags = actor->GetTags();
	if ( TagList::MatchAny( m_teleportedActorsTags, entityTags ) == false )
	{
		return;
	}

	static TDynArray< CNode* >	nodes;
	nodes.ClearFast();
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( m_destinationNodesTags, nodes );
	if( nodes.Empty() )
	{
		return;
	}
	
	Uint32 destinationNodeIdx = GEngine->GetRandomNumberGenerator().Get< Uint32 >( nodes.Size() );
	THandle< CActor > hActor( actor );
	THandle< CNode > hDest( nodes[destinationNodeIdx] );
	if ( CallEvent( CNAME( OnTeleported ), hActor, hDest ) == false )
	{
		actor->Teleport( nodes[destinationNodeIdx] );
	}
	else
	{
		m_teleportedActors.Insert( actor );
	}
}

void CTeleporter::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	static TDynArray< CNode* >	nodes;
	nodes.ClearFast();
	world->GetTagManager()->CollectTaggedNodes( m_destinationNodesTags, nodes );

	Vector teleporterPos = GetWorldPosition();

	for ( TDynArray< CNode* >::const_iterator it = nodes.Begin(); it != nodes.End(); ++it )
	{
		//Vector dir = (*it)->GetWorldPosition() - teleporterPos;
		frame->AddDebugLine( teleporterPos, (*it)->GetWorldPosition(), Color::YELLOW, true );
	}
}

void CTeleporter::funcUseTeleporter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, hActor, NULL );
	GET_PARAMETER( THandle< CNode >, hDest, NULL );
	FINISH_PARAMETERS;

	CActor* actor = hActor.Get();
	CNode* dest = hDest.Get();
	if ( actor && dest )
	{
		actor->Teleport( dest );
		m_teleportedActors.Erase( actor );
	}
}
