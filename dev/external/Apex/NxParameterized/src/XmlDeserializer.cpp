// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "PsUtilities.h"

#include "NxParameters.h"
#include "NxParameterizedTraits.h"
#include "NxTraitsInternal.h"

#include "XmlDeserializer.h"

using FAST_XML::getAttribute;

#define XML_WARNING(_format, ...) \
	NX_PARAM_TRAITS_WARNING(mTraits, "XML serializer: " _format, ##__VA_ARGS__)

namespace NxParameterized
{

	static physx::PxU32 ReadVersion(physx::PxI32 argc, const char **argv)
	{
		const char *versionText = getAttribute("version", argc, argv);

		// If there's no version, assume version is 0.0
		if( !versionText )
			return 0;

		//XML stores versions in "x.y"-format
		//FIXME: strtoul is unsafe

		const char *dot = strchr(versionText, '.');
		physx::PxU32 minor = dot ? strtoul(dot + 1, 0, 10) : 0;

		physx::PxU32 major = strtoul(versionText, 0, 10);

		return (major << 16) + minor;
	}

	static PX_INLINE bool isWhiteSpace(char c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
	}

	static PX_INLINE const char * skipWhiteSpace(const char *scan)
	{
		while ( isWhiteSpace(*scan) && *scan ) scan++;
		return *scan ? scan : 0;
	}

	static PX_FORCE_INLINE const char * skipNonWhiteSpace(const char* scan)
	{
		while ( !isWhiteSpace(*scan) && *scan ) scan++;
		return *scan ? scan : 0;
	}

	bool XmlDeserializer::verifyObject(Interface *obj, physx::PxU32 argc, const char **argv)
	{
		if( ReadVersion(argc, argv) != obj->version() )
		{
			XML_WARNING("unknown error");
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_UNKNOWN;
			return false;
		}

		char *checksum = (char *)getAttribute("checksum", argc, argv);
		if( checksum && !DoIgnoreChecksum(*obj) )
		{
			physx::PxU32 objBits;
			const physx::PxU32 *objChecksum = obj->checksum(objBits);

			physx::PxU32 bits = 0;

			char *cur = checksum, *next = 0;
			bool sameBits = true;
			for(physx::PxU32 i = 0; ; ++i, cur = next)
			{
				physx::PxU32 val = (physx::PxU32)strtoul(cur, &next, 0); //FIXME: strtoul is not safe
				if( cur == next )
					break;

				bits += 32;
				if( bits > objBits || val != objChecksum[i] )
				{
					NX_PARAM_TRAITS_WARNING(
						mTraits,
						"Schema checksum is different for object of class %s and version %u, "
							"asset may be corrupted",
						obj->className(),
						(unsigned)obj->version()
						);
					sameBits = false;
					break;
				}
			}

			if( objBits != bits && sameBits )
			{
				NX_PARAM_TRAITS_WARNING(
					mTraits,
					"Schema checksum is different for object of class %s and version %u, "
						"asset may be corrupted",
					obj->className(),
					(unsigned)obj->version()
					);
			}
		} //if( checksum )

		return true;
	}

	bool XmlDeserializer::initAddressString(char *dest, physx::PxU32 len, const char *name)
	{
		char *end = dest + len;

		for (physx::PxU32 i = 0; i < tos().getIndex(); i++)
		{
			FieldInfo &field = tos().getFieldInfo(i);

			const char *n = field.name;
			FieldType type = field.type;

			if( SKIP == type )
				continue;

			while ( n && *n && dest < end )
				*dest++ = *n++;

			if ( ARRAY == type )
			{
				char temp[512];
				physx::string::sprintf_s(temp, 512, "[%d]", field.idx);

				if( dest + ::strlen(temp) >= end )
				{
					XML_WARNING("buffer overflow");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
					return false;
				}

				const char *scan = temp;
				while ( *scan && dest < end )
				{
					*dest++ = *scan++;
				}
			}
			else
			{
				if( dest + 1 >= end )
				{
					XML_WARNING("buffer overflow");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
					return false;
				}

				*dest++ = '.';
			}
		}

		while ( name && *name && dest < end )
		{
			*dest++ = *name++;
		}

		*dest = 0;
		//printf("Fully qualified name: %s\n", scratch );

		return true;
	}

	bool XmlDeserializer::processClose(const char *tag,physx::PxU32 depth,bool &isError)
	{
		isError = true; //By default if we return with false it's due to error

		if( strcmp(tag, "NxParameters") == 0 )
		{
			mInRootElement = false;

			// in xml there's only 1 root element allowed,
			// so we want to stop after the first NxParameters
			isError = false;
			return false;
		}

		static const char *validTags[] = {
			"struct",
			"value",
			"array"
		};

		for(physx::PxU32 i = 0; i < PX_ARRAY_SIZE(validTags); ++i)
		{
			if( 0 != ::strcmp(validTags[i], tag) )
				continue;

#	ifndef NDEBUG
			physx::PxU32 idx = tos().getIndex();
			DEBUG_ASSERT( idx > 0 );

			static FieldType validTypes[] = {
				STRUCT,
				VALUE,
				ARRAY
			};

			// Make gcc happy
			const FieldType* tmp = &validTypes[0];
			PX_UNUSED(tmp);

			FieldType type = tos().getFieldInfo(idx - 1).type;
			PX_UNUSED(type);
			DEBUG_ASSERT( type == SKIP || type == validTypes[i] );
#	endif

			if( !popField() )
				return false;

			if (depth == 1 && mRootIndex > MAX_ROOT_OBJ)
			{
				DEBUG_ASSERT(i == 1);
				mObjects[0].getObject()->destroy();
			}

			return tos().getIndex() ? depth != 0 : popObj();
		}

		return false;
	}

	bool XmlDeserializer::processElement(
		const char *elementName,
		physx::PxI32 argc,
		const char **argv,
		const char *elementData,
		physx::PxI32 /*lineno*/)
	{
		//Force DOCTYPE
		if( !mHasDoctype )
		{
			XML_WARNING("DOCTYPE is missing");
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_MISSING_DOCTYPE;
			return false;
		}

		if( strcmp(elementName, "NxParameters")  == 0 )
		{
			if( mObjIndex )
			{
				XML_WARNING("NxParameters must be root element");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_MISSING_ROOT_ELEMENT;
				return false;
			}

			if( mInRootElement )
			{
				XML_WARNING("More than one root element encountered");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			physx::PxU32 ver = ReadVersion(argc, argv);
			if( ver != mVer )
			{
				XML_WARNING("unknown version of APX file format: %u", (unsigned)ver);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_FILE_VERSION;
				return false;
			}

			const char* numObjects = getAttribute("numObjects", argc, argv);
			if (numObjects != NULL)
			{
				const physx::PxU32 num = atoi(numObjects);
				if (num > MAX_ROOT_OBJ)
				{
					XML_WARNING("APX file has more than %d root objects, only %d will be read", num, MAX_ROOT_OBJ);
				}
			}


			++mRootTags;
			mInRootElement = true;

			return true;
		}

		if( mRootTags > 0 && !mInRootElement )
		{
			XML_WARNING("element %s not under root element", elementName);
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_MISSING_ROOT_ELEMENT;
			return false;
		}

		if ( strcmp(elementName, "struct") == 0 )
		{
			const char *name = getAttribute("name", argc, argv);

			if( !mObjIndex )
			{
				XML_WARNING("struct-element %s not under value-element", name);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			pushField(name, STRUCT);
		}
		else if ( strcmp(elementName, "value") == 0 )
		{
			if( !mObjIndex ) //Root object?
			{
				const char *className = getAttribute("className", argc, argv);

				physx::PxU32 version = ReadVersion(argc, argv);

				Interface *obj = mTraits->createNxParameterized(className, version);
				if( !obj )
				{
					XML_WARNING("failed to create object of type %s and version %u", className, (unsigned)version);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_OBJECT_CREATION_FAILED;
					return false;
				}

				const char *objectName = getAttribute("objectName", argc, argv);
				if( objectName )
					obj->setName(objectName);

				if (mRootIndex < MAX_ROOT_OBJ)
				{
					mRootObjs[mRootIndex] = obj;
				}
				mRootIndex++;

				pushObj(obj);
				pushField("", SKIP); //Root <value> should not be used in initAddressString

				return true;
			}

			const char *name = getAttribute("name", argc, argv);

			char scratch[2048];
			if( !initAddressString(scratch, sizeof(scratch), name) )
			{
				return false;
			}

			pushField(name, VALUE);

			Interface *obj = tos().getObject();
			if( !obj )
			{
				XML_WARNING("unknown error");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_UNKNOWN;
				return false;
			}

			Handle handle(*obj, scratch);
			if( !handle.isValid() )
			{
				XML_WARNING("%s: invalid path", scratch);
				DEBUG_ALWAYS_ASSERT();
//				mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
				return true;
			}

			const char *type = getAttribute("type", argc, argv),
				*expectedType = typeToStr(handle.parameterDefinition()->type());
			if ( type && 0 != physx::string::stricmp(type, expectedType) )
			{
				XML_WARNING("%s: invalid type %s (expected %s)", scratch, type, expectedType);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_ATTR;
				return false;
			}

			const char *included = getAttribute("included", argc, argv);
			if ( included )
			{
				bool isIncludedRef = 0 != atoi(included);
				if( isIncludedRef != handle.parameterDefinition()->isIncludedRef() )
				{
					XML_WARNING("%s: unexpected included-attribute", scratch);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INVALID_ATTR;
					return false;
				}

				const char *className = getAttribute("className", argc, argv);
				if( !className )
				{
					if( getAttribute("classNames", argc, argv) )
					{
						// Ref is NULL

						Interface *oldObj = 0;
						if( NxParameterized::ERROR_NONE != handle.getParamRef(oldObj) )
							return false;
						if( oldObj )
							oldObj->destroy();

						handle.setParamRef(0);

						return true;
					}
					else
					{
						XML_WARNING("%s: missing both className and classNames attribute", scratch);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_INVALID_ATTR;
						return false;
					}
				}

				physx::PxU32 version = ReadVersion(argc, argv);

				Interface *refObj = 0;
				if( isIncludedRef )
					refObj = mTraits->createNxParameterized(className, version);
				else
				{
					void *buf = mTraits->alloc(sizeof(NxParameters));
					refObj = PX_PLACEMENT_NEW(buf, NxParameters)(mTraits);

					refObj->setClassName(className);
				}

				if( !refObj )
				{
					XML_WARNING("%s: failed to create object of type %s and version %u", scratch, className, (unsigned)version);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_OBJECT_CREATION_FAILED;
					return false;
				}

				if( refObj && (-1 == handle.parameterDefinition()->refVariantValIndex(refObj->className())) )
				{
					char longName[256];
					handle.getLongName(longName, sizeof(longName));
					NX_PARAM_TRAITS_WARNING(
						mTraits,
						"%s: setting reference of invalid class %s",
						longName,
						refObj->className()
						);
				}

				if( NxParameterized::ERROR_NONE != handle.setParamRef(refObj) )
				{
					XML_WARNING("%s: failed to set reference of type %s", scratch, className);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INVALID_REFERENCE;
					return false;
				}

				const char *objectName = getAttribute("objectName", argc, argv);
				if( objectName && refObj )
					refObj->setName(objectName);

				if( isIncludedRef )
					pushObj(refObj);
				else if ( elementData && refObj )
					refObj->setName(elementData);
			}
			else
			{
				if ( elementData == 0 )
					elementData = "";

				const char *isNull = getAttribute("null", argc, argv);
				if( isNull && 0 != atoi(isNull) )
				{
					//Only strings and enums may be NULL so it's safe to call setParamString

					DataType t = handle.parameterDefinition()->type();
					PX_UNUSED(t);
					DEBUG_ASSERT( TYPE_STRING == t || TYPE_ENUM == t );

					handle.setParamString(0);
				}
				else
				{
					if( NxParameterized::ERROR_NONE != handle.strToValue(elementData, 0) )
					{
						XML_WARNING("%s: failed to convert string to value: %10s", scratch, elementData);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_STRING2VAL_FAILED;
						return false;
					}
				}
			} //if ( included )
		}
		else if ( strcmp(elementName, "array") == 0 )
		{
			const char *name = getAttribute("name", argc, argv);

			if( !mObjIndex )
			{
				XML_WARNING("array-element %s not under value-element", name);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			physx::PxU32 arraySize = 0;
			if ( const char *sz = getAttribute("size", argc, argv) )
			{
				arraySize = (physx::PxU32)atoi(sz);
			}

			if ( arraySize > 0 )
			{
				Interface *obj = tos().getObject();
				if( !obj )
				{
					XML_WARNING("unknown error");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_UNKNOWN;
					return false;
				}

				char scratch[2048];
				if( !initAddressString(scratch, sizeof(scratch), name) )
				{
					return false;
				}

				Handle handle(*obj, scratch);
				if( !handle.isValid() )
				{

					mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
					XML_WARNING("%s: invalid path", scratch);
					DEBUG_ALWAYS_ASSERT();

					return false;
				}

				if( !handle.parameterDefinition()->arraySizeIsFixed() )
					if( NxParameterized::ERROR_NONE != handle.resizeArray(arraySize) )
					{
						XML_WARNING("%s: failed to resize array", scratch);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_INVALID_ARRAY;
						return false;
					}
					if ( elementData )
					{
						const char *scan = elementData;

						handle.set(0);
						const Definition *paramDef = handle.parameterDefinition();
						handle.popIndex();


						if ( paramDef->type() == TYPE_STRUCT )
						{
							// read the structElements field
							const char* structElements = getAttribute("structElements", argc, argv);

							physx::PxI32* simpleStructRedirect = getSimpleStructRedirect(paramDef->numChildren());
							physx::PxU32 numRedirects = 0;
							while (structElements && *structElements)
							{
								char fieldName[64];
								char type[16];

								size_t count = 0;
								while(*structElements != 0 && *structElements != ',' && *structElements != '(')
									fieldName[count++] = *structElements++;
								if( count >= sizeof(fieldName) )
								{
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
									return false;
								}
								fieldName[count] = 0;

								if (*structElements == '(')
								{
									structElements++;
									count = 0;
									while(*structElements != 0 && *structElements != ',' && *structElements != ')')
										type[count++] = *structElements++;
									if( count >= sizeof(type) )
									{
										DEBUG_ALWAYS_ASSERT();
										mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
										return false;
									}
									type[count] = 0;

								}
								if (*structElements == ')')
									structElements++;
								if (*structElements == ',')
									structElements++;

								const Definition* childDef = paramDef->child(fieldName, simpleStructRedirect[numRedirects]);
								const char* trueType = childDef ? typeToStr(childDef->type()) : 0;
								if (childDef && ::strcmp(trueType, type) != 0)
								{
									XML_WARNING(
										"%s[].%s: unexpected type: %s (must be %s)",
										scratch, fieldName, type, trueType ? trueType : "");
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INVALID_DATA_TYPE;
									return false;
								}

								// -2 means to reed the data but not storing it
								// -1 means to not read the data as it was not serialized'
								// i = [0 .. n] means to read the data and store it in child i
								if (childDef == NULL)
								{
									simpleStructRedirect[numRedirects] = -2;

									// Fail fast
									XML_WARNING("%s[]: unexpected structure field: %s", scratch, fieldName);
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
									return false;
								}

								numRedirects++;
							}

							const physx::PxI32 numChildren = paramDef->numChildren();
							for(physx::PxU32 i = 0; i < arraySize; ++i)
							{
								handle.set(i);
								for( physx::PxI32 j=0; j<numChildren; j++ )
								{
									if (simpleStructRedirect[j] < 0)
									{
										if (simpleStructRedirect[j] < -1)
										{
											// read the data anyways
											scan = skipWhiteSpace(scan);
											if (scan != NULL)
												scan = skipNonWhiteSpace(scan);
										}
										continue;
									}

									scan = skipWhiteSpace(scan);
									if ( !scan ) break;

									handle.set(simpleStructRedirect[j]);

									if( NxParameterized::ERROR_NONE != handle.strToValue(scan, &scan) )
									{
										XML_WARNING("%s: failed to convert string to value: %10s...", scan);
										DEBUG_ALWAYS_ASSERT();
										mError = Serializer::ERROR_STRING2VAL_FAILED;
										return false;
									}

									handle.popIndex();
									if ( !scan ) break;
								}
								handle.popIndex();
								if ( !scan ) break;
							}
						}
						else
						{
							// LRR: wall clock time is the same for this simple loop as the previous
							//      "unrolled" version
							for (physx::PxU32 i=0; i<arraySize; i++)
							{
								handle.set(i);
								if( NxParameterized::ERROR_NONE != handle.strToValue(scan, &scan) )
								{
									XML_WARNING("%s: failed to convert string to value: %10s", scratch, scan);
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_STRING2VAL_FAILED;
									return false;
								}
								handle.popIndex();
							}
						}
					} //if( elementData )
			} //if ( arraySize > 0 )

			pushField(name, ARRAY);
		}
		else
		{
			XML_WARNING("unknown element %s", elementName);
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_UNKNOWN_XML_TAG;
			return false;
		}

		return true;
	}

	physx::PxI32* XmlDeserializer::getSimpleStructRedirect(physx::PxU32 size)
	{
		if (mSimpleStructRedirectSize < size)
		{
			if (mSimpleStructRedirect != NULL)
				mTraits->free(mSimpleStructRedirect);

			if (size < 16)
				size = 16; // just to not allocate all these small things more than once

			mSimpleStructRedirect = (physx::PxI32*)mTraits->alloc(sizeof(physx::PxI32) * size);
			mSimpleStructRedirectSize = size;
		}

		memset(mSimpleStructRedirect, -1, sizeof(physx::PxI32) * size);
		return mSimpleStructRedirect;
	}

}
