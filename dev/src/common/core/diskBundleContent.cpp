/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskBundleContent.h"

IMPLEMENT_ENGINE_CLASS( CDiskBundleContent );

CDiskBundleContent::CDiskBundleContent()
{
}

CDiskBundleContent::~CDiskBundleContent()
{
}

void CDiskBundleContent::SetResources( TDynArray< THandle< CResource > >& resources )
{
	m_resources = std::move( resources );
}

void CDiskBundleContent::ExtractResources( const CClass* resourceClass, TDynArray< THandle< CResource > >& outResources ) const
{
	// count resource to extract
	Uint32 numResources = 0;
	for ( Uint32 i=0; i<m_resources.Size(); ++i )
	{
		if ( m_resources[i] && m_resources[i]->IsA( resourceClass ) )
		{
			numResources += 1;
		}
	}

	// copy the handles
	const Uint32 base = (Uint32) outResources.Grow( numResources );
	for ( Uint32 i=0; i<m_resources.Size(); ++i )
	{
		if ( m_resources[i] && m_resources[i]->IsA( resourceClass ) )
		{
			outResources[ base + i ] = m_resources[i];
		}
	}
}

void CDiskBundleContent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	file << m_resources;
}

