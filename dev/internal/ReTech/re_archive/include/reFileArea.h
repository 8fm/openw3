#pragma once
#include "reFileBaseNode.h"

#ifndef RE_AREA_SIZE
#define RE_AREA_SIZE	4
#endif // !RE_AREA_SIZE


class ReFileArea : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileArea& operator = ( const ReFileArea& );

public:
	ReFileArea();
	ReFileArea( ReFileArea* reArea );
	ReFileArea( const ReFileArea& );
	~ReFileArea();

	bool				operator==(const ReFileArea& other);

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'area';}
	static int			TypeStatic() {return 'area';}
	void				setAreaName( const ReFileString& name ){ mAreaName.set(name.getData()); }
	const ReFileString&	getAreaName() const { return mAreaName; }

public: //TODO
	float			mAreas[RE_AREA_SIZE];

private:
	ReFileString	mAreaName;
};
