/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "baseCacheSplitter.h"
#include "textureCacheProcessing.h"


class CTextureCacheSplitter : public IBaseCacheSplitter
{
	DECLARE_RTTI_SIMPLE_CLASS( CTextureCacheSplitter );

public:
	CTextureCacheSplitter();
	~CTextureCacheSplitter();

	// interface
	virtual Bool Initialize( const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool LoadInput( const String& absolutePath ) override;
	virtual void GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const override;
	virtual Bool SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const override;

	// description
	virtual const Char* GetName() const override{ return TXT("textures"); }
	virtual const Char* GetDescription() const override { return TXT("Split texture cache data"); }

private:
	IFile*						m_inputFile;
	CTextureCacheData			m_data;
	Red::System::DateTime		m_originalTimeStamp;

};

BEGIN_CLASS_RTTI( CTextureCacheSplitter )
	PARENT_CLASS( IBaseCacheSplitter );
END_CLASS_RTTI();

// wrapper for a single entry in the texture cache
class CTextureCacheSplitEntry : public IBaseCacheEntry
{
public:
	CTextureCacheSplitEntry( Bool resource, const Uint32 entryIndex, const CTextureCacheData* data )
		: m_entryIndex( entryIndex )
		, m_data( data )
		, m_isResource( resource )
	{
	}

	virtual String GetResourcePath() const override
	{
		return m_data->GetEntryName( m_entryIndex );
	}

	virtual Uint32 GetApproxSize() const override
	{
		return m_data->GetEntryDiskSize( m_entryIndex );
	}

public:
	Uint32						m_entryIndex;
	const CTextureCacheData*	m_data;
	Bool						m_isResource;
};


//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( CTextureCacheSplitter );

CTextureCacheSplitter::CTextureCacheSplitter()
{
}

CTextureCacheSplitter::~CTextureCacheSplitter()
{
}

Bool CTextureCacheSplitter::Initialize( const ICommandlet::CommandletOptions& additonalOptions )
{
	// no additional options in here
	return true;
}

Bool CTextureCacheSplitter::LoadInput( const String& absolutePath )
{
	return m_data.LoadFromFile( absolutePath );
}

void CTextureCacheSplitter::GetEntries( TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	const Uint32 numEntries = m_data.GetNumEntries();
	for ( Uint32 i = 0; i < numEntries; ++i )
	{
		allEntries.PushBack( new CTextureCacheSplitEntry( true, i, &m_data ) );
	}
}

Bool CTextureCacheSplitter::SaveOutput( const String& absolutePath, const TDynArray< IBaseCacheEntry* >& allEntries ) const
{
	CTextureCacheDataSaver newCacheSaver( absolutePath );
	if ( !newCacheSaver )
	{
		return false;
	}

	for ( const IBaseCacheEntry* entry : allEntries )
	{
		const CTextureCacheSplitEntry* cacheEntry = static_cast< const CTextureCacheSplitEntry* >( entry );
		if ( !newCacheSaver.SaveEntry( cacheEntry->m_entryIndex, *cacheEntry->m_data ) )
		{
			ERR_WCC( TXT("TextureCacheSplitter failed while saving entries") );
			return false;
		}
	}

	return true;
}
