#pragma once

/// Resource linker dependency loader
class CResVerifierDependencyLoader : public IDependencyLinker
{
public:
	CResVerifierDependencyLoader( IFile& file );
	~CResVerifierDependencyLoader();

	// Check if file need update - has embedded resource with .link file
	Bool GetImports( TDynArray< String >& importFiles );

private:
	// map
	CObject *MapObject( Int16 index ); 
	CName MapName( Uint16 index ); 

public:
	// Dependency serialization interface
	virtual IFile &operator<<( CObject *&object );
	virtual IFile &operator<<( CName& name );
};