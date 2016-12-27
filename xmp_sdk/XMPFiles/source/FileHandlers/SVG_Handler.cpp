// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2015 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// This file includes implementation of SVG metadata, according to Scalable Vector Graphics (SVG) 1.1 Specification. 
// "https://www.w3.org/TR/2003/REC-SVG11-20030114/"
// Copyright © 1994-2002 World Wide Web Consortium, (Massachusetts Institute of Technology, 
// Institut National de Recherche en Informatique et en Automatique, Keio University). 
// All Rights Reserved . http://www.w3.org/Consortium/Legal
//
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"

#include "XMPFiles/source/FileHandlers/SVG_Handler.hpp"

using namespace std;

/*
	 Currently supporting only UTF-8 encoded SVG
*/

// =================================================================================================
// SVG_CheckFormat
// ===============

bool SVG_CheckFormat( XMP_FileFormat format,
	XMP_StringPtr  filePath,
	XMP_IO *       fileRef,
	XMPFiles *     parent )
{
	// 8K buffer is provided just to handle maximum SVG files
	// We can't check for SVG element in whole file which could take a lot of time for valid XML files
	IgnoreParam( filePath ); IgnoreParam( parent );

	XMP_Assert( format == kXMP_SVGFile );

	fileRef->Rewind();

	XMP_Uns8 buffer[ 1024 ];
	
	// Reading 4 bytes for BOM
	XMP_Uns32 bytesRead = fileRef->Read( buffer, 4 );
	if ( bytesRead != 4 )
		return false;

	// Checking for UTF-16 BOM and UTF-32 BOM
	if ( ( buffer[ 0 ] == 0xFF && buffer[ 1 ] == 0xFE ) || ( buffer[ 0 ] == 0xFE && buffer[ 1 ] == 0xFF ) || ( buffer[ 0 ] == buffer[ 1 ] == 0x00 && buffer[ 2 ] == 0xFE && buffer[ 3 ] == 0xFF ) )
	{
		return false;
	}

	// Initially we are intersted only in "svg" element.
	SVG_Adapter * svgChecker = new SVG_Adapter();
	if ( svgChecker == 0 )
		return false;

	bool isSVG = false;

	fileRef->Rewind();
	for ( XMP_Uns8 index = 0; index < 8; ++index )
	{
		XMP_Int32 ioCount = fileRef->Read( buffer, sizeof( buffer ) );
		if ( ioCount == 0 ) break;

		// Checking for well formed XML
		if ( !svgChecker->ParseBufferNoThrow( buffer, ioCount, false /* not the end */ ) )
			break;

		if ( svgChecker->tree.GetNamedElement( "http://www.w3.org/2000/svg", "svg" ) )
		{
			isSVG = true;
			break;
		}
	}

	if ( svgChecker )
		delete ( svgChecker );

	return isSVG;

}	// SVG_CheckFormat

// =================================================================================================
// SVG_MetaHandlerCTor
// ===================

XMPFileHandler * SVG_MetaHandlerCTor( XMPFiles * parent )
{
	return new SVG_MetaHandler( parent );

}	// SVG_MetaHandlerCTor

// =================================================================================================
// SVG_MetaHandler::SVG_MetaHandler
// ================================

SVG_MetaHandler::SVG_MetaHandler( XMPFiles * _parent ) : svgNode( 0 ), svgAdapter( 0 ), isTitleUpdateReq( false ), isDescUpdateReq( false )
{
	this->parent = _parent;
	this->handlerFlags = kSVG_HandlerFlags;
	this->stdCharForm = kXMP_Char8Bit;

}

// =================================================================================================
// SVG_MetaHandler::~SVG_MetaHandler
// =================================

SVG_MetaHandler::~SVG_MetaHandler()
{

	if ( this->svgAdapter != 0 ) 
	{
		delete ( this->svgAdapter );
		this->svgAdapter = 0;
	}
}

// =================================================================================================
// SVG_MetaHandler::GetSerializeOptions
// ===================================
//
// Override default implementation to ensure Canonical packet.

XMP_OptionBits SVG_MetaHandler::GetSerializeOptions()
{

	return ( kXMP_UseCanonicalFormat );

} // SVG_MetaHandler::GetSerializeOptions

// =================================================================================================
// SVG_MetaHandler::CacheFileData
// ==============================

void SVG_MetaHandler::CacheFileData() 
{
	XMP_Assert( !this->containsXMP );

	XMP_IO * fileRef = this->parent->ioRef;
	
	XMP_Uns8 marker[ 4 ];
	fileRef->Rewind();
	fileRef->Read( marker, 4 );

	// Checking for UTF-16 BOM and UTF-32 BOM
	if ( ( marker[ 0 ] == 0xFF && marker[ 1 ] == 0xFE ) || ( marker[ 0 ] == 0xFE && marker[ 1 ] == 0xFF ) || ( marker[ 0 ] == marker[ 1 ] == 0x00 && marker[ 2 ] == 0xFE && marker[ 3 ] == 0xFF ) )
	{
		XMP_Error error( kXMPErr_BadXML, "Invalid SVG file" );
		this->NotifyClient( &this->parent->errorCallback, kXMPErrSev_OperationFatal, error );
	}
		
	// Creating a new SVG Parser
	svgAdapter = new SVG_Adapter();
	if ( svgAdapter == 0 )
		XMP_Throw( "SVG_MetaHandler: Can't create SVG adapter", kXMPErr_NoMemory );
	svgAdapter->SetErrorCallback( &this->parent->errorCallback );

	// Registering all the required tags to SVG Parser
	svgAdapter->RegisterPI( "xpacket" );
	svgAdapter->RegisterElement( "metadata", "svg" );
	svgAdapter->RegisterElement( "xmpmeta", "metadata" );
	svgAdapter->RegisterElement( "RDF", "metadata" );
	svgAdapter->RegisterElement( "title", "svg" );
	svgAdapter->RegisterElement( "desc", "svg" );

	// Parsing the whole buffer
	fileRef->Rewind();
	XMP_Uns8 buffer[ 64 * 1024 ];
	while ( true ) {
		XMP_Int32 ioCount = fileRef->Read( buffer, sizeof( buffer ) );
		if ( ioCount == 0 || !svgAdapter->IsParsingRequire() ) break;
		svgAdapter->ParseBuffer( buffer, ioCount, false /* not the end */ );
	}
	svgAdapter->ParseBuffer( 0, 0, true );	// End the parse.

	XML_Node & xmlTree = this->svgAdapter->tree;
	XML_NodePtr rootElem = 0;

	for ( size_t i = 0, limit = xmlTree.content.size(); i < limit; ++i )
	{
		if ( xmlTree.content[ i ]->kind == kElemNode ) {
			rootElem = xmlTree.content[ i ];
		}
	}
	if ( rootElem == 0 )
		XMP_Throw( "Not a valid SVG File", kXMPErr_BadFileFormat );

	XMP_StringPtr rootLocalName = rootElem->name.c_str() + rootElem->nsPrefixLen;

	if ( ! XMP_LitMatch( rootLocalName, "svg" ) )
		XMP_Throw( "Not able to parse such SVG File", kXMPErr_BadFileFormat );
	
	// Making SVG node as Root Node
	svgNode = rootElem;

	bool FoundPI = false;
	bool FoundWrapper = false;
	XML_NodePtr metadataNode = svgNode->GetNamedElement( rootElem->ns.c_str(), "metadata" );

	// We are intersted only in the Metadata tag of outer SVG element
	// XMP should be present only in metadata Node of SVG
	if ( metadataNode != NULL )
	{
		XMP_Int64 packetLength = -1;
		XMP_Int64 packetOffset = -1;
		XMP_Int64 PIOffset = svgAdapter->GetPIOffset( "xpacket", 1 );
		OffsetStruct wrapperOffset = svgAdapter->GetElementOffsets( "xmpmeta" );
		OffsetStruct rdfOffset = svgAdapter->GetElementOffsets( "RDF" );
		
		// Checking XMP PI's position
		if ( PIOffset != -1 )
		{
			if ( wrapperOffset.startOffset != -1 && wrapperOffset.startOffset < PIOffset )
				packetOffset = wrapperOffset.startOffset;
			else
			{
				XMP_Int64 trailerOffset = svgAdapter->GetPIOffset( "xpacket", 2 );
				XML_NodePtr trailerNode = metadataNode->GetNamedElement( "", "xpacket", 1 );
				if ( trailerOffset != -1 || trailerNode != 0 )
				{
					packetLength = 2;								// "<?" = 2
					packetLength += trailerNode->name.length();		// Node's name
					packetLength += 1;								// Empty Space after Node's name
					packetLength += trailerNode->value.length();	// Value
					packetLength += 2;								// "?>" = 2
					packetLength += ( trailerOffset - PIOffset );
					packetOffset = PIOffset;
				}
			}
		}
		else if ( wrapperOffset.startOffset != -1 )		// XMP Wrapper is present without PI
		{
			XML_NodePtr wrapperNode = metadataNode->GetNamedElement( "adobe:ns:meta/", "xmpmeta" );
			if ( wrapperNode != 0 )
			{
				std::string trailerWrapper = "</x:xmpmeta>";
				packetLength = trailerWrapper.length();
				packetLength += ( wrapperOffset.endOffset - wrapperOffset.startOffset );
				packetOffset = wrapperOffset.startOffset;
			}
		}
		else		// RDF packet is present without PI and wrapper
		{
			XML_NodePtr rdfNode = metadataNode->GetNamedElement( "http://www.w3.org/1999/02/22-rdf-syntax-ns#", "RDF" );
			if ( rdfNode != 0 )
			{
				std::string rdfTrailer = "</rdf:RDF>";
				packetLength = rdfTrailer.length();
				packetLength += ( rdfOffset.endOffset - rdfOffset.startOffset );
				packetOffset = rdfOffset.startOffset;
			}
		}

		// Fill the necesarry information and packet with XMP data
		if ( packetOffset != -1 )
		{
			this->packetInfo.offset = packetOffset;
			this->packetInfo.length = ( XMP_Int32 ) packetLength;
			this->xmpPacket.assign( this->packetInfo.length, ' ' );
			fileRef->Seek( packetOffset, kXMP_SeekFromStart );
			fileRef->ReadAll( ( void* )this->xmpPacket.data(), this->packetInfo.length );
			FillPacketInfo( this->xmpPacket, &this->packetInfo );
			this->containsXMP = true;
			return;
		}
	}
	this->containsXMP = false;	

}	// SVG_MetaHandler::CacheFileData

// =================================================================================================
// SVG_MetaHandler::ProcessXMP
// ==============================

void SVG_MetaHandler::ProcessXMP()
{
	//
	// Here we are intersted in Only 2 childs, title and desc
	//
	this->processedXMP = true;	// Make sure we only come through here once.
	
	if ( svgNode == NULL )
		return;

	if ( !this->xmpPacket.empty() ) {
		XMP_Assert( this->containsXMP );
		this->xmpObj.ParseFromBuffer( this->xmpPacket.c_str(), ( XMP_StringLen )this->xmpPacket.size() );
	}
	
	// Description
	XML_NodePtr descNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "desc" );
	if ( descNode != 0 && descNode->content.size() == 1 && descNode->content[0]->kind == kCDataNode )
	{
		this->xmpObj.SetLocalizedText( kXMP_NS_DC, "description", "", "x-default", descNode->content[0]->value, kXMP_DeleteExisting );
		this->containsXMP = true;
	}

	// Title
	XML_NodePtr titleNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "title" );
	if ( titleNode != 0 && titleNode->content.size() == 1 && titleNode->content[ 0 ]->kind == kCDataNode )
	{
		this->xmpObj.SetLocalizedText( kXMP_NS_DC, "title", "", "x-default", titleNode->content[0]->value, kXMP_DeleteExisting );
		this->containsXMP = true;
	}
	
}	// SVG_MetaHandler::ProcessXMP

// =================================================================================================
// SVG_MetaHandler::ProcessTitle
// ===========================
// It is handling the updation and deletion case
void SVG_MetaHandler::ProcessTitle( XMP_IO* sourceRef, XMP_IO * destRef, const std::string &value, XMP_Int64 &currentOffset, const OffsetStruct & titleOffset )
{
	if ( value.empty() )
	{
		XIO::Copy( sourceRef, destRef, titleOffset.startOffset - currentOffset );
		sourceRef->Seek( titleOffset.nextOffset, kXMP_SeekFromStart );
		currentOffset = titleOffset.nextOffset;
	}
	else
	{
		std::string titleElement = "<title>";
		XIO::Copy( sourceRef, destRef, titleOffset.startOffset - currentOffset + titleElement.length() );
		destRef->Write( value.c_str(), static_cast< int >( value.length() ) );
		sourceRef->Seek( titleOffset.endOffset, kXMP_SeekFromStart );
		currentOffset = titleOffset.endOffset;
	}
}	// SVG_MetaHandler::ProcessTitle

// =================================================================================================
// SVG_MetaHandler::ProcessDescription
// ===========================
// It is handling the updation and deletion case
void SVG_MetaHandler::ProcessDescription( XMP_IO* sourceRef, XMP_IO * destRef, const std::string &value, XMP_Int64 &currentOffset, const OffsetStruct & descOffset )
{
	if ( value.empty() )
	{
		XIO::Copy( sourceRef, destRef, descOffset.startOffset - currentOffset );
		sourceRef->Seek( descOffset.nextOffset, kXMP_SeekFromStart );
		currentOffset = descOffset.nextOffset;
	}
	else
	{
		std::string descElement = "<desc>";
		XIO::Copy( sourceRef, destRef, descOffset.startOffset - currentOffset + descElement.length() );
		destRef->Write( value.c_str(), static_cast< int >( value.length() ) );
		sourceRef->Seek( descOffset.endOffset, kXMP_SeekFromStart );
		currentOffset = descOffset.endOffset;
	}

}	// SVG_MetaHandler::ProcessDescription

// =================================================================================================
// SVG_MetaHandler::InsertNewTitle
// ===========================
// It is handling the insertion case
void SVG_MetaHandler::InsertNewTitle( XMP_IO * destRef, const std::string &value )
{
	std::string titleElement = "<title>";
	destRef->Write( titleElement.c_str(), static_cast< int >( titleElement.length() ) );
	destRef->Write( value.c_str(), static_cast< int >( value.length() ) );
	titleElement = "</title>\n";
	destRef->Write( titleElement.c_str(), static_cast< int >( titleElement.length() ) );

}	// SVG_MetaHandler::InsertNewTitle

// =================================================================================================
// SVG_MetaHandler::InsertNewDescription
// ===========================
// It is handling the insertion case
void SVG_MetaHandler::InsertNewDescription( XMP_IO * destRef, const std::string &value )
{
	std::string descElement = "<desc>";
	destRef->Write( descElement.c_str(), static_cast< int >( descElement.length() ) );
	destRef->Write( value.c_str(), static_cast< int >( value.length() ) );
	descElement = "</desc>\n";
	destRef->Write( descElement.c_str(), static_cast< int >( descElement.length() ) );

}	// SVG_MetaHandler::InsertNewDescription

// =================================================================================================
// SVG_MetaHandler::InsertNewMetadata
// ===========================
// It is handling the insertion case
void SVG_MetaHandler::InsertNewMetadata( XMP_IO * destRef, const std::string &value )
{

	std::string metadataElement = "<metadata>";
	destRef->Write( metadataElement.c_str(), static_cast< int >( metadataElement.length() ) );
	destRef->Write( value.c_str(), static_cast< int >( value.length() ) );
	metadataElement = "</metadata>\n";
	destRef->Write( metadataElement.c_str(), static_cast< int >( metadataElement.length() ) );

}	// SVG_MetaHandler::InsertNewMetadata

// =================================================================================================
// SVG_MetaHandler::UpdateFile
// ===========================

void SVG_MetaHandler::UpdateFile( bool doSafeUpdate )
{
	XMP_Assert( !doSafeUpdate );	// This should only be called for "unsafe" updates.
	
	XMP_IO* sourceRef = this->parent->ioRef;

	if ( sourceRef == NULL || svgNode == NULL )
		return;

	// Checking whether Title updation requires or not
	std::string title;
	XML_NodePtr titleNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "title" );
	(void) this->xmpObj.GetLocalizedText( kXMP_NS_DC, "title", "", "x-default", 0, &title, 0 );
	if ( ( titleNode == NULL ) == ( title.empty() ) )
	{
		if ( titleNode != NULL && titleNode->content.size() == 1 && titleNode->content[ 0 ]->kind == kCDataNode && !XMP_LitMatch( titleNode->content[ 0 ]->value.c_str(), title.c_str() ) )
			isTitleUpdateReq = true;
	}
	else
		isTitleUpdateReq = true;

	// Checking whether Description updation requires or not
	std::string description;
	XML_NodePtr descNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "desc" );
	( void ) this->xmpObj.GetLocalizedText( kXMP_NS_DC, "description", "", "x-default", 0, &description, 0 );
	if ( ( descNode == NULL ) == ( description.empty() ) )
	{
		if ( descNode != NULL && descNode->content.size() == 1 && descNode->content[ 0 ]->kind == kCDataNode && !XMP_LitMatch( descNode->content[ 0 ]->value.c_str(), description.c_str() ) )
			isDescUpdateReq = true;
	}
	else
		isDescUpdateReq = true;

	//	If any updation is required then don't do inplace replace
	bool isUpdateRequire = isTitleUpdateReq | isDescUpdateReq | (this->packetInfo.offset == kXMPFiles_UnknownOffset);

	// Inplace Updation of XMP
	if ( !isUpdateRequire && this->xmpPacket.size() == this->packetInfo.length )
	{
		sourceRef->Seek( this->packetInfo.offset, kXMP_SeekFromStart );
		sourceRef->Write( this->xmpPacket.c_str(), static_cast< int >( this->xmpPacket.size() ) );
	}
	else
	{
		// Inplace is not possibe, So perform full updation
		try
		{
			XMP_IO* tempRef = sourceRef->DeriveTemp();
			this->WriteTempFile( tempRef );
		}
		catch ( ... )
		{
			sourceRef->DeleteTemp();
			throw;
		}

		sourceRef->AbsorbTemp();
	}

	this->needsUpdate = false;

}	// SVG_MetaHandler::UpdateFile

// =================================================================================================
// SVG_MetaHandler::WriteTempFile
// ==============================
//
void SVG_MetaHandler::WriteTempFile( XMP_IO* tempRef )
{
	XMP_Assert( this->needsUpdate );

	XMP_IO* sourceRef = this->parent->ioRef;
	if ( sourceRef == NULL || svgNode == NULL )
		return;

	tempRef->Rewind();
	sourceRef->Rewind();

	XMP_Int64 currentOffset = svgAdapter->firstSVGElementOffset;
	XIO::Copy( sourceRef, tempRef, currentOffset );

	OffsetStruct titleOffset = svgAdapter->GetElementOffsets( "title" );
	OffsetStruct descOffset = svgAdapter->GetElementOffsets( "desc" );
	OffsetStruct metadataOffset = svgAdapter->GetElementOffsets( "metadata" );

	std::string title;
	std::string description;

	XML_NodePtr titleNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "title" );
	( void ) this->xmpObj.GetLocalizedText( kXMP_NS_DC, "title", "", "x-default", 0, &title, 0 );

	XML_NodePtr descNode = svgNode->GetNamedElement( svgNode->ns.c_str(), "desc" );
	( void ) this->xmpObj.GetLocalizedText( kXMP_NS_DC, "description", "", "x-default", 0, &description, 0 );

	// Need to cover the case of both workflows
	// This would have been called after inplace is not possible
	// This would have called for safe update
	if ( !isTitleUpdateReq )
	{
		if ( ( titleNode == NULL ) == ( title.empty() ) )
		{
			if ( titleNode != NULL && titleNode->content.size() == 1 && titleNode->content[ 0 ]->kind == kCDataNode && !XMP_LitMatch( titleNode->content[ 0 ]->value.c_str(), title.c_str() ) )
				isTitleUpdateReq = true;
		}
		else
			isTitleUpdateReq = true;
	}
	if ( !isDescUpdateReq )
	{
		if ( ( descNode == NULL ) == ( description.empty() ) )
		{
			if ( descNode != NULL && descNode->content.size() == 1 && descNode->content[ 0 ]->kind == kCDataNode &&  !XMP_LitMatch( descNode->content[ 0 ]->value.c_str(), description.c_str() ) )
				isDescUpdateReq = true;
		}
		else
			isDescUpdateReq = true;
	}

	// Initial Insertion/Updation

	// Insert/Update Title if requires
	// Don't insert/update it if Metadata or desc child comes before title child
	bool isTitleWritten = !isTitleUpdateReq;
	if ( isTitleUpdateReq )
	{
		// Insertion Case
		if ( titleNode == NULL )
		{
			InsertNewTitle( tempRef, title );
			isTitleWritten = true;
		}
		else if ( ( descOffset.startOffset == -1 || titleOffset.startOffset < descOffset.startOffset )	// Updation/Deletion Case
			&& ( metadataOffset.startOffset == -1 || titleOffset.startOffset < metadataOffset.startOffset ) )
		{
			ProcessTitle( sourceRef, tempRef, title, currentOffset, titleOffset );
			isTitleWritten = true;
		}
	}

	// Insert/Update Description if requires
	// Don't insert/update it if Metadata child comes before desc child
	bool isDescWritten = !isDescUpdateReq;
	if ( isDescUpdateReq )
	{
		if ( descNode == NULL )
		{
			if ( titleOffset.nextOffset != -1 )
			{
				XIO::Copy( sourceRef, tempRef, titleOffset.nextOffset - currentOffset );
				currentOffset = titleOffset.nextOffset;
			}
			InsertNewDescription( tempRef, description );
			isDescWritten = true;
		}
		else if ( metadataOffset.startOffset == -1 || descOffset.startOffset < metadataOffset.startOffset )
		{
			ProcessDescription( sourceRef, tempRef, description, currentOffset, descOffset );
			isDescWritten = true;
		}
	}

	// Insert/Update Metadata if requires
	// Don't insert/update it if case is DTM
	bool isMetadataWritten = false;
	if ( metadataOffset.startOffset == -1 )
	{
		if ( descOffset.nextOffset != -1 )
		{
			XIO::Copy( sourceRef, tempRef, descOffset.nextOffset - currentOffset );
			currentOffset = descOffset.nextOffset;
		}
		else if ( titleOffset.nextOffset != -1 )
		{
			XIO::Copy( sourceRef, tempRef, titleOffset.nextOffset - currentOffset );
			currentOffset = titleOffset.nextOffset;
		}
		InsertNewMetadata( tempRef, this->xmpPacket );
		isMetadataWritten = true;
	}
	else if ( !( !isTitleWritten && isDescWritten && titleOffset.startOffset < metadataOffset.startOffset ) )		// Not DTM
	{
		// No XMP packet was present in the file
		if ( this->packetInfo.offset == kXMPFiles_UnknownOffset )
		{
			std::string metadataElement = "<metadata>";
			XIO::Copy( sourceRef, tempRef, metadataOffset.startOffset - currentOffset + metadataElement.length() );
			currentOffset = sourceRef->Offset();
			tempRef->Write( this->xmpPacket.c_str(), static_cast< int >( this->xmpPacket.length() ) );
		}
		else	// Replace XMP Packet
		{
			XIO::Copy( sourceRef, tempRef, this->packetInfo.offset - currentOffset );
			tempRef->Write( this->xmpPacket.c_str(), static_cast< int >( this->xmpPacket.length() ) );
			sourceRef->Seek( this->packetInfo.offset + this->packetInfo.length, kXMP_SeekFromStart );
			currentOffset = sourceRef->Offset();
		}
		isMetadataWritten = true;
	}

	// If simple cases was followed then copy rest file
	if ( isTitleWritten && isDescWritten && isMetadataWritten )
	{
		XIO::Copy( sourceRef, tempRef, ( sourceRef->Length() - currentOffset ) );
		return;
	}

	// If the case is not Simple (TDM) then perform these operations
	if ( isDescWritten )		// TDM, DTM, DMT
	{
		if ( !isTitleWritten )		// DTM, DMT
		{
			if ( titleOffset.startOffset < metadataOffset.startOffset )		// DTM
			{
				ProcessTitle( sourceRef, tempRef, title, currentOffset, titleOffset );
				isTitleWritten = true;

				if ( this->packetInfo.offset == kXMPFiles_UnknownOffset )
				{
					std::string metadataElement = "<metadata>";
					XIO::Copy( sourceRef, tempRef, metadataOffset.startOffset - currentOffset + metadataElement.length() );
					currentOffset = sourceRef->Offset();
					tempRef->Write( this->xmpPacket.c_str(), static_cast< int >( this->xmpPacket.length() ) );
				}
				else
				{
					XIO::Copy( sourceRef, tempRef, this->packetInfo.offset - currentOffset );
					tempRef->Write( this->xmpPacket.c_str(), static_cast< int >( this->xmpPacket.length() ) );
					sourceRef->Seek( this->packetInfo.offset + this->packetInfo.length, kXMP_SeekFromStart );
					currentOffset = sourceRef->Offset();
				}
				isMetadataWritten = true;

			}
			else	// DMT
			{
				ProcessTitle( sourceRef, tempRef, title, currentOffset, titleOffset );
				isTitleWritten = true;
			}
		}
		// Else
		// Would have already covered this case: TDM

	}
	else		//  TMD, MDT, MTD
	{
		if ( isTitleWritten )		// TMD
		{
			ProcessDescription( sourceRef, tempRef, description, currentOffset, descOffset );
			isDescWritten = true;
		}
		else		// MDT or MTD
		{
			if ( titleOffset.startOffset < descOffset.startOffset )	// MTD
			{
				ProcessTitle( sourceRef, tempRef, title, currentOffset, titleOffset );
				isTitleWritten = true;

				ProcessDescription( sourceRef, tempRef, description, currentOffset, descOffset );
				isDescWritten = true;
			}
			else		// MDT
			{
				ProcessDescription( sourceRef, tempRef, description, currentOffset, descOffset );
				isDescWritten = true;

				ProcessTitle( sourceRef, tempRef, title, currentOffset, titleOffset );
				isTitleWritten = true;
			}
		}
	}

	// Finally Everything would have been written
	XMP_Enforce( isTitleWritten && isDescWritten && isMetadataWritten );
	XIO::Copy( sourceRef, tempRef, ( sourceRef->Length() - currentOffset ) );
	this->needsUpdate = false;

}	// SVG_MetaHandler::WriteTempFile
