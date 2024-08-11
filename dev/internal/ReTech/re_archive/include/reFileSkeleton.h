#pragma once

#include <windows.h>
#include <vector>
#include <set>
#include <map>

#include "..\..\re_math\include\reMath.h"
#include "reFileBaseNode.h"
#include "reFileVersion.h"

#ifdef __GNUC__
#	define RE_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#	define RE_DEPRECATED __declspec(deprecated)
#else
#	pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#	define RE_DEPRECATED
#endif


class ReFileSkeleton : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileSkeleton( const ReFileSkeleton& );
	ReFileSkeleton& operator = ( const ReFileSkeleton& );

public:
	ReFileSkeleton();
	ReFileSkeleton( ReFileSkeleton* s );
	~ReFileSkeleton();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override { return 'skel'; }
	static int			TypeStatic() {return 'skel';}
	void				setSkeletonData( int numb, int numc=0 );
	void				setBoneData( const ReFileString& nam, int ind, int p, const qtransform_scale& tr );
	void				setfloat( const ReFileString& nam, int ind, float v );
	int					getNumBones(){ return mNumBones; }
	int					getNumChannels(){ return mNumChannels; }

private:
	void				Reset();

public: //TODO: change to private
	qtransform_scale*	mBonesTransforms;
	float*				mBonesCurves;
	int*				mBonesParents;
	ReFileString*		mBonesNames;
	ReFileString*		mBonesChannels;

private:
	int					mNumBones;
	int					mNumChannels;
};

