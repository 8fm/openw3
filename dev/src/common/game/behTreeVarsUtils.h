#pragma once

namespace BehTreeVarsUtils
{
	Bool OnPropertyTypeMismatch( IScriptable* obj, CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// On property type mismatch helper for pointer types refactoring
	// NOTICE: Its temporary place for this code, that might also be temporary if we would one day support such refactoring by the engine.
	Bool ConvertPointerTypes( IScriptable* container, const CProperty* prop, const IRTTIType* sourceType, const void* sourceData );

	// Can property be tranlated from one type to other
	Bool IsPropertyTranslatable( const IRTTIType* typeDefinition, const IRTTIType* typeInstance );

	// Translate definition property into instance property
	void Translate( const IScriptable* definition, IScriptable* instance, const CProperty* propDefinition, const CProperty* propInstance, CBehTreeSpawnContext& context );
};