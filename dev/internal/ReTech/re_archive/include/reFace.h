#pragma once
#include "reFileBaseNode.h"
#include "reFileArchive.h"
#include "reFileSkeleton.h"
#include "reFileMesh.h"
#include "reFileMixer.h"
#include "reFileBitmap.h"
#include "reFileArea.h"
#include "reFilePose.h"
#include "reFileFilter.h"

class ReFace
{
public:
	ReFace();
	virtual ~ReFace();

public:
	bool load( const char* path );
	bool save( const char* path );

public:
	ReFileSkeleton*					mSkeleton;
	std::vector<ReFileMesh*>		mMeshes;
	std::vector<ReFileMixer*>		mMixers;
	std::vector<ReFileBitmap*>		mTextures;
	std::vector<ReFileArea*>		mAreas;
	std::vector<ReFilePose*>		mPoses;
	std::vector<ReFileFilter*>		mFilters;
};

