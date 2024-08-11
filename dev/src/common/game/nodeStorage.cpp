#include "build.h"
#include "nodeStorage.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CNodesBinaryStorage );

///////////////////////////////////////////////////////////////////////////////

void CNodesBinaryStorage::funcInitializeFromTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;

	if ( tag.Empty() )
	{
		RETURN_INT( 0 );
		return;
	}

	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	ASSERT( tagMgr );

	TagList tags;
	tags.AddTag( tag );

	TDynArray< CNode* > nodes;
	tagMgr->CollectTaggedNodes( tags, nodes );

	for ( TDynArray< CNode* >::const_iterator it = nodes.Begin();
		it != nodes.End(); ++it )
	{
		CNode* node = *it;
		if ( !node )
		{
			continue;
		}

		Add( THandle< CNode >( node ) );
	}

	RETURN_INT( (Int32)Size() );
}

void CNodesBinaryStorage::funcInitializeWithNodes( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< THandle< CNode > >, nodes, TDynArray< THandle< CNode > >() );
	FINISH_PARAMETERS;

	for ( TDynArray< THandle< CNode > >::const_iterator it = nodes.Begin();
		it != nodes.End(); ++it )
	{
		const THandle< CNode >& nodeHandle = *it;
		if ( nodeHandle.Get() == NULL )
		{
			continue;
		}

		Add( nodeHandle );
	}

	RETURN_INT( (Int32)Size() );
}

void CNodesBinaryStorage::funcGetClosestToNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, centralNode, NULL );
	GET_PARAMETER_REF( TDynArray< THandle< CNode > >, output, TDynArray< THandle< CNode > >() );
	GET_PARAMETER_OPT( Vector, relMinBound, Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX ) );
	GET_PARAMETER_OPT( Vector, relMaxBound, Vector( FLT_MAX, FLT_MAX, FLT_MAX ) );
	GET_PARAMETER_OPT( Bool, useZBounds, true );
	GET_PARAMETER_OPT( Int32, maxNodes, INT_MAX );
	FINISH_PARAMETERS;

	CNode *pCentralNode = centralNode.Get();
	if ( pCentralNode == NULL )
	{
		return;
	}

	PC_SCOPE( NodesBinaryStorage );

	Query( *pCentralNode, output, Box( relMinBound, relMaxBound ), useZBounds, maxNodes, NULL, 0 );
}

void CNodesBinaryStorage::funcGetClosestToPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector( 0, 0, 0 ) );
	GET_PARAMETER_REF( TDynArray< THandle< CNode > >, output, TDynArray< THandle< CNode > >() );
	GET_PARAMETER_OPT( Vector, relMinBound, Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX ) );
	GET_PARAMETER_OPT( Vector, relMaxBound, Vector( FLT_MAX, FLT_MAX, FLT_MAX ) );
	GET_PARAMETER_OPT( Bool, useZBounds, true );
	GET_PARAMETER_OPT( Int32, maxNodes, INT_MAX );
	FINISH_PARAMETERS;

	PC_SCOPE( NodesBinaryStorage );

	Query( position, output, Box( relMinBound, relMaxBound ), useZBounds, maxNodes, NULL, 0 );
}

///////////////////////////////////////////////////////////////////////////////
