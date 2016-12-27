// =================================================================================================
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

/**
* Tutorial solution for the AssetRelationships.pdf. An AssetManagement Object is created from
* composed metadata. Then a part is used for deleting all the relationship from composed Asset.
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
#include "XMPCore/Interfaces/IXMPStructureNode.h"

// Ensure XMP templates are instantiated
#include "public/include/XMP.incl_cpp"

// Provide access to the API
#include "public/include/XMP.hpp"

#include "XMPAssetManagement/XMPAssetManagementDefines.h"
#include "XMPAssetManagement/XMPAssetManagementFwdDeclarations.h"
#include "XMPCore/Interfaces/IXMPMetadata.h"
#include "XMPCommon/Interfaces/IUTF8String.h"
#include "XMPAssetManagement/XMPAssetManagement.h"
#include "XMPAssetManagement/Interfaces/IAssetUtilities.h"
#include "XMPAssetManagement/Interfaces/IAssetPart.h"
#include "XMPAssetManagement/Interfaces/IComposedAssetManager.h"
#include "XMPAssetManagement/Interfaces/ICompositionRelationship.h"
#include "XMPCore/Interfaces/IXMPCoreObjectFactory.h"
#include "XMPAssetManagement/XMPAssetManagementFwdDeclarations.h"

using namespace std;

static string GetStringFromFile(string filename)
{
	string buffer;
	string line;

	ifstream in(filename);
	if (in.is_open()) {
		while (getline(in, line))
			buffer = buffer + "\n" + line;
		in.close();
	}
	else {
		cout << "Connot open file for converting into string\n";
	}
	return buffer;
}

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

NS_XMPCORE::spIXMPMetadata static GetIXMPMetadataFromRDF(string filename)
{
	string buffer = GetStringFromFile(filename);

	NS_XMPCORE::spIXMPDOMImplementationRegistry DOMRegistry = NS_XMPCORE::IXMPDOMImplementationRegistry::GetDOMImplementationRegistry();
	NS_XMPCORE::spIXMPDOMParser DOMParser = DOMRegistry->CreateParser("rdf");

	NS_XMPCORE::spIXMPMetadata ixmpmeta = DOMParser->Parse(buffer.c_str());

	return ixmpmeta;

}


/**
*	Initializes the toolkit (XMPCore, XMPFiles & XMPAssetManagement and attempts to open a file
* for reading metadata.  Initially an attempt to open the file is done with a handler, if this
* fails then the file is opened with packet scanning. Once opened, meta is obtained, from meta
* ixmpmeta (DOM based Metadata object) is get. AssetManagement object is created from composed
* ixmpmeta (the above ixmpmetadata). A part is created and used for search & deletion of that
* part from relationship.
*/

int main(int argc, const char * argv[])
{
	string composed_file = "AML_SearchAndDelete_Sample_Input.xmp";
	string composed_out_file = "AML_SearchAndDelete_Sample_Output.xmp";

	if (!SXMPMeta::Initialize())
	{
		cout << "Could not initialize toolkit!";
		return -1;
	}
	NS_XMPCORE::spIXMPMetadata& composed_ixmp_meta = GetIXMPMetadataFromRDF(composed_file);


	//for both the files, metadata has been obtained, get IXMPMeta from SXMPMeta
	NS_XMPCORE::pIXMPCoreObjectFactory factoryObj = NS_XMPCORE::IXMPCoreObjectFactory::GetInstance();
	NS_XMPASSETMANAGEMENT::XMPAM_ClientInitialize();

	//Check if composed / component is not trackable, make it
	NS_XMPASSETMANAGEMENT::pIAssetUtilities utilityObj = NS_XMPASSETMANAGEMENT::IAssetUtilities::GetAssetUtilities();
	if (utilityObj->IsTrackable(composed_ixmp_meta) == false)
		utilityObj->MakeTrackable(composed_ixmp_meta);

	//composed/toPart
	NS_XMPASSETMANAGEMENT::spIAssetPart composedassetpart = NS_XMPASSETMANAGEMENT::IAssetPart::CreateStandardGenericPart(NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartComponentMetadata);

	//creating ComposedAssetManager
	NS_XMPASSETMANAGEMENT::spIComposedAssetManager composedAsset = NS_XMPASSETMANAGEMENT::IComposedAssetManager::CreateComposedAssetManager(composed_ixmp_meta);

	//Get list of all relationship and traverse it
	NS_XMPASSETMANAGEMENT::spcICompositionRelationshipList relationshipNodes = composedAsset->GetAllRelationships();
	for (NS_XMPASSETMANAGEMENT::cICompositionRelationshipList::iterator it = relationshipNodes->begin(); it != relationshipNodes->end(); ++it) {
		//get part for part specific data
		NS_XMPASSETMANAGEMENT::spcIAssetPart assetPart = (*it)->GetComponentPart();
		
		switch (assetPart->GetType()) {
		case NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeGeneric:
			cout << "Part type is NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeGeneric";
			break;
		case NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeTime:
			cout << "Part type is NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeTime";
			break;
		case NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeArtboard:
			cout << "Part type is NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeArtboard";
			break;
		case NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeLayer:
			cout << "Part type is NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypeLayer";
			break;
		case NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypePage:
			cout << "Part type is NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartTypePage";
			break;
		default:
			;
		}
		//get other relationship properties
		NS_XMPCOMMON::spcIUTF8String filepath = (*it)->GetComponentFilePath();
	}


	//call GetRelationships(toPart) to get all relationship which matches a part value, and traverse it
	relationshipNodes = composedAsset->GetRelationships(composedassetpart);
	for (NS_XMPASSETMANAGEMENT::cICompositionRelationshipList::iterator it = relationshipNodes->begin(); it != relationshipNodes->end(); ++it) {
		//get part for part specific data
		NS_XMPASSETMANAGEMENT::spcIAssetPart assetPart =  (*it)->GetComponentPart();

		//get other relationship properties
		NS_XMPCOMMON::spcIUTF8String filepath = (*it)->GetComponentFilePath();
	}

	////// Remove relationship which maches the part
	composedAsset->RemoveComponents(composedassetpart);

	//Get all the relationship and check verify all the Relationship having the above part must not be present
	relationshipNodes = composedAsset->GetAllRelationships();
	for (NS_XMPASSETMANAGEMENT::cICompositionRelationshipList::iterator it = relationshipNodes->begin(); it != relationshipNodes->end(); ++it) {
		//get part for part specific data
		NS_XMPASSETMANAGEMENT::spcIAssetPart assetPart = (*it)->GetComponentPart();

		//get other relationship properties
		NS_XMPCOMMON::spcIUTF8String filepath = (*it)->GetComponentFilePath();
	}

	//verify the added node from pantry
	SerializeIXMPMetadata(composed_ixmp_meta, composed_out_file);

	NS_XMPASSETMANAGEMENT::XMPAM_ClientTerminate();

	SXMPMeta::Terminate();

	return 0;
}

