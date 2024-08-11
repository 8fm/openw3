#pragma once
#include "reFileBaseNode.h"
#include "..\..\re_math\include\reMath.h"


class ReFileCollisionBox : public ReFileBaseCollisionNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileCollisionBox( const ReFileCollisionBox& );
	ReFileCollisionBox& operator = ( const ReFileCollisionBox& );

public:
	ReFileCollisionBox();
	ReFileCollisionBox( ReFileCollisionBox* box );
	~ReFileCollisionBox();

	//move ctor
	ReFileCollisionBox(ReFileCollisionBox&& box);

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	int						Type() const { return 'pbox'; }
	void					set( const ReFileString& nam, const ReFileString& mat, float w, float l, float h );
	ReFileString&			getMaterialName(){ return mMaterialName; }
	const ReFileString&		getMaterialName() const { return mMaterialName; }
	float					getWidth() const { return mWidth; }
	float					getHeight() const { return mHeight; }
	float					getLength() const { return mLength; }
	float3					getDimensions() const;

private:
	ReFileString	mMaterialName;
	float			mWidth;
	float			mLength;
	float			mHeight;
};

