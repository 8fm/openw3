/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "FACFormat.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/animMath.h"
#include "../../common/engine/mimicFac.h"
#include "../../common/core/importer.h"
#include "ReFileHelpers.h"

static const Float SCALE = 0.01f;

class CFACMimicFaceImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CFACMimicFaceImporter, IImporter, 0 );

public:
	CFACMimicFaceImporter();
	virtual CResource*	DoImport( const ImportOptions& options );
	virtual Bool		PrepareForImport( const String& /*filePath*/, ImportOptions& options ) override;
};

BEGIN_CLASS_RTTI( CFACMimicFaceImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFACMimicFaceImporter );

CFACMimicFaceImporter::CFACMimicFaceImporter()
{
	m_resourceClass = ClassID< CMimicFace >();
	m_formats.PushBack( CFileFormat( TXT("re"), TXT("RE File") ) );
}

namespace
{
	void CreateSkeleton( ReFileSkeleton& reSkeleton, CSkeleton::FactoryInfo& outInfo )
	{
		Int32 numBones = reSkeleton.getNumBones();
		Int32 numChannels = reSkeleton.getNumChannels();

		outInfo.m_bones.Resize(numBones);
		outInfo.m_tracks.Resize(numChannels);

		for (Int32 i=0; i<numChannels; ++i )
		{
			const int numChars = reSkeleton.mBonesChannels[i].getLength();
			char* name = new char[numChars+1];
			Red::System::MemoryCopy( name, reSkeleton.mBonesNames[i].getData(), numChars );
			name[numChars] = '\0';
			outInfo.m_tracks[i].m_name = name;
			delete [] name;
		}

		for (Int32 i=0; i<numBones; ++i )
		{
			CSkeleton::FactoryInfo::BoneImportInfo& boneInfo = outInfo.m_bones[i];

			{
				const int numChars = reSkeleton.mBonesNames[i].getLength();
				char* name = new char[numChars+1];
				Red::System::MemoryCopy( name, reSkeleton.mBonesNames[i].getData(), numChars );
				name[numChars] = '\0';
				boneInfo.m_name = name;
				delete [] name;
			}

			boneInfo.m_lockTranslation = false;

			RED_ASSERT( reSkeleton.mBonesParents[i] <= MAXINT16, TXT("parentindex overflow while importing, skeleton will be broken") );

			boneInfo.m_parentIndex = static_cast<Uint16>( reSkeleton.mBonesParents[i] );

			if (boneInfo.m_parentIndex == -1)
			{
				boneInfo.m_referencePose = RedQsTransform::IDENTITY;
			}
			else
			{
				const qtransform_scale& ptrans = reSkeleton.mBonesTransforms[i];
				const RedVector4 translation( ptrans.pos.x * SCALE, ptrans.pos.y * SCALE, ptrans.pos.z * SCALE, 1.0f );
				const RedVector3 scale( ptrans.mScale.x, ptrans.mScale.y, ptrans.mScale.z );

				boneInfo.m_referencePose.Translation.Set( translation );
				boneInfo.m_referencePose.Rotation.Set( -ptrans.rot.x, -ptrans.rot.y, -ptrans.rot.z, ptrans.rot.w );
				boneInfo.m_referencePose.Scale.Set( scale, 1.0f );
			}
		}
	}

	void CreateFloatTrackSkeleton( const ReFileMixer& reMixer, CSkeleton::FactoryInfo& outInfo )
	{
		const int extraTracks = 16 + 3; // + normal blend tracks and head tracks...3
		Int32 numKeys = reMixer.getNumKeys();

		outInfo.m_bones.Resize(2); // why 2 ??
		outInfo.m_tracks.Resize( numKeys + extraTracks);

		for (Int32 i=0; i < numKeys ; ++i)
		{
			const int numChars = reMixer.mNames[i].getLength();
			char* name = new char[numChars+1];
			Red::System::MemoryCopy( name, reMixer.mNames[i].getData(), numChars );
			name[ numChars ] = 0;
			outInfo.m_tracks[i].m_name = name;
			delete [] name;
		}

		outInfo.m_tracks[numKeys + 0].m_name = "normal0";
		outInfo.m_tracks[numKeys + 1].m_name = "normal1";
		outInfo.m_tracks[numKeys + 2].m_name = "normal2";
		outInfo.m_tracks[numKeys + 3].m_name = "normal3";
		outInfo.m_tracks[numKeys + 4].m_name = "normal4";
		outInfo.m_tracks[numKeys + 5].m_name = "normal5";
		outInfo.m_tracks[numKeys + 6].m_name = "normal6";
		outInfo.m_tracks[numKeys + 7].m_name = "normal7";
		outInfo.m_tracks[numKeys + 8].m_name = "normal8";
		outInfo.m_tracks[numKeys + 9].m_name = "normal9";
		outInfo.m_tracks[numKeys + 10].m_name = "normal10";
		outInfo.m_tracks[numKeys + 11].m_name = "normal11";
		outInfo.m_tracks[numKeys + 12].m_name = "normal12";
		outInfo.m_tracks[numKeys + 13].m_name = "normal13";
		outInfo.m_tracks[numKeys + 14].m_name = "normal14";
		outInfo.m_tracks[numKeys + 15].m_name = "normal15";

		outInfo.m_tracks[numKeys + 16].m_name = "head1";
		outInfo.m_tracks[numKeys + 17].m_name = "head2";
		outInfo.m_tracks[numKeys + 18].m_name = "head3";

		outInfo.m_bones[0].m_name = "neck";
		outInfo.m_bones[0].m_lockTranslation = false;
		outInfo.m_bones[0].m_parentIndex = -1;
		outInfo.m_bones[0].m_referencePose = RedQsTransform::IDENTITY;

		outInfo.m_bones[1].m_name = "head";
		outInfo.m_bones[1].m_lockTranslation = false;
		outInfo.m_bones[1].m_parentIndex = 0;
		outInfo.m_bones[1].m_referencePose = RedQsTransform::IDENTITY;
	}

	void CreateMimicPoses( const ReFileMixer& reMixer, CMimicFace::FactoryInfo& info )
	{
		const int numKeys = reMixer.getNumKeys();
		const int numBonesMixer = reMixer.getNumBones();

		info.m_mapper.Resize( numBonesMixer );

		for( int i=0; i<numBonesMixer; i++)
		{
			info.m_mapper[i] = reMixer.mMappings[i];
		}

		const int numBonesInfo = numBonesMixer;

		info.m_poses = new AnimQsTransform[ numKeys*numBonesInfo ];
		info.m_numBones = numBonesInfo;
		info.m_numPoses = numKeys;

		for( int k=0; k<numKeys; ++k )
		{
			for( int b=0; b<numBonesMixer; ++b )
			{
				const int index = b+(numBonesMixer*k);

				const float* ptrans = reMixer.mKeys + index * 7;

				AnimQsTransform delta;
				delta.SetTranslation( AnimVector4( ptrans[0] * SCALE, ptrans[1] * SCALE, ptrans[2] * SCALE, 1.0f ) );
				delta.SetRotation( AnimQuaternion(-ptrans[3],-ptrans[4],-ptrans[5],ptrans[6]) );
				delta.SetScale( AnimVector4( 1.0f,1.0f,1.0f,1.0f ) );
				info.m_poses[index] = delta;
			}
		}
	}

	void CreateTrackPoses( const TDynArray< ReFilePose >& trackPoses, Float defaultValue, CMimicFace::FactoryInfo& info )
	{
		static const Float THRES = 0.01f;

		TDynArray< SMimicTrackPose >& poses = info.m_mimicTrackPoses;

		for ( Uint32 i = 0; i < trackPoses.Size(); ++i )
		{
			const ReFilePose& trackPose = trackPoses[ i ];
			Int32 numChannels = trackPose.getNumChannels();

			const CName poseName = CName( ANSI_TO_UNICODE( trackPose.getPoseName().getData() ) );

			const Uint32 poseIndex = static_cast< Uint32 >( poses.Grow( 1 ) );

			SMimicTrackPose& outputPose = poses[ poseIndex ];
			outputPose.m_name = poseName;

			Bool allSet = true;
			for ( Int32 t=0; t<numChannels; ++t )
			{
				if ( MAbs( trackPose.mPoseKeys[ t ] - defaultValue ) > THRES )
				{
					allSet = false;
					break;
				}
			}

			if ( allSet )
			{
				outputPose.m_mapping.Clear();
				outputPose.m_tracks.Reserve( numChannels );

				for ( Int32 t=0; t<numChannels; ++t )
				{
					outputPose.m_tracks.PushBack( trackPose.mPoseKeys[ t ] );
				}
			}
			else
			{
				for ( Int32 t=0; t<numChannels; ++t )
				{
					const Float w = trackPose.mPoseKeys[ t ];

					if ( MAbs( w - defaultValue ) > THRES )
					{
						outputPose.m_mapping.PushBack( t );
						outputPose.m_tracks.PushBack( w );
					}
				}
			}
		}
	}

	void CreateTrackFilters( const TDynArray< ReFileFilter >& reTrackFilters, Float defaultValue, CMimicFace::FactoryInfo& info )
	{
		static const Float THRES = 0.01f;

		TDynArray< SMimicTrackPose >& poses = info.m_mimicFilterPoses;

		for ( Uint32 i = 0; i < reTrackFilters.Size(); ++i )
		{
			const ReFileFilter& reTrackFilter = reTrackFilters[ i ];
			Int32 numChannels = reTrackFilter.getNumChannels();

			const CName poseName = CName( ANSI_TO_UNICODE( reTrackFilter.getFilterName().getData() ) );
			const Uint32 poseIndex = static_cast< Uint32 >( poses.Grow( 1 ) );

			SMimicTrackPose& outputPose = poses[ poseIndex ];
			outputPose.m_name = poseName;

			Bool allSet = true;
			for ( Int32 t=0; t<numChannels; ++t )
			{
				if ( MAbs( reTrackFilter.mFilterKeys[ t ] - defaultValue ) > THRES )
				{
					allSet = false;
					break;
				}
			}

			if ( allSet )
			{
				outputPose.m_mapping.Clear();
				outputPose.m_tracks.Reserve( numChannels );

				for ( Int32 t=0; t<numChannels; ++t )
				{
					outputPose.m_tracks.PushBack( reTrackFilter.mFilterKeys[ t ] );
				}
			}
			else
			{
				for ( Int32 t=0; t<numChannels; ++t )
				{
					const Float w = reTrackFilter.mFilterKeys[ t ];

					if ( MAbs( w - defaultValue ) > THRES )
					{
						outputPose.m_mapping.PushBack( t );
						outputPose.m_tracks.PushBack( w );
					}
				}
			}
		}
	}

	void CreateMimicAreas( const TDynArray< ReFileArea >& reAreas, CMimicFace::FactoryInfo& info )
	{
		info.m_areas = new Vector[ reAreas.Size() ];

		for ( Uint32 i=0; i<reAreas.Size(); ++i )
		{
			const ReFileArea& reArea = reAreas[ i ];
			info.m_areas[ i ] = Vector( reArea.mAreas[ 0 ], reArea.mAreas[ 1 ], reArea.mAreas[ 2 ], reArea.mAreas[ 3 ] );
		}
	}
}

CResource* CFACMimicFaceImporter::DoImport( const ImportOptions& options )
{
	// create factory info
	CMimicFace::FactoryInfo info;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CMimicFace >( options.m_existingResource );

	const char* path = UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() );
	ReFile reFile = ReFileLoader::OpenFile( path );
	if ( !reFile )
	{
		return false;
	}

	ReFileHeader2 hdr;
	if( !ReFileLoader::Read( reFile, hdr ) ) 
	{
		return false;
	}
	const ReFileString* currentVersionPtr = &hdr.getReFileVersion();

	// Mimic requires few nodes
	ReFileSkeleton	reSkeleton;
	ReFileMixer		reMixer;

	TDynArray< ReFilePose > rePoses;
	TDynArray< ReFileFilter > reFilters;
	TDynArray< ReFileArea > reAreas;

	for ( int i = 0; i < (int)reFile.mHeaders.size(); ++i )
	{
		const ReFileArchiveHeader& header = reFile.mHeaders[i];
		if( header.mType == 'skel' )
		{
			ReFileLoader::Read( reFile.mFile, header, reSkeleton, currentVersionPtr );
		}
		else if( header.mType == 'pose' )
		{
			rePoses.Grow();
			ReFileLoader::Read( reFile.mFile, header, rePoses.Back(), currentVersionPtr );
		}
		else if( header.mType == 'fltr' )
		{
			reFilters.Grow();
			ReFileLoader::Read( reFile.mFile, header, reFilters.Back(), currentVersionPtr );
		}
		else if( header.mType == 'mixr' )
		{
			ReFileLoader::Read( reFile.mFile, header, reMixer, currentVersionPtr );
		}
		else if( header.mType == 'area' )
		{
			reAreas.Grow();
			ReFileLoader::Read( reFile.mFile, header, reAreas.Back(), currentVersionPtr );
		}
	}

	// check
	if( reSkeleton.getNumBones() > 0 )
	{
		CreateSkeleton( reSkeleton, info.m_mimicSkeleton );
	}
	else
	{
		GFeedback->ShowWarn( TXT("File doesn't have skeleton.") );
	}

	if( reMixer.getNumBones() > 0 )
	{
		// check
		CreateMimicPoses( reMixer, info );
		CreateFloatTrackSkeleton( reMixer, info.m_floatTrackSkeleton );
	}
	else
	{
		GFeedback->ShowWarn( TXT("File doesn't have mixer.") );
	}

	// dyn arr
	CreateTrackPoses( rePoses, 0.f, info );
	CreateTrackFilters( reFilters, 1.f, info );
	CreateMimicAreas( reAreas, info );

	// Create object
	CMimicFace* retVal = CMimicFace::Create( info );

	// Set import file
	if( retVal != nullptr )
	{
		retVal->SetImportFile( options.m_sourceFilePath );
	}

	return retVal;
}

Bool CFACMimicFaceImporter::PrepareForImport(const String& str, ImportOptions& options)
{
	return CReFileHelpers::ShouldImportFile( options );
}
