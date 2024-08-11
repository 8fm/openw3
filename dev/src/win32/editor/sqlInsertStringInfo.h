// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Inserts string info into String DB.

Inserts row into STRING_INFO table.
*/
class SqlInsertStringInfo
{
public:
	SqlInsertStringInfo();
	~SqlInsertStringInfo();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( const StringInfoRow& stringInfoRow );

private:
	SqlInsertStringInfo( const SqlInsertStringInfo& );					// cctor - not defined
	SqlInsertStringInfo& operator=( const SqlInsertStringInfo& );		// op= - not defined

	SQLHSTMT m_stmt;													// Statement handle.

	SQLUINTEGER m_paramStringId;										// Bound to statement parameter containing string id.

	static const SQLULEN m_resourceColumnSize = 4000;					// Size (in chars) of RESOURCE column as defined in String DB.
	SQLLEN m_paramResourceLenInd;										// Length/indicator buffer for m_paramResource.
	SQLWCHAR m_paramResource[ m_resourceColumnSize + 1 ];				// Bound to statement parameter containing resource name. + 1 for null.

	static const SQLULEN m_propertyNameColumnSize = 4000;				// Size (in chars) of PROPERTY_NAME column as defined in String DB.
	SQLLEN m_paramPropertyNameLenInd;									// Length/indicator buffer for m_paramProperty.
	SQLWCHAR m_paramPropertyName[ m_propertyNameColumnSize + 1 ];		// Bound to statement parameter containing property name. + 1 for null.

	static const SQLULEN m_voiceoverNameColumnSize = 32;				// Size (in chars) of VOICEOVER_NAME column as defined in String DB.
	SQLLEN m_paramVoiceoverNameLenInd;									// Length/indicator buffer for m_paramVoiceover.
	SQLWCHAR m_paramVoiceoverName[ m_voiceoverNameColumnSize + 1 ];		// Bound to statement parameter containing voiceover name. + 1 for null.

	static const SQLULEN m_stringKeyColumnSize = 128;					// Size (in chars) of STRING_KEY column as defined in String DB.
	SQLLEN m_paramStringKeyLenInd;										// Length/indicator buffer for m_paramStringKey.
	SQLWCHAR m_paramStringKey[ m_stringKeyColumnSize + 1 ];				// Bound to statement parameter containing string key. + 1 for null.
};
