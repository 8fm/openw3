
#include "build.h"
#include "defaultCharactersIterator.h"
#include "../../common/core/gatheredResource.h"

CGatheredResource resDefaultCharacters( TXT("gameplay\\globals\\default_characters.csv"), RGF_Startup );

void DefaultCharactersIterator::LoadPaths()
{
	String activeResource;
	if ( GetActiveResource( activeResource, ClassID< CEntityTemplate >() ) )
	{
		CFilePath path( activeResource );
		String name = String::Printf( TXT("Selected [%s]"), path.GetFileName().AsChar() );

		m_names.PushBack( name );
		m_cats.PushBack( TXT("") );
		m_paths.PushBack( activeResource );
	}

	C2dArray* arr = resDefaultCharacters.LoadAndGet< C2dArray >();
	ASSERT( arr );
	if ( arr )
	{
		Uint32 uRowNum, uColNum;
		arr->GetSize( uColNum, uRowNum );

		ASSERT( uColNum >= 2 );

		for ( Uint32 i=0; i<uRowNum; ++i )
		{
			String name = arr->GetValue( 0, i );
			String cat = arr->GetValue( 1, i );
			String path = arr->GetValue( 2, i );

			m_names.PushBack( name );
			m_cats.PushBack( cat );
			m_paths.PushBack( path );
		}
	}
	else
	{
		ASSERT( arr );
	}
}
