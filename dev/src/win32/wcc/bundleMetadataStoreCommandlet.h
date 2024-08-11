/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef __BUNDLE_METADATA_STORE_COMMANDLET_H__
#define __BUNDLE_METADATA_STORE_COMMANDLET_H__

#include "../../common/core/commandlet.h"

// Used as a helper to streamline world analysis pipelines
// Options parsed: wcc metadatastore [path=absolute/path/to/bundles/directory]
class CBundleMetadataStoreCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CBundleMetadataStoreCommandlet, ICommandlet, 0 );

public:
	CBundleMetadataStoreCommandlet();
	virtual ~CBundleMetadataStoreCommandlet();

private:
	virtual Bool Execute( const CommandletOptions& options ) override final;
	virtual const Char* GetOneLiner() const override final;
	virtual void PrintHelp() const override final;
};

BEGIN_CLASS_RTTI( CBundleMetadataStoreCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

#endif // __BUNDLE_METADATA_STORE_COMMANDLET_H__
