/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyBuilder.h"

CProperty* RedPropertyBuilder::CreateProperty
(
	CClass* parentClass,
	size_t offset,
	const CName& name,
	const CName& typeName,
	const String& hint,
	Uint32 flags,
	Float,
	Float,
	const String& customEditor,
	Bool customEditorArray
)
{
	IRTTIType* type = SRTTI::GetInstance().FindType( typeName );
	if ( !type )
	{
		WARN_CORE
		(
			TXT( "Trying to create property %s in class %s - type %s unknown!" ),
			name.AsString().AsChar(),
			parentClass ? parentClass->GetName().AsString().AsChar() : TXT( "<unknown>" ),
			typeName.AsString().AsChar()
		);

		return NULL;
	}

	return new CProperty
	(
		type,
		parentClass,
		static_cast< Uint32 >( offset ),
		name,
		hint,
		flags | PF_Native,
		customEditor,
		customEditorArray
	);
}
