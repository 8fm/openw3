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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef SIMPLE_XML_WRITER_H
#define SIMPLE_XML_WRITER_H
#include "PvdFoundation.h"

namespace physx { namespace debugger { namespace repx {

	class XmlWriter
	{
	public:
		
		struct STagWatcher
		{
			typedef XmlWriter TXmlWriterType;
			TXmlWriterType& mWriter;
			STagWatcher( const STagWatcher& inOther );
			STagWatcher& operator-( const STagWatcher& inOther );
			STagWatcher( TXmlWriterType& inWriter, const char* inTagName )
				: mWriter( inWriter )
			{
				mWriter.beginTag( inTagName );
			}
			~STagWatcher() { mWriter.endTag(); }
		private:
			// DJS: I added a dummy assignment operator to make MSVC happy
			STagWatcher& operator=(const STagWatcher&) {}
		};

		virtual ~XmlWriter(){}
		virtual void beginTag( const char* inTagname ) = 0;
		virtual void endTag() = 0;
		virtual void addAttribute( const char* inName, const char* inValue ) = 0;
		virtual void writeContentTag( const char* inTag, const char* inContent ) = 0;
		virtual void addContent( const char* inContent ) = 0;
	};

	template<typename TStreamType>
	class XmlWriterImpl : public XmlWriter
	{
		TStreamType& mStream;
		XmlWriterImpl( const XmlWriterImpl& inOther );
		XmlWriterImpl& operator=( const XmlWriterImpl& inOther );
		ForwardingArray<const char*>				mTags;
		bool						mTagOpen;
		PxU32						mInitialTagDepth;
	public:

		XmlWriterImpl( TStreamType& inStream, PxAllocatorCallback& inAllocator, PxU32 inInitialTagDepth = 0 )
			: mStream( inStream )
			, mTags( inAllocator, "char*" )
			, mTagOpen( false )
			, mInitialTagDepth( inInitialTagDepth )
		{
		}
		virtual ~XmlWriterImpl()
		{
			while( mTags.size() )
				endTag();
		}
		PxU32 tabCount() { return static_cast<PxU32>(mTags.size()) + mInitialTagDepth; }

		void writeTabs( PxU32 inSize )
		{
			inSize += mInitialTagDepth;
			for ( PxU32 idx =0; idx < inSize; ++idx )
				mStream << "\t";
		}
		void beginTag( const char* inTagname )
		{
			closeTag();
			writeTabs(static_cast<PxU32>(mTags.size()));
			mTags.pushBack( inTagname );
			mStream <<  "<" << inTagname;
			mTagOpen = true;
		}
		void addAttribute( const char* inName, const char* inValue )
		{
			PX_ASSERT( mTagOpen );
			mStream << " " << inName << "=" << "\"" << inValue << "\"";		
		}
		void closeTag(bool useNewline = true)
		{
			if ( mTagOpen )
			{
				mStream << " " << ">";				
				if (useNewline )
					mStream << "\n";
			}
			mTagOpen = false;
		}
		void doEndOpenTag()
		{
			mStream << "</" << mTags.back() << ">" << "\n";
		}
		void endTag()
		{
			PX_ASSERT( mTags.size() );
			if ( mTagOpen )
			{
				mStream << " " << "/>" << "\n";				
			}
			else
			{
				writeTabs(static_cast<PxU32>(mTags.size())-1);
				doEndOpenTag();
			}
			mTagOpen = false;
			mTags.popBack();
		}
		static bool IsNormalizableWhitespace(char c) { return c == 0x9 || c == 0xA || c == 0xD; }
		static bool IsValidXmlCharacter(char c) { return IsNormalizableWhitespace(c) || c >= 0x20; }

		void addContent( const char* inContent )
		{
			closeTag(false);

			//escape xml
			for( ; *inContent; inContent++ )
			{
				switch (*inContent) 
				{
				case '<':
					mStream << "&lt;";
					break;
				case '>':
					mStream << "&gt;";
					break;
				case '&':
					mStream << "&amp;";
					break;
				case '\'':
					mStream << "&apos;";
					break;
				case '"':
					mStream << "&quot;";
					break;
				default:
					if (IsValidXmlCharacter(*inContent)) {
						if (IsNormalizableWhitespace(*inContent))
						{
							char s[32];
							sprintf(s, "&#x%02X;", unsigned(*inContent));
							mStream << s;
						}
						else
							mStream << *inContent;
					}
					break;
				}
			}			
		}
		void writeContentTag( const char* inTag, const char* inContent )
		{
			beginTag( inTag );
			addContent( inContent );
			doEndOpenTag();
			mTags.popBack();
		}
		void insertXml( const char* inXml )
		{
			closeTag();
			mStream << inXml;			
		}
	};

	struct BeginTag
	{
		const char* mTagName;
		BeginTag( const char* inTagName )
			: mTagName( inTagName ) { }
	};

	struct EndTag
	{
		EndTag() {}
	};

	struct Att
	{
		const char* mAttName;
		const char* mAttValue;
		Att( const char* inAttName, const char* inAttValue )
			: mAttName( inAttName )
			, mAttValue( inAttValue ) {	}
	};

	struct Content
	{
		const char* mContent;
		Content( const char* inContent )
			: mContent( inContent ) { }
	};

	
	struct ContentTag
	{
		const char* mTagName;
		const char* mContent;
		ContentTag( const char* inTagName, const char* inContent )
			: mTagName( inTagName )
			, mContent( inContent ) { }
	};

	inline XmlWriter& operator<<( XmlWriter& inWriter, const BeginTag& inTag ) { inWriter.beginTag( inTag.mTagName ); return inWriter; }
	inline XmlWriter& operator<<( XmlWriter& inWriter, const EndTag& /*inTag*/ ) { inWriter.endTag(); return inWriter; }
	inline XmlWriter& operator<<( XmlWriter& inWriter, const Att& inTag ) { inWriter.addAttribute(inTag.mAttName, inTag.mAttValue); return inWriter; }
	inline XmlWriter& operator<<( XmlWriter& inWriter, const Content& inTag ) { inWriter.addContent(inTag.mContent); return inWriter; }
	inline XmlWriter& operator<<( XmlWriter& inWriter, const ContentTag& inTag ) { inWriter.writeContentTag(inTag.mTagName, inTag.mContent); return inWriter; }
	

	inline void writeProperty( XmlWriter& inWriter, ForwardingMemoryBuffer& tempBuffer, const char* inPropName )
	{
		inWriter.writeContentTag( inPropName,  tempBuffer.cStr() );  //reinterpret_cast<const char*>(tempBuffer.begin())
		tempBuffer.clear();
	}
	template<typename TDataType>
	inline void writeProperty( XmlWriter& inWriter, ForwardingMemoryBuffer& tempBuffer, const char* inPropName, TDataType inValue )
	{
		tempBuffer << inValue;
		writeProperty( inWriter, tempBuffer, inPropName );
	}

}}}

#endif