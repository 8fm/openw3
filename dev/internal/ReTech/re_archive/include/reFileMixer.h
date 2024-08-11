#pragma once
#include "reFileBaseNode.h"


class ReFileMixer : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileMixer( const ReFileMixer& );
	ReFileMixer& operator = ( const ReFileMixer& );

public:
	ReFileMixer();
	ReFileMixer( ReFileMixer* m );
	~ReFileMixer();

	void Reset();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'mixr';}
	static int			TypeStatic() {return 'mixr';}
	void				set(int numk, int numb );
	float*				get(int b, int k);
	void				set(int b, int k, float*  dat);
	void				setNumKeys( int newVal ) { mNumKeys = newVal; }
	void				setNumBones( int newVal ) { mNumBones = newVal; }
	int					getNumKeys() const { return mNumKeys; }
	int					getNumBones() const { return mNumBones; }
	const ReFileString&	getMixerName() const { return mMixerName; }
	void				setMixerName( const ReFileString& name ){ mMixerName.set(name.getData()); }

public:
	float*				mKeys;
	int*				mMappings;
	ReFileString*		mNames;

private:
	ReFileString		mMixerName;
	int					mNumKeys;
	int					mNumBones;
};
