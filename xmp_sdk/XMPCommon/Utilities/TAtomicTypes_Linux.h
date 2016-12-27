#ifndef __TAtomicTypes_Linux_h__
#define __TAtomicTypes_Linux_h__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2014 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#if XMP_UNIXBuild

#ifndef __TAtomicTypes_h__
	#error	"This file is supposed to be included from TAtomicTypes and not directly"
#endif

#if GCC_VERSION > 40400 && GCC_VERSION < 40899
	#include <cstdatomic>

	namespace AdobeXMPCommon {
		typedef std::atomic_size_t	atomic_sizet;
	}

	#define NOT_DEFINED_ATOMIC_SIZE_T 0
#endif

#endif  // XMP_UNIXBuild

#endif  // __TAtomicTypes_Linux_h__
