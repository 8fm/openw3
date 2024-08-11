/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "property.h"
#include "class.h"

namespace RedPropertyBuilder
{
	CProperty* CreateProperty
	(
		CClass*			parentClass,
		size_t			offset,
		const CName&	name,
		const CName&	typeName,
		const String&	hint,
		Uint32			flags,
		Float			minValue,
		Float			maxValue,
		const String&	customEditor,
		Bool			customEditorArray
	);

	template<typename T>
	static CProperty* CreateRangedProperty
		(
		CClass* parentClass,
		size_t offset,
		const CName& name,
		const CName& typeName,
		const String& hint,
		Uint32 flags,
		T* tag,
		Float minValue,
		Float maxValue,
		const String& customEditor,
		Bool customEditorArray
		)
	{
		IRTTIType* type = SRTTI::GetInstance().FindType( typeName );
		if ( !type )
		{
			WARN_CORE
			(
				TXT( "Trying to create ranged property %s in class %s - type %s unknown!" ),
				name.AsString().AsChar(),
				parentClass ? parentClass->GetName().AsString().AsChar() : TXT( "<unknown>" ),
				typeName.AsString().AsChar()
			);

			return NULL;
		}

		return new CRangedProperty<T>
		(
			type,
			parentClass,
			static_cast< Uint32 >( offset ),
			name,
			hint,
			flags | PF_Native,
			tag,
			(T)minValue,
			(T)maxValue,
			customEditor,
			customEditorArray
		);
	}
};
