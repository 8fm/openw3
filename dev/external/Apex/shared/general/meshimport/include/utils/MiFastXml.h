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

#ifndef MI_FAST_XML_H
#define MI_FAST_XML_H

#include "MiPlatformConfig.h"
#include "MiFileBuf.h"		// defines the basic file stream interface.

namespace mimp
{

class FastXml
{
public:
	/***
	* Callbacks to the user with the contents of the XML file properly digested.
	*/
	class Callback
	{
	public:

		virtual bool processComment(const char *comment) = 0; // encountered a comment in the XML

		// 'element' is the name of the element that is being closed.
		// depth is the recursion depth of this element.
		// Return true to continue processing the XML file.
		// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
		// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
		virtual bool processClose(const char *element,MiU32 depth,bool &isError) = 0;	  // process the 'close' indicator for a previously encountered element

		// return true to continue processing the XML document, false to skip.
		virtual bool processElement(
			const char *elementName,   // name of the element
			MiI32 argc,         // number of attributes pairs
			const char **argv,         // list of attributes.
			const char  *elementData,  // element data, null if none
			MiI32 lineno) = 0;  // line number in the source XML file

		// process the XML declaration header
		virtual bool processXmlDeclaration(
			MiI32 /*argc*/,
			const char ** /*argv*/,
			const char  * /*elementData*/,
			MiI32 /*lineno*/)
		{
			return true;
		}

		virtual bool processDoctype(
			const char * /*rootElement*/, //Root element tag
			const char * /*type*/,        //SYSTEM or PUBLIC
			const char * /*fpi*/,         //Formal Public Identifier
			const char * /*uri*/)         //Path to schema file
		{
			return true;
		}

		virtual void *  fastxml_malloc(MiU32 size) = 0;
		virtual void	fastxml_free(void *mem) = 0;

	};

	virtual bool processXml(MiFileBuf &buff,bool streamFromMemory=false) = 0;

	virtual const char *getError(MiI32 &lineno) = 0; // report the reason for a parsing error, and the line number where it occurred.

	virtual void release(void) = 0;
};

const char *getAttribute(const char *attr, MiI32 argc, const char **argv);

FastXml * createFastXml(FastXml::Callback *iface);

}; // end of namespace mimp

#endif // FAST_XML_H
