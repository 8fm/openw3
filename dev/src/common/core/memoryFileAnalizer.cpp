
#include "build.h"
#include "memoryFileAnalizer.h"
#include "class.h"
#include "dependencySaver.h"

Uint32 CObjectMemoryAnalizer::CalcObjectSize( CObject* object )
{
	TDynArray< CObject* > objects;
	objects.PushBack( object );

	return CalcObjectsSize( objects );
}

Uint32 CObjectMemoryAnalizer::CalcObjectSize( CClass* objClass, void* data )
{
	CMemoryFileSizeAnalizer anal;

	objClass->Serialize( anal, data );

	return static_cast< Uint32 >( anal.GetSize() );
}

Uint32 CObjectMemoryAnalizer::CalcObjectsSize( const TDynArray< CObject* >& objects )
{
	CMemoryFileSizeAnalizer anal;

	CDependencySaver saver( anal, NULL );
	DependencySavingContext context( objects );
	context.m_saveTransient = true;
	context.m_saveReferenced = true;
	saver.SaveObjects( context );

	return static_cast< Uint32 >( anal.GetSize() );
}

Uint64 CObjectHashGenerator::CalcObjectHash( CObject* object )
{
	TDynArray< CObject* > objects;
	objects.PushBack( object );

	return CalcObjectsHash( objects );
}

Uint64 CObjectHashGenerator::CalcObjectsHash( const TDynArray< CObject* >& objects )
{
	CFileHashGenerator generator;

	CDependencySaver saver( generator, NULL );
	DependencySavingContext context( objects );
	saver.SaveObjects( context );

	return generator.GetHash();
}
