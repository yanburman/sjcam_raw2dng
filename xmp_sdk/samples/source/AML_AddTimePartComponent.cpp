// =================================================================================================
// Copyright 2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

/**
* Tutorial solution for the AssetRelationships.pdf. Demonstration of time part, how it's created,
* it's values are set and how it is added into composed document using AddComponent.
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
//time part
#include "XMPAssetManagement/Interfaces/IAssetTimePart.h"

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
* ixmpmeta (DOM based Metadata object) is get. A time part is created, it's values are set,
* and later used in AddComponent for composition.
*/

int main(int argc, const char * argv[])
{
	string composed_file = "AddBasicPart_InSimpleDocument_input_composite_doc.xmp";
	string component_file = "AddBasicPart_InSimpleDocument_input_component_doc.xmp";
	string outfilename = "AML_TimePartAddition_Sample_OutPut.xmp";

	if (!SXMPMeta::Initialize())
	{
		cout << "Could not initialize toolkit!";
		return -1;
	}

	NS_XMPCORE::spIXMPMetadata& composed_ixmp_meta = GetIXMPMetadataFromRDF(composed_file);
	NS_XMPCORE::spIXMPMetadata& component_ixmp_meta = GetIXMPMetadataFromRDF(component_file);


	//for both the files, metadata has been obtained, get IXMPMeta from SXMPMeta
	NS_XMPCORE::pIXMPCoreObjectFactory factoryObj = NS_XMPCORE::IXMPCoreObjectFactory::GetInstance();
	NS_XMPASSETMANAGEMENT::XMPAM_ClientInitialize();

	//Check if composed / component is not trackable, make it
	NS_XMPASSETMANAGEMENT::pIAssetUtilities utilityObj = NS_XMPASSETMANAGEMENT::IAssetUtilities::GetAssetUtilities();
	if (utilityObj->IsTrackable(composed_ixmp_meta) == false)
		utilityObj->MakeTrackable(composed_ixmp_meta);
	if (utilityObj->IsTrackable(component_ixmp_meta) == false)
		utilityObj->MakeTrackable(component_ixmp_meta);

	//create a time toPart
	NS_XMPCOMMON::UInt64  toPartStartFrameCount = 0, toPartStartNumerator = 80, toPartStartBaseRate = 20;
	NS_XMPCOMMON::UInt64  toPartDurationFrameCount = 2000, toPartDurationNumerator = 80, toPartDurationBaseRate = 20;
	NS_XMPASSETMANAGEMENT::spIAssetTimePart composedassettimepart = NS_XMPASSETMANAGEMENT::IAssetTimePart::CreateStandardTimePart(NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartComponentMetadata);
	composedassettimepart->SetFrameCountDuration(toPartStartFrameCount, toPartDurationFrameCount);
	composedassettimepart->SetFrameRateForCountOfFrames(toPartStartNumerator, toPartStartBaseRate);
	composedassettimepart->SetFrameRateForStartFrameCount(toPartDurationNumerator, toPartDurationBaseRate);

	//component/fromPart
	NS_XMPCOMMON::UInt64  fromPartStartFrameCount = 5000, fromPartStartNumerator = 80, fromPartStartBaseRate = 20;
	NS_XMPCOMMON::UInt64  fromPartDurationFrameCount = 2000, fromPartDurationNumerator = 80, fromPartDurationBaseRate = 20;
	NS_XMPASSETMANAGEMENT::spIAssetTimePart componentassettimepart = NS_XMPASSETMANAGEMENT::IAssetTimePart::CreateStandardTimePart(NS_XMPASSETMANAGEMENT::IAssetPart_v1::kAssetPartComponentAudio);
	componentassettimepart->SetFrameCountDuration(fromPartStartFrameCount, fromPartDurationFrameCount);
	componentassettimepart->SetFrameRateForCountOfFrames(fromPartStartNumerator, fromPartStartBaseRate);
	componentassettimepart->SetFrameRateForStartFrameCount(fromPartDurationNumerator, fromPartDurationBaseRate);

	//creating ComposedAssetManager
	NS_XMPASSETMANAGEMENT::spIComposedAssetManager composedAsset = NS_XMPASSETMANAGEMENT::IComposedAssetManager::CreateComposedAssetManager(composed_ixmp_meta);

	//call AddComponent API (toPart, from ixmpmeta, fromPart) to compose fromPart into composed doc at toPart
	//on calling the AddComponent, following will be entry in ingredient
	//<stRef:toPart>/metadata/time:0f80s20d2000f80s80
	//<stRef:fromPart>/content/audio/time:5000f80s20d2000f80s80
	//for detail time format refer to  XMP Specification Part 2 : Additional Properties sector 1.2.6.4 FrameRate
	NS_XMPASSETMANAGEMENT::spICompositionRelationship relationshipNode = composedAsset->AddComponent(composedassettimepart, component_ixmp_meta, componentassettimepart);
	//a mapping function
	relationshipNode->SetMappingFunctionName("linear"); 

	//assuming there are not ingredient entry, if there is fix the index, or iterate using iterator
	//verify toPart and ComponentPart form Ingredients entry
	NS_XMPASSETMANAGEMENT::spIXMPArrayNode spComposedIngredientsNode = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPArrayNode>(composed_ixmp_meta->GetNode(kXMP_NS_XMP_MM, "Ingredients"));
	NS_XMPCORE::spIXMPStructureNode IngredientEntrynode = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPStructureNode>(spComposedIngredientsNode->GetNode(1));

	NS_XMPCORE::spIXMPSimpleNode node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "toPart"));
	std::string toPartString = node->GetValue()->c_str();
	node = NS_XMPCORE::dynamic_pointer_cast<NS_XMPCORE::IXMPSimpleNode>(IngredientEntrynode->GetNode(kXMP_NS_XMP_ResourceRef, "fromPart"));
	std::string fromPartString = node->GetValue()->c_str();

	//verify the added node from pantry

	SerializeIXMPMetadata(composed_ixmp_meta, outfilename);

	NS_XMPASSETMANAGEMENT::XMPAM_ClientTerminate();

	SXMPMeta::Terminate();


	return 0;
}

