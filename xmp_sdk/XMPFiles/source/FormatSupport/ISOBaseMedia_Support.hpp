#ifndef __ISOBaseMedia_Support_hpp__
#define __ISOBaseMedia_Support_hpp__     1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"     // ! This must be the first include.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"

#include <set>

// =================================================================================================
/// \file ISOBaseMedia_Support.hpp
/// \brief XMPFiles support for the ISO Base Media File Format.
///
/// \note These classes are for use only when directly compiled and linked. They should not be
/// packaged in a DLL by themselves. They do not provide any form of C++ ABI protection.
// =================================================================================================

namespace ISOMedia {

#define ISOBoxList \
	ISOboxType(k_ftyp,0x66747970UL)SEPARATOR /* File header Box, no version/flags.*/ \
	\
	ISOboxType(k_mp41,0x6D703431UL)SEPARATOR /* Compatible brand codes*/             \
	ISOboxType(k_mp42,0x6D703432UL)SEPARATOR \
	ISOboxType(k_f4v ,0x66347620UL)SEPARATOR \
	ISOboxType(k_avc1,0x61766331UL)SEPARATOR \
	ISOboxType(k_qt  ,0x71742020UL)SEPARATOR \
	ISOboxType(k_isom,0x69736F6DUL)SEPARATOR \
	ISOboxType(k_3gp4,0x33677034UL)SEPARATOR \
	ISOboxType(k_3g2a,0x33673261UL)SEPARATOR \
	ISOboxType(k_3g2b,0x33673262UL)SEPARATOR \
	ISOboxType(k_3g2c,0x33673263UL)SEPARATOR \
	\
	ISOboxType(k_moov,0x6D6F6F76UL)SEPARATOR /* Container Box, no version/flags. */ \
	ISOboxType(k_mvhd,0x6D766864UL)SEPARATOR /* Data FullBox, has version/flags. */ \
	ISOboxType(k_hdlr,0x68646C72UL)SEPARATOR \
	ISOboxType(k_udta,0x75647461UL)SEPARATOR /* Container Box, no version/flags. */ \
	ISOboxType(k_cprt,0x63707274UL)SEPARATOR /* Data FullBox, has version/flags. */ \
	ISOboxType(k_uuid,0x75756964UL)SEPARATOR /* Data Box, no version/flags.      */ \
	ISOboxType(k_free,0x66726565UL)SEPARATOR /* Free space Box, no version/flags.*/ \
	ISOboxType(k_mdat,0x6D646174UL)SEPARATOR  /* Media data Box, no version/flags.*/ \
	\
	ISOboxType(k_trak,0x7472616BUL)SEPARATOR /* Types for the QuickTime timecode track.*/ \
	ISOboxType(k_tkhd,0x746B6864UL)SEPARATOR \
	ISOboxType(k_edts,0x65647473UL)SEPARATOR \
	ISOboxType(k_elst,0x656C7374UL)SEPARATOR \
	ISOboxType(k_mdia,0x6D646961UL)SEPARATOR \
	ISOboxType(k_mdhd,0x6D646864UL)SEPARATOR \
	ISOboxType(k_tmcd,0x746D6364UL)SEPARATOR \
	ISOboxType(k_mhlr,0x6D686C72UL)SEPARATOR \
	ISOboxType(k_minf,0x6D696E66UL)SEPARATOR \
	ISOboxType(k_stbl,0x7374626CUL)SEPARATOR \
	ISOboxType(k_stsd,0x73747364UL)SEPARATOR \
	ISOboxType(k_stsc,0x73747363UL)SEPARATOR \
	ISOboxType(k_stco,0x7374636FUL)SEPARATOR \
	ISOboxType(k_co64,0x636F3634UL)SEPARATOR \
	ISOboxType(k_dinf,0x64696E66UL)SEPARATOR \
	ISOboxType(k_dref,0x64726566UL)SEPARATOR \
	ISOboxType(k_alis,0x616C6973UL)SEPARATOR \
	\
	ISOboxType(k_meta,0x6D657461UL)SEPARATOR /* Types for the iTunes metadata boxes.*/ \
	ISOboxType(k_ilst,0x696C7374UL)SEPARATOR \
	ISOboxType(k_mdir,0x6D646972UL)SEPARATOR \
	ISOboxType(k_mean,0x6D65616EUL)SEPARATOR \
	ISOboxType(k_name,0x6E616D65UL)SEPARATOR \
	ISOboxType(k_data,0x64617461UL)SEPARATOR \
	ISOboxType(k_hyphens,0x2D2D2D2DUL)SEPARATOR \
	\
	ISOboxType(k_skip,0x736B6970UL)SEPARATOR /* Additional classic QuickTime top level boxes.*/ \
	ISOboxType(k_wide,0x77696465UL)SEPARATOR \
	ISOboxType(k_pnot,0x706E6F74UL)SEPARATOR \
	\
	ISOboxType(k_XMP_,0x584D505FUL) /* The QuickTime variant XMP box.*/ 

#define ISOBoxPrivateList 
#define ISOboxType(x,y) x=y
#define SEPARATOR ,
	enum {
		ISOBoxList
		ISOBoxPrivateList
	};
#undef ISOboxType
#undef SEPARATOR


	bool IsKnownBoxType(XMP_Uns32 boxType) ;
	void TerminateGlobals();

	static XMP_Uns8 k_xmpUUID [16] = { 0xBE, 0x7A, 0xCF, 0xCB, 0x97, 0xA9, 0x42, 0xE8, 0x9C, 0x71, 0x99, 0x94, 0x91, 0xE3, 0xAF, 0xAC };

	struct BoxInfo {
		XMP_Uns32 boxType;         // In memory as native endian!
		XMP_Uns32 headerSize;      // Normally 8 or 16, less than 8 if available space is too small.
		XMP_Uns64 contentSize;     // Always the real size, never 0 for "to EoF".
		XMP_Uns8 idUUID[16];		   // ID of the uuid atom if present
		BoxInfo() : boxType(0), headerSize(0), contentSize(0)
		{
			memset( idUUID, 0, 16 );
		};
	};

	// Get basic info about a box in memory, returning a pointer to the following box.
	const XMP_Uns8 * GetBoxInfo ( const XMP_Uns8 * boxPtr, const XMP_Uns8 * boxLimit,
		                          BoxInfo * info, bool throwErrors = false );

	// Get basic info about a box in a file, returning the offset of the following box. The I/O
	// pointer is left at the start of the box's content. Returns the offset of the following box.
	XMP_Uns64 GetBoxInfo ( XMP_IO* fileRef, const XMP_Uns64 boxOffset, const XMP_Uns64 boxLimit,
		                   BoxInfo * info, bool doSeek = true, bool throwErrors = false );

	//     XMP_Uns32 CountChildBoxes ( XMP_IO* fileRef, const XMP_Uns64 childOffset, const XMP_Uns64 childLimit );

}      // namespace ISO_Media

// =================================================================================================

#endif // __ISOBaseMedia_Support_hpp__
