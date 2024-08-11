#include "build.h"

#include "scriptInvoker.h"

#include "testFramework.h"
#include "../core/scriptingSystem.h"
#include "debugConsole.h"

CScriptInvoker::TObjectMap CScriptInvoker::m_objects;

Bool CScriptInvoker::Parse( const String &commandLine )
{
	// Try script functions
	if ( GDebugConsole )
	{
		GDebugConsole->SetAcceptMessages( true );
	}
	const Bool isExecFunctionCalled = GScriptingSystem->CallGlobalExecFunction( commandLine, false );
	if ( GDebugConsole )
	{
		GDebugConsole->SetAcceptMessages( false );
	}

	if ( isExecFunctionCalled )
	{
		return true;
	}

#ifndef NO_TEST_FRAMEWORK
	if ( STestFramework::GetInstance().ParseCommandline( commandLine ) )
	{
		STestFramework::GetInstance().OnStart( nullptr );
		return true;
	}
#endif

/*	// First find '=' character
	const Int32 equalIndex = commandLine.GetIndex(TXT('='));
	if ( equalIndex == -1 )
	{
		// Get object represent by command line string		
		void *data = NULL;
		IRTTIType *type = NULL;
		CDebugMessage message;
		if ( !ParsePropertyPath(commandLine, data, type, message ))
		{
			return message;
		}

		// And get it's value
		String value = GetValue(type, data);
		if ( value.Empty() )
		{
			return CDebugMessage( String::Printf( TXT("Cannot obtain value from \"%s\""), commandLine.AsChar()), DMT_Warning );
		}
		else
		{
			return CDebugMessage(value);
		}
	}

	// Set expression
	else if ( equalIndex > 0 )
	{
		String leftSide = commandLine.LeftString(equalIndex);
		String rightSide = commandLine.RightString(commandLine.GetLength() - equalIndex - 1);
		leftSide.Trim();
		rightSide.Trim();

		// Get object represented by left part of command line ...
		void *data = NULL;
		IRTTIType *type = NULL;
		CDebugMessage message;
		if ( !ParsePropertyPath( leftSide, data, type, message ) )
		{
			return message;
		}

		// ... and set it's value to the value represented by right side of the command line
		if ( !SetValue( type, data, rightSide ))
		{
			return CDebugMessage(String::Printf(TXT("Cannot set value \"%s\" to the object \"%s\""), rightSide.AsChar(), leftSide.AsChar()), DMT_Warning);
		}
		else	
		{
			return CDebugMessage();
		}
	}

	// Invalid 	
	return CDebugMessage(TXT("Unable to parse command line."), DMT_Error);*/

	// Not parsed
	return false;
}

void CScriptInvoker::GetAvailableProperties(const String &text, int pos, TDynArray<CProperty *> &properties)
{
	// Clear property collection
	properties.Clear();

	// Validate position
	if (pos < 0 || pos > (Int32)text.GetLength())
	{
		return;
	}

	// Limit right side of propertyPath
	String propertyPath = text.LeftString(pos);

	// Limit left side of propertyPath
	for (Int32 i = (Int32)propertyPath.GetLength() - 1; i >= 0; --i)
	{
		if (!IsPropertyPathCharacter(propertyPath[i]))
		{
			propertyPath = propertyPath.RightString(propertyPath.GetLength() - i - 1);
			break;
		}
	}

	// Return if limited propertyPath is empty
	if (propertyPath.Empty())
	{
		return;
	}

	// Split propertyPath to property items
	TDynArray<String> items;
	SplitToPropertyItems(propertyPath, items);

	// Find class which we seek properties for
	void *data = NULL;
	IRTTIType *type = NULL;
	String filter;
	Bool emptyItem = false;
	CDebugMessage message;
	for (Uint32 i = 0; i < items.Size(); ++i)
	{
		if (!GetObjectByName(items[i], data, type, message))
		{
			emptyItem = items[i].Empty();
			filter = items[i];
			break;
		}
	}

	// Get properties
	Bool isDotAtTheEnd = (propertyPath.AsChar()[propertyPath.GetLength() - 1] == TXT('.'));
	if (!emptyItem && (isDotAtTheEnd || !filter.Empty()))
	{
		if (data && type && type->GetType() == RT_Class)
		{
			CClass *classPtr = (CClass *)type;
			classPtr->GetProperties(properties);
		}

		// Filter properties
		TDynArray<CProperty *>::iterator it;
		for (it = properties.Begin(); it < properties.End(); )
		{
			CProperty *prop = *it;
			if (!prop->GetName().AsString().BeginsWith(filter))
				properties.Erase(it);
			else
				++it;
		}
	}
}

Bool CScriptInvoker::ParsePropertyPath(const String &propertyPath, void *&data, IRTTIType *&type, CDebugMessage &message)
{
	// Set default values
	data = NULL;
	type = NULL;

	// Check if propertyPath is empty
	if (propertyPath.Empty())
	{
		message = CDebugMessage(TXT("Empty string."), DMT_Error);
		return false;
	}

	// Split propertyPath to property items
	TDynArray<String> items;
	SplitToPropertyItems(propertyPath, items);

	// Find final object
	for (Uint32 i = 0; i < items.Size(); ++i)
	{
		if (!GetObjectByName(items[i], data, type, message))
		{
			return false;
		}
	}

	return true;
}

Bool CScriptInvoker::GetObjectByName(const String &item, void *&data, IRTTIType *&type, CDebugMessage &message)
{
	if (data == NULL || type == NULL)
	{
		if (data != NULL || type != NULL)
		{
			message = CDebugMessage(String::Printf(TXT("Internal error parsing item %s - object and class must not be empty."), item.AsChar()), DMT_Error);
			return false;
		}

		ObjectData od;

		/*if (item == TXT("game"))
		{
			data = (void *)GGame;
			type = CGame::GetStaticClass();
			message = CDebugMessage();
			return true;
		}
		else*/ if( m_objects.Find( item, od ) )
		{			
			data = (void *)od.m_object;
			type = od.m_staticClass;
			message = CDebugMessage();
			return true;
		}
		else
		{
			//message = CDebugMessage(TXT("The expression should start with \"game\"."), DMT_Error);

			String text=TXT("The expression should start with:\n");
			TObjectMap::iterator iter;
			for( iter=m_objects.Begin(); iter!=m_objects.End(); ++iter )
			{				
				text += iter->m_first + TXT("\n");
			}

			message = CDebugMessage( text, DMT_Error );

			return false;
		}
	}

	ASSERT(data);
	ASSERT(type);

	if (type->GetType() == RT_Class)
	{
		CClass *classPtr = (CClass *)type;
		CProperty *propertyPtr = classPtr->FindProperty(CName(item));
		if (!propertyPtr)
		{
			message = CDebugMessage(String::Printf(TXT("Property \"%s\" not found."), item.AsChar()), DMT_Error);
			return false;
		}

		data = propertyPtr->GetOffsetPtr( data );
		type = propertyPtr->GetType();

		// Change type of pointer to object into a type of class
		while (type && type->GetType() == RT_Pointer)
		{
			CRTTIPointerType *pointer = (CRTTIPointerType *)type;
			data = pointer->GetPointed(data);
			type = pointer->GetPointedType();
		}

		return true;
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();

		void *arrayData = data;//OffsetPtr(data, propertyPtr->GetOffset());
		Int32 arrayIndex;
		Red::System::StringToInt( arrayIndex, item.AsChar(), nullptr, Red::System::BaseTen );
		const Uint32 size = arrayType->GetArraySize(arrayData);
		if (arrayIndex < 0 || arrayIndex >= (Int32)size)
		{
			message = CDebugMessage(TXT("Index out of range."), DMT_Error);
			return false;
		}

		data = arrayType->GetArrayElement(arrayData, arrayIndex);
		type = innerType;

		// Change type of pointer to object into a type of class
		while (type && type->GetType() == RT_Pointer)
		{
			CRTTIPointerType *pointer = (CRTTIPointerType *)type;
			data = pointer->GetPointed(data);
			type = pointer->GetPointedType();
		}

		return true;
	}
	else if (type->GetType() == RT_Simple || type->GetType() == RT_Enum || type->GetType() == RT_Pointer || type->GetType() == RT_Fundamental )
	{
		message = CDebugMessage(String::Printf(TXT("Wrong type of property \"%s\". Class or Array type expected."), item.AsChar()), DMT_Error);
		return false;
	}
	else
	{
		message = CDebugMessage(String::Printf(TXT("Unknown type of property \"%s\"."), item.AsChar()), DMT_Error);
		return false;
	}
}

String CScriptInvoker::GetValue(IRTTIType *type, void *data)
{
	String result = String::EMPTY;
	if (type->GetType() == RT_Simple || type->GetType() == RT_Enum || type->GetType() == RT_Fundamental)
	{
		type->ToString(data, result);
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		const Uint32 size = arrayType->GetArraySize(data);

		// Get info about array
		result = String::Printf( TXT( "Array of %d element%s of type %s." ), size, ( size == 1 ) ? TXT( "" ) : TXT( "s" ), innerType->GetName().AsString().AsChar() );

		// If elements within the array are simple types or enums then display content of the array
		if (innerType->GetType() == RT_Simple || innerType->GetType() == RT_Enum || type->GetType() == RT_Fundamental)
		{
			result += TXT("\n");
			for (Uint32 i = 0; i < size; ++i)
			{
				String value;
				void *element = arrayType->GetArrayElement(data, i);
				if (innerType->ToString(element, value))
					result += String::Printf(TXT("  [%d] %s\n"), i, value.AsChar());
				else
					result += String::Printf(TXT("  [%d] unknown\n"), i);
			}
		}
	}
	else
	{
		return GetObjectInfo(data, type);
	}

	return result;
}

Bool CScriptInvoker::SetValue(IRTTIType *type, void *data, const String &value)
{
	if (type->GetType() == RT_Simple || type->GetType() == RT_Enum || type->GetType() == RT_Fundamental)
	{
		return type->FromString(data, value);
	}

	return false;
}

void CScriptInvoker::SplitToPropertyItems(const String &propertyPath, TDynArray<String> &items)
{
	const String dotSeparator = TXT(".");
	propertyPath.Slice(items, dotSeparator);
	
	// Separate each array element into an array object and an array index
	for (Uint32 i = 0; i < items.Size(); ++i)
	{
		String item = items[i];
		const Int32 leftBracket = static_cast< Int32 >( item.GetIndex( TXT( '[' ) ) );
		const Int32 rightBracket = static_cast< Int32 >( item.GetIndex( TXT( ']' ) ) );
		if (leftBracket != -1 && leftBracket + 1 < rightBracket)
		{
			items[i] = item.LeftString(leftBracket);
			String arrayIndexString = item.LeftString(rightBracket).RightString(rightBracket - leftBracket - 1) + item.RightString(item.GetLength() - rightBracket - 1);
			items.Insert(i + 1, arrayIndexString);
		}
	}
}

String CScriptInvoker::GetObjectInfo(void *object, IRTTIType *type)
{
	String info = String::Printf(TXT("Address 0x%p     Type: %s"), object, type->GetName().AsString().AsChar());
	String properties = GetClassProperties(object, type);
	if (!properties.Empty())
	{
		return String::Printf(TXT("%s\n%s"), info.AsChar(), properties.AsChar());
	}

	return info;
}

String CScriptInvoker::GetClassProperties(void *object, IRTTIType *type)
{
	if (!type || type->GetType() != RT_Class)
	{
		return String::EMPTY;
	}

	CClass *classPtr = (CClass *)type;

	TDynArray<CProperty *> properties;
	classPtr->GetProperties(properties);
	String result = TXT("");
	for (Uint32 i = 0; i < properties.Size(); ++i)
	{
		result += String::Printf(TXT("%s : %s\n"), properties[i]->GetName().AsString().AsChar(), properties[i]->GetType()->GetName().AsString().AsChar());
	}

	return result;
}

Bool CScriptInvoker::IsPropertyPathCharacter(Char c)
{
	Bool isLetter = (c >= TXT('a') && c <= TXT('z')) || (c >= TXT('A') && c <= TXT('Z'));
	Bool isDigit = (c >= TXT('0') && c <= TXT('9'));
	if (isLetter || isDigit)
	{
		return true;
	}

	if (c == TXT('.') || c == TXT('[') || c == TXT(']') || c == TXT('(') || c == TXT(')'))
	{
		return true;
	}

	return false;
}
