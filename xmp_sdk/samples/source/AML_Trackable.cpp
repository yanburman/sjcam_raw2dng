// =================================================================================================
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

/**
* Tutorial solution for the Walkthrough 1 in the XMP Programmers Guide, Opening files and reading XMP.
* Demonstrates the basic use of the XMPAssetManagement, obtaining XMP from a file  and examining it
* for tracking puropose. A document if trackable only if(xmpMM:OriginalDocumentID, xmpMM:DocumentID
* and xmpMM:InstanceID) are present 
*/

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>

// Must be defined to instantiate template classes
#define TXMP_STRING_TYPE std::string 

// Must be defined to give access to XMPFiles
#define XMP_INCLUDE_XMPFILES 0

#define ENABLE_NEW_DOM_MODEL 1

#include "XMPCore/Interfaces/IXMPDOMImplementationRegistry.h"
#include "XMPCore/Interfaces/IXMPDOMParser.h"
#include "XMPCore/Interfaces/IXMPDOMSerializer.h"
#include "XMPCore/Interfaces/IXMPMetadata.h"
#include "XMPCore/Interfaces/IXMPCoreObjectFactory.h"
#include "XMPCore/Interfaces/IXMPSimpleNode.h"

// Ensure XMP templates are instantiated
#include "public/include/XMP.incl_cpp"

// Provide access to the API
#include "public/include/XMP.hpp"

#include "XMPCore/Interfaces/IXMPMetadata.h"
#include "XMPCommon/Interfaces/IUTF8String.h"
#include "XMPAssetManagement/XMPAssetManagementFwdDeclarations.h"
#include "XMPAssetManagement/XMPAssetManagement.h"
#include "XMPAssetManagement/Interfaces/IAssetUtilities.h"

using namespace std; 

void static SerializeIXMPMetadata(NS_XMPCORE::spIXMPMetadata imetadata, string outfile = "")
{
	NS_XMPCORE::spIXMPDOMImplementationRegistry DOMRegistry = NS_XMPCORE::IXMPDOMImplementationRegistry::GetDOMImplementationRegistry();
	NS_XMPCORE::spIXMPDOMSerializer DOMSerializer = DOMRegistry->CreateSerializer("rdf");
	std::string targetfilebuffer = DOMSerializer->Serialize(imetadata)->c_str();

	//std::cout << targetfilebuffer << std::endl;

	//write into file
	if (!outfile.empty()) {
		ofstream out(outfile);
		if (out.is_open()) {
			out << targetfilebuffer;
			out.close();
		}
	}
}

static string GetStringFromFile(string& filename)
{
	string buffer;
	string line;

	ifstream in(filename);
	if (in.is_open()) {
		while (getline(in, line))
			buffer = buffer + "\n" + line;
		in.close();
	}
	return buffer;
}

NS_XMPCORE::spIXMPMetadata static GetIXMPMetadataFromRDF(string filename)
{
	string buffer = GetStringFromFile(filename);

	NS_XMPCORE::spIXMPDOMImplementationRegistry DOMRegistry = NS_XMPCORE::IXMPDOMImplementationRegistry::GetDOMImplementationRegistry();
	NS_XMPCORE::spIXMPDOMParser DOMParser = DOMRegistry->CreateParser("rdf");

	NS_XMPCORE::spIXMPMetadata ixmpmeta = DOMParser->Parse(buffer.c_str());

	return ixmpmeta;

}

/**
*	Initializes the toolkit and attempts to open a file for reading metadata.  Initially
* an attempt to open the file is done with a handler, if this fails then the file is opened with
* packet scanning. Once the file is open, it's metadata is obtained. From which a check for IDs
* is done, if missing these IDs are embedded.
*/
int main ( int argc, const char * argv[] )
{
	string input_file = "noids_source_input.xmp";
	string output_file = "noids_source_output.xmp";

	if (!SXMPMeta::Initialize())
	{
		cout << "Could not initialize toolkit!";
		return -1;
	}
	XMP_OptionBits options = 0;

	NS_XMPCORE::spIXMPMetadata ixmpmeta = GetIXMPMetadataFromRDF(input_file);

	NS_XMPCORE::pIXMPCoreObjectFactory factoryObj = NS_XMPCORE::IXMPCoreObjectFactory::GetInstance();
	NS_XMPASSETMANAGEMENT::XMPAM_ClientInitialize();

	NS_XMPASSETMANAGEMENT::pIAssetUtilities utilityObj = NS_XMPASSETMANAGEMENT::IAssetUtilities::GetAssetUtilities();
	bool isTrackable = utilityObj->IsTrackable(ixmpmeta);

	if (isTrackable) {
		cout << "File " << input_file << " is Trackable" << endl;

		NS_XMPCORE::spIXMPSimpleNode node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "OriginalDocumentID"));
		if (node)
			cout << "xmpMM:OriginalDocumentID is " << node->GetValue()->c_str() << endl;
				
		node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "DocumentID"));
		if (node)
			cout << "xmpMM:DocumentID is " << node->GetValue()->c_str() << endl;
				
		node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "InstanceID"));
		if (node)
			cout << "xmpMM:InstanceID is " << node->GetValue()->c_str() << endl;
	}
	else {
		bool ret = utilityObj->MakeTrackable(ixmpmeta);
		
		SerializeIXMPMetadata(ixmpmeta, output_file); //serialize ixmpmeta into o/p file

		if (ret) {
			NS_XMPCORE::spIXMPSimpleNode node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "OriginalDocumentID"));
			if (node)
				cout << "xmpMM:OriginalDocumentID is " << node->GetValue()->c_str() << endl;

			node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "DocumentID"));
			if (node)
				cout << "xmpMM:DocumentID is " << node->GetValue()->c_str() << endl;

			node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(ixmpmeta->GetNode(kXMP_NS_XMP_MM, "InstanceID"));
			if (node)
				cout << "xmpMM:InstanceID is " << node->GetValue()->c_str() << endl;
		}
	}

	NS_XMPASSETMANAGEMENT::XMPAM_ClientTerminate();

	SXMPMeta::Terminate();

	return 0;
}

