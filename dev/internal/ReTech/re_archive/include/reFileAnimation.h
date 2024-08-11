#pragma once

#include "reFileBaseNode.h"
#include "..\..\re_math\include\reMath.h"

class ReFileAnimation : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileAnimation( const ReFileAnimation& );
	ReFileAnimation& operator = ( const ReFileAnimation& );

public:
	ReFileAnimation();
	ReFileAnimation( ReFileAnimation* anim );
	~ReFileAnimation();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'anim';}
	void				set(int numk, int numb, float len);
	void				set(int b, int k, const qtransform_scale& dat);
	qtransform_scale*	get(int b, int k);
	int					getNumKeys(){ return mNumKeys; }
	int					getNumBones(){ return mNumBones; }
	float				getAnimationLength(){ return mAnimationLength; }

public:
	bool operator == ( const ReFileAnimation& other );

public: //TODO: change to private
	qtransform_scale*	mAnimationKeys;

private:
	int		mNumKeys;
	int		mNumBones;
	float	mAnimationLength;
};

