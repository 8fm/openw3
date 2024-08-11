/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "expCooked.h"
#include "expCooking.h"
#include "expComponent.h"

IMPLEMENT_ENGINE_CLASS( CCookedExplorations );

void CCookedExplorations::OnSerialize( class IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( false == file.IsGarbageCollector() && file.IsReader() )
	{
		Uint64 offset = file.GetOffset();

		// check if the file contains exploration data
		Uint32 magic = 0;
		Uint32 dataSize = 0;
		file << dataSize;
		file << magic;						 
		if ( magic == EXPLORATION_DATA_MAGIC && dataSize > 0 )
		{
			// yes!
			m_data = new DataBuffer( TDataBufferAllocator< MC_ExplorationData >::GetInstance() );
		}

		// seek back
		file.Seek( offset );
	}

	if ( m_data )
	{
		m_data->Serialize( file ); // GC checked inside
	}
}

CCookedExploration::CCookedExploration( CExplorationComponent* other )
	: m_start( other->m_start )
	, m_end( other->m_end )
	, m_explorationId( other->m_explorationId )
{
	other->GetMatWS( m_mat );
	other->GetParentMatWS( m_parentMat );
}

void CCookedExploration::GetMatWS( Matrix & mat ) const			// this method will be removed soon, don't use
{
	mat = m_mat;
}

void CCookedExploration::GetParentMatWS( Matrix& mat ) const	// this method will be removed soon, don't use
{
	mat = m_parentMat;
}

void CCookedExploration::GetEdgeWS( Vector& p1, Vector& p2 ) const
{
	p1 = m_mat.TransformPoint( m_start );
	p2 = m_mat.TransformPoint( m_end );
}

void CCookedExploration::GetNormal( Vector& n ) const
{
	n = m_mat.GetRow( 1 ).Normalized3();
}

Int32 CCookedExploration::GetId() const
{
	return m_explorationId;
}

CObject* CCookedExploration::GetObjectForEvents() const
{
	// TODO
	return nullptr;
}

Uint32 CCookedExploration::ComputeDataSize() const
{
	return								// We save:
		Uint32(	sizeof( Matrix )		// 1. m_mat				TO BE REMOVED
		+ sizeof( Matrix )				// 2. m_parentMat		TO BE REMOVED
		+ sizeof( Vector3 )				// 3. m_start
		+ sizeof( Vector3 )				// 4. m_end
		+ sizeof( Int8 )				// 5. m_explorationID
		);
}

void CCookedExploration::Serialize( IFile& file )
{
	file << m_mat;
	file << m_parentMat;
	file << m_start;
	file << m_end;
	file << m_explorationId;
}
