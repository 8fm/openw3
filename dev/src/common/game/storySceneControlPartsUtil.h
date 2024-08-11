// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#pragma once

class CStorySceneControlPart;
class CStorySceneLinkElement;

// =================================================================================================
namespace StorySceneControlPartUtils {
// =================================================================================================

class EvalSceneNodeResult
{
public:
	enum class EResolution
	{
		R_Invalid,								// argNode evaluation finished with error (resultNode is nullptr) or EvalSceneNodeResult doesn't describe any result (it may have been just created)
		R_Success,								// argNode evaluation finished successfully yielding resultNode as result
		R_CantEvaluate							// argNode can't be evaluated (see EvalSceneGraphNode() for more info), resultNode is nullptr
	};

	EvalSceneNodeResult()
	: resolution( EResolution::R_Invalid ), argNode( nullptr ), resultNode ( nullptr )
	{}

	EResolution resolution;
	const CStorySceneLinkElement* argNode;		// evaluated node
	const CStorySceneLinkElement* resultNode;	// resulting node
};

class EvalSceneGraphResult
{
public:
	enum class EResolution
	{
		R_Invalid,								// argNode evaluation finished with error (resultNode is nullptr) or EvalSceneGraphResult doesn't describe any result (it may have been just created)
		R_FoundMatch,							// argNode evaluation returned resultNode for which matching function returns true
		R_ReachedEnd,							// reached end of scene graph, argNode is a last evaluated node, resultNode is nullptr
		R_CantEvaluate,							// argNode can't be evaluated (see EvalSceneGraphNode() for more info), resultNode is nullptr
		R_MaxStepsReached						// maximum number of steps have been performed, argNode is a last evaluated node, resultNode is a last result
	};

	EvalSceneGraphResult()
	: resolution( EResolution::R_Invalid ), argNode( nullptr ), resultNode ( nullptr )
	{}

	EResolution resolution;
	const CStorySceneLinkElement* argNode;		// evaluated node
	const CStorySceneLinkElement* resultNode;	// resulting node
};

EvalSceneNodeResult EvalSceneGraphNode( const CStorySceneLinkElement* evalNode );
template< typename Func > EvalSceneGraphResult EvalSceneGraph( const CStorySceneLinkElement* startNode, Func func, Uint32 maxSteps = 100 );

// TODO: Please keep in mind that we have fuinctions like CEdSceneEditor::GetFollowingSections...

CStorySceneControlPart* GetControlPartFromLink( CStorySceneLinkElement* link );
const CStorySceneControlPart* GetControlPartFromLink( const CStorySceneLinkElement* link );

void GetNextConnectedElements( const CStorySceneLinkElement* elem, TDynArray< const CStorySceneLinkElement*>& out );
void GetPrevConnectedElements( const CStorySceneLinkElement* elem, TDynArray< const CStorySceneLinkElement*>& out );
void GetPrevConnectedElements( const CStorySceneLinkElement* elem, TDynArray< const CStorySceneLinkElement*>& out, Int32 selectedInput );

namespace
{
	template< typename T, typename K >
	void GetElementsOfType( const CStorySceneLinkElement* elem, TDynArray< const T* >& out, K nextEl )
	{
		TDynArray< const CStorySceneLinkElement* > stack;
		TDynArray< const CStorySceneLinkElement* > visited;
		TDynArray< const CStorySceneLinkElement* > linkedElements;
		stack.PushBack( elem );

		const T* result = nullptr;
		while( !stack.Empty() )
		{	
			const CStorySceneLinkElement* link = stack.PopBackFast();
			if ( link )
			{
				linkedElements.ClearFast();
				(*nextEl)( link, linkedElements );
				for ( Int32 i = 0; i < linkedElements.SizeInt(); ++i )
				{
					if ( linkedElements[i] && !visited.Exist( linkedElements[i] ) )
					{
						visited.PushBack( linkedElements[i] );
						result = Cast<const T>( linkedElements[i] );
						if ( result )
						{
							out.PushBack( result );
						}
						else
						{
							stack.PushBack( linkedElements[i] );
						}
					}
				}
			}
		}
	}
}


template< typename T >
void GetPrevElementsOfType( const  CStorySceneLinkElement* elem, TDynArray< const T* >& out )
{	
	GetElementsOfType< T, void (*)( const CStorySceneLinkElement* , TDynArray< const CStorySceneLinkElement*>& ) >( elem, out, GetPrevConnectedElements );
}

//intended as a replacement for CStorySceneControlPart::CollectControlParts 
template< typename T >
void GetNextElementsOfType( const  CStorySceneLinkElement* elem, TDynArray< const T* >& out )
{
	GetElementsOfType( elem, out, GetNextConnectedElements );
}

/*
Evaluates scene graph.

\param startNode Starting node. Must not be nullptr.
\param func Function taking single const CStorySceneLinkElement* argument and returning Bool. This function is
called for result nodes obtained at each evaluation step - if func returns true then evaluation is stopped.
\param maxSteps Maximum number of evaluation steps to perform. This will prevent endless loops in bad scene graphs.
\return EvalSceneGraphResult object describing evaluation result. EvalSceneGraphResult::resultNode will be nullptr
if evalNode can't be evaluated or if it doesn't link to any other nodes.

See EvalSceneGraphNode() for more info on evaluating each type of scene graph node.
*/
template< typename Func >
EvalSceneGraphResult EvalSceneGraph( const CStorySceneLinkElement* startNode, Func func, Uint32 maxSteps /* = 100 */ )
{
	RED_FATAL_ASSERT( startNode, "EvalSceneGraph(): startNode must not be nullptr." );

	EvalSceneNodeResult evalNodeResult;
	evalNodeResult.resultNode = startNode;

	Bool evalCompleted = false;

	for( Uint32 iStep = 0; iStep < maxSteps && !evalCompleted; ++iStep )
	{
		evalNodeResult = EvalSceneGraphNode( evalNodeResult.resultNode );

		if( evalNodeResult.resolution != EvalSceneNodeResult::EResolution::R_Success || !evalNodeResult.resultNode || func( evalNodeResult.resultNode ) )
		{
			evalCompleted = true;
		}
	}

	EvalSceneGraphResult evalGraphResult;
	evalGraphResult.argNode = evalNodeResult.argNode;
	evalGraphResult.resultNode = evalNodeResult.resultNode;

	if( evalCompleted )
	{
		if( evalNodeResult.resolution == EvalSceneNodeResult::EResolution::R_Success )
		{
			evalGraphResult.resolution = evalNodeResult.resultNode? EvalSceneGraphResult::EResolution::R_FoundMatch : EvalSceneGraphResult::EResolution::R_ReachedEnd;
		}
		else if( evalNodeResult.resolution == EvalSceneNodeResult::EResolution::R_CantEvaluate )
		{
			evalGraphResult.resolution = EvalSceneGraphResult::EResolution::R_CantEvaluate;
		}
		// else graphResult.result is EvalSceneGraphResult::EResolution::R_Invalid
	}
	else
	{
		evalGraphResult.resolution = EvalSceneGraphResult::EResolution::R_MaxStepsReached;
	}

	return evalGraphResult;
}

// =================================================================================================
} // namespace StorySceneUtils
// =================================================================================================
