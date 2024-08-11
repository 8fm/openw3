/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "importer.h"

IMPLEMENT_ENGINE_CLASS( IImporter );

Bool IImporter::SupportsResource( CClass* resourceClass ) const
{
	return m_resourceClass == resourceClass;
}

Bool IImporter::SupportsFormat( const String& fileFormat ) const
{
	// Linear search :P
	for ( Uint32 i=0; i<m_formats.Size(); i++ )
	{
		if ( m_formats[i].GetExtension().EqualsNC( fileFormat ) )
		{
			return true;
		}
	}

	// Not supported
	return false;
}

IImporter* IImporter::FindImporter( CClass* resourceClass, const String& fileFormat )
{
	// Request importer classes
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IImporter>(), importerClasses );


	while ( resourceClass )
	{
		// Linear search :P
		for ( Uint32 i=0; i<importerClasses.Size(); i++ )
		{
			IImporter* importer = importerClasses[i]->GetDefaultObject< IImporter >();
			if ( importer->SupportsResource( resourceClass ) )
			{
				if ( importer->SupportsFormat( fileFormat ))
				{
					return importer;
				}
			}
		}

		// If we didn't find a importer, check resourceClass' base class to see if we have an importer for that.
		//
		// This fixes problems where an importer is given for class A, but actually creates some base class B or C
		// depending on the contents of the file. Without this check, we are unable to reimport such an object, since
		// no importer exists specifically for B or C.
		resourceClass = resourceClass->GetBaseClass();
	}

	// Format not supported
	return NULL;
}

void IImporter::EnumImportClasses( TDynArray< CClass* >& importClasses )
{
	// Request importer classes
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IImporter>(), importerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<importerClasses.Size(); i++ )
	{
		IImporter* importer = importerClasses[i]->GetDefaultObject< IImporter >();
		importClasses.PushBackUnique( importer->m_resourceClass );
	}
}

void IImporter::EnumImportFormats( CClass* resourceClass, TDynArray< CFileFormat >& formats )
{
	// Request importer classes
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IImporter>(), importerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<importerClasses.Size(); i++ )
	{
		IImporter* importer = importerClasses[i]->GetDefaultObject< IImporter >();
		if ( importer->SupportsResource( resourceClass ) )
		{
			formats.PushBack( importer->m_formats );
		}
	}
}
