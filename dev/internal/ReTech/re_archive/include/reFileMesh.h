#pragma once
#include "reFileBaseNode.h"

#include <windows.h>
#include <vector>
#include <set>
#include <map>
#include <iostream>

#include "..\..\re_math\include\reMath.h"


// ----------------------------------------------------------------------------
// ReFileRigmatrix
// ----------------------------------------------------------------------------
class ReFileRigmatrix : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileRigmatrix( const ReFileRigmatrix& );
	ReFileRigmatrix& operator = ( const ReFileRigmatrix& );

public:
	ReFileRigmatrix();

public:
	virtual void		write(ReFileBuffer* buf) override;
	virtual void		read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*	writeHdf5(H5::CommonFG * g) override;
	virtual void		readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void				setName( const ReFileString& name ) { mName.set( name.getData() ); }
	inline float*		getRigMatrix() { return mRigMatrix; }
	const ReFileString&	getName() const { return mName; }
	void				Fill( const ReFileRigmatrix& rm );

public:	//todo : to priv
	float				mRigMatrix[RIGMATRIXSIZE];

private:
	ReFileString		mName;
};


// ----------------------------------------------------------------------------
// ReFileVertex
// ----------------------------------------------------------------------------
class ReFileVertex
{
public:
	ReFileVertex();

public:
	const float3	getPosition() const { return mPosition; }
	const float3	getNormal() const { return mNormal; }
	const float3	getTangent() const { return mTangent; }
	const float3	getBinormal() const { return mBinormal; }
	const float4	getColor() const { return mColor; }
	const float3	getColorRGB() const { return float3( mColor.x, mColor.y, mColor.z ); }
	const float		getColorAlpha() const { return mColor.w; }
	const float2	getUV1() const { return mUV1; }
	const float2	getUV2() const { return mUV2; }
	const float4	getWeights() const { return mWeights; }
	const float4	getBoneIndices() const { return mBonesIndices; }

	void			setPosition( const float3& val ){ mPosition = val; }
	void			setNormal( const float3& val ){ mNormal = val; }
	void			setTangent( const float3& val ){ mTangent = val; }
	void			setBinormal( const float3& val ){ mBinormal = val; }
	void			setColor( const float4& val ){ mColor = val; }
	void			setWeights( const float4& val ){ mWeights = val; }
	void			setBoneIndices( const float4& val ){ mBonesIndices = val; }
	void			setUV1( const float2& val ){ mUV1 = val; }
	void			setUV2( const float2& val ){ mUV2 = val; }

private:
	float3			mPosition;
	float3			mNormal;
	float3			mTangent;
	float3			mBinormal;
	float4			mColor;
	float4			mWeights;
	float4			mBonesIndices;
	float2			mUV1;
	float2			mUV2;
};
static_assert( sizeof( ReFileVertex ) == 112, "Wrong vertex size." );

// ----------------------------------------------------------------------------
// ReFileFatVertex
// ----------------------------------------------------------------------------
class ReFileFatVertex : public ReFileVertex
{
public:
	ReFileFatVertex();

public:
	// getters
	const float4	getExtra4Weights() const { return mExtraWeights; }
	const float4	getExtra4BoneIndices() const { return mExtraBonesIndices; }

	// set extra 4 bones skinning data
	void			setExtra4Weights( const float4& val ){ mExtraWeights = val; }
	void			setExtra4BoneIndices( const float4& val ){ mExtraBonesIndices = val; }

private:
	// extra streams for 8 bone skinning
	float4			mExtraWeights;
	float4			mExtraBonesIndices;
};

static_assert( sizeof( ReFileFatVertex ) == 144, "Wrong fat vertex size." );

class ReFileFace
{
public:
	ReFileFace();

public:
	int mIndices[3];
	int mId;
};


// ----------------------------------------------------------------------------
// ReFileMaterial
// ----------------------------------------------------------------------------
class ReFileMaterial : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileMaterial( const ReFileMaterial& );
	ReFileMaterial& operator = ( const ReFileMaterial& );

public:
	ReFileMaterial();
	ReFileMaterial( const ReFileString& nam, const ReFileString& diff, const ReFileString& nor, const ReFileString& blend );
	~ReFileMaterial();

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int				Type() const override {return 'mtrl';}
	void					setMaterialData( const ReFileMaterial& mat );
	void					setMaterialData( const ReFileString& matName, const ReFileString& diffPath, const ReFileString& normalPath, const ReFileString& blendMaskPath );
	const ReFileString&		getMaterialName()	const { return mMaterialName; }
	const ReFileString&		getDiffuse()		const { return mDiffuse; }
	const ReFileString&		getNormal()			const { return mNormal; }
	const ReFileString&		getBlend()			const { return mBlend; }

private:
	ReFileString	mMaterialName;
	ReFileString	mDiffuse;
	ReFileString	mNormal;
	ReFileString	mBlend;
};


// ----------------------------------------------------------------------------
// ReFileMeshChunk
// ----------------------------------------------------------------------------
class ReFileMeshChunk : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileMeshChunk( const ReFileMeshChunk& );
	ReFileMeshChunk& operator = ( const ReFileMeshChunk& );

public:
	ReFileMeshChunk();
	ReFileMeshChunk( int numv, int numf, const ReFileMaterial& mat );
	~ReFileMeshChunk();

public:
	virtual int				Type() const override {return RE_NODE_MESH_CHUNK;}

	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void					setMeshChunkData( int numv, int numf, const ReFileMaterial& mat );
	void					copy( ReFileMeshChunk* ch );
	ReFileMeshChunk*		slice_chunk( int* barr, int id );
	ReFileMeshChunk*		copy();
	int						getNumVertices() const { return mNumVertices; }
	int						getNumFaces() const { return mNumFaces; }
	ReFileMaterial&			getMaterial() { return mMaterial; }
	const ReFileMaterial&	getMaterial() const { return mMaterial; }
	void					Fill( const ReFileMeshChunk& ch );

public: //TODO change to private
	ReFileFatVertex*		mVertices;
	ReFileFace*				mFaces;

private:
	ReFileMaterial			mMaterial;
	int						mNumVertices;
	int						mNumFaces;
};


// ----------------------------------------------------------------------------
// ReFileMesh
// ----------------------------------------------------------------------------
class ReFileMesh : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileMesh& operator = ( const ReFileMesh& );
	//copy ctor
	ReFileMesh( const ReFileMesh& );

public:
	ReFileMesh();
	ReFileMesh( ReFileMesh* m );
	~ReFileMesh();

	//move constructor
	ReFileMesh( ReFileMesh&& );

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	virtual int				Type() const override {return RE_NODE_MESH;}
	void					set( const ReFileString& nam, int numc, int numr );

	ReFileRigmatrix&		getRigMatrix(int idx){ return mRigMatrices[idx]; }
	const ReFileRigmatrix&	getRigMatrix(int idx) const { return mRigMatrices[idx]; }

	ReFileMeshChunk&		getMeshChunk(int idx){ return mMeshChunks[idx]; }
	const ReFileMeshChunk&	getMeshChunk(int idx) const { return mMeshChunks[idx]; }

	int						getNumChunks() const { return mNumChunks; }
	int						getNumRigMatrices() const { return mNumRigMatrices; }
	const ReFileString&		getMeshName() const { return mMeshName; }
	void					setMeshName( const ReFileString& name ){ mMeshName.set(name.getData()); }
	int						getVertexSplitCount() const { return mNumVertsToSplit; }

private:
	void					divideMeshChunk();
	void					split();

public:		//TODO: to priv
	ReFileRigmatrix*		mRigMatrices;
	ReFileMeshChunk*		mMeshChunks;
	float					mTransformMatrix[RIGMATRIXSIZE]; //TODO

private:
	ReFileString			mMeshName;
	int						mNumRigMatrices;
	int						mNumChunks;
	int						mNumVertsToSplit;
};
