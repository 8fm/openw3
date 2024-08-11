#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/engine/sectorPrefetchRuntimeCache.h"
#include "../../common/engine/sectorPrefetchMemoryBuffer.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "patchBundles.h"

class CPatchStreamingCacheCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CPatchStreamingCacheCommandlet, ICommandlet, 0 );

public:
	CPatchStreamingCacheCommandlet();

	virtual const Char* GetOneLiner( ) const;
	virtual void PrintHelp( ) const;
	virtual bool Execute( const CommandletOptions& options );

private:
};

DEFINE_SIMPLE_RTTI_CLASS( CPatchStreamingCacheCommandlet, ICommandlet );
IMPLEMENT_ENGINE_CLASS( CPatchStreamingCacheCommandlet );

CPatchStreamingCacheCommandlet::CPatchStreamingCacheCommandlet()
{
	m_commandletName = CName( TXT("patchstreamingcache") );
}

const Char* CPatchStreamingCacheCommandlet::GetOneLiner( ) const
{
	return TXT( "Generate diff between patched buffers and streaming.cache" );
}

void CPatchStreamingCacheCommandlet::PrintHelp( ) const
{
//	LOG_WCC( TXT( "Usage:" ) );
//	LOG_WCC( TXT( "  loadtest") );
}

extern Bool GPatchingMod;

bool CPatchStreamingCacheCommandlet::Execute( const CommandletOptions& options )
{
	String stringCachePath;
	if ( !options.GetSingleOptionValue( TXT("streamingcache"), stringCachePath ) )
	{
		return false;
	}
	String dataBasePath;
	if ( !options.GetSingleOptionValue( TXT("db"),TXT("cookdb"), dataBasePath ) )
	{
		return false;
	}

	String baseBundelsDir;
	if ( !options.GetSingleOptionValue( TXT("basebundelsdir"), baseBundelsDir ) )
	{
		return false;
	}

	String patchedBundelsDir;
	if ( !options.GetSingleOptionValue( TXT("patchedbundelsdir"), patchedBundelsDir ) )
	{
		return false;
	}

	String outDir;
	if ( !options.GetSingleOptionValue( TXT("outdir"), outDir ) )
	{
		return false;
	}

	// load the cook.db
	CCookerDataBase db;
	if ( !db.LoadFromFile( dataBasePath ) )
	{
		ERR_WCC( TXT("Failed to load cooker data base from '%ls'. Invalid file?"), dataBasePath.AsChar() );
		return false;
	}

	CSectorPrefetchMemoryBuffer sectorPrefetchMemoryBuffer;
	CSectorPrefetchRuntimeCache* streamingCache = new CSectorPrefetchRuntimeCache( sectorPrefetchMemoryBuffer );

	streamingCache->Initialize(stringCachePath);
	
	TDynArray< CCookerResourceEntry > filesToProcess;

	// get the files using the entries of cook.db
	TDynArray< String > classesToProcess;
	classesToProcess.PushBack( TXT("w2mesh") );
	db.QueryResources( [&]( const CCookerDataBase& db, const CCookerResourceEntry& entry ) 
		{
			filesToProcess.PushBack( entry );
			return true;
		}
		,
		CookDataBaseHelper::PerExtensionFilter( classesToProcess )
		);

	/// Hack for loading all bundles
	GPatchingMod = true;

	CPatchBundles* baseBundles = CPatchBundles::LoadBundles(baseBundelsDir);
	CPatchBundles* patchBundles = CPatchBundles::LoadBundles(patchedBundelsDir);
	
	GPatchingMod = false;

	TDynArray< IBasePatchContentBuilder::IContentToken* > outBaseTokens;
	baseBundles->GetTokens( outBaseTokens );

	TDynArray< IBasePatchContentBuilder::IContentToken* > outPatchedTokens;
	patchBundles->GetTokens( outPatchedTokens );

	TDynArray< StringAnsi > patchedFiles;

	for(CCookerResourceEntry file : filesToProcess)
	{
		StringAnsi bufferFileName = file.GetFilePath() + ".1.buffer";
		const Uint64 bufferFileNameHash = Red::CalculateHash64( bufferFileName.AsChar() );
		for(IBasePatchContentBuilder::IContentToken* tokenPatched : outPatchedTokens)
		{
			if(tokenPatched->GetTokenHash() == bufferFileNameHash)
			{
				for(IBasePatchContentBuilder::IContentToken* tokenBase : outBaseTokens)
				{
					if(tokenPatched->GetTokenHash() == tokenBase->GetTokenHash() )
					{
						if(tokenPatched->GetDataCRC() != tokenBase->GetDataCRC())
						{
							patchedFiles.PushBackUnique(file.GetFilePath());
							break;
						}
					}
				}
			}
		}
	}

	LOG_WCC(TXT("Patched files count: %d"), patchedFiles.Size() );

	C2dArray* patchedMeshesInStreamingCache = C2dArray::FactoryInfo< C2dArray >().CreateResource();
	patchedMeshesInStreamingCache->AddColumn(TXT("DepotPath"), TXT(""));
	TDynArray<String> rowData(2);

	Uint32 patchedEntries = 0;
	
	for(StringAnsi file : patchedFiles)
	{
		Char* fileDepotPath = ANSI_TO_UNICODE(file.AsChar());
		const Uint64 resourceHash = Red::CalculatePathHash64(fileDepotPath);
		if( streamingCache->Exist(resourceHash) )
		{
			LOG_WCC(TXT("Patched streaming cache entry: %hs"), file.AsChar());
			rowData[0] = fileDepotPath;
			patchedMeshesInStreamingCache->AddRow(rowData);

			++patchedEntries;
		}
	}
	
	LOG_WCC(TXT("Streaming cache has %d entries patched."), patchedEntries);

	CDirectory outDirectory(outDir.AsChar(), outDir.Size()-1, NULL);

	size_t lastSeparatorIndex = 0;
	stringCachePath.FindCharacter( '\\', lastSeparatorIndex, true );
	String streamingCacheFileName = stringCachePath.RightString(stringCachePath.GetLength() - (lastSeparatorIndex+1));
	String streamingCacheIgnoreListFileName = streamingCacheFileName + TXT(".ignore.csv");

	String streamingCacheIgnoreListAbsPath = outDir + streamingCacheIgnoreListFileName;

	GFileManager->DeleteFile( streamingCacheIgnoreListAbsPath );
	
	if( patchedMeshesInStreamingCache->SaveAs( &outDirectory, streamingCacheIgnoreListFileName ) == false )
	{
		ERR_WCC( TXT("Failed to save '%ls'. Invalid file?"), streamingCacheIgnoreListAbsPath.AsChar() );
	}
	patchedMeshesInStreamingCache->Discard();

	delete patchBundles;
	delete baseBundles;
	delete streamingCache;

	return true;
}
