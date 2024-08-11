#pragma once
#include "reFileBaseNode.h"

/*
// TODO: Deprecated. To be removed.
class ReFilePreset : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFilePreset( const ReFilePreset& );
	ReFilePreset& operator = ( const ReFilePreset& );

public:
	ReFilePreset();
	~ReFilePreset();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'pres';}
	void				set( const ReFileString& nam, int num );
	int					getPresetCount() const { return mCount; }
	void				setPresetName( const ReFileString& name ){ mPresetName.set(name.getData()); }
	ReFileString&		getPresetName(){ return mPresetName; }

public:
	int*			mData;

private:
	ReFileString	mPresetName;
	int				mCount;
};
*/
