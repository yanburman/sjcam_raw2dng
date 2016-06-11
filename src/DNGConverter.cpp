/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <stdio.h>
#include <string>
#include <dng_flags.h>

#if qMacOS
#ifndef MAC_ENV
#define MAC_ENV 1
#endif
#endif

#if qWinOS
#ifndef WIN_ENV
#define WIN_ENV 1
#endif
#endif

#if qLinux
#ifndef UNIX_ENV
#define UNIX_ENV 1
#endif
#endif

// Must be defined to instantiate template classes
#define TXMP_STRING_TYPE std::string

// Must be defined to give access to XMPFiles
#define XMP_INCLUDE_XMPFILES 1

#include <XMP.incl_cpp>

#include <dng_camera_profile.h>
#include <dng_image.h>
#include <dng_negative.h>
#include <dng_rational.h>
#include <dng_lens_correction.h>
#include <dng_opcodes.h>
#include <dng_exif.h>
#include <dng_image_writer.h>
#include <dng_host.h>
#include <dng_file_stream.h>
#include <dng_render.h>
#include <dng_xmp_sdk.h>
#include <dng_globals.h>

#include <assert.h>

#include "DNGConverter.h"

#include "CFAReader.h"

const dng_urational DNGConverter::m_oZeroURational(0, 100);
const Exif DNGConverter::m_oDefaultExif;
const dng_matrix_3by3 DNGConverter::m_oIdentityMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1);

// Derived from MATLAB using least square linear regression with
//          MacBeth ColorChecker classic
const dng_matrix_3by3 DNGConverter::m_olsD65Matrix(2.3150,
                                                   0.0711,
                                                   0.1455,
                                                   0.9861,
                                                   0.7815,
                                                   -0.2192,
                                                   0.2082,
                                                   -0.3349,
                                                   1.6432);
// Derived from MATLAB using least square linear regression with
//          MacBeth ColorChecker classic
const dng_matrix_3by3 DNGConverter::m_olsAMatrix(1.6335,
                                                 0.4718,
                                                 -0.0656,
                                                 0.5227,
                                                 1.0298,
                                                 -0.3416,
                                                 0.0475,
                                                 -0.2020,
                                                 1.2522);

// SETTINGS: 12-Bit RGGB BAYER PATTERN
uint8 DNGConverter::m_unColorPlanes = 3;
uint16 DNGConverter::m_unBayerType = 1; // RGGB
uint32 DNGConverter::m_ulBlackLevel = 0;

// SETTINGS: Names
const std::string DNGConverter::m_szMake = "SJCAM";

// Calculate bit limit
uint32_t DNGConverter::m_unBitLimit = 0x01 << 12;

DNGConverter::DNGConverter(Config &config)
{
  m_oConfig = config;

  dng_xmp_sdk::InitializeSDK();

  // SETTINGS: Whitebalance D65, Orientation "normal"
  m_oOrientation = dng_orientation::Normal();
  m_oWhitebalanceDetectedXY = D65_xy_coord();

  m_szPathPrefixOutput = "";
}

DNGConverter::~DNGConverter()
{
  dng_xmp_sdk::TerminateSDK();
}

template <typename T> void str2rational(const std::string &val, T &result)
{
  result.n = atoi(val.c_str());

  std::size_t idx = val.find_first_of('/');
  assert(idx != std::string::npos);

  result.d = atoi(&val.c_str()[idx + 1]);
}

static void XMPdate2DNGdate(const XMP_DateTime &xmp_date, dng_date_time_info &oDNGDate)
{
  dng_date_time dt(xmp_date.year, xmp_date.month, xmp_date.day, xmp_date.hour, xmp_date.minute, xmp_date.second);

  oDNGDate.SetDateTime(dt);

  char buff[32];

  snprintf(buff, sizeof(buff), "%d", xmp_date.nanoSecond);

  dng_string sub_seconds;
  sub_seconds.Set(buff);
  oDNGDate.SetSubseconds(sub_seconds);
}

int DNGConverter::ParseMetadata(const std::string &metadata, Exif &oExif)
{
  bool ok, exists;
  SXMPFiles myFile;
  ok = myFile.OpenFile(metadata, kXMP_JPEGFile, kXMPFiles_OpenForRead);
  if (!ok) {
    return -1;
  }

  // Create the xmp object and get the xmp data
  SXMPMeta meta;
  myFile.GetXMP(&meta);

  exists = meta.GetProperty(kXMP_NS_XMP, "CreatorTool", &oExif.m_szCreatorTool, NULL);
  if (!exists)
    return -1;

  std::string propValue;
  exists = meta.GetArrayItem(kXMP_NS_EXIF, "ISOSpeedRatings", 1, &propValue, 0);
  if (!exists)
    return -1;

  oExif.m_unISO = atoi(propValue.c_str());

  XMP_DateTime myDate;
  exists = meta.GetProperty_Date(kXMP_NS_EXIF, "DateTimeOriginal", &myDate, NULL);
  if (!exists)
    return -1;

  XMPdate2DNGdate(myDate, oExif.m_oOrigDate);

  exists = meta.GetProperty(kXMP_NS_EXIF, "ExposureTime", &propValue, NULL);
  if (!exists)
    return -1;

  str2rational(propValue, oExif.m_oExposureTime);

  exists = meta.GetProperty(kXMP_NS_EXIF, "ExposureBiasValue", &propValue, NULL);
  if (!exists)
    return -1;

  str2rational(propValue, oExif.m_oExposureBias);

  XMP_Int32 ls;
  exists = meta.GetProperty_Int(kXMP_NS_EXIF, "LightSource", &ls, NULL);
  if (!exists)
    return -1;

  oExif.m_uLightSource = ls;

  // Close the SXMPFile.  The resource file is already closed if it was
  // opened as read only but this call must still be made.
  myFile.CloseFile();

  return 0;
}

dng_error_code DNGConverter::ConvertToDNG(const std::string &m_szInputFile, const Exif &exif)
{
  // SETTINGS: Names
  const std::string &szProfileName = exif.m_szCameraModel;
  const std::string &szProfileCopyright = m_szMake;

  // Form output filenames
  std::string szBaseFilename = "";
  std::string m_szOutputFile = "";
  std::string m_szRenderFile = "";
  size_t unIndex = m_szInputFile.find_last_of(".");
  if (unIndex == std::string::npos) {
    szBaseFilename = m_szInputFile;
  } else {
    szBaseFilename = m_szInputFile.substr(0, unIndex);
  }
  m_szOutputFile = m_szPathPrefixOutput + szBaseFilename + ".dng";
  m_szRenderFile = m_szPathPrefixOutput + szBaseFilename + ".tiff";

  // Create DNG
  try {
    dng_host oDNGHost;

    // -------------------------------------------------------------
    // Print settings
    // -------------------------------------------------------------

    printf("RAW: %s\n", m_szInputFile.c_str());

    printf("\nConverting...\n");

    // -------------------------------------------------------------
    // BAYER input file settings
    // -------------------------------------------------------------

    CFAReader reader;
    int ret = reader.open(m_szInputFile.c_str(), (exif.m_ulWidth * exif.m_ulHeight * 12) / 8);
    if (ret)
      return dng_error_unknown;

    AutoPtr<dng_memory_block> oBayerData(oDNGHost.Allocate(exif.m_ulWidth * exif.m_ulHeight * TagTypeSize(ttShort)));

    uint8_t *buf = (uint8_t *)oBayerData->Buffer();
    for (unsigned int i = 0; i < exif.m_ulWidth * exif.m_ulHeight; ++i, buf += 2) {
      reader.read(buf);
    }

    // -------------------------------------------------------------
    // DNG Host Settings
    // -------------------------------------------------------------

    // Set DNG version
    // Remarks: Tag [DNGVersion] / [50706]
    oDNGHost.SetSaveDNGVersion(dngVersion_SaveDefault);

    // Set DNG type to RAW DNG
    // Remarks: Store Bayer CFA data and not already processed data
    oDNGHost.SetSaveLinearDNG(false);

    // -------------------------------------------------------------
    // DNG Image Settings
    // -------------------------------------------------------------

    dng_rect vImageBounds(exif.m_ulHeight, exif.m_ulWidth);

    AutoPtr<dng_image> oImage(oDNGHost.Make_dng_image(vImageBounds, m_unColorPlanes, ttShort));

    dng_pixel_buffer oBuffer;

    oBuffer.fArea = vImageBounds;
    oBuffer.fPlane = 0;
    oBuffer.fPlanes = 1;
    oBuffer.fRowStep = oBuffer.fPlanes * exif.m_ulWidth;
    oBuffer.fColStep = oBuffer.fPlanes;
    oBuffer.fPlaneStep = 1;
    oBuffer.fPixelType = ttShort;
    oBuffer.fPixelSize = TagTypeSize(ttShort);
    oBuffer.fData = oBayerData->Buffer();

    oImage->Put(oBuffer);

    // -------------------------------------------------------------
    // DNG Negative Settings
    // -------------------------------------------------------------

    AutoPtr<dng_negative> oNegative(oDNGHost.Make_dng_negative());

    // Set camera model
    // Remarks: Tag [UniqueCameraModel] / [50708]
    oNegative->SetModelName(exif.m_szCameraModel.c_str());

    // Set localized camera model
    // Remarks: Tag [UniqueCameraModel] / [50709]
    oNegative->SetLocalName(exif.m_szCameraModel.c_str());

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetColorKeys(colorKeyRed, colorKeyGreen, colorKeyBlue);

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetBayerMosaic(m_unBayerType);

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetColorChannels(m_unColorPlanes);

#if 0
    // Set linearization table
    // Remarks: Tag [LinearizationTable] / [50712]
    AutoPtr<dng_memory_block> oCurve(oDNGHost.Allocate(sizeof(uint16) * m_unBitLimit));
    for (int64 i = 0; i < m_unBitLimit; i++) {
      uint16 *pulItem = oCurve->Buffer_uint16() + i;
      *pulItem = (uint16)(i);
    }
    oNegative->SetLinearization(oCurve);
#endif

    // Set black level to auto black level of sensor
    // Remarks: Tag [BlackLevel] / [50714]
    oNegative->SetBlackLevel(m_ulBlackLevel);

    // Set white level
    // Remarks: Tag [WhiteLevel] / [50717]
    oNegative->SetWhiteLevel(m_unBitLimit - 1);

    // Set scale to square pixel
    // Remarks: Tag [DefaultScale] / [50718]
    oNegative->SetDefaultScale(dng_urational(1, 1), dng_urational(1, 1));

    // Set scale to square pixel
    // Remarks: Tag [BestQualityScale] / [50780]
    oNegative->SetBestQualityScale(dng_urational(1, 1));

    // Set pixel area
    // Remarks: Tag [DefaultCropOrigin] / [50719]
    oNegative->SetDefaultCropOrigin(2, 2);

    // Set pixel area
    // Remarks: Tag [DefaultCropSize] / [50720]
    oNegative->SetDefaultCropSize(exif.m_ulWidth - 4, exif.m_ulHeight - 4);

    // Set base orientation
    // Remarks: See Restriction / Extension tags chapter
    oNegative->SetBaseOrientation(m_oOrientation);

    // Set camera white XY coordinates
    // Remarks: Tag [AsShotWhiteXY] / [50729]
    oNegative->SetCameraWhiteXY(m_oWhitebalanceDetectedXY);

    // Set baseline exposure
    // Remarks: Tag [BaselineExposure] / [50730]
    oNegative->SetBaselineExposure(0);

    // Set if noise reduction is already applied on RAW data
    // Remarks: Tag [NoiseReductionApplied] / [50935]
    oNegative->SetNoiseReductionApplied(m_oZeroURational);

    // Set baseline sharpness
    // Remarks: Tag [BaselineSharpness] / [50732]
    oNegative->SetBaselineSharpness(0.1); // we don't have AA filter

    // Set anti-alias filter strength
    // Remarks: Tag [AntiAliasStrength] / [50738]
    oNegative->SetAntiAliasStrength(m_oZeroURational); // we don't have AA filter

    // -------------------------------------------------------------
    // DNG EXIF Settings
    // -------------------------------------------------------------

    dng_exif *poExif = oNegative->GetExif();

    // Set Camera Make
    // Remarks: Tag [Make] / [EXIF]
    poExif->fMake.Set_ASCII(m_szMake.c_str());

    // Set creator tool
    // Remarks: Tag [CreatorTool]
    if (!exif.m_szCreatorTool.empty())
      poExif->fSoftware.Set_ASCII(exif.m_szCreatorTool.c_str());

    // Set Camera Model
    // Remarks: Tag [Model] / [EXIF]
    poExif->fModel.Set_ASCII(exif.m_szCameraModel.c_str());

    // Set Lens Model
    // Remarks: Tag [LensName] / [EXIF]
    poExif->fLensName.Set_ASCII("GP43520");

    // Set Lens Make
    // Remarks: Tag [LensMake] / [EXIF]
    poExif->fLensMake.Set_ASCII(m_szMake.c_str());

    // Set ISO speed
    // Remarks: Tag [ISOSpeed] / [EXIF]
    poExif->fISOSpeedRatings[0] = exif.m_unISO;
    poExif->fISOSpeedRatings[1] = 0;
    poExif->fISOSpeedRatings[2] = 0;

    // Set WB mode
    // Remarks: Tag [WhiteBalance] / [EXIF]
    poExif->fWhiteBalance = 0;

    // Set light source
    // Remarks: Tag [LightSource] / [EXIF]
    poExif->fLightSource = exif.m_uLightSource;

    // Set exposure program
    // Remarks: Tag [ExposureProgram] / [EXIF]
    poExif->fExposureProgram = epProgramNormal;

    // Set sensor type
    // Remarks: Tag [SensingMethod] / [EXIF]
    poExif->fSensingMethod = 2;

    // Set digital zoom
    // Remarks: Tag [DigitalZoomRatio] / [EXIF]
    poExif->fDigitalZoomRatio = m_oZeroURational;

    // Set flash modes
    // Remarks: Tag [Flash] / [EXIF]
    poExif->fFlash = 0;
    poExif->fFlashMask = 0x1;

    // Set metering mode
    // Remarks: Tag [MeteringMode] / [EXIF]
    poExif->fMeteringMode = mmCenterWeightedAverage;

    // Set metering mode
    // Remarks: Tag [ExposureBiasValue] / [EXIF]
    poExif->fExposureBiasValue = exif.m_oExposureBias;

    // Set aperture value
    // Remarks: Tag [ApertureValue] / [EXIF]
    poExif->SetFNumber(exif.m_dLensAperture);

    // Set exposure time
    // Remarks: Tag [ExposureTime] / [EXIF]
    poExif->fExposureTime = exif.m_oExposureTime;

    // Set focal length
    // Remarks: Tag [FocalLength] / [EXIF]
    poExif->fFocalLength.Set_real64(exif.m_dFocalLength, 1000);

    // Set 35mm equivalent focal length
    // Remarks: Tag [FocalLengthIn35mmFilm] / [EXIF]
    poExif->fFocalLengthIn35mmFilm = (uint32)round(exif.m_dFocalLength * 5.64);

    // Set lens info
    // Remarks: Tag [LensInfo] / [EXIF]
    poExif->fLensInfo[0].Set_real64(exif.m_dFocalLength, 10);
    poExif->fLensInfo[1].Set_real64(exif.m_dFocalLength, 10);
    poExif->fLensInfo[2].Set_real64(exif.m_dLensAperture, 10);
    poExif->fLensInfo[3].Set_real64(exif.m_dLensAperture, 10);

    // Set file source
    // Remarks: Tag [FileSource] / [EXIF]
    poExif->fFileSource = 3;

    // Set scene type
    // Remarks: Tag [SceneType] / [EXIF]
    poExif->fSceneType = 1;

    // Set custom rendered
    // Remarks: Tag [CustomRendered] / [EXIF]
    poExif->fCustomRendered = 0;

    // Update date from original file
    if (exif.m_oOrigDate.IsValid()) {
      poExif->fDateTimeOriginal = exif.m_oOrigDate;
      poExif->fDateTimeDigitized = exif.m_oOrigDate;
    }

    // -------------------------------------------------------------
    // DNG Profile Settings: Simple color calibration
    // -------------------------------------------------------------

    // Camera space RGB to XYZ matrix with D65 illumination
    dng_matrix_3by3 oCameraRGB_to_XYZ_D65;
    if (m_oConfig.m_bNoCalibration)
      oCameraRGB_to_XYZ_D65 = m_oIdentityMatrix;
    else
      oCameraRGB_to_XYZ_D65 = m_olsD65Matrix;
    uint32 ulCalibrationIlluminant1 = lsD65;

    // Camera space RGB to XYZ matrix with StdA illumination
    dng_matrix_3by3 oCameraRGB_to_XYZ_A;
    if (m_oConfig.m_bNoCalibration)
      oCameraRGB_to_XYZ_A = m_oIdentityMatrix;
    else
      oCameraRGB_to_XYZ_A = m_olsAMatrix;

    uint32 ulCalibrationIlluminant2 = lsStandardLightA;

    AutoPtr<dng_camera_profile> oProfile(new dng_camera_profile);

    // Set first illuminant color calibration if available
    if (ulCalibrationIlluminant1 != 0) {
      // Set calibration illuminant 1
      // Remarks: Tag [CalibrationIlluminant1] / [50778]
      oProfile->SetCalibrationIlluminant1(ulCalibrationIlluminant1);

      // Set color matrix 1
      // Remarks: Tag [ColorMatrix1] / [50721]
      oProfile->SetColorMatrix1(Invert(oCameraRGB_to_XYZ_D65));
    }

    // Set second illuminant color calibration if available
    if (ulCalibrationIlluminant2 != 0) {
      // Set calibration illuminant 2
      // Remarks: Tag [CalibrationIlluminant2] / [50779]
      oProfile->SetCalibrationIlluminant2(ulCalibrationIlluminant2);

      // Set color matrix 1
      // Remarks: Tag [ColorMatrix2] / [50722]
      oProfile->SetColorMatrix2(Invert(oCameraRGB_to_XYZ_A));
    }

    // Set name of profile
    // Remarks: Tag [ProfileName] / [50936]
    oProfile->SetName(szProfileName.c_str());

    // Set copyright of profile
    // Remarks: Tag [ProfileCopyright] / [50942]
    oProfile->SetCopyright(szProfileCopyright.c_str());

    // Force flag read from DNG to make sure this profile will be embedded
    oProfile->SetWasReadFromDNG(true);

    // Set policy for profile
    // Remarks: Tag [ProfileEmbedPolicy] / [50941]
    oProfile->SetEmbedPolicy(pepAllowCopying);

    // Add camera profile to negative
    oNegative->AddProfile(oProfile);

    // -------------------------------------------------------------
    // Lens corrections
    // -------------------------------------------------------------
    if (m_oConfig.m_bLensCorrections) {
      const dng_point_real64 oCenter(0.5, 0.5);
      std::vector<real64> oVignetteGainParams(dng_vignette_radial_params::kNumTerms);
      oVignetteGainParams[0] = 0.2;
      oVignetteGainParams[1] = 0.2;
      oVignetteGainParams[2] = 0.2;
      oVignetteGainParams[3] = 0.2;
      oVignetteGainParams[4] = 0.2;

      dng_vignette_radial_params oVignetteParams(oVignetteGainParams, oCenter);
      AutoPtr<dng_opcode> oFixVignetteOpcode(new dng_opcode_FixVignetteRadial(oVignetteParams, dng_opcode::kFlag_None));
      oNegative->OpcodeList3().Append(oFixVignetteOpcode);
    }

    // -------------------------------------------------------------
    // Write DNG file
    // -------------------------------------------------------------

    // Assign Raw image data.
    oNegative->SetStage1Image(oImage);

    // Compute linearized and range mapped image
    oNegative->BuildStage2Image(oDNGHost);

    // Compute demosaiced image (used by preview and thumbnail)
    oNegative->BuildStage3Image(oDNGHost);

    // Update XMP / EXIF
    oNegative->SynchronizeMetadata();

    // Update IPTC
    oNegative->RebuildIPTC(true);

    // Create stream writer for output file
    dng_file_stream oDNGStream(m_szOutputFile.c_str(), true);

    // Write DNG file to disk
    AutoPtr<dng_image_writer> oWriter(new dng_image_writer());
    oWriter->WriteDNG(oDNGHost, oDNGStream, *oNegative.Get());

    if (m_oConfig.m_bTiff) {
      // -------------------------------------------------------------
      // Write TIFF file
      // -------------------------------------------------------------

      // Create stream writer for output file
      dng_file_stream oTIFFStream(m_szRenderFile.c_str(), true);

      // Create render object
      dng_render oRender(oDNGHost, *oNegative);

      // Set exposure compensation
      oRender.SetExposure(0.0);

      // Create final image
      AutoPtr<dng_image> oFinalImage;

      // Render image
      oFinalImage.Reset(oRender.Render());
      oFinalImage->Rotate(oNegative->Orientation());

      // Write TIFF file to disk
      oWriter->WriteTIFF(oDNGHost,
                         oTIFFStream,
                         *oFinalImage.Get(),
                         piRGB,
                         ccUncompressed,
                         oNegative.Get(),
                         &oRender.FinalSpace());
    }
  } catch (const dng_exception &except) {
    return except.ErrorCode();
  } catch (...) {
    return dng_error_unknown;
  }

  printf("Conversion complete\n");

  return dng_error_none;
}
