
#include <cstdlib>
#include <string>

// Must be defined to instantiate template classes
#define TXMP_STRING_TYPE std::string

// Must be defined to give access to XMPFiles
#define XMP_INCLUDE_XMPFILES 1

#define XMP_StaticBuild 1

// Ensure XMP templates are instantiated
#include <XMP.incl_cpp>

#include <iostream>
#include <fstream>

using namespace std;

static void write_RDF(string *rdf, const string &szInputFile)
{
  ofstream oOutFile;
  oOutFile.open(szInputFile.c_str(), ios::out);
  oOutFile << *rdf;
  oOutFile.close();
}

static int process_file(const string &szInputFile)
{
  int iRet = EXIT_SUCCESS;

  try {
    // Options to open the file with - read only and use a file handler
    XMP_OptionBits opts = kXMPFiles_OpenForRead | kXMPFiles_OpenUseSmartHandler;

    bool bOk;
    SXMPFiles oFile;
    string szStatus;

    // First we try and open the file
    bOk = oFile.OpenFile(szInputFile, kXMP_UnknownFile, opts);
    if (!bOk) {
      szStatus += "No smart handler available for " + szInputFile + "\nTrying packet scanning.\n";

      opts = kXMPFiles_OpenForUpdate | kXMPFiles_OpenUsePacketScanning;
      bOk = oFile.OpenFile(szInputFile, kXMP_UnknownFile, opts);
    }

    // If the file is open then read the metadata
    if (bOk) {
      // Form output filename
      string szBaseFilename;
      size_t unIndex = szInputFile.find_last_of(".");
      if (unIndex == string::npos) {
        szBaseFilename = szInputFile;
      } else {
        szBaseFilename = szInputFile.substr(0, unIndex);
      }
      string szOutputFile = szBaseFilename + ".xmp";

      cout << szStatus << endl;
      cout << szInputFile << " is opened successfully" << endl;
      // Create the xmp object and get the xmp data
      SXMPMeta oMeta;
      oFile.GetXMP(&oMeta);

      string sMetaBuffer;
      oMeta.SerializeToBuffer(&sMetaBuffer, kXMP_OmitPacketWrapper | kXMP_UseCompactFormat, 0, "\n");
      write_RDF(&sMetaBuffer, szOutputFile);

      cout << "XMP written to " << szOutputFile << endl;

      oFile.CloseFile();
    } else {
      cout << "Unable to open " << szInputFile << endl;
      iRet = EXIT_FAILURE;
    }
  } catch (XMP_Error &e) {
    cout << "ERROR: " << e.GetErrMsg() << endl;
    iRet = EXIT_FAILURE;
  }

  return iRet;
}

int main(int argc, const char *argv[])
{
  if (argc != 2) // 2 := command and 1 parameter
  {
    cout << "usage: extract_xmp <filename>" << endl;
    return EXIT_SUCCESS;
  }

  if (!SXMPMeta::Initialize()) {
    cout << "Could not initialize toolkit!";
    return EXIT_FAILURE;
  }
  XMP_OptionBits options = 0;
#if UNIX_ENV
  options |= kXMPFiles_ServerMode;
#endif
  // Must initialize SXMPFiles before we use it
  if (!SXMPFiles::Initialize(options)) {
    cout << "Could not initialize SXMPFiles.";
    return EXIT_FAILURE;
  }

  int ret = process_file(argv[1]);

  // Terminate the toolkit
  SXMPFiles::Terminate();
  SXMPMeta::Terminate();

  return ret;
}
