#pragma once
#include "reFileBaseNode.h"


class ReFileTriMesh : public ReFileBaseCollisionNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileTriMesh( const ReFileTriMesh& );
	ReFileTriMesh& operator = ( const ReFileTriMesh& );

public:
	ReFileTriMesh();
	ReFileTriMesh( ReFileTriMesh* tri );
	~ReFileTriMesh();

	// move ctor
	ReFileTriMesh(ReFileTriMesh&& tri);

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override {return 'trim';}
	void				set( const ReFileString& nam, int numv, int numf );
	float&				getVertexById(int idx) { return mVertices[idx]; }
	int&				getIndexById(int idx) { return mIndices[idx]; }
	int					getNumVertices(){ return mNumVertices; }
	int					getNumFaces(){ return mNumFaces; }

public:			//TODO: to priv
	float*			mVertices;
	int*			mIndices;
	float			mTransformMatrix[RIGMATRIXSIZE];
	int				mTrimeshType;

private:
	int				mNumVertices;
	int				mNumFaces;
};
