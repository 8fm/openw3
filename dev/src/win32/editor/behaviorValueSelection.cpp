/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphContainerNode.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphTransitionNode.h"

#include "behaviorProperties.h"
#include "behaviorValueSelection.h"
#include "../../common/engine/skeleton.h"

struct StringCompareFunc
{
	static RED_INLINE Bool Less( const String& key1, const String& key2 )
	{
		return Red::System::StringCompare( key1.ToLower().AsChar(), key2.ToLower().AsChar() ) < 0;
	}
};

//////////////////////////////////////////////////////////////////////////

void CBehaviorParentInputSelection::FillChoices()
{	
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );

	while( currNode )
	{
		CBehaviorGraphContainerNode *containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{			
			const TDynArray< CName >& inputs = containerNode->GetAnimationInputs();

			for( Uint32 i=0; i<inputs.Size(); ++i )
				m_ctrlChoice->AppendString( inputs[i].AsString().AsChar() );
		}

		currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorParentValueInputSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );

	while( currNode )
	{
		CBehaviorGraphContainerNode *containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{			
			const TDynArray< CName >& inputs = containerNode->GetValueInputs();

			for( Uint32 i=0; i<inputs.Size(); ++i )
				m_ctrlChoice->AppendString( inputs[i].AsString().AsChar() );
		}

		currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorParentVectorValueInputSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );

	while( currNode )
	{
		CBehaviorGraphContainerNode *containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{			
			const TDynArray< CName >& inputs = containerNode->GetVectorValueInputs();

			for( Uint32 i=0; i<inputs.Size(); ++i )
				m_ctrlChoice->AppendString( inputs[i].AsString().AsChar() );
		}

		currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorMimicParentInputSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );

	while( currNode )
	{
		CBehaviorGraphContainerNode *containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{			
			const TDynArray< CName >& inputs = containerNode->GetMimicInputs();

			for( Uint32 i=0; i<inputs.Size(); ++i )
				m_ctrlChoice->AppendString( inputs[i].AsString().AsChar() );
		}

		currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorParentValueInputTransitionConditionSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	//while( baseItem->GetParent() )
	//	baseItem = baseItem->GetParent();

	IBehaviorStateTransitionCondition *transition = baseItem->GetParentObject( 0 ).As< IBehaviorStateTransitionCondition >();
	ASSERT( transition );

	CBehaviorGraphNode* currNode = transition->GetParentNode();
	ASSERT( currNode );

	while( currNode )
	{
		CBehaviorGraphContainerNode *containerNode = Cast< CBehaviorGraphContainerNode >( currNode );

		if ( containerNode )
		{			
			const TDynArray< CName >& inputs = containerNode->GetValueInputs();

			for( Uint32 i=0; i<inputs.Size(); ++i )
				m_ctrlChoice->AppendString( inputs[i].AsString().AsChar() );
		}

		currNode = Cast< CBehaviorGraphNode >( currNode->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorVariableSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	const CBehaviorGraph *graph = currNode->GetGraph();
	const CBehaviorVariablesList &list = m_forInternalVariable? graph->GetInternalVariables() : graph->GetVariables();

	TSortedArray<String, StringCompareFunc> vars;

	auto srcVars = list.GetVariables();
	for ( auto it = srcVars.Begin(), end = srcVars.End(); it != end; ++it )
	{
		vars.Insert( it->m_first.AsChar() );
	}

	for (Uint32 i=0; i<vars.Size(); i++)
	{
		m_ctrlChoice->AppendString( vars[i].AsChar() );
	}

	m_ctrlChoice->AppendString( TXT("<new>" ) );
	m_ctrlChoice->AppendString( TXT("") );
}

void CBehaviorVariableSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();

	// <new>
// 	if ( selected == m_ctrlChoice->GetCount()-2 )
// 	{
// 		String variableName;
// 
// 		if ( InputBox( m_propertyItem->GetPage(), TXT("New variable"), TXT("Enter variable name"), variableName ) )
// 		{
// 			CBasePropItem *baseItem  = m_propertyItem;
// 			while( baseItem->GetParent() )
// 				baseItem = baseItem->GetParent();
// 
// 			CBehaviorGraphNode *currNode = ((CClass*)m_propertyItem->GetParentObjectType(0))->CastTo< CBehaviorGraphNode >( baseItem->GetParentObject(0) );
// 			ASSERT( currNode );
// 
// 			const CBehaviorGraph *graph = currNode->GetGraph();
// 
// 			graph->GetVariables().AddVariable( variableName );
// 
// 			m_ctrlChoice->Clear();
// 			FillChoices();
// 			m_ctrlChoice->SetStringSelection( variableName.AsChar() );
// 
// 			m_propertyItem->SavePropertyValue();
// 		}
// 		else
// 		{
// 			m_ctrlChoice->SetSelection( wxNOT_FOUND );
// 		}
// 	}
// 	else
	{
		m_propertyItem->SavePropertyValue();
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorVectorVariableSelection::FillChoices()
{
	CBasePropItem *baseItem  = m_propertyItem;
	while( baseItem->GetParent() )
		baseItem = baseItem->GetParent();

	CBehaviorGraphNode *currNode = baseItem->GetParentObject( 0 ).As< CBehaviorGraphNode >();

	ASSERT( currNode );

	const CBehaviorGraph *graph = currNode->GetGraph();
	const CBehaviorVectorVariablesList &list = m_forInternalVariable? graph->GetInternalVectorVariables() : graph->GetVectorVariables();

	TSortedArray<String, StringCompareFunc> vars;

	auto srcVars = list.GetVariables();

	for ( auto it = srcVars.Begin(), end = srcVars.End(); it != end; ++it )
	{
		vars.Insert( it->m_first.AsChar() );
	}

	for (Uint32 i=0; i<vars.Size(); i++)
	{
		m_ctrlChoice->AppendString( vars[i].AsChar() );
	}

	m_ctrlChoice->AppendString( TXT("<new>" ) );
	m_ctrlChoice->AppendString( TXT("" ) );
}

void CBehaviorVectorVariableSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();

	// last item selected - create new
// 	if ( selected == m_ctrlChoice->GetCount()-2 )
// 	{
// 		String variableName;
// 
// 		if ( InputBox( m_propertyItem->GetPage(), TXT("New vector variable"), TXT("Enter variable name"), variableName ) )
// 		{
// 			CBasePropItem *baseItem  = m_propertyItem;
// 			while( baseItem->GetParent() )
// 				baseItem = baseItem->GetParent();
// 
// 			CBehaviorGraphNode *currNode = ((CClass*)m_propertyItem->GetParentObjectType(0))->CastTo< CBehaviorGraphNode >( baseItem->GetParentObject(0) );
// 			ASSERT( currNode );
// 			CBehaviorGraph *graph = currNode->GetGraph();
// 
// 			graph->GetVectorVariables().AddVariable( variableName );
// 
// 			m_ctrlChoice->Clear();
// 			FillChoices();
// 			m_ctrlChoice->SetStringSelection( variableName.AsChar() );
// 
// 			m_propertyItem->SavePropertyValue();
// 		}
// 		else
// 		{
// 			m_ctrlChoice->SetSelection( wxNOT_FOUND );
// 		}
// 	}
// 	else
	{
		m_propertyItem->SavePropertyValue();
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorEventSelection::FillChoices()
{
	IBehaviorGraphProperty* graphProperty = m_propertyItem->FindPropertyParentWithInterface< IBehaviorGraphProperty >( 0 );

	ASSERT( graphProperty );
	if ( ! graphProperty )
	{
		return;
	}
	const CBehaviorGraph *graph = graphProperty->GetParentGraph();
	if ( !graph )
	{
		return;
	}

	const CBehaviorEventsList &list = graph->GetEvents();

	TSortedArray<String, StringCompareFunc> vars;

	for( Uint32 i=0; i<list.GetNumEvents(); ++i )
	{
		CBehaviorEventDescription *currEvent = list.GetEvent( i );
		vars.Insert(currEvent->m_eventName.AsString());
	}

	for (Uint32 i=0; i<vars.Size(); i++)
	{
		m_ctrlChoice->AppendString( vars[i].AsChar() );
	}

	if ( m_withNew )
	{
		m_ctrlChoice->AppendString( TXT("<new>" ) );
	}
	m_ctrlChoice->AppendString( TXT("") );
}

void CBehaviorEventSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();

	m_propertyItem->SavePropertyValue();
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorNotificationSelection::FillChoices()
{
	IBehaviorGraphProperty* graphProperty = m_propertyItem->FindPropertyParentWithInterface< IBehaviorGraphProperty >( 0 );

	ASSERT( graphProperty );
	const CBehaviorGraph *graph = graphProperty->GetParentGraph();
	if ( !graph )
	{
		return;
	}

	// search the graph for its nodes and gather all notifications you encounter
	TSortedArray< CName > notifications;
	TDynArray< CBehaviorGraphNode* > behaviorNodes;
	graph->GetAllNodes( behaviorNodes );
	for ( TDynArray< CBehaviorGraphNode* >::iterator it = behaviorNodes.Begin();
		it != behaviorNodes.End(); ++it )
	{
		CBehaviorGraphNode* node = *it;

		if ( node->GetActivateNotification() != CName::NONE )
		{
			notifications.PushBackUnique( node->GetActivateNotification() );
		}

		if ( node->GetDeactivateNotification() != CName::NONE )
		{
			notifications.PushBackUnique( node->GetDeactivateNotification() );
		}
	}

	// go through all notifications and add them to the choices list
	for ( TSortedArray< CName >::iterator it = notifications.Begin();
		it != notifications.End(); ++it )
	{
		m_ctrlChoice->AppendString( it->AsString().AsChar() );
	}
	m_ctrlChoice->AppendString( TXT("") );
}

void CBehaviorNotificationSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();

	m_propertyItem->SavePropertyValue();
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorTrackSelection::FillChoices()
{
	String currentTrackName;
	m_propertyItem->Read( &currentTrackName );

	IBehaviorGraphBonesPropertyOwner *bonesOwner = dynamic_cast< IBehaviorGraphBonesPropertyOwner* >( m_propertyItem->GetRootObject( 0 ).AsObject() );
	ASSERT(bonesOwner);

	CBehaviorGraphInstance* instance = m_propertyItem->GetPage()->QueryBehaviorEditorProperties()->GetBehaviorGraphInstance();
	ASSERT( instance );

	CSkeleton* skeleton = bonesOwner->GetBonesSkeleton( instance->GetAnimatedComponentUnsafe() );
	//dex++: switched to general CSkeleton interface
	if ( NULL != skeleton )
	//dex--
	{
		TDynArray<String> vars;

		//dex++
		const Uint32 trackNum = skeleton->GetTracksNum();
		//dex--

		for( Uint32 i=0; i<trackNum; ++i )
		{
			//dex++
			const String trackName = skeleton->GetTrackName(i);
			//dex--
			vars.PushBack( trackName );
		}

		for (Uint32 i=0; i<vars.Size(); i++)
		{
			m_ctrlChoice->AppendString( vars[i].AsChar() );
		}
	}
}
