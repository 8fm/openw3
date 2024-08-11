#pragma once
#include "reFileBaseNode.h"

class ReFileCollisionSphere : public ReFileBaseCollisionNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileCollisionSphere( const ReFileCollisionSphere& );
	ReFileCollisionSphere& operator = ( const ReFileCollisionSphere& );

public:
	ReFileCollisionSphere();
	ReFileCollisionSphere( ReFileCollisionSphere* s );
	~ReFileCollisionSphere();

	//move ctor
	ReFileCollisionSphere(ReFileCollisionSphere&& sphere);

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	int					Type() const {return 'psph';}
	void				set( const ReFileString& nam, const ReFileString& mat, float r );
	const ReFileString&	getMaterialName() const { return mMaterialName; }
	float				getRadius() const { return mRadius; }

private:
	ReFileString	mMaterialName;
	float			mRadius;
};

