/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "cacheBuilderCommandlet.h"

/// Utility class that allows you to build
class ICacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( ICacheBuilder );

public:
	ICacheBuilder();
	virtual ~ICacheBuilder(){}

	// interface
	virtual void GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const = 0; // return list of classes you want to process
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) = 0; // return false to fail
	virtual Bool Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess ) = 0;
	virtual Bool Save() = 0; // called only if there were no errors

	// interface
	virtual const Char* GetName() const = 0;
	virtual const Char* GetDescription() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI(ICacheBuilder);
END_CLASS_RTTI();

///------

/// Helper class that is calling "ProcessFile" function for every file in the input list
class IFileBasedCacheBuilder : public ICacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( IFileBasedCacheBuilder );

public:
	IFileBasedCacheBuilder();

	// ICacheBuilder - loop over the list of files
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool Process( const CCookerDataBase& db, const TDynArray< CCookerDataBase::TCookerDataBaseID >& filesToProcess ) override;

	// IFileBasedCacheBuilder interface
	virtual Bool ProcessFile( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, const String& depotPath ) = 0;

protected:
	Bool		m_ignoreErrors;
};

BEGIN_ABSTRACT_CLASS_RTTI(IFileBasedCacheBuilder);
	PARENT_CLASS(ICacheBuilder);
END_CLASS_RTTI();

///------

/// Helper class that is managed loading/unloading of the resources and casss the "ProcessResource" function for every file in the input list
class IResourceBasedCacheBuilder : public IFileBasedCacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( IResourceBasedCacheBuilder );

public:
	IResourceBasedCacheBuilder();

	// IFileBasedCacheBuilder
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool ProcessFile( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, const String& depotPath ) override;

	// IFileBasedCacheBuilder interface
	virtual Bool ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource ) = 0;

private:
	Uint32		m_gcFrequency;
	Uint32		m_gcCount;
};

BEGIN_ABSTRACT_CLASS_RTTI(IResourceBasedCacheBuilder);
	PARENT_CLASS(IFileBasedCacheBuilder);
END_CLASS_RTTI();
