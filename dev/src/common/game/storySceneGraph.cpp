#include "build.h"
#include "storySceneGraph.h"

#include "storySceneSectionBlock.h"
#include "storySceneSection.h"
#include "storyScene.h"
#include "../core/feedback.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneGraph )

CStorySceneGraph::CStorySceneGraph(void)
{
}

CObject *CStorySceneGraph::GraphGetOwner()
{
	return this;
}

Vector CStorySceneGraph::GraphGetBackgroundOffset() const
{
	return m_backgroundOffset;
}

void CStorySceneGraph::GraphSetBackgroundOffset( const Vector& offset )
{
	m_backgroundOffset = offset;
}

void CStorySceneGraph::OnPreSave()
{
#ifndef NO_EDITOR
	CheckConsistency( NULL, true, false );
#endif //NO_EDITOR

	TBaseClass::OnPreSave();
}

void CStorySceneGraph::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

//#ifndef NO_EDITOR
//	CheckConsistency();
//#endif //NO_EDITOR
}

void CStorySceneGraph::GraphStructureModified()
{
	//CheckConsistency( NULL, true, false );
}

#ifndef NO_EDITOR
Bool CStorySceneGraph::CheckConsistency( SBrokenSceneGraphInfo* graphInfo /*= NULL*/, Bool doNotShowInfoIfItsOK /*= true*/, Bool doNotShowInfoIfItsNotOK /*= true*/ )
{
	SBrokenSceneGraphInfo singleGraphInfo;

	// Check links in graph blocks
	for ( Uint32 i=0; i<m_graphBlocks.Size(); ++i )
	{
		CStorySceneGraphBlock* block = Cast< CStorySceneGraphBlock >( m_graphBlocks[i] );
		ASSERT( block );
		if ( block )
		{
			CStorySceneControlPart* part = block->GetControlPart();
			ASSERT( part );
			if ( part )
			{
				if ( graphInfo )
				{
					// checking from scene diagnostic tool
					part->ValidateLinks( graphInfo );
				}
				else
				{
					// checking single graph from OnPreSave or menu option
					part->ValidateLinks( &singleGraphInfo );
				}
			}
		}
	}

	if ( !graphInfo )
	{
		// checking single graph, need to give feedback if required
		if ( !singleGraphInfo.m_path.Empty() )
		{
			if ( !doNotShowInfoIfItsNotOK )
			{
				String text;
				text = TXT("BROKEN GRAPH IN:\n") + singleGraphInfo.m_path + TXT("\n\n");
				for ( Uint32 i = 0; i < singleGraphInfo.m_brokenLinks.Size(); i++ )
				{
					text += TXT("\"") + singleGraphInfo.m_brokenLinks[ i ].m_first + TXT("\" has broken link to \"") + singleGraphInfo.m_brokenLinks[ i ].m_second + TXT("\"\n\n");
				}
				text += TXT("Please fix links by deleting and recreating them");
				GFeedback->ShowError( text.AsChar() );
			}
		}
		else
		{
			if ( !doNotShowInfoIfItsOK )
			{
				GFeedback->ShowMsg( TXT("Consistency check"), TXT("Graph is OK") );
			}
		}
	}

	return singleGraphInfo.m_path.Empty();
}
#endif //NO_EDITOR