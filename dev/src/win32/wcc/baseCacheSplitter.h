/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/commandlet.h"

/// Abstract cache entry - you should provide your own implementation
class IBaseCacheEntry
{
public:
	virtual ~IBaseCacheEntry() {};

	//! Get the resource this entry is for
	virtual String GetResourcePath() const = 0;

	//! Get approximate size of the entry (stats)
	virtual Uint32 GetApproxSize() const = 0;
};

/// Interface for base cache splitter
class IBaseCacheSplitter
{
	DECLARE_RTTI_SIMPLE_CLASS( IBaseCacheSplitter );

public:
	virtual ~IBaseCacheSplitter() {};

	// Get cache splitter name
	virtual const Char* GetName() const = 0;

	// Get description
	virtual const Char* GetDescription() const = 0;

	// Initialize - may parse additional arguments
	virtual Bool Initialize( const ICommandlet::CommandletOptions& additonalOptions ) = 0;

	// Load input cache
	virtual Bool LoadInput( const String& absolutePath ) = 0;

	// Get entries from cache
	virtual void GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const = 0;

	// Create output cache from given entry list
	virtual Bool SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI(IBaseCacheSplitter);
END_CLASS_RTTI();
