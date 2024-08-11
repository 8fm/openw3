
#include "build.h"
#include "mimicFac.h"

#include "../../common/core/gatheredResource.h"
#include "normalBlendComponent.h"

namespace
{
	CGatheredResource resCommonMimicFace( TXT("animations\\mimics\\common.w3fac"), RGF_Startup );
}

IMPLEMENT_ENGINE_CLASS( SMimicTrackPose );
IMPLEMENT_ENGINE_CLASS( CMimicFace );

//////////////////////////////////////////////////////////////////////////

CSkeleton* CMimicFace::GetSkeleton() const
{
	return m_mimicSkeleton.Get();
}

CSkeleton* CMimicFace::GetFloatTrackSkeleton() const
{
	return m_floatTrackSkeleton.Get();
}

Uint32 CMimicFace::GetMimicTrackPosesNum() const
{
	return m_mimicTrackPoses.Size();
}

void CMimicFace::GetNeckAndHead( Int32& neck, Int32& head ) const
{
	neck = m_neckIndex;
	head = m_headIndex;
}

void CMimicFace::GetNormalBlendAreasNormalized( TDynArray< Vector >& areas ) const
{
	// The face editor operates in 1k coordinates, rather than normalized to 0-1 (apparently requested by artists).
	// To provide a consistent view in the editor, we keep those same values, but we need to be able to get normalized
	// values for use by the normalblend shader.
	areas.Resize( m_normalBlendAreas.Size() );
	for ( Uint32 i = 0; i < m_normalBlendAreas.Size(); ++i )
	{
		areas[i] = m_normalBlendAreas[i] / 1024.0f;
	}
}

Int32 CMimicFace::FindMimicTrackPose( const CName& name ) const
{
	const Int32 size = m_mimicTrackPoses.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( m_mimicTrackPoses[ i ].m_name == name )
		{
			return i;
		}
	}
	return -1;
}

Uint32 CMimicFace::GetMimicFilterPosesNum() const
{
	return m_mimicFilterPoses.Size();
}

Int32 CMimicFace::FindMimicFilterPose( const CName& name ) const
{
	const Int32 size = m_mimicFilterPoses.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( m_mimicFilterPoses[ i ].m_name == name )
		{
			return i;
		}
	}
	return -1;
}

Bool CMimicFace::AddTrackPose( Int32 poseNum, Float weight, SBehaviorGraphOutput& output ) const
{
	if ( poseNum < 0 || m_mimicTrackPoses.SizeInt() <= poseNum )
	{
		return false;
	}

	const SMimicTrackPose& input = m_mimicTrackPoses[ poseNum ];

	const Uint32 iSize = input.m_tracks.Size();
	if ( iSize == 0 )
	{
		return true;
	}

	if ( !input.m_mapping.Empty() )
	{
		const Int32 floatNum = (Int32)output.m_numFloatTracks;

		for ( Uint32 i=0; i<iSize; ++i )
		{
			const Float val = input.m_tracks[ i ];
			const Int32 index = input.m_mapping[ i ];

			ASSERT( index < floatNum );

			if ( index < floatNum )
			{
				output.m_floatTracks[ index ] += weight * val;
			}
		}
	}
	else
	{
		Uint32 size = Min< Uint32 >( iSize, output.m_numFloatTracks );
		for ( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[ i ] += weight * input.m_tracks[ i ];
		}
	}

	return true;
}

Bool CMimicFace::AddFilterPose( Int32 poseNum, Float weight, SBehaviorGraphOutput& output ) const
{
	if ( poseNum < 0 || m_mimicFilterPoses.SizeInt() <= poseNum )
	{
		return false;
	}

	const SMimicTrackPose& input = m_mimicFilterPoses[ poseNum ];

	const Uint32 iSize = input.m_tracks.Size();
	if ( iSize == 0 )
	{
		return true;
	}

	if ( !input.m_mapping.Empty() )
	{
		const Int32 floatNum = (Int32)output.m_numFloatTracks;

		for ( Uint32 i=0; i<iSize; ++i )
		{
			const Float val = input.m_tracks[ i ];
			const Int32 index = input.m_mapping[ i ];

			ASSERT( index < floatNum );

			if ( index < floatNum )
			{
				output.m_floatTracks[ index ] *= weight * val;
			}
		}
	}
	else
	{
		Uint32 size = Min< Uint32 >( iSize, output.m_numFloatTracks );
		for ( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[ i ] *= weight * input.m_tracks[ i ];
		}
	}

	return true;
}

const CName& CMimicFace::GetMimicTrackPoseName( const Int32 poseIndex ) const
{
	if ( poseIndex != -1 && poseIndex < m_mimicTrackPoses.SizeInt() )
	{
		return m_mimicTrackPoses[ poseIndex ].m_name;
	}
	return CName::NONE;
}

const CName& CMimicFace::GetMimicTrackFilterName( const Int32 filterIndex ) const
{
	if ( filterIndex != -1 && filterIndex < m_mimicFilterPoses.SizeInt() )
	{
		return m_mimicFilterPoses[ filterIndex ].m_name;
	}
	return CName::NONE;
}

Bool CMimicFace::GetMimicPose( Uint32 num, SBehaviorGraphOutput& pose ) const
{
	if ( m_mimicPoses.Size() > num && pose.m_eventsFired == NULL && pose.m_numBones == 0 && pose.m_numFloatTracks == 0 )
	{
		const TMimicPose& mimicPose = m_mimicPoses[ num ];
		const Uint32 bonesNum = mimicPose.Size();

		EngineQsTransform* eTrans = const_cast< EngineQsTransform* >( mimicPose.TypedData() );

		SBehaviorGraphOutputParameter param =
		{
			bonesNum, 
			0,
			reinterpret_cast< AnimQsTransform* >( eTrans ),
			nullptr,
			nullptr,
			false
		};

		pose.Init( param );
		return true;
	}
	else
	{
		ASSERT( pose.m_numBones == 0 );
		ASSERT( pose.m_numFloatTracks == 0 );
		ASSERT( pose.m_eventsFired == NULL );
	}

	return false;
}

void CMimicFace::AddMimicPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& additivePose, Float weight ) const
{
	ASSERT( m_mapping.Size() == additivePose.m_numBones );

	const Uint32 size = Min( m_mapping.Size(), additivePose.m_numBones );

	const AnimFloat simdWeight = weight;
	AnimQsTransform temp;

	for ( Uint32 i=0; i<size; ++i )
	{
		const Uint32 index = m_mapping[ i ];
		if ( index < mainPose.m_numBones )
		{
#ifdef USE_HAVOK_ANIMATION // VALID
			temp.setMul( additivePose.m_outputPose[i], mainPose.m_outputPose[index] );
			mainPose.m_outputPose[index].setInterpolate4( mainPose.m_outputPose[index], temp, simdWeight );
#else
			//temp.SetMul( additivePose.m_outputPose[i], mainPose.m_outputPose[index] );
			//mainPose.m_outputPose[index].Lerp( mainPose.m_outputPose[index], temp, simdWeight );

			temp.Lerp( RedQsTransform::IDENTITY, additivePose.m_outputPose[i], weight );
			mainPose.m_outputPose[index].SetMul( mainPose.m_outputPose[index], temp );
#endif
		}
	}
}

#ifndef NO_RESOURCE_IMPORT

CMimicFace* CMimicFace::Create( const FactoryInfo& data )
{
	CMimicFace* face = data.CreateResource();
	if ( face )
	{
		// Mimic skeleton
		{
			CSkeleton::FactoryInfo skeletonInfo;
			skeletonInfo.m_parent = face;
			skeletonInfo.m_reuse = data.m_reuse ? data.m_reuse->m_mimicSkeleton.Get() : NULL;
			skeletonInfo.m_bones = data.m_mimicSkeleton.m_bones;
			skeletonInfo.m_tracks = data.m_mimicSkeleton.m_tracks;

			CSkeleton* skeleton = CSkeleton::Create( skeletonInfo );
			if ( skeleton == NULL )
			{
				return NULL;
			}

			face->m_mimicSkeleton = skeleton;
		}

		// Float track skeleton
		{
			CSkeleton::FactoryInfo skeletonInfo;
			skeletonInfo.m_parent = face;
			skeletonInfo.m_reuse = data.m_reuse ? data.m_reuse->m_floatTrackSkeleton.Get() : NULL;
			skeletonInfo.m_bones = data.m_floatTrackSkeleton.m_bones;
			skeletonInfo.m_tracks = data.m_floatTrackSkeleton.m_tracks;

			CSkeleton* skeleton = CSkeleton::Create( skeletonInfo );
			if ( skeleton == NULL )
			{
				return NULL;
			}

			face->m_floatTrackSkeleton = skeleton;
		}

		// Track poses and filters
		{
			face->m_mimicTrackPoses = data.m_mimicTrackPoses;
			face->m_mimicFilterPoses = data.m_mimicFilterPoses;
		}
		
		// Areas
		if ( data.m_areas != NULL )
		{
			face->m_normalBlendAreas.Resize( NUM_NORMALBLEND_AREAS );
			for ( Uint32 i=0; i<NUM_NORMALBLEND_AREAS; ++i )
			{
				face->m_normalBlendAreas[i] = data.m_areas[i];
			}

			delete [] data.m_areas;
		}
		

		// Bone index
		if ( face->m_mimicSkeleton )
		{
			face->m_neckIndex = face->m_mimicSkeleton->FindBoneByName( TXT("neck") );
			face->m_headIndex = face->m_mimicSkeleton->FindBoneByName( TXT("head") );
		}
		// Mapper
		{
			face->m_mapping = data.m_mapper;
		}

		// Mimic poses
		if( data.m_poses )
		{
			face->m_mimicPoses.Resize( data.m_numPoses );

			for ( Uint32 i=0; i<face->m_mimicPoses.Size(); ++i )
			{
				TMimicPose&	mimicPose = face->m_mimicPoses[ i ];
				mimicPose.Resize( data.m_numBones );

				for ( Uint32 j=0; j<mimicPose.Size(); ++j )
				{
					const AnimQsTransform& srcTrans = data.m_poses[j+(data.m_numBones*i)];

#ifdef USE_HAVOK_ANIMATION // VALID
					HkToEngineQsTransform( srcTrans, mimicPose[ j ] );
#else
					mimicPose[ j ].SetPosition( Vector( srcTrans.Translation.X, srcTrans.Translation.Y, srcTrans.Translation.Z, 1.0f ) );
					mimicPose[ j ].SetRotation( Vector( srcTrans.Rotation.Quat.X, srcTrans.Rotation.Quat.Y, srcTrans.Rotation.Quat.Z, srcTrans.Rotation.Quat.W ) );
					mimicPose[ j ].SetScale( Vector( srcTrans.Scale.X, srcTrans.Scale.Y, srcTrans.Scale.Z, 1.0f ) );
#endif
				}
			}

			delete [] data.m_poses;
		}
	}

	return face;
}

#endif

/*
Ctor.

\param customMimics CMimicFace* to use as custom mimics. Must not be nullptr. Ownership is not transferred.
\param categoryMimics CMimicFace* to use as category mimics. May be nullptr. Onwership is not transferred.
*/
CExtendedMimics::CExtendedMimics( CMimicFace* customMimics, CMimicFace* categoryMimics )
: m_customMimics(customMimics)
, m_categoryMimics(categoryMimics)
, m_commonMimics(nullptr)
{
	ASSERT( m_customMimics );

	// note that common mimics will actually be loaded only the first time any CExtendedMimics does this
	Bool resLoaded = resCommonMimicFace.Load();
	if( resLoaded )
	{
		m_commonMimics = SafeCast< CMimicFace >( resCommonMimicFace.GetResource() );
	}
}

/*
Dtor.
*/
CExtendedMimics::~CExtendedMimics()
{}

/*
Returns number of track poses.

\return Number of track poses in custom, category and common mimics.
*/
Uint32 CExtendedMimics::GetNumTrackPoses() const
{
	Uint32 numCustomPoses = m_customMimics->GetMimicTrackPosesNum();
	Uint32 numCategoryPoses = m_categoryMimics? m_categoryMimics->GetMimicTrackPosesNum() : 0;
	Uint32 numCommonPoses = m_commonMimics? m_commonMimics->GetMimicTrackPosesNum() : 0;
	
	return numCustomPoses + numCategoryPoses + numCommonPoses;
}

/*
Finds track pose with given name and returns its index.

\param name Name of track pose to find.
\return Index of a track pose with specified name. -1 - no track pose with specified name found.

Note that custom poses override category poses with the same name. Likewise, category poses override
common poses with the same name.
*/
Int32 CExtendedMimics::FindTrackPose( const CName& name ) const
{
	const Uint32 numMimics = 3;
	const CMimicFace* mimics[ numMimics ] = { m_customMimics, m_categoryMimics, m_commonMimics };

	Uint32 baseIndex = 0;
	for( Uint32 i = 0; i < numMimics; ++i )
	{
		if( mimics[ i ] )
		{
			Int32 localIndex = mimics[ i ]->FindMimicTrackPose( name );
			if( localIndex != -1 )
			{
				// pose found - return its index
				return baseIndex + localIndex;
			}
			else
			{
				// pose not found in current mimics - move base index
				baseIndex += mimics[ i ]->GetMimicTrackPosesNum();
			}
		}
	}

	// pose not found
	return -1;
}

/*
Returns name of track pose at specified index.

\param trackPoseIndex Index of track pose whose name to get.
\return Name of track pose at specified index or CName::NONE if index is out of range.
*/
const CName& CExtendedMimics::GetTrackPoseName( const Int32 trackPoseIndex ) const
{
	ASSERT( trackPoseIndex != -1 );
	if ( trackPoseIndex == -1 )
	{
		return CName::NONE;
	}

	const CMimicFace* mimicsToUse = 0;
	Int32 indexToUse = -1;
	TranslateTrackPoseIndex( trackPoseIndex, indexToUse, mimicsToUse );
	return mimicsToUse->GetMimicTrackPoseName( indexToUse );
}

/*
Returns number of filter poses.

\return Number of filter poses in custom, category and common mimics.
*/
Uint32 CExtendedMimics::GetNumFilterPoses() const
{
	Uint32 numCustomPoses = m_customMimics->GetMimicFilterPosesNum();
	Uint32 numCategoryPoses = m_categoryMimics? m_categoryMimics->GetMimicFilterPosesNum() : 0;
	Uint32 numCommonPoses = m_commonMimics? m_commonMimics->GetMimicFilterPosesNum() : 0;

	return numCustomPoses + numCategoryPoses + numCommonPoses;
}

/*
Finds filter pose with given name and returns its index.

\param name Name of filter pose to find.
\return Index of a filter pose with specified name. -1 - no filter pose with specified name found.

Note that custom poses override category poses with the same name. Likewise, category poses override
common poses with the same name.
*/
Int32 CExtendedMimics::FindFilterPose( const CName& name ) const
{
	const Uint32 numMimics = 3;
	const CMimicFace* mimics[ numMimics ] = { m_customMimics, m_categoryMimics, m_commonMimics };

	Uint32 baseIndex = 0;
	for( Uint32 i = 0; i < numMimics; ++i )
	{
		if( mimics[ i ] )
		{
			Int32 localIndex = mimics[ i ]->FindMimicFilterPose( name );
			if( localIndex != -1 )
			{
				// pose found - return its index
				return baseIndex + localIndex;
			}
			else
			{
				// pose not found in current mimics - move base index
				baseIndex += mimics[ i ]->GetMimicFilterPosesNum();
			}
		}
	}

	// pose not found
	return -1;
}

/*
Returns name of filter pose at specified index.

\param filterPoseIndex Index of filter pose whose name to get.
\return Name of filter pose at specified index or CName::NONE if index is out of range.
*/
const CName& CExtendedMimics::GetFilterPoseName( const Int32 filterPoseIndex ) const
{
	ASSERT( filterPoseIndex != -1 );
	if ( filterPoseIndex == -1 )
	{
		return CName::NONE;
	}

	const CMimicFace* mimicsToUse = 0;
	Int32 indexToUse = -1;
	TranslateFilterPoseIndex( filterPoseIndex, indexToUse, mimicsToUse );
	return mimicsToUse->GetMimicTrackFilterName( indexToUse );
}

/*
Applies track pose with specified weight to specified output.
*/
Bool CExtendedMimics::ApplyTrackPose( Int32 trackPoseIndex, Float weight, SBehaviorGraphOutput& output ) const
{
	const CMimicFace* mimicsToUse = 0;
	Int32 indexToUse = -1;
	TranslateTrackPoseIndex( trackPoseIndex, indexToUse, mimicsToUse );
	return mimicsToUse->AddTrackPose( indexToUse, weight, output );
}

/*
Applies filter pose with specified weight to specified output.
*/
Bool CExtendedMimics::ApplyFilterPose( Int32 filterPoseIndex, Float weight, SBehaviorGraphOutput& output ) const
{
	const CMimicFace* mimicsToUse = 0;
	Int32 indexToUse = -1;
	TranslateFilterPoseIndex( filterPoseIndex, indexToUse, mimicsToUse );
	return mimicsToUse->AddFilterPose( indexToUse, weight, output );
}

/*
Finds out which mimics (custom, category or common) should be used for specified index and also computes index to use with chosen mimics.

\param trackPoseIndex Index of a track pose to translate.
\param outIndex (out) Index to use with outMimics. It is set to -1 if trackPoseIndex is out of range.
\param outMimics (out) CMimicFace to use. It is set to nullptr if trackPoseIndex is out of range.
*/
void CExtendedMimics::TranslateTrackPoseIndex( Int32 trackPoseIndex, Int32& outIndex, const CMimicFace*& outMimics ) const
{
	Int32 numCustomPoses = static_cast< Int32 >( m_customMimics->GetMimicTrackPosesNum() );
	Int32 numCategoryPoses = m_categoryMimics? static_cast< Int32 >( m_categoryMimics->GetMimicTrackPosesNum() ) : 0;
	Int32 numCommonPoses = m_commonMimics? static_cast< Int32 >( m_commonMimics->GetMimicTrackPosesNum() ) : 0;

	if( trackPoseIndex < numCustomPoses )
	{
		outIndex = trackPoseIndex;
		outMimics = m_customMimics;
	}
	else if( trackPoseIndex < numCustomPoses + numCategoryPoses )
	{
		outIndex = trackPoseIndex - numCustomPoses;
		outMimics = m_categoryMimics;
	}
	else if( trackPoseIndex < numCustomPoses + numCategoryPoses + numCommonPoses )
	{
		outIndex = trackPoseIndex - numCustomPoses - numCategoryPoses;
		outMimics = m_commonMimics;
	}
	else
	{
		outIndex = -1;
		outMimics = nullptr;
	}
}

/*
Finds out which mimics (custom, category or common) should be used for specified index and also computes index to use with chosen mimics.

\param filterPoseIndex Index of a filter pose to translate.
\param outIndex (out) Index to use with outMimics. It is set to -1 if trackPoseIndex is out of range.
\param outMimics (out) CMimicFace to use. It is set to nullptr if trackPoseIndex is out of range.
*/
void CExtendedMimics::TranslateFilterPoseIndex( Int32 filterPoseIndex, Int32& outIndex, const CMimicFace*& outMimics ) const
{
	Int32 numCustomPoses = static_cast< Int32 >( m_customMimics->GetMimicFilterPosesNum() );
	Int32 numCategoryPoses = m_categoryMimics? static_cast< Int32 >( m_categoryMimics->GetMimicFilterPosesNum() ) : 0;
	Int32 numCommonPoses = m_commonMimics? static_cast< Int32 >( m_commonMimics->GetMimicFilterPosesNum() ) : 0;

	if( filterPoseIndex < numCustomPoses )
	{
		outIndex = filterPoseIndex;
		outMimics = m_customMimics;
	}
	else if( filterPoseIndex < numCustomPoses + numCategoryPoses )
	{
		outIndex = filterPoseIndex - numCustomPoses;
		outMimics = m_categoryMimics;
	}
	else if( filterPoseIndex < numCustomPoses + numCategoryPoses + numCommonPoses )
	{
		outIndex = filterPoseIndex - numCustomPoses - numCategoryPoses;
		outMimics = m_commonMimics;
	}
	else
	{
		outIndex = -1;
		outMimics = nullptr;
	}
}
