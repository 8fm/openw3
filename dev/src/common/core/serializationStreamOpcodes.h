#pragma once

// EVERY CHANGE TO THIS FILE REQUIRES RECOOKING

//          ,---,_          ,
//           _>   `'-.  .--'/
//      .--'` ._      `/   <_
//       >,-' ._'.. ..__ . ' '-.
//    .-'   .'`         `'.     '.
//     >   / >`-.     .-'< \ , '._\
//    /    ; '-._>   <_.-' ;  '._>
//    `>  ,/  /___\ /___\  \_  /
//    `.-|(|  \o_/  \o_/   |)|`
//        \;        \      ;/
//          \  .-,   )-.  /
//           /`  .'-'.  `\
//          ;_.-`.___.'-.;
//                

// Serialization stream is a more performance-wise approach to saving and loading of the object's data
// Instead of using naive type->Serialize we use more "down to the metal" approach that is way faster in generic cases.
// The idea is that instead of saving simple list of properties that we need to manually resolve one by one during loading
// we save more versatile serialization stream:
//
// Serialization stream consists of interleaved VM code and data. Code is represented by opcodes, and some opcodes can carry additional data.
// That data is sometimes related to the opcode (like the offset to property) or is the actual object's data.
// 
// The stream is interpreted by deserializer that is capable of using it to restore internal object's state
//
// Example stream:
//   PushProp "CMyClass::test"
//   Data32 (value: 100)
//   Pop
//
// Example stream for structures:
//   PushProp "CMyClass::bbox"
//     PushProp "Box::min"
//       PushProp "Vector::X"
//        Data32 (data: float -1.0f)
//       Pop
//       PushProp "Vector::Y"
//        Data32 (data: float -2.0f)
//       Pop
//     Pop
//     PushProp "Box::max"
//       PushProp "Vector::X"
//        Data32 (data: float -1.0f)
//       Pop
//     Pop
//   Pop
//
//  Example stream for arrays:
//    PushProp "CMyClass::test_array"
//      PushArray (3, "array:Float")
//        Data32 (data: float 1.0f)
//        Advance
//        Data32 (data: float 1.0f)
//        Advance
//        Data32 (data: float 1.0f)
//      Pop
//    Pop
//
// All of the properties using in the serialization stream are mapped to indices and the indices are stored instead of the property names.
// At runtime when the file is loaded all of the used properties are resolved and actual offset is determined for every property.
//
// Missing properties are not resolved and data for them is skipped.
//

/// Serialization stream opcode
enum ESerializationOpcode
{
	// system
	eSerializationOpcode_Pop               = 1,		// pop head pointer from the stack
	eSerializationOpcode_Advance           = 2,		// advance to next element (arrays only, requires proper stride to be stored on stack)
	eSerializationOpcode_PushCurrent       = 3,		// push current offset on the stack, again
	eSerializationOpcode_PushProp          = 4,		// offset head pointer based on specific property and push it on the stack
	eSerializationOpcode_PushOffset        = 5,		// offset head pointer by given amount and push it on the stack
	eSerializationOpcode_PushScript        = 6,		// get the script data buffer for head object and push it on the stack
	eSerializationOpcode_PushDynArray8     = 7,		// resize the array at given pointer to given size
	eSerializationOpcode_PushDynArray32    = 8,		// resize the array at given pointer to given size 
	eSerializationOpcode_PushStaticArray8  = 9,		// resize the array at given pointer to given size
	eSerializationOpcode_PushStaticArray32 = 10,	// resize the array at given pointer to given size 
	eSerializationOpcode_PushNativeArray8  = 11,	// array of compile time defined size
	eSerializationOpcode_PushNativeArray32 = 12,	// array of compile time defined size

	// simple types
	eSerializationOpcode_DataTrue    = 30,		// store true at the current location
	eSerializationOpcode_DataFalse   = 31,		// store false at the current location
	eSerializationOpcode_Data8       = 32,		// move 1 byte from serialization stream to the current location
	eSerializationOpcode_Data16      = 33,		// move 2 bytes from serialization stream to the current location
	eSerializationOpcode_Data32      = 34,		// move 4 bytes from serialization stream to the current location
	eSerializationOpcode_Data64      = 35,		// move 8 bytes from serialization stream to the current location
	eSerializationOpcode_Zero8       = 40,		// zero 1 byte at the current location
	eSerializationOpcode_Zero16      = 41,		// zero 2 bytes at the current location
	eSerializationOpcode_Zero32      = 42,		// zero 4 bytes at the current location
	eSerializationOpcode_Zero64      = 43,		// zero 8 bytes at the current location
	eSerializationOpcode_RawBlock    = 44,		// block of raw data (array of fundamental types)

	// special embedded types
	eSerializationOpcode_StringEmpty     = 50,  // store String::EMPTY at the current location
	eSerializationOpcode_StringUnicode   = 51,  // length of the string is a byte
	eSerializationOpcode_StringAnsi      = 52,  // length of the string is a byte
	eSerializationOpcode_StringUnicode32 = 53,  // length of the string is a Uint32
	eSerializationOpcode_StringAnsi32    = 54,  // length of the string is a Uint32
	eSerializationOpcode_Name            = 55,  // generic name (mapped)
	eSerializationOpcode_Pointer         = 56,  // pointer
	eSerializationOpcode_Handle          = 57,  // handle
	eSerializationOpcode_SoftHandle      = 58,  // soft handle
	eSerializationOpcode_Enum1           = 59,  // 1 byte enum value
	eSerializationOpcode_Enum2           = 60,  // 2 bytes enum value
	eSerializationOpcode_Enum4           = 61,  // 4 bytes enum value
	eSerializationOpcode_BitField1       = 62,  // 1 byte bitfield value
	eSerializationOpcode_BitField2       = 63,  // 2 bytes bitfield value
	eSerializationOpcode_BitField4       = 64,  // 4 bytes bitfield value
	eSerializationOpcode_InlinedStruct   = 65,  // inlined structure data

	// generic type
	eSerializationOpcode_Generic         = 100,  // generic data

	// end of stream
	eSerializationOpcode_Start=0xBB,
	eSerializationOpcode_End=0xCC,
};