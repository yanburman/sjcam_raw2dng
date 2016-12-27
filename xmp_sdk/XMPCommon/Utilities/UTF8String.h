#ifndef __UTF8String_h__
#define __UTF8String_h__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2014 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPCommon/XMPCommonDefines_I.h"
#include "XMPCommon/Utilities/TAllocator.h"
#include <string>
#include <sstream>

namespace XMP_COMPONENT_INT_NAMESPACE {

	typedef std::basic_string< char, std::char_traits< char >, TAllocator< char > > UTF8String;
	typedef std::string UTF8StringUnmanaged;

	typedef std::basic_stringstream< char, std::char_traits< char >, TAllocator< char > > UTF8StringStream;
	typedef std::stringstream UTF8StringStreamUnmanaged;
}

#endif  // __UTF8String_h__
