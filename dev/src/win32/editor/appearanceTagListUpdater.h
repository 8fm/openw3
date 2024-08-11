#pragma once

#include "tagListUpdater.h"

class CEdAppearanceTagListUpdater : public CTagListProvider
{
public:

	CEdAppearanceTagListUpdater( CEntityTemplate *eTemplate );
	virtual String GetTagGroupName() const { return TXT("Appearances"); }
	Bool IsTagAllowed( const String& tag );

protected:

	virtual Int32 DoGetTags( STagNode &node, String& filter );

private:

	TDynArray<CName> m_allowedTags;
};