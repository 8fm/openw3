#pragma once
#include "reFileBaseNode.h"

class ReFileDyngNode : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileDyngNode( const ReFileDyngNode& );
	ReFileDyngNode& operator = ( const ReFileDyngNode& );

public:
	ReFileDyngNode();
	~ReFileDyngNode();

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void					set( const ReFileString& nam, const ReFileString& pnam, float m, float s, float d);
	const ReFileString&		getParentName() const { return mParentName; }
	const ReFileString&		getDyngName() const { return mDyngName; }
	float					getMass() const { return mMass; }
	float					getStiffness() const { return mStiffness; }
	float					getDistance() const { return mDistance; }
	void					Fill( const ReFileDyngNode& d );

private:
	ReFileString			mParentName;
	float					mMass;
	float					mStiffness;
	float					mDistance;
	ReFileString			mDyngName;

public:		//TODO: private
	float					mTransformMatrix[RIGMATRIXSIZE];
};

class ReFileDyngLink : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileDyngLink( const ReFileDyngLink& );
	ReFileDyngLink& operator = ( const ReFileDyngLink& );

public:
	ReFileDyngLink();
	~ReFileDyngLink();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void				set( int a, int b, float l, int t );
	const int			getIndexA() const { return mIndexA; }
	const int			getIndexB() const { return mIndexB; }
	const float			getLength() const { return mLength; }
	const int			getLinkType() const { return mLinkType; }
	void				Fill( const ReFileDyngLink& d );

private:
	int					mIndexA;
	int					mIndexB;
	float				mLength;
	int					mLinkType; //TODO: change to enum
};

class ReFileDyngTriangle : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileDyngTriangle( const ReFileDyngTriangle& );
	ReFileDyngTriangle& operator = ( const ReFileDyngTriangle& );

public:
	ReFileDyngTriangle();
	~ReFileDyngTriangle();

public:
	virtual void	write(ReFileBuffer* buf) override;
	virtual void	read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void			set( int a, int b, int c, int t );
	void			setIndices( int a, int b, int c );
	int				getIndexA() const { return mIndexA; }
	int				getIndexB() const { return mIndexB; }
	int				getIndexC() const { return mIndexC; }
	int				getTriangleType() const { return mDyngTriangleType; }
	void			Fill( const ReFileDyngTriangle& t );

public:
	int		mIndexA;
	int		mIndexB;
	int		mIndexC;
	int		mDyngTriangleType; //TODO: change to enum
};

class ReFileDyngShape : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileDyngShape( const ReFileDyngShape& );
	ReFileDyngShape& operator = ( const ReFileDyngShape& );

public:
	ReFileDyngShape();
	~ReFileDyngShape();

public:
	virtual void	write(ReFileBuffer* buf) override;
	virtual void	read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void					set( const ReFileString& parentName, float radius, float height );
	void					setParentName( const ReFileString& parName ){ mParentName = parName; }
	const ReFileString&		getParentName() const { return mParentName; }
	float					getRadius() const { return mRadius; }
	void					setRadius( float rad ){ mRadius = rad; }
	float					getHeight() const { return mHeight; }
	void					setHeight( float h ){ mHeight = h; }
	void					Fill( const ReFileDyngShape& s );

private:
	ReFileString	mParentName;

public:				//TODO
	float			mTransformMatrix[RIGMATRIXSIZE];

private:
	float			mRadius;
	float			mHeight;
};

class ReFileDyng : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileDyng( const ReFileDyng& );
	ReFileDyng& operator = ( const ReFileDyng& );

public:
	ReFileDyng();
	ReFileDyng( ReFileDyng* d );
	~ReFileDyng();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG* g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int			Type() const override { return 'dyng'; }
	void				set( const ReFileString& nam, int nn, int nl, int nt, int ns );
	void				create_triangles_from_links();
	bool				edges_connected( int a, int b , int* face, int* edg );
	bool				is_edge( int x, int y, int ind );
	int					getNumNodes() const { return mNumNodes; }
	int					getNumLinks() const { return mNumLinks; }
	int					getNumTriangles() const { return mNumTriangles; }
	int					getNumShapes() const { return mNumShapes; }
	const ReFileString&	getDyngName() const { return mDyngName; }

public:		//TODO: to private
	ReFileDyngNode*		mDyngNodes;
	ReFileDyngLink*		mDyngLinks;
	ReFileDyngTriangle*	mDyngTriangles;
	ReFileDyngShape*	mDyngShapes;

private:
	int					mNumNodes;
	int					mNumLinks;
	int					mNumTriangles;
	int					mNumShapes;
	ReFileString		mDyngName;
};

