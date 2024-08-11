/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFrame.h"
#include "worldShowFlags.h"
#include "node.h"

CWorldEditorFragmentsFilter::CWorldEditorFragmentsFilter()
{
}

CWorldEditorFragmentsFilter::~CWorldEditorFragmentsFilter()
{
}

void CWorldEditorFragmentsFilter::RegisterEditorFragment( CNode* node, EShowFlags showFlag )
{
#ifndef NO_EDITOR_FRAGMENTS
	if ( node )
	{
		ASSERT( showFlag < ARRAY_COUNT( m_categories ) );
		m_categories[ showFlag ].Insert( reinterpret_cast< MemUint >( node ), node );
	}						   
#endif
}

void CWorldEditorFragmentsFilter::UnregisterEditorFragment( CNode* node, EShowFlags showFlag )
{
#ifndef NO_EDITOR_FRAGMENTS
	if ( node )
	{
		ASSERT( showFlag < ARRAY_COUNT( m_categories ) );
		m_categories[ showFlag ].Erase( reinterpret_cast< MemUint >( node ) );
	}
#endif
}

void CWorldEditorFragmentsFilter::GenerateEditorFragments( CRenderFrame* frame )
{
#ifndef NO_EDITOR_FRAGMENTS
	// Generate fragments
	for ( Uint32 i=0; i < SHOW_MAX_INDEX; i++ )
	{
		EShowFlags flag = ( EShowFlags ) i;
		if ( !m_categories[ flag ].Empty() && frame->GetFrameInfo().IsShowFlagOn( flag ) )
		{
			// Collect fragments
			const THashMap< MemUint, THandle< CNode > >& list = m_categories[ flag ];
			for ( auto it = list.Begin(); it != list.End(); ++it )
			{
				if ( it.Value().IsValid() && it.Value()->ShouldGenerateEditorFragments( frame ) )
				{
					it.Value()->OnGenerateEditorFragments( frame, flag );
				}
			}
		}
	}
#endif
}

void CWorldEditorFragmentsFilter::UnregisterAllEditorFragmentsOfCategory( EShowFlags showFlag )
{
#ifndef NO_EDITOR_FRAGMENTS
	ASSERT( showFlag < ARRAY_COUNT( m_categories ) );
	m_categories[ showFlag ].Clear();
#endif
}
