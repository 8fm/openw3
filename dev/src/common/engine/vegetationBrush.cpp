/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "vegetationBrush.h"

#include "../core/factory.h"
#include "baseTree.h"
#include "../core/dataError.h"

IMPLEMENT_RTTI_ENUM( EVegetationAlignment );
IMPLEMENT_ENGINE_CLASS( CVegetationBrushEntry );
IMPLEMENT_ENGINE_CLASS( CVegetationBrush );

CVegetationBrushEntry::CVegetationBrushEntry()
	: m_resource( NULL )
	, m_size( 1.0f )
	, m_sizeVar( 0.0f )
	, m_radiusScale( 1.0f )
	, m_density( 1.0f )
{

}

CVegetationBrushEntry* CVegetationBrush::AddEntry( CSRTBaseTree* baseTree )
{
	if ( !baseTree )
	{
		return nullptr;
	}

	CVegetationBrushEntry* result = FindEntry( baseTree );

	// Don't add if already there
	if ( !result )
	{
		m_entries.PushBack( new CVegetationBrushEntry() );
		result = m_entries.Back();
		result->SetParent( this );
		result->m_resource = baseTree;
	}

	return result;
}

CVegetationBrushEntry* CVegetationBrush::FindEntry( CSRTBaseTree* baseTree )
{
	for ( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		if ( m_entries[i]->m_resource == baseTree )
		{
			return m_entries[i];
		}
	}

	return NULL;
}

void CVegetationBrush::GetEntries( TDynArray< CVegetationBrushEntry* >& entries ) const
{
	entries.PushBack( m_entries );
}

void CVegetationBrush::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	for (Uint32 i = 0; i < m_entries.Size(); ++i)
	{
		if ( m_entries[i] && m_entries[i]->GetBaseTree() == nullptr )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Vegetation brush '%ls' references missing SpeedTree asset"), GetFile()->GetDepotPath() );
			m_validEntries = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
/// Factory for the vegetation brush
//////////////////////////////////////////////////////////////////////////

class CVegetationBrushFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CVegetationBrushFactory, IFactory, 0 );

public:
	CVegetationBrushFactory()
	{
		m_resourceClass = ClassID< CVegetationBrush >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CVegetationBrushFactory )
	PARENT_CLASS( IFactory )	
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CVegetationBrushFactory );

CResource* CVegetationBrushFactory::DoCreate( const FactoryOptions& options )
{
	CVegetationBrush *brush = ::CreateObject< CVegetationBrush >( options.m_parentObject );
	return brush;
}
