// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

/*
Information needed to insert a row into STRING_INFO table.

Note that STRING_INFO tables from W2 and W3 are compatible.
*/
class StringInfoRow
{
public:
	Uint32 m_stringId;
	String m_resourceName;
	String m_propertyName;
	String m_voiceoverName;
	String m_stringKey;
};

/*
Information needed to insert a row into STRINGS table.

Note that STRINGS tables from W2 and W3 are compatible.

StringRow has no information for USER and MODIFICATION_DATE columns as these are automatically filled when inserting a row.
*/
class StringRow
{
public:
	Uint32 m_stringId;
	Uint32 m_langId;
	Uint32 m_version;
	String m_text;
};
