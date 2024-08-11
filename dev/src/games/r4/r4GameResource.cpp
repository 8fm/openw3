/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r4GameResource.h"
#include "../../common/core/factory.h"
#include "../../common/core/dataError.h"

IMPLEMENT_ENGINE_CLASS( CWitcherGameResource );

void CWitcherGameResource::EnumerateHuntingQuestEnums()
{
#ifndef EDITOR_SETTINGS
	if( m_huntingClueCategories.Empty() )
	{
#endif

		C2dArray* resource = m_huntingClueCategoryResource.Get();

		if( resource )
		{
			Uint32 numRows = static_cast< Uint32 >( resource->GetNumberOfRows() );
			m_huntingClueCategories.Reserve( numRows );

			for( Uint32 i = 0; i < numRows; ++i )
			{
				String enumName = resource->GetValue( 0, i );

				CEnum* enumInstance = SRTTI::GetInstance().FindEnum( CName( enumName ) );

				if( enumInstance != NULL )
				{
					m_huntingClueCategories.PushBack( enumInstance );
				}
				else
				{
					DATA_HALT( DES_Minor, resource, TXT( "Hunting Clues" ), TXT( "Missing or incorrect hunting clue category enum: %s" ), enumName.AsChar() );
				}
			}
		}

#ifndef EDITOR_SETTINGS
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

class CR4GameResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CR4GameResourceFactory, IFactory, 0 );

public:
	CR4GameResourceFactory()
	{
		m_resourceClass = ClassID< CWitcherGameResource >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		return ::CreateObject< CWitcherGameResource >( options.m_parentObject );
	}
};

BEGIN_CLASS_RTTI( CR4GameResourceFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CR4GameResourceFactory );
