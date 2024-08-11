/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dependencySaver2dArray.h"
#include "dependencyLinkerFactory.h"
#include "2darray.h"

Bool CDependencySaver2dArray::SaveObjects( DependencySavingContext& context )
{
	C2dArray* arr = Cast<C2dArray>( context.m_initialExports[0] );

	// Prepare data to save
	Uint32 dataLength = arr->CalculateDataLength();
	Char* data = new Char[ dataLength ];
	Red::System::MemorySet( data, 0, dataLength * sizeof( Char ) );

	arr->PrepareData( data );

	String dataToSave( data );
	// Save the data
	if ( arr->GetDepotPath().ContainsSubstring( TXT( "credits" ) )  && arr->GetDepotPath().ContainsSubstring( TXT( "csv" ) ) )
	{
		GFileManager->SaveStringToFileWithUTF16( *m_file, dataToSave );
	}
	else
	{
		m_file->Serialize( UNICODE_TO_ANSI( dataToSave.AsChar() ), dataToSave.GetLength()  );
	}

	delete[] data;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

CDependencySaver2dArrayFactory::CDependencySaver2dArrayFactory()
{
	SDependencyLinkerFactory::GetInstance().RegisterSaverFactory( ResourceExtension< C2dArray >(), this );
}

CDependencySaver2dArrayFactory::~CDependencySaver2dArrayFactory()
{
	SDependencyLinkerFactory::GetInstance().UnregisterSaverFactory( this );
}

IDependencySaver* CDependencySaver2dArrayFactory::CreateSaver( IFile& file, const CDiskFile* ) const
{
	return new CDependencySaver2dArray( file );
}
