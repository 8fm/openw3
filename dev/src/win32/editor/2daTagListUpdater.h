#pragma once

#include "tagListUpdater.h"

class CEd2daTagListUpdater : public CTagListProvider
{
protected:

	TDynArray< String > m_allowedTags;
	String m_groupName;

public:

	CEd2daTagListUpdater( const String& filename, const String& columnName );
	virtual String GetTagGroupName() const { return m_groupName; }
	virtual Bool IsTagAllowed( const String& tag );

protected:

	virtual Int32 DoGetTags( STagNode &node, String& filter );

};

class CEdConversationTagListUpdater : public CEd2daTagListUpdater
{
public:

	CEdConversationTagListUpdater();
	virtual String GetTagGroupName() const { return TXT( "Conversation tags" ); }
};
