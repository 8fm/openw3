
#include "build.h"
#include "expTreeMediator.h"

ExpTreeMediator::ExpTreeMediator()
	: m_idx( 0 )
{
	
}

Bool ExpTreeMediator::AddEdge( const Vector& p1, const Vector& p2, Int32 id )
{
	m_edges.PushBack( m_idx );

	m_pointSet.AddPoint( ( p1.X + p2.X ) / 2.f, ( p1.Y + p2.Y ) / 2.f );

	m_idx++;

	return true;
}

Int32 ExpTreeMediator::FindEdge( Int32 nnId ) const
{
	ASSERT( nnId < m_edges.SizeInt() );

	return nnId != -1 ? m_edges[ nnId ] : -1;
}

void ExpTreeMediator::Clear()
{
	m_idx = 0;
	m_edges.Clear();
	m_pointSet.Clear();
}

void ExpTreeMediator::RestoreFromCookedFile( IFile& reader )
{
	// 1. num points (4-byte)
	Uint32 numPoints = m_pointSet.GetPointNum();
	reader << numPoints;

	// 2. points ( i * k * 4-byte, where k == 2 ) 
	Uint32 pointsSize = numPoints * m_pointSet.GetPointDim() * sizeof( Float );
	if ( pointsSize > 0 )
	{
		reader.Serialize( m_pointSet.Resize( numPoints ), pointsSize ); 
	}

	// 3. edges ( n * 4-byte )	
	Uint32 edgesSize = numPoints * sizeof( Int32 );
	if ( edgesSize > 0 )
	{
		m_edges.Resize( numPoints );
		reader.Serialize( m_edges.TypedData(), edgesSize );
	}

	m_idx = numPoints;
}

#ifndef NO_EDITOR
void ExpTreeMediator::CookToFile( IFile& writer ) const
{
	// 1. num points (4-byte)
	Uint32 numPoints = m_pointSet.GetPointNum();
	writer << numPoints;

	// 2. points ( i * k * 4-byte, where k == 2 ) 
	Uint32 pointsSize = m_pointSet.GetPointNum() * m_pointSet.GetPointDim() * sizeof( Float );
	if ( pointsSize > 0 )
	{
		writer.Serialize( ( void* ) m_pointSet[ 0 ], pointsSize ); 
	}

	// 3. edges ( n * 4-byte )	
	Uint32 edgesSize = m_edges.Size() * sizeof( Int32 );
	if ( edgesSize > 0 )
	{
		writer.Serialize( ( void* ) m_edges.Data(), edgesSize );
	}
}

Uint32 ExpTreeMediator::ComputeCookedDataSize() const
{
	return																		// We save:
		Uint32( sizeof( Uint32 )												// 1. num points (4-byte)
		+ m_pointSet.GetPointNum() * m_pointSet.GetPointDim() * sizeof( Float )	// 2. points ( i * k * 4-byte, where k == 2 ) 
		+ m_edges.Size() * sizeof( Int32 )										// 3. edges ( n * 4-byte )
		);
}
#endif // NO_EDITOR
