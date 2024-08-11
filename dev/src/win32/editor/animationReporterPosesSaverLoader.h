
#pragma once

class CEdCompressedPosesSaver
{
public:
	static Bool Save( const String& animsetPath, CSkeletalAnimationSet::SCompressedPosesData& data )
	{
		// Path
		String path = animsetPath.StringBefore( TXT("."), true ) + TXT(".cposes");

		// Save
		IFile* f = GFileManager->CreateFileWriter( path, FOF_AbsolutePath );
		if ( f )
		{
			IFile& file = *f;

			// Info size
			Uint32 infoSize = data.m_infos.Size();
			file << infoSize;

			// Infos
			for ( Uint32 i=0; i<infoSize; ++i )
			{
				SCompressedPoseInfo& info = data.m_infos[ i ];
				
				file << info.m_animation;
				file << info.m_name;
				file << info.m_time;
			}

			// Anim size
			Uint32 animSize = data.m_poses.Size();
			file << animSize;

			// Poses
			for ( Uint32 i=0; i<animSize; ++i )
			{
				CName& animName = data.m_poses[ i ].m_first;
				CName& poseName = data.m_poses[ i ].m_second;

				file << animName;
				file << poseName;
			}

			delete f;

			return true;
		}

		return false;
	}
};

class CEdCompressedPosesLoader
{
public:
	static Bool Load( const String& animsetPath, CSkeletalAnimationSet::SCompressedPosesData& data )
	{
		// Path
		String path = animsetPath.StringBefore( TXT("."), true ) + TXT(".cposes");

		// Load
		IFile* f = GFileManager->CreateFileReader( path, FOF_AbsolutePath | FOF_Buffered );
		if ( f )
		{
			IFile& file = *f;

			// Info size
			Uint32 infoSize = 0;
			file << infoSize;
			data.m_infos.Resize( infoSize );

			// Infos
			for ( Uint32 i=0; i<infoSize; ++i )
			{
				SCompressedPoseInfo& info = data.m_infos[ i ];

				file << info.m_animation;
				file << info.m_name;
				file << info.m_time;
			}

			// Anim size
			Uint32 animSize = 0;
			file << animSize;
			data.m_poses.Resize( animSize );

			// Poses
			for ( Uint32 i=0; i<animSize; ++i )
			{
				CName& animName = data.m_poses[ i ].m_first;
				CName& poseName = data.m_poses[ i ].m_second;

				file << animName;
				file << poseName;
			}

			delete f;

			return true;
		}

		return false;
	}
};
