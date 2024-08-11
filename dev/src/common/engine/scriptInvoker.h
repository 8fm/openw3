#pragma once

#include "debugMessage.h"

// This class is basically a kind of "exec()" to turn a string into a script function call
class CScriptInvoker
{
public:

	// Parses the commandLine string and returns the result of parsing
	static Bool Parse(const String &commandLine);
	static void GetAvailableProperties(const String &text, int pos, TDynArray<CProperty *> &properties);

private:
	struct ObjectData
	{
		ObjectData()
			: m_object( NULL )
			, m_staticClass( NULL )
		{
		}

		CObject*	m_object;
		CClass*		m_staticClass;
	};

	typedef THashMap< String, ObjectData > TObjectMap;
	static TObjectMap m_objects;

	static Bool ParsePropertyPath(const String &propertyPath, void *&data, IRTTIType *&type, CDebugMessage &message);
	static Bool GetObjectByName(const String &item, void *&objectPtr, IRTTIType *&classPtr, CDebugMessage &message);
	static String GetValue(IRTTIType *type, void *data);
	static Bool SetValue(IRTTIType *type, void *data, const String &value);
	static void SplitToPropertyItems(const String &propertyPath, TDynArray<String> &items);
	static String GetObjectInfo(void *object, IRTTIType *type);
	static String GetClassProperties(void *object, IRTTIType *type);
	static Bool IsPropertyPathCharacter(Char c);

};
