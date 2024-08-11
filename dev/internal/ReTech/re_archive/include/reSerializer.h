#pragma once

#include <vector>
#include "reFileBaseNode.h"
#include "reFileNodes.h"
#include "reFileArchive.h"


class ReContainer
{
public:
	void			AddNode( ReFileBaseNode* node );
	void			Save( const ReFileString& path );

private:
	ReFileHeader2*						mFileHeader;
	std::vector< ReFileBaseNode* >		mReNodes;
};