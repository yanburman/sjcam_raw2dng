// =================================================================================================
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

/**
* Tutorial solution for the AssetRelationships.pdf. Creating generic parts, parts, AssetManager
* for composed document. Demonstrate the use of AddComponent() for composite document and parts.
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
* ixmpmeta and a relationship is added into it. And later verifies for the same from ingredient
* entry.
*/

int main ( int argc, const char * argv[] )
{
	string composed_file = "AddBasicPart_InSimpleDocument_input_composite_doc.xmp";
	string component_file = "AddBasicPart_InSimpleDocument_input_component_doc.xmp";
	string composed_out_file = "AddBasicPart_InSimpleDocument_OUTPUT_composite_doc.xmp";

	if (!SXMPMeta::Initialize())
	{
		cout << "Could not initialize toolkit!";
		return -1;
	}

	NS_XMPCORE::spIXMPMetadata& component_ixmp_meta = GetIXMPMetadataFromRDF(component_file);
	NS_XMPCORE::spIXMPMetadata& composed_ixmp_meta = GetIXMPMetadataFromRDF(composed_file);
	// Create the xmp object and get the xmp data
	//componentFile.GetXMP(&component_sxmp_meta);

	//Get IXMPMeta from SXMPMetadata
	//component_ixmp_meta = component_sxmp_meta.GetIXMPMetadata();

	//for both the files, metadata has been obtained, get IXMPMeta from SXMPMeta
	NS_XMPCORE::pIXMPCoreObjectFactory factoryObj = NS_XMPCORE::IXMPCoreObjectFactory::GetInstance();
	NS_XMPASSETMANAGEMENT::XMPAM_ClientInitialize();

	//create a standard generic part
	NS_XMPASSETMANAGEMENT::spIAssetPart composedassetpart = NS_XMPASSETMANAGEMENT::IAssetPart::CreateStandardGenericPart(NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartComponentMetadata);

	//create standard generic part
	NS_XMPASSETMANAGEMENT::spIAssetPart componentassetpart = NS_XMPASSETMANAGEMENT::IAssetPart::CreateStandardGenericPart(NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartComponentContent);


	//Check if composed / component is not trackable, make it
	NS_XMPASSETMANAGEMENT::pIAssetUtilities utilityObj = NS_XMPASSETMANAGEMENT::IAssetUtilities::GetAssetUtilities();
	if (utilityObj->IsTrackable(composed_ixmp_meta) == false)
		utilityObj->MakeTrackable(composed_ixmp_meta);

	if (utilityObj->IsTrackable(component_ixmp_meta) == false)
		utilityObj->MakeTrackable(component_ixmp_meta);


	//creating ComposedAssetManager
	NS_XMPASSETMANAGEMENT::spIComposedAssetManager composedAsset = NS_XMPASSETMANAGEMENT::IComposedAssetManager::CreateComposedAssetManager(composed_ixmp_meta);
		
	//call AddComponent API (toPart, from ixmpmeta, fromPart) to compose fromPart into composed doc at toPart
	const NS_XMPCORE::spcIXMPStructureNode & component = component_ixmp_meta;

	NS_XMPCORE::spcIXMPSimpleNode xmpMMInstanceIDNode = dynamic_pointer_cast<const NS_XMPCORE::IXMPSimpleNode>(component->GetNode(kXMP_NS_XMP_MM, "InstanceID"));

	NS_XMPASSETMANAGEMENT::spICompositionRelationship relationshipNode = composedAsset->AddComponent(composedassetpart, component_ixmp_meta, componentassetpart);

	//verify toPart and ComponentPart form Ingredients entry
	NS_XMPASSETMANAGEMENT::spIXMPArrayNode spComposedIngredientsNode = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPArrayNode>(composed_ixmp_meta->GetNode(kXMP_NS_XMP_MM, "Ingredients"));
	NS_XMPCORE::spIXMPStructureNode IngredientEntrynode = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPStructureNode>(spComposedIngredientsNode->GetNode(1));
		
	NS_XMPCORE::spIXMPSimpleNode node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "toPart"));
	std::string toPartString = node->GetValue()->c_str();
	node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "fromPart"));
	std::string fromPartString = node->GetValue()->c_str();

	//A relationship is mapping to a single entry in Ingredients. A relationship of composed assest's
	//toPart, from ixmpmeta & fromPart has been created, add some more property using the relationship.
	relationshipNode->SetComponentFilePath("/basefolder/subfolder/composedpath");
	relationshipNode->SetMappingFunctionName("linear");
	relationshipNode->SetMaskMarkersFlag(NS_XMPASSETMANAGEMENT::ICompositionRelationship::kCompositionMaskMarkersAll);

	//Get the stRef:filePath
	node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "filePath"));
	std::string filePathString = node->GetValue()->c_str();
	cout << "stRef:filePath is " << filePathString.c_str() << endl;
		
	//get the stRef:partMapping
	node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "partMapping"));
	std::string partMappingString = node->GetValue()->c_str();
	cout << "stRef:partMapping is " << partMappingString.c_str() << endl;

	node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "maskMarkers"));
	std::string maskMarkersString = node->GetValue()->c_str();
	cout << "stRef:makMarkers is " << maskMarkersString.c_str() << endl;

	//verify the added node from pantry
	SerializeIXMPMetadata(composed_ixmp_meta, composed_out_file);

	NS_XMPASSETMANAGEMENT::XMPAM_ClientTerminate();

	// Terminate the toolkit
	SXMPMeta::Terminate();


	return 0;
}

