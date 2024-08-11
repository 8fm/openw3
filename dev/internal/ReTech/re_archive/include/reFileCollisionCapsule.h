#pragma once
#include "reFileBaseNode.h"

class ReFileCollisionCapsule : public ReFileBaseCollisionNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileCollisionCapsule( const ReFileCollisionCapsule& );
	ReFileCollisionCapsule& operator = ( const ReFileCollisionCapsule& );

public:
	ReFileCollisionCapsule();
	ReFileCollisionCapsule( ReFileCollisionCapsule* c );
	~ReFileCollisionCapsule();

	//move ctor
	ReFileCollisionCapsule(ReFileCollisionCapsule&& caps);

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	int					Type() const {return 'pcap';}
	void				set( const ReFileString& nam, const ReFileString& mat, float r, float h );
	const ReFileString&	getMaterialName() const { return mMaterialName; }
	float				getRadius() const { return mRadius; }
	float				getHeight() const { return mHeight; }

private:
	ReFileString	mMaterialName;
	float			mRadius;
	float			mHeight;
};

