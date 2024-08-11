/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r6GameResource.h"
#include "../../common/core/factory.h"


IMPLEMENT_ENGINE_CLASS( CR6GameResource );

#define DEFAULT_TRAIT_DATA		TXT( "gameplay\\globals\\trait_data.trait" )
#define DEFAULT_CHARACTER_DB	TXT( "gameplay\\character_db\\" )
//////////////////////////////////////////////////////////////////////////
CR6GameResource::CR6GameResource() 
	: CCommonGameResource()
	, m_traitDataPath( DEFAULT_TRAIT_DATA )
	, m_characterDBRootDir( DEFAULT_CHARACTER_DB )
{

}

class CR6GameResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CR6GameResourceFactory, IFactory, 0 );

public:
	CR6GameResourceFactory()
	{
		m_resourceClass = ClassID< CR6GameResource >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		return ::CreateObject< CR6GameResource >( options.m_parentObject );
	}
};

BEGIN_CLASS_RTTI( CR6GameResourceFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CR6GameResourceFactory );
