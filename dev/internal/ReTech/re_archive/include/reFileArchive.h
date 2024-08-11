#pragma once
#include <stdio.h>
#include "../../re_archive/include/reFileBaseNode.h"
#include "../../re_archive/include/reFileNodes.h"
#include "reFileVersion.h"

#define DEFAULTARCHIVESIZE ( 1024 * 1024 * 5 )

class ReFileHeader2;

struct SNodeType
{
	ReFileBaseNode*		mNode;
	int					mType;

	SNodeType()
		: mNode( nullptr )
		, mType( -1 )
	{
	}
};

class ReFileArchive
{
public:
	ReFileArchive();
	~ReFileArchive();

public:
	int							Size(){ return (int)mBuffers.size(); }
	ReFileBuffer*				Append(ReFileBuffer* b);
	void						Save( const ReFileString& pat );
	void						Load( const ReFileString& pat );
	void						setCurrentVersion( const ReFileString& version );

public:
	inline ReFileBuffer*		operator[](int i){ return mBuffers[i]; }
	static const ReFileString	getReFileSystemVersion(){ return ReFileString(RE_FILE_SYSTEM_CURRENT_VERSION); }

private:
	std::vector<ReFileBuffer*>	mBuffers;

};

class ReFile
{
public:
	ReFile();
	ReFile( ReFile&& other );
	~ReFile();

public:
	inline operator bool () const
	{
		return mFile && mHeaderNode && !mNodes.empty() && !mHeaders.empty(); // mheaders to delete
	}

public:
	FILE*								mFile;

public: // #todo to private
	ReFileHeader2*						mHeaderNode;
	std::vector<SNodeType>				mNodes;

	std::vector<ReFileArchiveHeader>	mHeaders;
	bool								mContainsOldHeader;
};

namespace ReFileLoader
{
	void		Read( FILE* f, const ReFileArchiveHeader& hdr, ReFileBaseNode& obj, const ReFileString* fileVersion = nullptr );
	bool		Read( const ReFile& hdr, ReFileBaseNode& obj, const ReFileString* fileVersion = nullptr );
	bool		ReadHeaders( ReFile& file );
	ReFile		OpenFile( const char* filename );
};
