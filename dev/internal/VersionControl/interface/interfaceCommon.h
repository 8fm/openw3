/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_INTERFACE_COMMON_H__
#define __VERCON_INTERFACE_COMMON_H__

#include "interface.h"

namespace VersionControl
{
	/// Simple source control integration
	class InterfaceCommon : public Interface
	{
	public:
		InterfaceCommon() {}
		virtual ~InterfaceCommon() {}

		virtual bool Add( Filelist* list ) final;
		virtual bool Checkout( Filelist* list ) final;
		virtual bool Revert( Filelist* list ) final;
		virtual bool Submit( Filelist* list, const char* description ) final;
		virtual bool Delete( Filelist* list ) final;
		virtual bool Sync( Filelist* list ) final;

		virtual bool Add( const char* file ) final;
		virtual bool Checkout( const char* file ) final;
		virtual bool Revert( const char* file ) final;
		virtual bool Submit( const char* file, const char* description ) final;
		virtual bool Delete( const char* file ) final;
		virtual bool Sync( const char* file ) final;

		virtual Filelist* CreateFileList() final;
		virtual void DestroyFileList( Filelist* ptr ) final;

		//
		virtual bool Add( const char** files, unsigned int numFiles ) override { return false; }
		virtual bool Checkout( const char** files, unsigned int numFiles ) override { return false; }
		virtual bool Revert( const char** files, unsigned int numFiles ) override { return false; }
		virtual bool Submit( const char** files, unsigned int numFiles, const char* description ) override { return false; }
		virtual bool Delete( const char** files, unsigned int numFiles ) override { return false; }
		virtual bool Sync( const char** files, unsigned int numFiles ) override { return false; }
	};
}

#endif // __VERCON_INTERFACE_COMMON_H__
