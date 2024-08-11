#pragma once
#include "reFileBaseNode.h"


class ReFileCurve : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileCurve( const ReFileCurve& );
	ReFileCurve& operator = ( const ReFileCurve& );

public:
	ReFileCurve();
	ReFileCurve( ReFileCurve* c );
	~ReFileCurve();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'curv';}
	void				set(int numk, int numbc, float len);
	float&				get(int c, int k);
	int					getNumKeys(){ return mNumKeys; }
	int					getNumChannels(){ return mNumChannels; }
	float				getCurveLength(){ return mCurveLength; }

public: //TODO: to private
	float*	mCurveKeys;

private:
	int		mNumKeys;
	int		mNumChannels;
	float	mCurveLength;
};

