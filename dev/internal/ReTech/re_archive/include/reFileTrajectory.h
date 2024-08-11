#pragma once
#include "reFileBaseNode.h"


// Class representing the motion curve path?
class ReFileTrajectory : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileTrajectory( const ReFileTrajectory& );
	ReFileTrajectory& operator = ( const ReFileTrajectory& );

public:
	ReFileTrajectory();
	ReFileTrajectory( ReFileTrajectory* t );
	~ReFileTrajectory();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'traj';}
	void				set( const ReFileString& nam, int nump, int deg );
	float*				get(int k);
	int					getNumKeys() const { return mNumKeys; }
	int					getDegree() const { return mDegree; }
	const ReFileString&	getTrajectoryName() const { return mTrajectoryName; }

public:		//TODO to priv
	float*			mData;

private:
	int				mNumKeys;
	int				mDegree;
	ReFileString	mTrajectoryName;
};
