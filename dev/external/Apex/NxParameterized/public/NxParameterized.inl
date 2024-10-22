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


/*!
\brief NxParameterized inline implementation
*/

#if !defined(PX_PS4)
	#pragma warning(push)
	#pragma warning(disable: 4996)
#endif	//!PX_PS4

#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_DIGIT(c))
#define IS_IDENTCHAR(c) (IS_ALPHANUM(c) || (c) == ' ' || (c) == '_')

/**
\brief Enum of tokenizer result types
*/
enum TokenizerResultType
{
   TOKENIZER_RESULT_NONE,
   TOKENIZER_RESULT_BUFFER_OVERFLOW,
   TOKENIZER_RESULT_SYNTAX_ERROR,
   TOKENIZER_RESULT_STRUCT_MEMBER,
   TOKENIZER_RESULT_ARRAY_INDEX,
};

/**
\brief Get struct member token
*/
PX_INLINE TokenizerResultType getStructMemberToken(const char *long_name,
                                                char *token,
                                                physx::PxU32 max_token_len,
                                                physx::PxU32 &offset)
{
    PX_ASSERT(long_name != nullptr);
    PX_ASSERT(token != nullptr);
    PX_ASSERT(max_token_len > 1);
    PX_ASSERT(IS_IDENTCHAR(long_name[0]) || long_name[0] == '.');

    offset = 0;

    if(long_name[0] == '.')
      offset++;

    physx::PxU32 tokenLen = 0;
    while(IS_IDENTCHAR(long_name[offset]))
    {
        if(offset == max_token_len - 1)
           return(TOKENIZER_RESULT_BUFFER_OVERFLOW);

        token[tokenLen++] = long_name[offset++];
    }

    token[tokenLen++] = 0;

    if(long_name[offset] != 0 && long_name[offset] != '.' && long_name[offset] != '[')
        return(TOKENIZER_RESULT_SYNTAX_ERROR);

    return(TOKENIZER_RESULT_STRUCT_MEMBER);
}

/**
\brief Get array member token
*/
PX_INLINE TokenizerResultType getArrayMemberToken(const char *long_name,
                                               char *token,
                                               physx::PxU32 max_token_len,
                                               physx::PxU32 &offset)
{
    PX_ASSERT(long_name != nullptr);
    PX_ASSERT(token != nullptr);
    PX_ASSERT(max_token_len > 1);
    PX_ASSERT(long_name[0] == '[');

    offset = 1;

    physx::PxU32 tokenLen = 0;
    while(long_name[offset] && IS_DIGIT(long_name[offset]))
    {
        if(tokenLen == max_token_len - 1)
             return(TOKENIZER_RESULT_BUFFER_OVERFLOW);

        token[tokenLen++] = long_name[offset++];
    }

    token[tokenLen++] = 0;

    if(long_name[offset] != ']')
        return(TOKENIZER_RESULT_SYNTAX_ERROR);

    offset++;

    return(TOKENIZER_RESULT_ARRAY_INDEX);
}

/**
\brief Get next token
*/
PX_INLINE TokenizerResultType getNextToken(const char *long_name,
                                        char *token,
                                        physx::PxU32 max_token_len,
                                        physx::PxU32 &offset)
{
    PX_ASSERT(long_name != nullptr);
    PX_ASSERT(token != nullptr);
    PX_ASSERT(max_token_len > 1);

    if(long_name[0] == 0)
        return(TOKENIZER_RESULT_NONE);

    if(long_name[0] == '.' || IS_IDENTCHAR(long_name[0]))
        return(getStructMemberToken(long_name, token, max_token_len, offset));

    if(long_name[0] == '[')
        return(getArrayMemberToken(long_name, token, max_token_len, offset));

    return(TOKENIZER_RESULT_SYNTAX_ERROR);
}

#undef IS_ALPHA
#undef IS_DIGIT
#undef IS_ALPHANUM
#undef IS_IDENTCHAR

/*
 The local_strcat_s function appends strSource to strDestination and terminates the resulting string with a null character. 
 The initial character of strSource overwrites the terminating null character of  strDestination. The behavior of strcat_s is 
 undefined if the source and destination strings overlap. Note that the second parameter is the total size of the buffer, not 
 the remaining size
*/

/**
\brief The local_strcat_s function appends strSource to strDestination and terminates the resulting string with a null character.
*/
PX_INLINE int local_strcat_s(char* dest, size_t size, const char* src)
{
	size_t d, destStringLen, srcStringLen;

	if (	dest		== nullptr ||
			src			== nullptr ||
			size		== 0	)
	{
		return -1;
	}

	destStringLen	= strlen(dest);
	srcStringLen	= strlen(src);
	d				= srcStringLen + destStringLen;

	if ( size <= d )
	{
		return -1;
	}

	::memcpy( &dest[destStringLen], src, srcStringLen );
	dest[d] = '\0';

	return 0;
}

/**
\brief The local_sprintf_s function wraps the va_list functionality required for PxVxprintf
*/
PX_INLINE physx::PxI32 local_sprintf_s( char * _DstBuf, size_t _DstSize, const char * _Format, ...)
{
	if ( _DstBuf == nullptr || _Format == nullptr )
	{
		return -1;
	}

	va_list arg;
	va_start( arg, _Format );
	physx::PxI32 r = physx::PxVsprintf( _DstBuf, _DstSize, _Format, arg );
	va_end(arg);

	return r;
}


PX_INLINE Handle::Handle(::NxParameterized::Interface *iface)
{
	reset();
	mInterface = iface;
	mIsConst = false;
	if (mInterface != nullptr)
		mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(::NxParameterized::Interface &iface)
{
    reset();
	mInterface = &iface;
	mIsConst = false;
	mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(const ::NxParameterized::Interface &iface)
{
    reset();
	mInterface = const_cast< ::NxParameterized::Interface * >(&iface);
	mIsConst = true;
	mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(const Handle &param_handle)
{
    reset();

    if(param_handle.isValid())
    {
        mNumIndexes = param_handle.mNumIndexes;
        memcpy(mIndexList, param_handle.mIndexList, sizeof(physx::PxI32) * mNumIndexes);
        mParameterDefinition = param_handle.mParameterDefinition;
        mIsValid = param_handle.mIsValid;
		mIsConst = param_handle.mIsConst;
		mInterface = param_handle.mInterface;
    }
    else
        mIsConst = mIsValid = false;
}


PX_INLINE Handle::Handle(::NxParameterized::Interface &instance,const char *longName)
{
	mInterface = &instance;
	mIsConst = false;
    mInterface->getParameterHandle(longName, *this);
}

PX_INLINE Handle::Handle(const ::NxParameterized::Interface &instance,const char *longName)
{
	mInterface = const_cast< ::NxParameterized::Interface *>(&instance);
	mIsConst = true;
    mInterface->getParameterHandle(longName, *this);
}

PX_INLINE ErrorType Handle::getChildHandle(const ::NxParameterized::Interface *instance,const char *child_long_name,
                                                         Handle &handle)
{
    handle = *this;
    handle.mUserData = nullptr;
    return(handle.set(instance,child_long_name));
}

PX_INLINE ErrorType Handle::getParameter(const char *longName)
{
	if( !mInterface )
	{
		return ERROR_HANDLE_MISSING_INTERFACE_POINTER;
	}

	return mInterface->getParameterHandle(longName, *this);
}

PX_INLINE ErrorType  Handle::set(const ::NxParameterized::Interface *instance,const Definition *root,const char *child_long_name)
{
    PX_ASSERT(root->parent() == nullptr);

    reset();
    mParameterDefinition = root;
    mIsValid = true;

    return(set(instance,child_long_name));
}

PX_INLINE ErrorType Handle::set(const ::NxParameterized::Interface *instance,const char *child_long_name)
{
    PX_ASSERT(mParameterDefinition != nullptr);
    PX_ASSERT(child_long_name != nullptr);

    if(!isValid())
        return(ERROR_INVALID_PARAMETER_HANDLE);

    mUserData = nullptr;

    if(child_long_name[0] == 0)
    {
        return(ERROR_NONE);
    }

    physx::PxI32 indexLevel = 0;

    mIsValid = false;
    // while(1) causes C4127 warning
	for( ; ; )
    {
        char token[1024];
        physx::PxU32 offset;

        TokenizerResultType Result = getNextToken(child_long_name, token, sizeof(token), offset);

        switch(Result)
        {
            case TOKENIZER_RESULT_NONE:
                if(indexLevel == 0)
                    return(ERROR_INVALID_PARAMETER_NAME);
                 else
                    goto no_error;

            case TOKENIZER_RESULT_BUFFER_OVERFLOW:
                return(ERROR_RESULT_BUFFER_OVERFLOW);

            case TOKENIZER_RESULT_SYNTAX_ERROR:
                return(ERROR_SYNTAX_ERROR_IN_NAME);

            case TOKENIZER_RESULT_STRUCT_MEMBER:
                {
                    if(mParameterDefinition->type() != TYPE_STRUCT)
                        return(ERROR_NAME_DOES_NOT_MATCH_DEFINITION);

                    physx::PxI32 index;
                    mParameterDefinition = mParameterDefinition->child(token, index);
                    if(mParameterDefinition == nullptr)
                        return(ERROR_INVALID_PARAMETER_NAME);

                    pushIndex(index);
                }
                break;

            case TOKENIZER_RESULT_ARRAY_INDEX:
                {
                    if(mParameterDefinition->type() != TYPE_ARRAY)
                        return(ERROR_NAME_DOES_NOT_MATCH_DEFINITION);

                    physx::PxI32 index = atoi(token);
                    PX_ASSERT(index >= 0);

					physx::PxI32 arraySize=0;
					if ( instance )
					{
						Handle handle(*instance);
						ErrorType err = instance->getParameterHandle( mParameterDefinition->longName(), handle );
						if(err != ERROR_NONE)
							return(err);
						handle.getArraySize(arraySize);
					}
					else
					{
						arraySize = mParameterDefinition->arraySize();
					}

                    if(index >= arraySize )
                        return(ERROR_INDEX_OUT_OF_RANGE);

                    PX_ASSERT(mParameterDefinition->numChildren() == 1);
                    mParameterDefinition = mParameterDefinition->child(0);

                    pushIndex(index);
                }
                break;
        }

        child_long_name += offset;
        indexLevel++;
    }

no_error:

    mIsValid = true;
    return(ERROR_NONE);
}

PX_INLINE ErrorType Handle::set(physx::PxI32 child_index)
{
    PX_ASSERT(mParameterDefinition != nullptr);
    PX_ASSERT(child_index >= 0);

    switch(parameterDefinition()->type())
    {
        case TYPE_STRUCT:
            if(child_index < 0 || child_index >= parameterDefinition()->numChildren())
                return(ERROR_INDEX_OUT_OF_RANGE);
            mParameterDefinition = mParameterDefinition->child(child_index);
            pushIndex(child_index);

            break;


        case TYPE_ARRAY:
            if(child_index < 0)
                return(ERROR_INDEX_OUT_OF_RANGE);

			// parameterDefinition()->arraySize() does not work on dynamic arrays...
            if( parameterDefinition()->arraySizeIsFixed() &&
               child_index >= parameterDefinition()->arraySize())
                return(ERROR_INDEX_OUT_OF_RANGE);

            mParameterDefinition = mParameterDefinition->child(0);
            pushIndex(child_index);
            break;

        default:
            return(ERROR_IS_LEAF_NODE);
    }

    mIsValid = true;
    return(ERROR_NONE);
}

PX_INLINE ErrorType Handle::getChildHandle(physx::PxI32 index, Handle &handle)
{
    if(parameterDefinition()->type() != TYPE_ARRAY && parameterDefinition()->type() != TYPE_STRUCT)
        return(ERROR_IS_LEAF_NODE);

    if(!isValid())
        return(ERROR_INVALID_PARAMETER_HANDLE);

    handle = *this;
    handle.pushIndex(index);
    if(parameterDefinition()->type() == TYPE_STRUCT)
    {
        PX_ASSERT(parameterDefinition()->child(index) != nullptr);
		handle.mParameterDefinition = parameterDefinition()->child(index);
    }
    else
    {
        PX_ASSERT(parameterDefinition()->child(0) != nullptr);
        handle.mParameterDefinition = parameterDefinition()->child(0);
    }

    return(ERROR_NONE);
}

PX_INLINE bool Handle::getLongName(char *str, physx::PxU32 max_str_len) const
{
    PX_ASSERT(parameterDefinition() != nullptr);

    if(!isValid())
        return(false);

    if(numIndexes() < 1)
        return(false);

    const Definition *root = parameterDefinition()->root();

    *str = 0;
    const Definition *node = root->child(index(0));
    char tmpStr[32];
    for(physx::PxI32 i=1; i <= numIndexes(); ++i)
    {
        PX_ASSERT(node != nullptr);
        PX_ASSERT(node->parent() != nullptr);

        switch(node->parent()->type())
        {
            case TYPE_STRUCT:
                if(node->parent()->parent() != nullptr)
				{
					local_strcat_s(str, max_str_len, ".");
				}
                local_strcat_s(str, max_str_len, node->name());
				break;

            case TYPE_ARRAY:
                local_strcat_s(str, max_str_len, "[");

				local_sprintf_s(tmpStr, sizeof(tmpStr), "%d", index(i-1));

                local_strcat_s(str, max_str_len, tmpStr);
                local_strcat_s(str, max_str_len, "]");
                break;

            default:
                local_strcat_s(str, max_str_len, node->name());
        }

        switch(node->type())
        {
            case TYPE_STRUCT:
                node = node->child(index(i));
                break;

            case TYPE_ARRAY:
                node = node->child(0);
                break;

            default:
                node = nullptr;
        }
    }

    return(true);
}

PX_INLINE void Handle::reset(void)
{
    mNumIndexes = 0;
    mParameterDefinition = nullptr;
    mUserData = nullptr;
    mIsValid = false;
}

PX_INLINE void Handle::pushIndex(physx::PxI32 index)
{
    PX_ASSERT(mNumIndexes < MAX_DEPTH);
    PX_ASSERT(index >= 0);

    if(mNumIndexes < MAX_DEPTH)
        mIndexList[mNumIndexes++] = index;
}

PX_INLINE physx::PxI32 Handle::popIndex(physx::PxI32 levels)
{
    PX_ASSERT(levels > 0);
    PX_ASSERT(mNumIndexes >= levels);
    PX_ASSERT(mParameterDefinition != nullptr);

    if(mNumIndexes >= levels )
    {
        mNumIndexes -= levels;

        for(; levels > 0; --levels)
            mParameterDefinition = mParameterDefinition->parent();

        return(mIndexList[mNumIndexes]);
    }

    return(-1);
}

PX_INLINE ErrorType Handle::initParamRef(const char *chosenRefStr, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	return mInterface->initParamRef(*this, chosenRefStr, doDestroyOld);
}

// These functions wrap the raw(Get|Set)XXXXX() methods.  They deal with
// error handling and casting.

#define CHECK_CONST_HANDLE if( mIsConst ) return ERROR_MODIFY_CONST_HANDLE;

PX_INLINE ErrorType Handle::getParamBool(bool &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBool(*this,val);
}

PX_INLINE ErrorType Handle::setParamBool(bool val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBool(*this,val);
}

PX_INLINE ErrorType Handle::getParamBoolArray(bool *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBoolArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamBoolArray(const bool *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBoolArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::getParamString(const char *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamString(*this,val);
}

PX_INLINE ErrorType Handle::setParamString(const char *val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamString(*this,val);
}

PX_INLINE ErrorType Handle::getParamStringArray(char **array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamStringArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamStringArray(const char **array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamStringArray(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamEnum(const char *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamEnum(*this,val);
}

PX_INLINE ErrorType Handle::setParamEnum(const char *val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamEnum(*this,val);
}

PX_INLINE ErrorType Handle::getParamEnumArray(char **array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamEnumArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamEnumArray(const char **array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamEnumArray(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamRef(::NxParameterized::Interface *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamRef(*this,val);
}

PX_INLINE ErrorType Handle::setParamRef(::NxParameterized::Interface *val, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamRef(*this, val, doDestroyOld);
}

PX_INLINE ErrorType Handle::getParamRefArray(::NxParameterized::Interface **array, physx::PxI32 n, physx::PxI32 offset) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamRefArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamRefArray(::NxParameterized::Interface **array, physx::PxI32 n, physx::PxI32 offset, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamRefArray(*this,array,n,offset,doDestroyOld);
}

PX_INLINE ErrorType Handle::getParamI8(physx::PxI8 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI8(*this,val);
}

PX_INLINE ErrorType Handle::setParamI8(physx::PxI8 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI8(*this,val);
}

PX_INLINE ErrorType Handle::getParamI8Array(physx::PxI8 *_array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI8Array(*this,_array,n,offset);
}

PX_INLINE ErrorType Handle::setParamI8Array(const physx::PxI8 *val, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI8Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI16(physx::PxI16 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI16(*this,val);
}

PX_INLINE ErrorType Handle::setParamI16(physx::PxI16 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI16(*this,val);
}

PX_INLINE ErrorType Handle::getParamI16Array(physx::PxI16 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI16Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamI16Array(const physx::PxI16 *val, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI16Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI32(physx::PxI32 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI32(*this,val);
}

PX_INLINE ErrorType Handle::setParamI32(physx::PxI32 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI32(*this,val);
}

PX_INLINE ErrorType Handle::getParamI32Array(physx::PxI32 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI32Array(*this,array,n,offset ) ;
}

PX_INLINE ErrorType Handle::setParamI32Array(const physx::PxI32 *val, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI32Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI64(physx::PxI64 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI64(*this,val) ;
}

PX_INLINE ErrorType Handle::setParamI64(physx::PxI64 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI64(*this,val);
}

PX_INLINE ErrorType Handle::getParamI64Array(physx::PxI64 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamI64Array(const physx::PxI64 *val, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI64Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamU8(physx::PxU8 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU8(*this,val);
}

PX_INLINE ErrorType Handle::setParamU8(physx::PxU8 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU8(*this,val);
}

PX_INLINE ErrorType Handle::getParamU8Array(physx::PxU8 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU8Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU8Array(const physx::PxU8 *val, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU8Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamU16(physx::PxU16 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU16(*this,val);
}

PX_INLINE ErrorType Handle::setParamU16(physx::PxU16 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU16(*this,val);
}

PX_INLINE ErrorType Handle::getParamU16Array(physx::PxU16 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU16Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU16Array(const physx::PxU16 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU16Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamU32(physx::PxU32 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU32(*this,val);
}

PX_INLINE ErrorType Handle::setParamU32(physx::PxU32 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU32(*this,val);
}

PX_INLINE ErrorType Handle::getParamU32Array(physx::PxU32 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU32Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU32Array(const physx::PxU32 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU32Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamU64(physx::PxU64 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU64(*this,val);
}

PX_INLINE ErrorType Handle::setParamU64(physx::PxU64 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU64(*this,val);
}

PX_INLINE ErrorType Handle::getParamU64Array(physx::PxU64 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU64Array(const physx::PxU64 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU64Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::getParamF32(physx::PxF32 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF32(*this,val);
}

PX_INLINE ErrorType Handle::setParamF32(physx::PxF32 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF32(*this,val);
}

PX_INLINE ErrorType Handle::getParamF32Array(physx::PxF32 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF32Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamF32Array(const physx::PxF32 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF32Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamF64(physx::PxF64 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF64(*this,val);
}

PX_INLINE ErrorType Handle::setParamF64(physx::PxF64 val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF64(*this,val);
}

PX_INLINE ErrorType Handle::getParamF64Array(physx::PxF64 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamF64Array(const physx::PxF64 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF64Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec2(const physx::PxVec2 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec2(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec2(physx::PxVec2 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec2(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec2Array(physx::PxVec2 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec2Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec2Array(const physx::PxVec2 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec2Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec3(const physx::PxVec3 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec3(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec3(physx::PxVec3 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec3(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec3Array(physx::PxVec3 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec3Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec3Array(const physx::PxVec3 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec3Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec4(const physx::PxVec4 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec4(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec4(physx::PxVec4 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec4(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec4Array(physx::PxVec4 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec4Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec4Array(const physx::PxVec4 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec4Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamQuat(const physx::PxQuat &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamQuat(*this,val);
}

PX_INLINE ErrorType Handle::getParamQuat(physx::PxQuat &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamQuat(*this,val);
}

PX_INLINE ErrorType Handle::getParamQuatArray(physx::PxQuat *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamQuatArray(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamQuatArray(const physx::PxQuat *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamQuatArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamMat33(const physx::PxMat33 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat33(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat33(physx::PxMat33 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat33(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat33Array(physx::PxMat33 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat33Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamMat33Array(const physx::PxMat33 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat33Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamMat34(const physx::PxMat44 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat34(physx::PxMat44 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat34Array(physx::PxMat44 *array, physx::PxI32 n, physx::PxI32 offset) const
{
	Handle childHandle(*this);

	ErrorType error;

	physx::PxI32 size;
	if( ::NxParameterized::ERROR_NONE != (error = getArraySize(size)) )
		return error;

	for(physx::PxI32 i = offset; i < offset + n; ++i)
	{
		if( ::NxParameterized::ERROR_NONE != (error = childHandle.set(i)) )
			return error;

		if( ::NxParameterized::ERROR_NONE != (error = childHandle.getParamMat44(array[i - offset])) )
			return error;

		childHandle.popIndex();
	}

	return ::NxParameterized::ERROR_NONE;
}

PX_INLINE ErrorType Handle::setParamMat34Array(const physx::PxMat44 *array, physx::PxI32 n, physx::PxI32 offset)
{
	CHECK_CONST_HANDLE

	Handle childHandle(*this);

	ErrorType error;

	physx::PxI32 size;
	if( ::NxParameterized::ERROR_NONE != (error = getArraySize(size)) )
		return error;

	for(physx::PxI32 i = offset; i < offset + n; ++i)
	{
		if( ::NxParameterized::ERROR_NONE != (error = childHandle.set(i)) )
			return error;

		if( ::NxParameterized::ERROR_NONE != (error = childHandle.setParamMat44(array[i - offset])) )
			return error;

		childHandle.popIndex();
	}

	return ::NxParameterized::ERROR_NONE;
}

PX_INLINE ErrorType Handle::setParamMat44(const physx::PxMat44 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat44(physx::PxMat44 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat44Array(physx::PxMat44 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat44Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamMat44Array(const physx::PxMat44 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat44Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamBounds3(const physx::PxBounds3 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBounds3(*this,val);
}

PX_INLINE ErrorType Handle::getParamBounds3(physx::PxBounds3 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBounds3(*this,val);
}

PX_INLINE ErrorType Handle::getParamBounds3Array(physx::PxBounds3 *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBounds3Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamBounds3Array(const physx::PxBounds3 *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBounds3Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamTransform(const physx::PxTransform &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
		return mInterface->setParamTransform(*this,val);
}

PX_INLINE ErrorType Handle::getParamTransform(physx::PxTransform &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamTransform(*this,val);
}

PX_INLINE ErrorType Handle::getParamTransformArray(physx::PxTransform *array, physx::PxI32 n, physx::PxI32 offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamTransformArray(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamTransformArray(const physx::PxTransform *array, physx::PxI32 n, physx::PxI32 offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
		return mInterface->setParamTransformArray(*this,array,n,offset);
}



#define NX_PARAMETERIZED_TYPES_NO_LEGACY_TYPES
#define NX_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#define NX_PARAMETERIZED_TYPES_NO_STRING_TYPES
#define NX_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
	template <> PX_INLINE ::NxParameterized::ErrorType Handle::setParam<c_type>(const c_type &val) { return setParam##type_name(val); } \
	template <> PX_INLINE ::NxParameterized::ErrorType Handle::getParam<c_type>(c_type &val) const {return getParam##type_name(val); } \
	template <> PX_INLINE ::NxParameterized::ErrorType Handle::getParamArray<c_type>(c_type *array, physx::PxI32 n, physx::PxI32 offset) const { return getParam##type_name##Array(array, n, offset); } \
	template <> PX_INLINE ::NxParameterized::ErrorType Handle::setParamArray<c_type>(const c_type *array, physx::PxI32 n, physx::PxI32 offset) { return setParam##type_name##Array(array, n, offset); }
#include "NxParameterized_types.h"

PX_INLINE ErrorType Handle::valueToStr(char *buf, physx::PxU32 bufSize, const char *&ret)
{
	PX_ASSERT(mInterface);
	return mInterface->valueToStr(*this, buf, bufSize, ret);
}

PX_INLINE ErrorType Handle::strToValue(const char *str, const char **endptr) // assigns this string to the valu
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->strToValue(*this,str, endptr); // assigns this string to the value
}


PX_INLINE ErrorType Handle::resizeArray(physx::PxI32 new_size)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->resizeArray(*this,new_size);
}

PX_INLINE ErrorType Handle::getArraySize(physx::PxI32 &size, physx::PxI32 dimension ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getArraySize(*this,size,dimension );
}

PX_INLINE ErrorType Handle::swapArrayElements(physx::PxU32 firstElement, physx::PxU32 secondElement)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->swapArrayElements(*this, firstElement, secondElement);
}

#undef IS_ALPHA
#undef IS_DIGIT
#undef IS_ALPHANUM
#undef IS_IDENTCHAR
#undef CHECK_CONST_HANDLE

#if !defined(PX_PS4)
	#pragma warning(pop)
#endif	//!PX_PS4
