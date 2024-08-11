/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/engine/collisionCacheOptimizer.h"

/// Helper commandlet that can be used to optimize collision cache
class COptimizeCollisionCache : public ICommandlet
{
	DECLARE_ENGINE_CLASS( COptimizeCollisionCache, ICommandlet, 0 );

public:
	COptimizeCollisionCache();
	~COptimizeCollisionCache();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Optimize (resort) collision cache"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( COptimizeCollisionCache )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( COptimizeCollisionCache );

COptimizeCollisionCache::COptimizeCollisionCache()
{
	m_commandletName = CName( TXT("optimizecollisioncache") );
}

COptimizeCollisionCache::~COptimizeCollisionCache()
{
}

bool COptimizeCollisionCache::Execute( const CommandletOptions& options )
{
	// get out path
	String outPath;
	if ( !options.GetSingleOptionValue( TXT("out"), outPath ) )
	{
		ERR_WCC( TXT("Missing output file path") );
		return false;
	}

	// get input file path
	String inPath;
	if ( !options.GetSingleOptionValue( TXT("file"), inPath ) )
	{
		ERR_WCC( TXT("Missing intput file path") );
		return false;
	}

	// we cannot load and save the same file
	if ( inPath.EqualsNC( outPath ) )
	{
		ERR_WCC( TXT("You cannot load and save to the same file") );
		return false;
	}

	// process
	CCollisionCacheOptimizer optimizer;
	return optimizer.OptimizeFile( inPath, outPath );
}

void COptimizeCollisionCache::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  optimizecollisioncache -file=<path> -out=<path>" ) );
}
