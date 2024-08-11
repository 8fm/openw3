#pragma once
#include "reFileVersion.h"
#include "stdio.h"
#include <cstring>

#include "reFileVersion.h"

#ifdef USE_HDF5_HEADERS
#include <H5Cpp.h>
#endif

#ifdef USE_HDF5_HEADERS
namespace H5 {
	class Group;
	class CommonFG;
};
#endif

#define RIGMATRIXSIZE			12

// Those integers are used to identify the serialized node types.
// They are stored inside ReFileBaseNode::mClassId
#define RE_NODE_DEFAULT			-1
#define RE_NODE_MESH			'mesh'
#define RE_NODE_MESH_CHUNK		'chnk'


class ReFileBuffer;

class IReSerializable
{
public:
	virtual ~IReSerializable(){}

public:
	// Exposed interface
	virtual void			write( ReFileBuffer* buf ) = 0;
	virtual void			read( ReFileBuffer* buf ) = 0;

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*		writeHdf5(H5::CommonFG * g) = 0;
	virtual void			readHdf5(H5::Group * g) = 0;
#endif //USE_HDF5_HEADERS
};


class ReFileString
{
public:
	ReFileString();
	ReFileString( const char* str );
	ReFileString( const ReFileString& s );
	~ReFileString();

public:
	const char*	getData() const;
	void		set( const char* str );
	void		write( class ReFileBuffer* buf );
	void		read( class ReFileBuffer* buf );
	int			getLength() const { return mLenght; }
	void		Reset();

private:
	char*		mData;
	int			mLenght;

	//OPERATORS
public:
	inline ReFileString& operator= (const ReFileString& str)
	{
		this->set( str.getData() );
		return *this;
	}

	inline bool IsGood() const { return mData != nullptr; }

	inline bool operator < ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) < 0;
	}

	inline bool operator <= ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) <= 0;
	}

	inline bool operator > ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) > 0;
	}

	inline bool operator >= ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) >= 0;
	}

	inline bool operator == ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) == 0;
	}

	inline bool operator != ( const ReFileString& other ) const
	{
		return IsGood() && strcmp( mData , other.mData ) != 0;
	}

	inline bool operator < ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) < 0;
	}

	inline bool operator <= ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) <= 0;
	}

	inline bool operator > ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) > 0;
	}

	inline bool operator >= ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) >= 0;
	}

	inline bool operator == ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) == 0;
	}

	inline bool operator != ( const char* other ) const
	{
		return IsGood() && strcmp( mData , other ) != 0;
	}
};

//----------------------------------------------------------------------------
// This class is base class for all ReFile nodes.
//----------------------------------------------------------------------------
class ReFileBaseNode : public IReSerializable
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileBaseNode( const ReFileBaseNode& );
	ReFileBaseNode& operator = ( const ReFileBaseNode& );

public:
	ReFileBaseNode();
	virtual ~ReFileBaseNode();

	//move ctor
	ReFileBaseNode( ReFileBaseNode&& node );

public:
	virtual int					Type() const { return RE_NODE_DEFAULT; }
	int							getNumChildren() const { return mNumChildren; }

	// IMPLEMENTED INTERFACE
public:
	virtual void				write( ReFileBuffer* buf ) override;
	virtual void				read( ReFileBuffer* buf ) override;

#ifdef USE_HDF5_HEADERS
	
	// Serializes this node as HDF5 node. Provided H5::CommonFG will be a parent for this node.
	// By default you should pass the H5File if you want this node to be serialized on the top of
	// the file hierarchy.
	virtual H5::Group*			writeHdf5(H5::CommonFG * g) override;

	// Tries to deserialise the values of this node from the H5::Group. Certain attributes will be looked for
	// but if they're not found the default values will be used.
	// Redundant (not recognized) attributes/datasets will be omitted.
	virtual void				readHdf5(H5::Group * g) override;

	// Reads the value of number from the HDF5 Attribute of type NATIVE_FLOAT to the variable.
	// If the group has no such attribute the value remains unchanged so you need to make sure it has
	// some default value before you call this method
	static void					readMetaAttribute(H5::Group* g, const ReFileString& name, float &value);

	// Reads the value of number from the HDF5 Attribute of type NATIVE_INT to the variable.
	static void					readMetaAttribute(H5::Group* g, const ReFileString& name, int &value);

	// Reads the value of string from the HDF5 Attribute of type C_S1 to the ReFileString variable.
	static void					readMetaAttribute(H5::Group* g, const ReFileString& name, ReFileString &value);

	// Saves the value number to the HDF5 Attribute of type NATIVE_FLOAT on the group/file passed as g.
	static void					writeMetaAttribute(H5::Group* g, const ReFileString& name, float value );

	// Saves the value number to the HDF5 Attribute of type NATIVE_INT on the group/file passed as g.
	static void					writeMetaAttribute(H5::Group* g, const ReFileString& name, int value);

	// Saves the value number to the HDF5 Attribute of type C_S1 (default char[]) on the group/file passed as g.
	static void					writeMetaAttribute(H5::Group* g, const ReFileString& name, const ReFileString& value);

	static void readXformMatrixDataset(H5::Group* g, float* value);

	// Saves the value of float list to the HDF5 DataSet. The dataset name will be "xform". The list must have a length
	// of RIGMATRIXSIZE (12 right now).
	static void writeXformMatrixDataset(H5::Group* g, float* value);

	// Reads the dataset values to the float array.
	// You have to make sure that the float array has a proper size before you attempt to load data from dataset.
	static void					readArrayDataset(H5::Group* g, H5std_string name, float* value);
	static void					readArrayDataset(H5::Group* g, H5std_string name, int* value);
	static void					readArrayDataset(H5::Group* g, H5std_string name, ReFileString* value);

	// Saves the list of values to the single line dataset.
	static void					writeArrayDataset(H5::Group* g, const ReFileString& name, int length, float* value);
	static void					writeArrayDataset(H5::Group* g, const ReFileString& name, int length, int* value);

	// Saves the list of strings into a single line dataset
	static void					writeArrayDataset(H5::Group* g, const ReFileString& name, int length, ReFileString* value);

	// Saves the list of values to the 2 dimension dataset (input is one dimension array).
	// The input data will be split into rows with row length passed as a rowstride value.
	static void					writeArrayDataset(H5::Group* g, const ReFileString& name, int length, int rowstride, float* value);
	static void					writeArrayDataset(H5::Group* g, const ReFileString& name, int length, int rowstride, int* value);
#endif //USE_HDF5_HEADERS

private:
	int							mNumChildren;
	ReFileBaseNode*				mChildren;
};

class ReFileBaseCollisionNode : public ReFileBaseNode
{
	// DO NOT CHANGE TO PUBLIC> HAS TO BE PRIVATE!!!!
private:
	ReFileBaseCollisionNode( const ReFileBaseCollisionNode& );
	ReFileBaseCollisionNode& operator = ( const ReFileBaseCollisionNode& );

public:
	ReFileBaseCollisionNode(){}
	ReFileBaseCollisionNode( ReFileBaseCollisionNode* coll );
	~ReFileBaseCollisionNode(){}

	//move ctor
	ReFileBaseCollisionNode( ReFileBaseCollisionNode&& baseColl );

#ifdef USE_HDF5_HEADERS
	virtual H5::Group*			writeHdf5(H5::CommonFG* g) override;
	virtual void				readHdf5(H5::Group * g) override;
#endif //USE_HDF5_HEADERS

public:
	const ReFileString&			getCollisionName() const { return mCollisionName; }
	void						setCollisionName( const ReFileString& name ){ mCollisionName.set( name.getData() ); }
	float*						getTransfromMatrix() { return mTransformMatrix; }

public:
	float						mTransformMatrix[RIGMATRIXSIZE];

protected:
	ReFileString				mCollisionName;
};

struct ReFileArchiveHeader
{
	int mType;
	int mId;
	int mOffset;
	int mSize;
};



class ReFileBuffer
{
	friend class ReFileArchive;
public:
	ReFileBuffer(int typ=0, int i=0);
	~ReFileBuffer();

public:
	void			write(void* data, int s);
	void			read(void* data, int s);
	void			seek(int s,int dir = 1);
	void			reserve(int num);
	void			read_file( ReFileArchiveHeader& hdr, FILE* f);

	int				getType() const {return mBufferType;}
	int				getId() const {return mId;}
	int				getMaxBufferSize() const {return mMaxBufferSize;}
	int				getBufferSize() const {return mBufferSize;}
	char*			getBuffer() const {return mCharBuffer;}
	void			clear();
	void			resize();
	void			setsize( int num );
	void			setCurrentVersion( const ReFileString& ver ){ mCurrentVersion = ver; }
	const ReFileString& getCurrentVersion() const { return mCurrentVersion; }
	bool			isVersionValid( const ReFileString& version );

	//IO part.save load etc
	void			Save( FILE* f, int offsetFromStart );
	void			Load();

private:
	int				mBufferType;
	int				mId;
	int				mBufferSize;
	int				mMaxBufferSize;
	char*			mCharBuffer;
	ReFileString	mCurrentVersion;
};