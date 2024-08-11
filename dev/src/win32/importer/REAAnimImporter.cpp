/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "REAAnimImporter.h"
#include "../../common/engine/havokAnimationUtils.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/animationCompression.h"
#include "ReFileHelpers.h"

static const Float SCALE = 0.01f;

IMPLEMENT_ENGINE_CLASS(CREAAnimImporter);

CREAAnimImporter::CREAAnimImporter()
{
	m_formats.PushBack( CFileFormat( TXT("re"), TXT("Red Engine Animation") ) );
}

Bool CREAAnimImporter::PrepareForImport( const String& str, AnimImporterParams& animOptions )
{
	// #TODO #LH anim importer doesnt derive from IImporter.
	// we have to fix and unify it asap to have one data flow with limited possible bad paths.
	IImporter::ImportOptions options;
	options.m_sourceFilePath = str;
	bool result = CReFileHelpers::ShouldImportFile( options );
	animOptions.m_errorCode = options.m_errorCode;
	return result;
}

CSkeletalAnimation* CREAAnimImporter::DoImport( const AnimImporterParams& options )
{
	// create factory info
	CSkeletalAnimation::FactoryInfo info;
	info.m_animationSet = options.m_animationSet.Get();
	info.m_animationEntry = options.m_existingAnimationEntry;
	info.m_preferBetterQuality = options.m_preferBetterQuality;

	FILE* f = _wfopen( options.m_filePath.AsChar(),TXT("rb") );
	if ( !f )
	{
		return NULL;
	}

	Int32 numElements = 0;
	fread( &numElements, sizeof( int ), 1, f );
	if ( !numElements )
	{
		WARN_IMPORTER( TXT("No animations in file %s!"), options.m_filePath.AsChar() );
		fclose(f);
		return NULL;
	}

	String author = String::EMPTY;
	String source = String::EMPTY;
	String baseResourceFilePath = String::EMPTY;

	ReFileArchiveHeader* header = new ReFileArchiveHeader[numElements];
	fread( header, sizeof( ReFileArchiveHeader ) * numElements, 1, f);

	Int32 numSkeletons = 0;
	Int32 numAnimations = 0;
	Int32 numCurves = 0;
	Int32 numTrajectories = 0;

	for(Int32 i=0;i<numElements;i++)
	{
		if( header[i].mType =='skel' ){numSkeletons++;}
		if( header[i].mType =='anim' ){numAnimations++;}
		if( header[i].mType =='curv' ){numCurves++;}
		if( header[i].mType =='traj' ){numTrajectories++;}
	}

	// no animations
	if (numAnimations == 0)
	{
		WARN_IMPORTER( TXT("File %s contains no animation chunks!"), options.m_filePath.AsChar() );
		fclose(f);
		return NULL;
	}

	ReFileSkeleton* skeletons = new ReFileSkeleton[numSkeletons];
	ReFileAnimation* animations = new ReFileAnimation[numAnimations];
	ReFileCurve* curves = new ReFileCurve[numCurves];
	ReFileTrajectory* trajectories = new ReFileTrajectory[numTrajectories];

	Int32 skelIndex = 0;
	Int32 animIndex = 0;
	Int32 curveIndex = 0;
	Int32 trajIndex = 0;
	Int32 headerIndex = 0;
	ReFileString currentVersion("001");

	for(Int32 i=0;i<numElements;i++)
	{
		if( header[i].mType =='hed2' )
		{
			ReFileHeader2 hdr;
			ReFileBuffer buf;
			fseek( f, header[i].mOffset, SEEK_SET );
			buf.setsize( header[i].mSize );
			fread( buf.getBuffer(), header[i].mSize, 1, f );
			hdr.read(&buf);
			author = ANSI_TO_UNICODE( hdr.getAuthor().getData() );
			source = ANSI_TO_UNICODE( hdr.getExportFilePath().getData() );
			baseResourceFilePath = ANSI_TO_UNICODE( hdr.getSourceFilePath().getData() );
			currentVersion = hdr.getReFileVersion();
			headerIndex++;
		}
	}

	for(Int32 i=0;i<numElements;i++)
	{
		if( header[i].mType =='skel' )
		{
			ReFileBuffer buf;
			buf.setCurrentVersion( currentVersion );
			fseek(f, header[i].mOffset, SEEK_SET);
			buf.setsize(header[i].mSize);
			fread(buf.getBuffer(), header[i].mSize,1,f);
			skeletons[skelIndex].read(&buf);
			skelIndex++;
		}
		if( header[i].mType =='anim' )
		{
			ReFileBuffer buf;
			buf.setCurrentVersion( currentVersion );
			fseek(f, header[i].mOffset, SEEK_SET);
			buf.setsize(header[i].mSize);
			fread(buf.getBuffer(), header[i].mSize,1,f);
			animations[animIndex].read(&buf);
			animIndex++;
		}
		if( header[i].mType =='curv' )
		{
			ReFileBuffer buf;
			buf.setCurrentVersion( currentVersion );
			fseek(f, header[i].mOffset, SEEK_SET);
			buf.setsize(header[i].mSize);
			fread(buf.getBuffer(), header[i].mSize,1,f);
			curves[curveIndex].read(&buf);
			curveIndex++;
		}
		if( header[i].mType =='traj' )
		{
			ReFileBuffer buf;
			buf.setCurrentVersion( currentVersion );
			fseek(f, header[i].mOffset, SEEK_SET);
			buf.setsize(header[i].mSize);
			fread(buf.getBuffer(), header[i].mSize,1,f);
			trajectories[trajIndex].read(&buf);
			trajIndex++;
		}
	}

	fclose(f);

	// trajectories
	{
		for ( Int32 i=0; i<trajIndex; ++i )
		{
			const ReFileTrajectory& trajectory = trajectories[ i ];
			Int32 numKeys = trajectory.getNumKeys();

			const Int32 index = (Int32)info.m_trajectories.Grow( 1 );
			CSkeletalAnimation::FactoryInfo::Trajectory& traj = info.m_trajectories[ index ];

			traj.m_data.Resize( numKeys );
			traj.m_degree = Min< Int32 >( trajectory.getDegree(), 4 );
			traj.m_name = ANSI_TO_UNICODE( trajectory.getTrajectoryName().getData() );

			for ( Int32 k=0; k<numKeys; ++k )
			{
				traj.m_data[ k ] = Vector::ZERO_3D_POINT;
				
				for ( Int32 d=0; d<traj.m_degree; ++d )
				{
					traj.m_data[ k ].A[ d ] = trajectory.mData[ trajectory.getDegree() * k + d ] / 100.f; // cm - > m
				}
			}
		}
	}

	// not enough curves
	if (numCurves>0 && numCurves < numAnimations)
	{
		WARN_IMPORTER( TXT("Mismatch between curve and animation count in file %s: %d!=%d!"), options.m_filePath.AsChar(), numCurves, numAnimations );
		return NULL;
	}

	// calculate total animation duration
	Float totalDuration = 0.0f;
	Uint32 totalFrames = 0;
	for(Int32 i=0;i<numAnimations;i++)
	{
		ReFileAnimation* panm0 = &animations[i];
		totalDuration += panm0->getAnimationLength();
		totalFrames += panm0->getNumKeys() > 0 ? panm0->getNumKeys() - 1 : 0; // frames are always composed of two keys
	}

	// animation has no length
	if (totalDuration <= 0.0000001f )
	{
		WARN_IMPORTER( TXT("Animation has no length (file %s)"), options.m_filePath.AsChar() );
		return NULL;
	}

	// calculate FPS
	if (totalDuration > 0.0f)
	{
		info.m_framesPerSecond = (Float)totalFrames / totalDuration;
		info.m_duration = totalDuration;
	}
	else
	{
		// one frame animation - prevent it to has zero time
		info.m_framesPerSecond = 30.0f;
		info.m_duration = 0.1f;
	}
	info.m_animationData.m_dT = 1.f / info.m_framesPerSecond;

	// all animations and curves must have the same number of bones and float tracks
	// this is only important for multi part animation
	for(Int32 i=1;i<numAnimations;i++)
	{
		ReFileAnimation* panm0 = &animations[0];
		ReFileAnimation* panm = &animations[i];
		if( panm->getNumBones() != panm0->getNumBones() )
		{
			WARN_IMPORTER( TXT("Mismatch in animation bone count in file %s between animation %d in animation 0!"), 
				options.m_filePath.AsChar(), i );
			return NULL;
		}

		if (numCurves>0)
		{
			ReFileCurve* pcurv0 = &curves[0];
			ReFileCurve* pcurv = &curves[i];
			if( pcurv->getNumChannels() != pcurv0->getNumChannels() )
			{
				WARN_IMPORTER( TXT("Mismatch in curve channel count in file %s between curve %d in curve 0!"), options.m_filePath.AsChar(), i );
				return NULL;
			}
		}
	}

	info.m_animationData.m_parts.Resize(numAnimations);

#ifdef USE_REF_IK_MISSING_BONES_HACK
	Int32 referenceIndex = -1;
	if ( numSkeletons )
	{
		const Int32 numBonesSkel = skeletons[0].getNumBones();
		for( Int32 b=0; b< numBonesSkel; b++ )
		{
			if( strcmp( skeletons[0].mBonesNames[b].getData(), "Reference") == 0 )
			{
				referenceIndex = b;
				break;
			}
		}
	}
#endif

	for(Int32 i=0;i<numAnimations;i++)
	{
		ReFileAnimation* reAnimation = &animations[i];
		ReFileCurve* pcur = (numCurves!=0) ? &curves[i] : NULL;

		// base settings
		if (i == 0)
		{
			info.m_animationData.m_numBones = reAnimation->getNumBones();
			info.m_animationData.m_numTracks = pcur ? pcur->getNumChannels() : 0;

#ifdef USE_REF_IK_MISSING_BONES_HACK
			info.m_animationData.m_hasRefIKBones = referenceIndex != -1;
#endif
		}

		// start part data
		IAnimationBuffer::SourceData::Part& partInfo = info.m_animationData.m_parts[i];
		partInfo.m_numFrames = reAnimation->getNumKeys();

		// transforms
		const Int32 numBones = reAnimation->getNumBones();
		partInfo.m_bones = new RedQsTransform[ partInfo.m_numFrames * numBones ];

		Bool courrptData = false;

		for( Uint32 k=0; k < partInfo.m_numFrames; ++k )
		{
			for( Int32 b=0; b<numBones; ++b )
			{
				const qtransform_scale& tm = *( reAnimation->get( b, k ) );
				RedQsTransform& qs = const_cast<RedQsTransform&>(partInfo.m_bones[b+(numBones*k)]);

				const RedVector4 translation( tm.pos.x * SCALE, tm.pos.y * SCALE, tm.pos.z * SCALE, 1.f );
				const RedVector3 scale( tm.mScale.x, tm.mScale.y, tm.mScale.z );

				qs.Translation.Set( translation );
				qs.Rotation.Set( -tm.rot.x, -tm.rot.y, -tm.rot.z, tm.rot.w );
				qs.Scale.Set( scale, 1.f );

				if ( !qs.Rotation.Quat.IsOk() )
				{
					courrptData = true;
				}

				if ( !qs.Translation.IsOk() )
				{
					courrptData = true;
				}

				if ( !qs.Scale.IsOk() )
				{
					courrptData = true;
				}
			}
		}

		if ( courrptData )
		{
			for(Uint32 k=0;k<partInfo.m_numFrames;++k)
			{
				for( Int32 b=0; b<numBones; ++b )
				{
					RedQsTransform& qs = const_cast<RedQsTransform&>(partInfo.m_bones[b+(numBones*k)]);
					qs.SetIdentity();
				}
			}
		}

		// float tracks
		if( pcur != nullptr && pcur->getNumChannels())
		{
			const Int32 numChannels = pcur->getNumChannels();
			partInfo.m_track = new Float[partInfo.m_numFrames*numChannels];
			for(Uint32 k=0;k<partInfo.m_numFrames;++k)
			{
				for(Int32 c=0;c<numChannels;++c)
				{
					const float tr = pcur->get(c,k);
					const_cast<Float&>(partInfo.m_track[(numChannels*k)+c]) = tr;
				}
			}
		}
		else
		{
			partInfo.m_track = nullptr;
		}
	}


	if ( options.m_extractMotion && numSkeletons )
	{
		//...
		//info.m_motionExtraction = new hkaAnimatedReferenceFrame(NULL);

		const Int32 numBonesSkel = skeletons[0].getNumBones();
		Int32 trajIndex = -1;
		for( Int32 b=0; b< numBonesSkel; b++ )
		{
			if( strcmp(skeletons[0].mBonesNames[b].getData(), "Trajectory" ) == 0 )
			{
				trajIndex = b;
				break;
			}
		}
		//have trajectory index;

		if ( trajIndex >=0 )
		{
			for(Int32 i=0;i<numAnimations;i++)
			{
				ReFileAnimation* reAnimation = &animations[i];
				
				IAnimationBuffer::SourceData::Part& animationData = info.m_animationData.m_parts[i];

				const Int32 numKeys = reAnimation->getNumKeys();

				TDynArray<AnimQsTransform> TajectoryTrack;
				TajectoryTrack.Resize( numKeys );

				for( Int32 k=0; k<numKeys; k++)
				{
					//to powinien byc global, ale u nas local == global na trajektorii (mam nadzieje)
					const qtransform_scale& tm = *( reAnimation->get( trajIndex, k ) );

					AnimVector4 translation( tm.pos.x * SCALE, tm.pos.y * SCALE, tm.pos.z * SCALE );
					AnimQuaternion rotation( -tm.rot.x, -tm.rot.y, -tm.rot.z, tm.rot.w );
					AnimVector4 scale( tm.mScale.x, tm.mScale.y, tm.mScale.z, 1.f );

					TajectoryTrack[k].Set( translation, rotation, scale );
				} //k

				AnimVector4 previousProj;
				for( Int32 k=0; k<numKeys; k++)
				{
					//qtm is root transform
					const qtransform_scale& tm = *( animations[i].get( 0, k ) );

					AnimVector4 translation( tm.pos.x * SCALE, tm.pos.y * SCALE, tm.pos.z * SCALE );
					AnimQuaternion rotation( -tm.rot.x, -tm.rot.y, -tm.rot.z, tm.rot.w );
					AnimVector4 scale( tm.mScale.x, tm.mScale.y, tm.mScale.z, 1.f );

					AnimQsTransform qtm;
					qtm.Set( translation, rotation, scale );

					AnimQsTransform temp( AnimQsTransform::IDENTITY );
					temp.SetMulInverseMul( TajectoryTrack[k], qtm );

					AnimQsTransform me;
					me.SetMulInverseMul( TajectoryTrack[0], TajectoryTrack[k] );

					Float ang = 0.0f;
					AnimQuaternion rest;
					me.Rotation.DecomposeRestAxis(AnimVector4(0.0f,0.0f,1.0f,0.0f), rest, ang);

					if( k>0 )
					{
						while ( ang - previousProj.W > M_PI )
						{
							ang-= 2*M_PI;
						}
						while (ang - previousProj.W < -M_PI)
						{
							ang+= 2*M_PI;
						}
					}

					info.m_motionFrames.PushBack( Vector( me.Translation.X, me.Translation.Y, me.Translation.Z, ang ) );

					previousProj.Set( me.Translation.X, me.Translation.Y, me.Translation.Z, ang );

					AnimQsTransform& bone = const_cast< AnimQsTransform& >( animationData.m_bones[k*numBonesSkel] );
					bone = temp;
				}
			}//i
		}//trajectory index >=0
	}//if param


	/////////additives

	for ( Int32 i=0; i<numAnimations; ++i )
	{
		IAnimationBuffer::SourceData::Part& animationData = info.m_animationData.m_parts[i];

		// Additive
		if ( options.m_type == SAT_Additive )
		{
			if ( options.m_additiveType == AT_TPose )
			{
				ImportAnimationUtils::ConvertToAdditiveAnimation( options.m_tPoseTransform, options.m_tPoseTronsformNum, &animationData, info.m_animationData.m_numBones, info.m_animationData.m_numTracks );
			}
			else if ( options.m_additiveType == AT_Animation )
			{
				GFeedback->ShowMsg(TXT("Error"), TXT("Not Implemented."));
				/*
				hkLoader *loaderAdd = new hkLoader();	

				hkaAnimationContainer* ac = FindAnimationContainer( loaderAdd, params->m_addAnimationToExtractedFile );
				if ( ac && ac->m_numAnimations == 1 && ac->m_animations[ 0 ]->getType() == hkaAnimation::HK_INTERLEAVED_ANIMATION )
				{
					const hkaInterleavedUncompressedAnimation* animationToExtract = static_cast<hkaInterleavedUncompressedAnimation*>( ac->m_animations[ 0 ] );

					ImportAnimationUtils::ConvertToAdditiveAnimation( animationToExtract, rawAnimation );
				}

				loaderAdd->removeReference();
				*/
			}
			else
			{
				ImportAnimationUtils::ConvertToAdditiveAnimation( &animationData, info.m_animationData.m_numBones, options.m_additiveType );
			}
		}
		else if ( options.m_type == SAT_MS && options.m_skeleton )
		{
			GFeedback->ShowMsg(TXT("Error"), TXT("Not Implemented."));
		}
	}//i additives

	if ( options.m_resetRoot && info.m_animationData.m_numBones > 0 )
	{
		for ( Int32 i=0; i<numAnimations; ++i )
		{
			IAnimationBuffer::SourceData::Part& animationData = info.m_animationData.m_parts[i];
			Int32 numKeys = animationData.m_numFrames;
			
			for( Int32 j=0;j<numKeys; j++ )
			{
				RedQsTransform& rootBone = const_cast< RedQsTransform& >( animationData.m_bones[ j*info.m_animationData.m_numBones ] );
				rootBone.SetIdentity();
			}
		}
	}

	// name
	if ( options.m_existingAnimation )
	{
		info.m_name = options.m_existingAnimation->GetName().AsString();
	}
	else
	{
		CFilePath path( options.m_filePath );
		info.m_name = path.GetFileName();
	}

	// create object
	CSkeletalAnimation* retVal = CSkeletalAnimation::Create( info );

	// store info about import type
	if ( retVal )
	{
		retVal->StoreAnythingUsefulFromImporterParams( options );
		retVal->SetImportPath( options.m_filePath );
		retVal->SetAuthorName( author );
		retVal->SetBaseResourcePath( baseResourceFilePath );
	}

	// cleanup
	for (Uint32 i=0; i<info.m_animationData.m_parts.Size(); ++i)
	{
		if ( info.m_animationData.m_parts[i].m_bones )
		{
			delete [] info.m_animationData.m_parts[i].m_bones;
		}
		if ( info.m_animationData.m_parts[i].m_track )
		{
			delete [] info.m_animationData.m_parts[i].m_track;
		}
	}

	// cleanup
	delete [] header;
	delete [] skeletons;
	delete [] animations;
	delete [] curves;
	delete [] trajectories;

	return retVal;
}

Bool CREAAnimImporter::ImportSingleCurve( const String& file, Int32 curveIndex, TDynArray<Float> & data )
{
	if ( curveIndex == -1 )
	{
		return false;
	}


	FILE* f = _wfopen( file.AsChar(), TXT("rb") );
	if( !f )
	{
		return false;
	}

	Int32 numElements = 0;
	fread( &numElements, sizeof(int), 1, f );
	if( numElements == 0 )
	{
		fclose( f );
		return false;
	}

	ReFileArchiveHeader* header = new ReFileArchiveHeader[numElements];
	fread( header, sizeof( ReFileArchiveHeader ) * numElements, 1, f );

	for ( Int32 i=0; i<numElements; i++ )
	{
		if ( header[i].mType =='curv' )
		{
			ReFileCurve curve;

			ReFileBuffer buf;
			fseek(f, header[i].mOffset, SEEK_SET);
			buf.setsize( header[i].mSize );
			fread( buf.getBuffer(), header[i].mSize, 1, f );
			curve.read(&buf);

			const Int32 numTracks = curve.getNumChannels();
			if ( curveIndex < numTracks )
			{
				const Int32 numKeys = curve.getNumKeys();
				data.Resize( numKeys );

				for ( Int32 k=0; k<numKeys; ++k )
				{
					data[ k ] = curve.get( curveIndex, k );
				}
			}

			fclose( f );

			return true;
		}
	}

	fclose( f );

	return false;
}
