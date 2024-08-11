#pragma once
#include "reFileBaseNode.h"

class ReFilePath : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFilePath( const ReFilePath& );
	ReFilePath& operator = ( const ReFilePath& );

public:
	ReFilePath();
	ReFilePath( ReFilePath* p );
	~ReFilePath();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'path';}
	void				setPath( const ReFileString& path ){ mPath.set(path.getData()); }
	ReFileString&		getPath() { return mPath; }

private:
	ReFileString	mPath;
};

