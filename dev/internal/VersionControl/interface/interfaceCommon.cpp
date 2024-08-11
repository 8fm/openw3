/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "interfaceCommon.h"

namespace VersionControl
{
	Filelist* InterfaceCommon::CreateFileList()
	{
		return new Filelist();
	}

	void InterfaceCommon::DestroyFileList( Filelist* ptr )
	{
		delete ptr;
	}

	bool InterfaceCommon::Add( Filelist* list )
	{
		return Add( list->Get(), list->Size() );
	}

	bool InterfaceCommon::Add( const char* file )
	{
		return Add( &file, 1 );
	}

	bool InterfaceCommon::Checkout( Filelist* list )
	{
		return Checkout( list->Get(), list->Size() );
	}

	bool InterfaceCommon::Checkout( const char* file )
	{
		return Checkout( &file, 1 );
	}

	bool InterfaceCommon::Revert( Filelist* list )
	{
		return Revert( list->Get(), list->Size() );
	}

	bool InterfaceCommon::Revert( const char* file )
	{
		return Revert( &file, 1 );
	}

	bool InterfaceCommon::Submit( Filelist* list, const char* description )
	{
		return Submit( list->Get(), list->Size(), description );
	}

	bool InterfaceCommon::Submit( const char* file, const char* description )
	{
		return Submit( &file, 1, description );
	}

	bool InterfaceCommon::Delete( Filelist* list )
	{
		return Delete( list->Get(), list->Size() );
	}

	bool InterfaceCommon::Delete( const char* file )
	{
		return Delete( &file, 1 );
	}

	bool InterfaceCommon::Sync( Filelist* list )
	{
		return Sync( list->Get(), list->Size() );
	}

	bool InterfaceCommon::Sync( const char* file )
	{
		return Sync( &file, 1 );
	}

}
