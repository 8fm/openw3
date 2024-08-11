#pragma once
#include "reFileBaseNode.h"

class ReFileCollisionConvex : public ReFileBaseCollisionNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileCollisionConvex( const ReFileCollisionConvex& );
	ReFileCollisionConvex& operator = ( const ReFileCollisionConvex& );

public:
	ReFileCollisionConvex();
	ReFileCollisionConvex( ReFileCollisionConvex* coll );
	~ReFileCollisionConvex();

	//move constructor
	ReFileCollisionConvex( ReFileCollisionConvex&& c );

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int		Type() const override {return 'cl00';};
	void			set( const ReFileString& nam, int numv, int numf, int numm );
	float&			getVertexById(int idx) { return mVertices[idx]; }
	int&			getIndexById(int idx) { return mIndices[idx]; }
	void			setMaterialName( int idx, const ReFileString& name );
	int				getNumVertices() const { return mNumVertices; }
	int				getNumFaces() const { return mNumFaces; }
	int				getNumMaterials() const { return mNumMaterials; }

public:	//TODO to priv
	float*			mVertices;
	int*			mIndices;
	ReFileString*	mMaterials;

private:
	int				mNumVertices;
	int				mNumFaces;
	int				mNumMaterials;
};

