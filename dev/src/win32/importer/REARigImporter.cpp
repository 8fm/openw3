/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/importer.h"
#include "../../common/engine/skeleton.h"
#include "ReFileHelpers.h"

static const Float SCALE = 0.01f;

class CREARigImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CREARigImporter, IImporter, 0 );

public:
	CREARigImporter();
	virtual CResource*	DoImport( const ImportOptions& options );
	virtual Bool		PrepareForImport( const String&, ImportOptions& options ) override;
};

BEGIN_CLASS_RTTI( CREARigImporter )
	PARENT_CLASS(IImporter)
	END_CLASS_RTTI()
	IMPLEMENT_ENGINE_CLASS(CREARigImporter);

CREARigImporter::CREARigImporter()
{
	m_resourceClass = ClassID< CSkeleton >();
	m_formats.PushBack( CFileFormat( TXT("RE"), TXT("RE file") ) );
}

Bool CREARigImporter::PrepareForImport( const String& str, ImportOptions& options )
{
	return CReFileHelpers::ShouldImportFile( options );
}

CResource* CREARigImporter::DoImport( const ImportOptions& options )
{
	CSkeleton::FactoryInfo info;

	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CSkeleton >( options.m_existingResource );

	const char* path = UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() );
	ReFile reFile = ReFileLoader::OpenFile( path );
	if( !reFile )
	{
		return nullptr;
	}

	ReFileSkeleton* reSkeleton = new ReFileSkeleton();
	// no skeleton
	if ( !ReFileLoader::Read( reFile, *reSkeleton ) )
	{
		WARN_IMPORTER( TXT("File %s contains no skeleton chunks!"), options.m_sourceFilePath.AsChar() );
		delete reSkeleton;
		return nullptr;
	}

	Bool courrptData = false;

	// Skeleton
	{
		Int32 numBones = reSkeleton->getNumBones();
		Int32 numChannels = reSkeleton->getNumChannels();

		info.m_bones.Resize( numBones );
		info.m_tracks.Resize( numChannels );

		const Uint32 numFloats = numChannels;
		for( Uint32 i=0; i<numFloats; ++i )
		{
			AnsiChar trackName[32];
			sprintf_s(trackName, ARRAYSIZE(trackName), "Track%d", i);
			info.m_tracks[i].m_name = trackName;
		}

		for( Int32 i=0; i<numBones; ++i )
		{
			CSkeleton::FactoryInfo::BoneImportInfo& boneInfo = info.m_bones[i];
			{
				const int numChars = reSkeleton->mBonesNames[i].getLength();
				char* name = new char[numChars+1];
				Red::System::MemoryCopy( name, reSkeleton->mBonesNames[i].getData(), numChars );
				name[numChars]='\0';
				boneInfo.m_name = name;
				delete [] name;
			}

			boneInfo.m_lockTranslation = false;

			RED_ASSERT( reSkeleton->mBonesParents[i] <= MAXINT16, TXT("parentindex overflow while importing, skeleton will be broken") );
			if ( reSkeleton->mBonesParents[i] >= MAXINT16 )
			{
				courrptData = true;
			}

			boneInfo.m_parentIndex = static_cast<Uint16>( reSkeleton->mBonesParents[i] );

			if ( boneInfo.m_parentIndex == -1 )
			{
				boneInfo.m_referencePose = RedQsTransform::IDENTITY;
			}
			else
			{
				const qtransform_scale& tm = reSkeleton->mBonesTransforms[ i ];

				const RedVector4 translation( tm.pos.x * SCALE, tm.pos.y * SCALE, tm.pos.z * SCALE, 1.0f );
				const RedVector3 scale( tm.mScale.x, tm.mScale.y, tm.mScale.z );

				boneInfo.m_referencePose.Translation.Set( translation );
				boneInfo.m_referencePose.Rotation.Set( -tm.rot.x, -tm.rot.y, -tm.rot.z, tm.rot.w );
				boneInfo.m_referencePose.Scale.Set( scale, 1.0f );

				if ( !boneInfo.m_referencePose.Rotation.Quat.IsOk() )
				{
					courrptData = true;
				}

				if ( !boneInfo.m_referencePose.Translation.IsOk() )
				{
					courrptData = true;
				}

				if ( !boneInfo.m_referencePose.Scale.IsOk() )
				{
					courrptData = true;
				}
			}
		}

		CSkeleton* retVal = !courrptData ? CSkeleton::Create( info ) : nullptr;

		// cleanup
		delete reSkeleton;

		return retVal;
	}
}
