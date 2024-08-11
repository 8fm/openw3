/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../../common/redSystem/crt.h"
#include "REAAnimExporter.h"
#include "../../common/engine/havokAnimationUtils.h"
#include "../../common/engine/skeleton.h"
#include "re_archive/include/reFileNodes.h"

static const Float SCALE = 0.01f;

IMPLEMENT_ENGINE_CLASS(CREAAnimExporter);

CREAAnimExporter::CREAAnimExporter()
{
	m_formats.PushBack( CFileFormat( TXT("re"), TXT("Red Engine Animation") ) );
}

static void TransformToReQsTransform( qtransform_scale& output, const RedQsTransform& transform )
{
	output.pos.x = transform.GetTranslation().X / SCALE;
	output.pos.y = transform.GetTranslation().Y / SCALE;
	output.pos.z = transform.GetTranslation().Z / SCALE;
	output.rot.x = -transform.GetRotation().Quat.X;
	output.rot.y = -transform.GetRotation().Quat.Y;
	output.rot.z = -transform.GetRotation().Quat.Z;
	output.rot.w = transform.GetRotation().Quat.W;

	//new added support for scale
	output.mScale.x = transform.GetScale().X;
	output.mScale.y = transform.GetScale().Y;
	output.mScale.z = transform.GetScale().Z;
}

static void FillSkeleton( ReFileSkeleton* reSkeleton, const CSkeleton* skeleton )
{
	reSkeleton->setSkeletonData( skeleton->GetBonesNum() );
	// set all bones
	TDynArray< ISkeletonDataProvider::BoneInfo > bones;
	skeleton->GetBones( bones );
	int boneIdx = 0;
	for ( auto iBone = bones.Begin(); iBone != bones.End(); ++ iBone, ++ boneIdx )
	{
		qtransform_scale boneQSTransform;
		TransformToReQsTransform( boneQSTransform, skeleton->GetBoneLS( boneIdx ) );
		AnsiChar ansiBoneName[ 256 ];
		Red::System::StringConvert( ansiBoneName, iBone->m_name.AsChar(), ARRAY_COUNT( ansiBoneName ) );

		Int32 par = skeleton->GetParentBoneIndex(boneIdx);
		reSkeleton->setBoneData( ansiBoneName, boneIdx, par, boneQSTransform );
	}
}

struct AnimExporterParamsHelper
{
	Int32 m_trajectoryBoneIdx;
	AnimExporterParamsHelper( const AnimExporterParams& options )
		: m_trajectoryBoneIdx( -1 )
	{
		if ( options.m_skeleton )
		{
			m_trajectoryBoneIdx = options.m_skeleton->FindBoneByName( CNAME( Trajectory ) );
		}
	}
};

static void FillAnimationAndCurves( const AnimExporterParams& options, ReFileAnimation* reAnimation, ReFileCurve* reCurve, const CSkeletalAnimation* animation, Uint32 partIdx )
{
	AnimExporterParamsHelper optionsHelper( options );
	IAnimationBuffer const * wholeAnimBuf = animation->GetAnimBuffer();
	IAnimationBuffer const * animBuf = animation->GetAnimBuffer()->GetPart( partIdx );
	Float length = animation->GetDuration() * ((Float) animBuf->GetFramesNum()) / ((Float) wholeAnimBuf->GetFramesNum());
	reAnimation->set( animBuf->GetFramesNum(), animBuf->GetBonesNum(), length );
	if ( reCurve )
	{
		reCurve->set( animBuf->GetFramesNum(), animBuf->GetTracksNum(), length );
	}
	RedQsTransform* frameTransforms = new RedQsTransform[ animBuf->GetBonesNum() ];
	Float* frameTracks = new Float[ animBuf->GetTracksNum() ];
	for ( Uint32 f = 0; f < animBuf->GetFramesNum(); ++ f )
	{
		animBuf->Sample( Float(f), animBuf->GetBonesNum(), frameTransforms, animBuf->GetTracksNum(), frameTracks );
		for ( Uint32 b = 0; b < animBuf->GetBonesNum(); ++ b )
		{
			qtransform_scale boneQSTransform;
			if ( ! options.m_exportTrajectory && ( ( b == static_cast<Uint32>( optionsHelper.m_trajectoryBoneIdx ) && optionsHelper.m_trajectoryBoneIdx >= 0 ) || b == 0 ) )
			{
				TransformToReQsTransform( boneQSTransform, RedQsTransform::IDENTITY );
			}
			else
			{
				TransformToReQsTransform( boneQSTransform, frameTransforms[ b ] );
			}
			reAnimation->set( (int)b, (int)f, boneQSTransform );
		}
		if ( reCurve )
		{
			for ( Uint32 c = 0; c < animBuf->GetTracksNum(); ++ c )
			{
				reCurve->get( (int)c, (int)f ) =  (float)frameTracks[ c ];
			}
		}
	}
	delete [] frameTransforms;
	delete [] frameTracks;
}

static void FillAnimationAndCurves( const AnimExporterParams& options, ReFileAnimation* reAnimation, ReFileCurve* reCurve, const IAnimationBuffer::SourceData& sourceData, Uint32 partIdx )
{
	AnimExporterParamsHelper optionsHelper( options );
	Uint32 totalFramesCount = 0;
	for ( Uint32 p = 0; p < sourceData.m_parts.Size(); ++ p )
	{
		totalFramesCount += sourceData.m_parts[p].m_numFrames;
	}
	IAnimationBuffer::SourceData::Part const & part = sourceData.m_parts[partIdx];
	Float length = sourceData.m_totalDuration * ((Float) part.m_numFrames) / ((Float) totalFramesCount);
	reAnimation->set( part.m_numFrames, sourceData.m_numBones, length );
	if ( reCurve )
	{
		reCurve->set( part.m_numFrames, sourceData.m_numTracks, length );
	}
	RedQsTransform* frameTransforms = new RedQsTransform[ sourceData.m_numBones ];
	Float* frameTracks = new Float[ sourceData.m_numTracks ];
	for ( Uint32 f = 0; f < part.m_numFrames; ++ f )
	{
		RedQsTransform const * sourceBones = part.GetFrameBoneData( f, sourceData.m_numBones );
		Float const * sourceTracks = part.GetFrameTrackData( f, sourceData.m_numTracks );
		for ( Uint32 b = 0; b < sourceData.m_numBones; ++ b, ++ sourceBones )
		{
			qtransform_scale boneQSTransform;
			if ( ! options.m_exportTrajectory && ( ( b == static_cast<Uint32>( optionsHelper.m_trajectoryBoneIdx ) && optionsHelper.m_trajectoryBoneIdx >= 0 ) || b == 0 ) )
			{
				TransformToReQsTransform( boneQSTransform, RedQsTransform::IDENTITY );
			}
			else
			{
				TransformToReQsTransform( boneQSTransform, *sourceBones );
			}
			reAnimation->set( (int)b, (int)f, boneQSTransform );
		}
		if ( reCurve )
		{
			for ( Uint32 c = 0; c < sourceData.m_numTracks; ++ c, ++ sourceTracks )
			{
				reCurve->get( (int)c, (int)f ) = (float)( *sourceTracks );
			}
		}
	}
	delete [] frameTransforms;
	delete [] frameTracks;
}

Bool CREAAnimExporter::DoExport( CSkeletalAnimation* animation, const AnimExporterParams& options )
{
	if ( ! options.m_skeleton )
	{
		return false;
	}

	FILE* f = _wfopen( options.m_filePath.AsChar(),TXT("wb+") );
	if ( ! f )
	{
		return false;
	}

	// check whether we may use source data
	Bool useSourceData = false;
	IAnimationBuffer::SourceData sourceData;
#ifndef NO_EDITOR
	if ( LatentSourceAnimDataBuffer* sourceAnimData = animation->GetSourceAnimData() )
	{
		// use source data only when source data exists, is in proper format, etc
		useSourceData = sourceAnimData->ReadAnimDataTo( sourceData, animation );
	}
#endif

	//
	TDynArray< ReFileSkeleton* > skeletons;
	TDynArray< ReFileAnimation* > animations;
	TDynArray< ReFileCurve* > curves;
	TDynArray< ReFileTrajectory* > trajectories;
	ReFileHeader2 reHeader;

	// fill data
	skeletons.PushBack( new ReFileSkeleton() );
	FillSkeleton( skeletons.Last(), options.m_skeleton );

	if ( ! useSourceData )
	{
		for ( Uint32 partIdx = 0; partIdx < animation->GetAnimBuffer()->GetPartsNum(); ++ partIdx )
		{
			animations.PushBack( new ReFileAnimation() );
			if ( animation->GetTracksNum() )
			{
				curves.PushBack( new ReFileCurve() );
				FillAnimationAndCurves( options, animations.Last(), curves.Last(), animation, partIdx );
			}
			else
			{
				FillAnimationAndCurves( options, animations.Last(), nullptr, animation, partIdx );
			}
		}
	}
	else
	{
		for ( Uint32 partIdx = 0; partIdx < sourceData.m_parts.Size(); ++ partIdx )
		{
			animations.PushBack( new ReFileAnimation() );
			if ( sourceData.m_numTracks )
			{
				curves.PushBack( new ReFileCurve() );
				FillAnimationAndCurves( options, animations.Last(), curves.Last(), sourceData, partIdx );
			}
			else
			{
				FillAnimationAndCurves( options, animations.Last(), nullptr, sourceData, partIdx );
			}
		}
	}
	
	// calculate how much data do we have
	Int32 numSkeletons = skeletons.SizeInt();
	Int32 numAnimations = animations.SizeInt();
	Int32 numCurves = curves.SizeInt();
	Int32 numTrajectories = trajectories.SizeInt();
	Int32 numHeaders = 1;

	Int32 numElements = numSkeletons + numAnimations + numCurves + numTrajectories + numHeaders;
	ReFileArchiveHeader* header = new ReFileArchiveHeader[numElements];

	// build header
	Int32 elementIdx = 0;
	for(Int32 i=0;i<numHeaders;i++)
	{
		header[elementIdx ++].mType ='hed2';
	}
	for(Int32 i=0;i<numSkeletons;i++)
	{
		header[elementIdx ++].mType ='skel';
	}
	for(Int32 i=0;i<numAnimations;i++)
	{
		header[elementIdx ++].mType ='anim';
	}
	for(Int32 i=0;i<numCurves;i++)
	{
		header[elementIdx ++].mType ='curv';
	}
	for(Int32 i=0;i<numTrajectories;i++)
	{
		header[elementIdx ++].mType ='traj';
	}

	// write number of elements
	fwrite(&numElements,sizeof(int),1,f);

	// write header to reserve space for it
	unsigned int headerOffset = ftell(f);
	fwrite( header, sizeof( ReFileArchiveHeader ) * numElements, 1, f );

	// write data
	Int32 skelIndex = 0;
	Int32 animIndex = 0;
	Int32 curveIndex = 0;
	Int32 trajIndex = 0;

	for(Int32 i=0;i<numElements;i++)
	{
		if( header[i].mType =='hed2' )
		{
			ReFileBuffer buf;
			header[i].mOffset = ftell( f );
			reHeader.write(&buf);

			header[i].mSize = buf.getMaxBufferSize();
			fwrite( buf.getBuffer(), header[i].mSize, 1, f );
		}
		if( header[i].mType =='skel' )
		{
			ReFileBuffer buf;
			header[i].mOffset = ftell( f );
			skeletons[skelIndex]->write(&buf);
			skelIndex++;
			header[i].mSize = buf.getMaxBufferSize();
			fwrite( buf.getBuffer(), header[i].mSize, 1, f );
		}
		if( header[i].mType =='anim' )
		{
			ReFileBuffer buf;
			header[i].mOffset = ftell( f );
			animations[animIndex]->write(&buf);
			animIndex++;
			header[i].mSize = buf.getMaxBufferSize();
			fwrite( buf.getBuffer(), header[i].mSize, 1, f );
		}
		if( header[i].mType =='curv' )
		{
			ReFileBuffer buf;
			header[i].mOffset = ftell( f );
			curves[curveIndex]->write(&buf);
			curveIndex++;
			header[i].mSize = buf.getMaxBufferSize();
			fwrite( buf.getBuffer(), header[i].mSize, 1, f );
		}
		if( header[i].mType =='traj' )
		{
			ReFileBuffer buf;
			header[i].mOffset = ftell( f );
			trajectories[trajIndex]->write(&buf);
			trajIndex++;
			header[i].mSize = buf.getMaxBufferSize();
			fwrite( buf.getBuffer(), header[i].mSize, 1, f );
		}
	}

	// write header again, with proper offsets and sizes
	fseek( f, headerOffset, SEEK_SET );
	fwrite( header, sizeof( ReFileArchiveHeader ) * numElements, 1, f );

	fclose(f);

	// cleanup
	delete [] header;
	for (auto i = skeletons.Begin(); i != skeletons.End(); ++ i) delete *i;
	for (auto i = animations.Begin(); i != animations.End(); ++ i) delete *i;
	for (auto i = curves.Begin(); i != curves.End(); ++ i) delete *i;
	for (auto i = trajectories.Begin(); i != trajectories.End(); ++ i) delete *i;

	sourceData.CleanUp();

	return true;
}

void CREAAnimExporter::ExportSingleCurve( const String& file, Int32 curveIndex, TDynArray<Float> & data )
{
	/*
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

	pat_archive::REX_header* header = new pat_archive::REX_header[numElements];
	fread( header, sizeof(pat_archive::REX_header)*numElements, 1, f );

	for ( Int32 i=0; i<numElements; i++ )
	{
		if ( header[i].type=='curv' )
		{
			pat_archive::pcurve curve;

			pat_archive::pat_buffer buf;
			fseek(f, header[i].offset, SEEK_SET);
			buf.setsize(header[i].size);
			fread(buf.buffer, header[i].size,1,f);
			curve.read(&buf);

			const Int32 numTracks = curve.numChannels;
			if ( curveIndex < numTracks )
			{
				const Int32 numKeys = curve.numKeys;
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
	*/
}
