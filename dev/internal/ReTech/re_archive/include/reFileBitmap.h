#pragma once
#include "reFileBaseNode.h"

class ReFileBitmap : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileBitmap( const ReFileBitmap& );
	ReFileBitmap& operator = ( const ReFileBitmap& );

public:
	ReFileBitmap();
	ReFileBitmap( ReFileBitmap* bm );
	~ReFileBitmap();

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int				Type() const override {return 'bmap';}
	void					set( const ReFileString& nam, int rx, int ry, int dp );
	int						getResX(){ return mResolutionX; }
	int						getResY(){ return mResolutionY; }
	int						getBitDepth(){ return mBitDepth; }
	ReFileString&			getBitmapName(){ return mBitmapName; }
	void					setBitmapName( const ReFileString& name ){ mBitmapName.set(name.getData()); }

public:	//TODO
	char*			mData;

private:
	ReFileString	mBitmapName;
	int				mResolutionX;
	int				mResolutionY;
	int				mBitDepth;
};

