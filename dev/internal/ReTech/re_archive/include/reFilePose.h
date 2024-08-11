#pragma once
#include "reFileBaseNode.h"


// Represents a single phoneme (it's not a skeleton pose!)
class ReFilePose : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFilePose( const ReFilePose& );
	ReFilePose& operator = ( const ReFilePose& );

public:
	ReFilePose();
	ReFilePose( ReFilePose* p );
	~ReFilePose();

public:
	virtual void	write(ReFileBuffer* buf) override;
	virtual void	read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'pose';}
	static int			TypeStatic() {return 'pose';}
	void				set( const ReFileString& nam, int numbc );
	const ReFileString&	getPoseName() const { return mPoseName; }
	int					getNumChannels() const { return mNumChannels; }

public:
	float*			mPoseKeys;

private:
	ReFileString	mPoseName;
	int				mNumChannels;
};

