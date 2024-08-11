#pragma once
#include "reFileBaseNode.h"

class ReFileFilter : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileFilter( const ReFileFilter& );
	ReFileFilter& operator = ( const ReFileFilter& );

public:
	ReFileFilter();
	ReFileFilter( ReFileFilter* f );
	~ReFileFilter();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'fltr';}
	static int			TypeStatic() {return 'fltr';}
	void				set( const ReFileString& nam, int numbc );
	int					getNumChannels() const { return mNumChannels; }
	const ReFileString&	getFilterName() const { return mFilterName; }

public:		//TODO
	float*			mFilterKeys;

private:
	ReFileString	mFilterName;
	int				mNumChannels;
};

