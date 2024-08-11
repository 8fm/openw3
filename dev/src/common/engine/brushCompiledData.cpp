/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushCompiledData.h"
#include "brushCSG.h"
#include "brushComponent.h"
#include "renderProxy.h"
#include "world.h"
#include "layer.h"

IMPLEMENT_ENGINE_CLASS( CBrushCompiledData );

void CBrushCompiledData::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Serialize render data
	//file << m_renderData;
}

void CBrushCompiledData::AttachedToWorld( CWorld* world )
{
	// Create face rendering proxies
	IRenderScene* renderScene = world->GetRenderSceneEx();
	m_renderData.CreateRenderingProxies( renderScene );
}

void CBrushCompiledData::DetachedFromWorld( CWorld* world )
{
	// Destroy face rendering proxies
	IRenderScene* renderScene = world->GetRenderSceneEx();
	m_renderData.DestroyRenderingProxies( renderScene );
}

void CBrushCompiledData::AddBrush( CBrushComponent* brush )
{
	ASSERT( brush );	

	// Allocate brush index
	Uint32 brushIndex = m_brushes.Size();
	ASSERT( !m_brushes.Exist( brush ) );
	m_brushes.PushBack( brush );

	// Bind brush to entry in brush list
	brush->m_brushIndex = brushIndex;
}

void CBrushCompiledData::AddBrushes( const TDynArray< CBrushComponent* > &brushes )
{
	// Sort brushes in some order
	TDynArray< CBrushComponent* > tempBrushes = brushes;
	SortBrushes( tempBrushes );

	// Remove current linking in a soft way
	for ( Uint32 i=0; i<tempBrushes.Size(); i++ )
	{
		m_brushes.Remove( tempBrushes[i] );
		tempBrushes[i]->m_brushIndex = -1;
	}

	// Reindex rest of the brushes
	for ( Uint32 i=0; i<m_brushes.Size(); i++ )
	{
		ASSERT( m_brushes[i] );
		m_brushes[i]->m_brushIndex = i;
	}

	// Add brushes to the list
	for ( Uint32 i=0; i<tempBrushes.Size(); i++ )
	{
		AddBrush( tempBrushes[i] );
	}
}

void CBrushCompiledData::RemoveBrush( CBrushComponent* brush )
{
	ASSERT( brush );

	// Not a real brush
	if ( brush->m_brushIndex != -1 )
	{
		// Unlink brush index
		ASSERT( brush->m_brushIndex != -1 );
		ASSERT( m_brushes[ brush->m_brushIndex ] == brush );
		m_brushes.Remove( brush );
		brush->m_brushIndex = -1;

		// Reindex rest of the brushes
		for ( Uint32 i=0; i<m_brushes.Size(); i++ )
		{
			ASSERT( m_brushes[i] );
			m_brushes[i]->m_brushIndex = i;
		}
	}
}

static int BrushCmpFumc( const void* elem0, const void* elem1 )
{
	const CBrushComponent* brush0 = *(const CBrushComponent**)elem0;
	const CBrushComponent* brush1 = *(const CBrushComponent**)elem1;
	return brush0->GetBrushIndex() - brush1->GetBrushIndex();
}

void CBrushCompiledData::SortBrushes( TDynArray< CBrushComponent* >& brushes )
{
	// Sort, qsort is still better that our functions...
	qsort( brushes.TypedData(), brushes.Size(), sizeof( CBrushComponent* ), &BrushCmpFumc );
}

static CWorld* GetAttachedWorld( CBrushCompiledData* data )
{
	// Get the layer
	CLayer* layer = data->FindParent< CLayer >();
	if ( layer && layer->IsAttached() )
	{
		// Get world that layer is attached to
		return layer->GetWorld();
	}

	// No attached world
	return NULL;
}

void CBrushCompiledData::Compile()
{
	// Destroy rendering proxies
	CWorld* attachedToWorld = GetAttachedWorld( this );
	if ( attachedToWorld )
	{
		DetachedFromWorld( attachedToWorld );
	}

	// Clear rendering data
	m_renderData.ResetGeometry();

	// Compile BSP
	CCSGCompiler compiler;
	compiler.CompileCSG( m_brushes, m_renderData );

	// Reattach
	if ( attachedToWorld )
	{
		AttachedToWorld( attachedToWorld );
	}
}

