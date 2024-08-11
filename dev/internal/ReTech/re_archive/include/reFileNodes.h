#pragma once

#include <windows.h>
#include <vector>
#include <set>
#include <map>
#include "reFileBaseNode.h"
#include "..\..\re_math\include\reMath.h"

#include "reFace.h"
#include "reFileAnimation.h"
#include "reFileArchive.h"
#include "reFileArea.h"
#include "reFileBitmap.h"
#include "reFileCollisionBox.h"
#include "reFileCollisionCapsule.h"
#include "reFileCollisionConvex.h"
#include "reFileCollisionSphere.h"
#include "reFileCurve.h"
#include "reFileDyngNodes.h"
#include "reFileFilter.h"
#include "reFileMesh.h"
#include "reFileMixer.h"
#include "reFileNodes.h"
#include "reFilePath.h"
#include "reFilePose.h"
#include "reFilePreset.h"
#include "reFileSkeleton.h"
#include "reFileTrajectory.h"
#include "reFileTriMesh.h"


#define VERTEX_USERDATA_SIZE	256


#ifdef __GNUC__
#	define RE_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#	define RE_DEPRECATED __declspec(deprecated)
#else
#	pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#	define RE_DEPRECATED
#endif

// deprecated. dont use. only for compatibiliy purposes
class ReFileHeader : public ReFileBaseNode
{
public:
	ReFileHeader() {};
	~ReFileHeader() {};

public:
	virtual int				Type() const override { return 'hedr'; }

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

	ReFileString			mAuthor;
	ReFileString			mSource;
};


class ReFileHeader2 : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileHeader2( const ReFileHeader2& );
	ReFileHeader2& operator = ( const ReFileHeader2& );

public:
	ReFileHeader2( bool isLegacy = false );
	ReFileHeader2( ReFileHeader2* hdr );
	ReFileHeader2(	const ReFileString& author,
		const ReFileString& exportPath,
		const ReFileString& srcFilePath,
		const ReFileString& extra1,
		const ReFileString& extra2,
		const ReFileString& extra3,
		const ReFileString& extra4 );
	~ReFileHeader2() {};

public:
	virtual void			write(ReFileBuffer* buf) override;
	virtual void			read(ReFileBuffer* buf) override;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG* g) override;
	virtual void			readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	void Fill( const ReFileHeader2& o );

	void set(	const ReFileString& aut,
		const ReFileString& src,
		const ReFileString& modellerPath, //max/maya/mobu path
		const ReFileString& extra1,
		const ReFileString& extra2,
		const ReFileString& extra3,
		const ReFileString& extra4
		);

	virtual int					Type() const override { return 'hed2'; }
	void						setAuthor( const ReFileString& authorName ){ mAuthor.set( authorName.getData() ); }
	void						setSourceFilePath( const ReFileString& s ){ mSourceFilePath.set( s.getData() ); }
	const ReFileString&			getAuthor() const { return mAuthor; }
	const ReFileString&			getExportFilePath() const { return mExportFilePath; }
	const ReFileString&			getSourceFilePath() const { return mSourceFilePath; }

	// new feature. simple file version stored in extra5 field
	const ReFileString&			getReFileVersion() const { return getExtra5(); }

	const ReFileString&			getExtra1() const { return mExtra1; }
	const ReFileString&			getExtra2() const { return mExtra2; }
	const ReFileString&			getExtra3() const { return mExtra3; }
	const ReFileString&			getExtra4() const { return mExtra4; }

private:
	const ReFileString&			getExtra5() const { return mExtra5; }

private:
	ReFileString	mExportFilePath;
	ReFileString	mSourceFilePath;
	ReFileString	mAuthor;

	ReFileString	mExtra1;
	ReFileString	mExtra2;
	ReFileString	mExtra3;
	ReFileString	mExtra4;

	// This field is reserved for file version etc. Do not expose any accessor for this
	ReFileString	mExtra5;
};

//////////////////////////////////////////////////////////////////////////
// this is exposed to python
struct vertex
{
	vertex(
		float3 p	=	float3(),
		float3 n	=	float3(0.0f,0.0f,1.0f),
		float3 t	=	float3(1.0f,0.0f,0.0f),
		float3 b	=	float3(0.0f,1.0f,0.0f),
		float4 c	=	float4(0.f,0.f,0.f,1.0f),
		float4 sw	=	float4(),
		float4 sb	=	float4(),
		float2 u1	=	float2(),
		float2 u2	=	float2(),
		int i		=	-1
		)
	{
		pos	= p;
		nor	= n;
		tan	= t;
		bin	= b;
		col	= c;
		wei	= sw;
		bon	= sb;
		uv1	= u1;
		uv2	= u2;
		index = i;
	}

	float3	pos;
	float3	nor;
	float3	tan;
	float3	bin;
	float4	col;
	float4	wei;
	float4	bon;
	float2	uv1;
	float2	uv2;
	int		index;
};

namespace ReTessellator
{
	class exp_vertex
	{
	public:
		exp_vertex();
		exp_vertex( const vertex& v );

	public:
		float3	pos;
		float3	nor;
		float3	tan;
		float3	bin;
		float4	col;
		float4	wei;
		float4	bon;
		float2	uv1;
		float2	uv2;
		// this one is ok. we need it.
		int		index;
	};

	class exp_face
	{
	public:
		exp_face();

	public:
		exp_vertex*		ind[3];
		int				mMaterialId;
	};

	class exp_int3
	{
	public:
		int x;
		int y;
		int z;
	};

	bool operator < ( const exp_vertex & a, const exp_vertex & b );

	class exp_vertex_array
	{
	public:
		exp_vertex_array() {};

	public:
		exp_vertex*				insert( const exp_vertex & v );
		void					to_vector( std::vector<exp_vertex*> & arr );

	private:
		std::set<exp_vertex>	mVertices;
	};

	class exp_face_array
	{
	public:
		exp_face_array() {};

	public:
		exp_face*		AddFace( const exp_face & v );
		void			to_vector( std::vector<exp_int3> & arr );

	private:
		std::vector<exp_face> mFaces;
	};

	class exp_chunk
	{
	public:
		exp_chunk(){};

	public:
		// #todo #horzel shouldn't be exposed at all
		void AddFace( const exp_vertex & vert0, const exp_vertex & vert1, const exp_vertex & vert2 );
		void output( std::vector<exp_vertex*> & varr, std::vector<exp_int3> & farr );

	private:
		exp_vertex_array	mVertices;
		exp_face_array		mFaces;
	};

	class exp_mesh
	{
	public:
		exp_mesh(){};

	public:
		void	AddFaceToGraphicalChunk( const exp_vertex& vert0, const exp_vertex& vert1, const exp_vertex& vert2, int id );
		void	output( std::vector<exp_vertex*> & varr, std::vector<exp_int3> & farr, int id );
		void	output_chunks_ids( std::vector<int>& arr );
		int		getGraphicalChunksSize(){ return (int)mGraphicalChunks.size(); }

	public:
		std::map< int, exp_chunk > mGraphicalChunks;
	};
}
