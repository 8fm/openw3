#include "build.h"
#include "animationValidator.h"
#include "..\..\common\core\directory.h"
#include "..\..\common\engine\skeleton.h"
#include "..\..\common\engine\skeletalAnimationSet.h"
#include "resourceIterator.h"
#include "..\..\common\engine\skeletalAnimationEntry.h"
#include "..\..\common\engine\skeletalAnimation.h"

CAnimationValidator::CAnimationValidator( Bool andFixSourceAnimData )
: m_fixSourceAnimData( andFixSourceAnimData )
{
}

void CAnimationValidator::Execute( const TDynArray< CDirectory* > dirs, Uint32& numProblemsFound )
{

	for ( CResourceIterator< CSkeletalAnimationSet > animSet( dirs, TXT("Collecting animation files"), RIF_ReadOnly ); animSet; ++animSet )
	{
		m_changedAnything = false;
		m_markAnimSetDirtyWhenFixing = true;
		m_curentFilePath = animSet->GetFile()->GetDepotPath();

		ValidateSkeleton( animSet->GetSkeleton() );
		ValidateAnimations( animSet->GetAnimations() );

		if ( m_fixSourceAnimData && m_changedAnything )
		{
			if ( CDiskFile* file = animSet->GetFile() )
			{
				file->GetStatus(); 

				if ( file->IsLocal() || file->IsCheckedOut() )
				{
					file->Save();
					ReportError( String::Printf( TXT( "Saved file" ) ) );
				}
				else
				{
					ReportError( String::Printf( TXT( "Not saved! Please, check out and save." ) ) );
				}
			}
		}
	}

	numProblemsFound = m_errors.Size();
}

void CAnimationValidator::DumpErrorsToFile( String filename, CDirectory* saveDirectory )
{
	//CResource::FactoryInfo< C2dArray > info;
	//C2dArray* debugDumpInfo = info.CreateResource();
	String dataString = String::EMPTY;

	String columnPathName = TXT("Path");
	String columnProblemsName = TXT("Problems");

	dataString += columnPathName;
	dataString += TXT(";");
	dataString += columnProblemsName;
	dataString += TXT(";\n");

	// clear file
	GFileManager->SaveStringToFile( saveDirectory->GetAbsolutePath() + filename, dataString, false );
	dataString = String::EMPTY;

	//debugDumpInfo->AddColumn( columnPathName, TXT("") );
	//debugDumpInfo->AddColumn( columnProblemsName, TXT("") );		

	Uint32 rowIndex = 0;
	Uint32 j = 1;

	for( auto it = m_errors.Begin(); it != m_errors.End(); ++it )
	{		
		//String allProblems = String::EMPTY;		
		//debugDumpInfo->AddRow( it->m_first );		

		dataString += it->m_first;
		dataString += TXT(";");

		for( Uint32 i=0; i<it->m_second.Size(); ++i )
		{
			//allProblems += it->m_second[i];
			//if( i != it->m_second.Size()-1 ) allProblems += TXT("  -  ");			
			
			dataString += it->m_second[i];
			dataString += TXT("\n");
			dataString += TXT(";");
		}
		
		dataString += TXT(";\n");

		//debugDumpInfo->SetValue( allProblems, columnProblemsName, rowIndex );				

		// for the sake of the string overflow
		if( rowIndex > j*100 )
		{			
			GFileManager->SaveStringToFile( saveDirectory->GetAbsolutePath() + filename, dataString, true );

			dataString = String::EMPTY;
			++j;
		}

		++rowIndex;
	}	


	GFileManager->SaveStringToFile( saveDirectory->GetAbsolutePath() + filename, dataString, true );

	//debugDumpInfo->SaveAs( saveDirectory, filename, true );	
	//debugDumpInfo->Discard();
}

void CAnimationValidator::ValidateSkeleton( const CSkeleton* skeleton )
{
	if( !skeleton )
	{
		// Not a real error
		// ReportError( TXT("No skeleton in this animset") );
		return;
	}
		
	// validate the skeleton
	const AnimQsTransform* transform = skeleton->GetReferencePoseLS();
	const Int32 numBones = (Int32)skeleton->GetBonesNum();
	for ( Int32 i=0; i<numBones; ++i )
	{
		if ( /*!transform[i].Rotation.HasValidAxis() ||*/ !transform[i].Rotation.IsOk() ) // do not check for valid axis!
		{
			ReportError( String::Printf( TXT("%ls - Skeleton reference pose rotation data is corrupted!"), skeleton->GetBoneName( i ).AsChar() ) );
		}
		if ( !transform[i].Translation.IsOk() )
		{
			ReportError( String::Printf( TXT("%ls - Skeleton reference pose translation data is corrupted!"), skeleton->GetBoneName( i ).AsChar() ) );
		}
		if ( !transform[i].Scale.IsOk() )
		{
			ReportError( String::Printf( TXT("%ls - Skeleton reference pose scale data is corrupted!"), skeleton->GetBoneName( i ).AsChar() ) );
		}
	}

}

void CAnimationValidator::ValidateAnimations( const TDynArray< CSkeletalAnimationSetEntry* >& animationSets )
{
	// validate skeletal animations 
	for ( Uint32 i = 0; i < animationSets.Size(); ++i )
	{
		
		CSkeletalAnimation* anim = animationSets[i]->GetAnimation();
		if ( anim )
		{
			ValidateAnimation( anim );
		}			
	}	
}

void CAnimationValidator::ValidateAnimation( const CSkeletalAnimation* animation )
{
	const Float timeSample = 0.033f;
	const Float duration = animation->GetDuration();
	Float t = 0.f;
	TDynArray< AnimQsTransform > transform;
	TDynArray< AnimFloat > tracks;

	//RED_LOG( TXT( "AnimationValidation" ), animation->GetName().AsChar(), animation->GetImportFile().AsChar() );

	animation->AddUsage();
	animation->SyncLoad();

	const CName& animName = animation->GetEntry()->GetName();

	if (animation->HasAnimBuffer())
	{
		Uint32 bonesNum = animation->GetBonesNum();

		// Sample animation over time
		while ( t <= duration )
		{
			Bool ret = animation->Sample( t, transform, tracks );
			if ( !ret )
			{
				ReportError( String::Printf( TXT( "Couldn't sample animation %ls" ), animName.AsChar() ) );
				return;
			}

			if ( bonesNum != transform.Size() )
			{
				ReportError( String::Printf( TXT( "Animation sample has different bones number than animation itself, %ls" ), animName.AsChar() ) );
			}

			for ( Uint32 i = 0; i < transform.Size(); ++i )
			{
				// we read compressed data so axis might not be normalized at this point
				if ( !transform[i].Rotation.IsOk() ) //!transform[i].Rotation.HasValidAxis() 
				{
					ReportError( String::Printf( TXT("%ls - Rotation data is corrupted!"), animName.AsChar() ) );
				}
				if ( !transform[i].Translation.IsOk() )
				{
					ReportError( String::Printf( TXT("%ls - Translation data is corrupted!"), animName.AsChar() ) );
				}
				if ( !transform[i].Scale.IsOk() )
				{
					ReportError( String::Printf( TXT("%ls - Scale data is corrupted!"), animName.AsChar() ) );
				}
			}

			t += timeSample;
		}

		// hack since there is no const getter in CSkeletalAnimation
		CSkeletalAnimation* hackConstAnimation = (CSkeletalAnimation*)animation;
		LatentSourceAnimDataBuffer* buff = hackConstAnimation->GetSourceAnimData( false );
		Bool valid = false;

		if( buff ) 
		{
			valid = buff->Load() && buff->GetSize();
			IAnimationBuffer::SourceData animationData;

			if( valid )
			{
				buff->ReadAnimDataTo( animationData, hackConstAnimation );
				valid = animationData.IsValid( animation );

				if( !valid ) ReportError( String::Printf( TXT( "Source buffer data is invalid - %ls" ), animName.AsChar() ) );
			}
			else
				ReportError( String::Printf( TXT( "Source buffer data have invalid size - %ls" ), animName.AsChar() ) );

			buff->Unload();						
		}
		else
		{
			ReportError( String::Printf( TXT( "Source buffer data doesn't exist - %ls" ), animName.AsChar() ) );
		}

		if ( !valid && m_fixSourceAnimData )
		{
			m_changedAnything = true;
			hackConstAnimation->MakeSureSourceAnimDataExists();
			if ( m_markAnimSetDirtyWhenFixing )
			{
				hackConstAnimation->GetEntry()->GetAnimSet()->MarkModified();
				// we already tried to mark it dirty
				m_markAnimSetDirtyWhenFixing = false;
			}
			ReportError( String::Printf( TXT( "Recreated, fixed source buffer data - %ls" ), animName.AsChar() ) );
		}
	}
	else
	{
		ReportError( String::Printf( TXT( "Animation doesn't have anim buffer - %ls" ), animName.AsChar() ) );
	}

	animation->ReleaseUsage();
	
}

void CAnimationValidator::ReportError( const String& error )
{
	TDynArray< String >& errors = m_errors.GetRef( m_curentFilePath );
	errors.PushBackUnique( error );
}
