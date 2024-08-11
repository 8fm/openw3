/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Serialization stream data
class CSerializationStream
{
public:

	// Prepare serialization stream data that will capture difference between given object and the specified base
	// If the base is not specified the whole object state is staved
	// NOTE: This is still using the IFile because writing performance is NOT our concern ATM and it's just safer
	// NOTE: You need to know the RTTI layout of the object you want to save this way
	// NOTE: It still requires dependency linker to capture pointers between objects
	//
	// Parameters:
	//    writer - The output stream will be written here
	//    objectClass - RTTI info about the object you try to save (cannot be NULL)
	//    data - Object data (cannot be NULL)
	//    base - Base data to compare with (can be NULL but should not be)
	static void Build( IFile& writer, const CClass* objectClass, const void* data, const void* baseData );

	// Restore (deserialize) the object state
	// You need premapped data to do that
	static void Parse( IFile& reader, void* data, const CClass* objectClass );
};