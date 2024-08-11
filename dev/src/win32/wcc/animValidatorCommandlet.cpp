#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../editor/animationValidator.h"
#include "../../common/engine/skeleton.h"

//#pragma optimize("",off)

class CAnimationValidatorCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CAnimationValidatorCommandlet, ICommandlet, 0 );

public:
	CAnimationValidatorCommandlet();

	virtual const Char* GetOneLiner() const { return TXT( "Check if all animations and skeletons are valid." ); }

	virtual bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CAnimationValidatorCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimationValidatorCommandlet );
RED_DEFINE_STATIC_NAME( animationvalidator )

CAnimationValidatorCommandlet::CAnimationValidatorCommandlet()
{
	m_commandletName = CNAME( animationvalidator );
}

namespace
{
	template< typename F > void ProcessResources( String& log, const String& ex, F& func )
	{
		TDynArray< String > paths;
		GDepot->FindResourcesByExtension( ex, paths );

		for ( Uint32 i=0; i<paths.Size(); i++ )
		{
			CResource* fileResource( nullptr );
			CDiskFile* diskFile = GDepot->FindFile( paths[i] );

			const Bool alreadyLoaded = diskFile->IsLoaded(); 
			if ( alreadyLoaded )
			{
				fileResource = diskFile->GetResource();
			}
			else
			{
				fileResource = LoadResource< CResource >( paths[i] );
			}

			func( fileResource );

			if ( !alreadyLoaded )
			{
				diskFile->Unload();
			}
		}
	}
}

Bool CAnimationValidatorCommandlet::Execute( const CommandletOptions& options )
{
	const auto& arguments = options.GetFreeArguments();
	if( arguments.Size() != 1 )
	{
		ERR_WCC( TXT( "Output file not specified!" ) );
		PrintHelp();
		return false;
	}

	const String outputPath = arguments[0];

	// Animation Validate is broken
	/*
	CAnimationValidator validator;
	
	Uint32 numProblemsFound = 0;
	TDynArray< CDirectory* > dirs;
	dirs.PushBack( dir );

	validator.Execute( dirs, numProblemsFound );
	if ( numProblemsFound > 0 )
	{
		validator.DumpErrorsToFile( TXT("animationCheckLog.csv"), outputPath );
	}
	*/

	String log;

	// 1. Check all animations
	ProcessResources( log, TXT("w2anims"), 
		[&]( CResource* resource )
	{
		CSkeletalAnimationSet* set = Cast< CSkeletalAnimationSet >( resource );
		if ( set )
		{
			TDynArray< CName > outCorruptedAnimations;
			if ( set->CheckAllAnimations( outCorruptedAnimations ) == false )
			{
				LOG_WCC( TXT("Animset '%ls' contains currupted animations:"), set->GetDepotPath().AsChar() );
				log += set->GetDepotPath();
				log += TXT("\n");

				for ( CName n : outCorruptedAnimations )
				{
					LOG_WCC( TXT("   >%ls"), n.AsChar() );
					log += n.AsChar();
					log += TXT("\n");
				}

				log += TXT("\n");
			}
		}
	});

	// 2. Check all skeletons
	ProcessResources( log, TXT("w2rig"), 
		[&]( CResource* resource )
	{
		const CSkeleton* skeleton = Cast< CSkeleton >( resource );
		if ( skeleton )
		{
			const Uint32 numBones = skeleton->GetBonesNum();
			const AnimQsTransform* bones = skeleton->GetReferencePoseLS();

			for ( Uint32 i=0; i<numBones; ++i )
			{
				const AnimQsTransform& b = bones[ i ];
				const Bool isOk = b.Translation.IsOk() && b.Rotation.Quat.IsOk() && b.Scale.IsOk();
				if ( !isOk )
				{
					LOG_WCC( TXT("Skeleton '%ls' contains currupted bones"), skeleton->GetDepotPath().AsChar() );
					log += skeleton->GetDepotPath();
					log += TXT("\n");

					break;
				}
			}
		}
	});

	if ( !GFileManager->SaveStringToFile( outputPath, log ) )
	{
		ERR_WCC(TXT("Failed to save file '%ls'"), outputPath.AsChar() );
		return false;
	}

	return true;
}

void CAnimationValidatorCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc animationvalidator out_absolute_path" ) );
}

//#pragma optimize("",on)
