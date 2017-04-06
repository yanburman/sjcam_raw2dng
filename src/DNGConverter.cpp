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
#include <dng_preview.h>
#include <dng_xmp_sdk.h>
#include <dng_xmp.h>
#include <dng_globals.h>

#include <assert.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#include "DNGConverter.h"
#include "helpers.h"
#include "CFAReader.h"
#include "StopWatch.h"
#include "utils.h"

// Enable this for some profiling
// #define TIME_PROFILE

const dng_urational DNGConverter::m_oZeroURational(0, 100);
const dng_urational DNGConverter::m_oOneURational(1, 1);

const dng_matrix_3by3 DNGConverter::m_oIdentityMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1);

const static dng_matrix_3by3 g_olsD65Matrix(0.8391, 0.1327, 0.0705, 0.4219, 0.4622, -0.1067, 0.0684, -0.0648, 0.8437);

const static dng_matrix_3by3 g_olsAMatrix(0.8535, 0.2414, -0.0780, 0.4064, 0.6032, -0.2464, 0.0280, -0.0436, 0.8861);

static const dng_point_real64 g_oCenter(0.5, 0.5);

const dng_matrix_3by3 DNGConverter::m_olsD65Matrix(Invert(g_olsD65Matrix));
const dng_matrix_3by3 DNGConverter::m_olsAMatrix(Invert(g_olsAMatrix));

// SETTINGS: 12-Bit RGGB BAYER PATTERN
uint8 DNGConverter::m_unColorPlanes = 3;
uint16 DNGConverter::m_unBayerType = 1; // RGGB

// SETTINGS: Names
const std::string DNGConverter::m_szMake = "SJCAM";

// Calculate bit limit
const uint32_t DNGConverter::m_unBitLimit = 0x01 << 12;

DNGConverter::DNGConverter(Config &config) : m_oNeutralWB(3)
{
  m_oConfig = config;

  dng_xmp_sdk::InitializeSDK();

  // SETTINGS: Whitebalance D65, Orientation "normal"
  m_oOrientation = dng_orientation::Normal();

  if (m_oConfig.m_bNoCalibration)
    m_oNeutralWB.SetIdentity(3);
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

  int res = atoi(propValue.c_str());
  assert(res < 0x10000);

  oExif.m_unISO = (uint16_t)res;

  exists = meta.GetProperty(kXMP_NS_TIFF, "Model", &propValue, NULL);
  if (!exists)
    return -1;
  oExif.m_szCameraModel = propValue;

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

class LensCalibration
{
  protected:
  std::vector<real64> m_oVignetteGainParams;

  public:
  dng_vignette_radial_params m_oVignetteParams;

  LensCalibration(double val)
          : m_oVignetteGainParams(dng_vignette_radial_params::kNumTerms),
            m_oVignetteParams(m_oVignetteGainParams, g_oCenter)
  {
    m_oVignetteGainParams[0] = val;
    m_oVignetteGainParams[1] = val;
    m_oVignetteGainParams[2] = val;
    m_oVignetteGainParams[3] = val;
    m_oVignetteGainParams[4] = val;
  }
};

const static LensCalibration SJ5000xCalib(0.2);

struct CameraProfile {
  CameraProfile(uint32 w,
                uint32 h,
                uint32 black_level,
                double r,
                double g,
                double b,
                double base_noise,
                const char *name,
                const LensCalibration *calib = NULL)
          : m_ulWidth(w), m_ulHeight(h), m_szCameraModel(name), m_ulBlackLevel(black_level), m_oNeutralWB(3),
            m_oCalib(calib), m_fBaselineNoise(base_noise)
  {
    m_ulFileSize = (m_ulWidth * m_ulHeight * 12) / 8;
    m_oNeutralWB[0] = r;
    m_oNeutralWB[1] = g;
    m_oNeutralWB[2] = b;
  }

  uint32 m_ulWidth;
  uint32 m_ulHeight;
  std::string m_szCameraModel;
  uint32 m_ulBlackLevel;
  double m_fBaselineNoise;
  size_t m_ulFileSize;
  dng_vector m_oNeutralWB;
  const LensCalibration *m_oCalib;
};

struct SJ5000xProfile : public CameraProfile {
  SJ5000xProfile(uint32 w, uint32 h) : CameraProfile(w, h, 0, 1, 1.3, 1, 3, "SJ5000X", &SJ5000xCalib)
  {
  }
};

struct M20Profile : public CameraProfile {
  M20Profile(uint32 w, uint32 h) : CameraProfile(w, h, 200, 0.51, 1, 0.64, 4, "M20")
  {
  }
};

struct SJ6Profile : public CameraProfile {
  SJ6Profile(uint32 w, uint32 h) : CameraProfile(w, h, 0, 0.5, 1, 0.55, 4, "SJ6 LEGEND")
  {
  }
};

const static CameraProfile gRawSizes[] = {SJ5000xProfile(4000, 3000),
                                          SJ5000xProfile(3032, 2272),
                                          SJ5000xProfile(2640, 1980),
                                          M20Profile(4608, 3456),
                                          SJ6Profile(4624, 3488),
                                          SJ6Profile(4024, 3036),
                                          SJ6Profile(3760, 2832)};

static const CameraProfile *get_CameraProfile(size_t sz)
{
  const CameraProfile *oResult = NULL;

  for (unsigned int i = 0; i < sizeof(gRawSizes) / sizeof(gRawSizes[0]); ++i) {
    if (gRawSizes[i].m_ulFileSize == sz) {
      oResult = &gRawSizes[i];
      break;
    }
  }

  return oResult;
}

dng_error_code DNGConverter::ConvertToDNG(const std::string &m_szInputFile, const std::string &m_szMetadataFile)
{
#ifdef TIME_PROFILE
  StopWatch oProfilerTotal;
  oProfilerTotal.run();
#endif

  struct stat sb;
  int ret = stat(m_szInputFile.c_str(), &sb);
  if (ret) {
    perror("stat");
    return dng_error_unknown;
  }

  const CameraProfile *oCamProfile = get_CameraProfile(sb.st_size);
  if (NULL == oCamProfile) {
    fprintf(stderr, "%s: Unsupported format\n", m_szInputFile.c_str());
    return dng_error_bad_format;
  }

  Exif exif;

#ifdef TIME_PROFILE
  StopWatch oProfiler;
#endif

  if (!m_szMetadataFile.empty()) {
#ifdef TIME_PROFILE
    oProfiler.reset();
    oProfiler.run();
#endif
    ret = ParseMetadata(m_szMetadataFile, exif);
#ifdef TIME_PROFILE
    oProfiler.stop();
    printf("DNGConverter::ParseMetadata() time: %lu usec\n", oProfiler.elapsed_usec());
#endif
    if (ret)
      return dng_error_unknown;

    if (exif.m_szCameraModel != oCamProfile->m_szCameraModel) {
      fprintf(stderr, "%s: Unsupported camera\n", m_szInputFile.c_str());
      return dng_error_bad_format;
    }
  }

  // SETTINGS: Names
  const std::string &szProfileName = oCamProfile->m_szCameraModel;
  const std::string &szProfileCopyright = m_szMake;

  // Form output filenames
  std::string szBaseFilename;
  size_t unIndex = m_szInputFile.find_last_of(".");
  if (unIndex == std::string::npos) {
    szBaseFilename = m_szInputFile;
  } else {
    szBaseFilename = m_szInputFile.substr(0, unIndex);
  }

  if (!m_oConfig.m_szPathPrefixOutput.empty()) {
    unIndex = szBaseFilename.find_last_of(DIR_DELIM);
    if (unIndex != std::string::npos)
      szBaseFilename = szBaseFilename.substr(unIndex + 1, szBaseFilename.length());
  }

  std::string m_szOutputFile;
  m_szOutputFile.reserve(m_oConfig.m_szPathPrefixOutput.size() + szBaseFilename.size() + dng_suffix.size());
  m_szOutputFile = m_oConfig.m_szPathPrefixOutput;
  m_szOutputFile += szBaseFilename;
  m_szOutputFile += dng_suffix;

  // Create DNG
  try {
    dng_host oDNGHost;

    // -------------------------------------------------------------
    // Print settings
    // -------------------------------------------------------------

    printf("RAW: %s [%s] (%s)\n", m_szInputFile.c_str(), m_szMetadataFile.c_str(), szProfileName.c_str());

    // -------------------------------------------------------------
    // BAYER input file settings
    // -------------------------------------------------------------

    unsigned int ulNumPixels = oCamProfile->m_ulWidth * oCamProfile->m_ulHeight;
    AutoPtr<dng_memory_block> oBayerData(oDNGHost.Allocate(ulNumPixels * TagTypeSize(ttShort)));

    {
      CFAReader reader;
      ret = reader.open(m_szInputFile.c_str(), oCamProfile->m_ulFileSize);
      if (ret)
        return dng_error_unknown;

#ifdef TIME_PROFILE
      oProfiler.reset();
      oProfiler.run();
#endif
      reader.read((uint8_t *)oBayerData->Buffer(), ulNumPixels);

#ifdef TIME_PROFILE
      oProfiler.stop();
      printf("CFAReader::read() time: %lu usec\n", oProfiler.elapsed_usec());
#endif
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

    dng_rect vImageBounds(oCamProfile->m_ulHeight, oCamProfile->m_ulWidth);

    AutoPtr<dng_image> oImage(oDNGHost.Make_dng_image(vImageBounds, m_unColorPlanes, ttShort));

    dng_pixel_buffer oBuffer;

    oBuffer.fArea = vImageBounds;
    oBuffer.fPlane = 0;
    oBuffer.fPlanes = 1;
    oBuffer.fRowStep = oBuffer.fPlanes * oCamProfile->m_ulWidth;
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
    oNegative->SetModelName(oCamProfile->m_szCameraModel.c_str());

    // Set localized camera model
    // Remarks: Tag [UniqueCameraModel] / [50709]
    oNegative->SetLocalName(oCamProfile->m_szCameraModel.c_str());

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
    oNegative->SetBlackLevel(oCamProfile->m_ulBlackLevel);

    // Set white level
    // Remarks: Tag [WhiteLevel] / [50717]
    oNegative->SetWhiteLevel(m_unBitLimit - 1);

    // Set scale to square pixel
    // Remarks: Tag [DefaultScale] / [50718]
    oNegative->SetDefaultScale(m_oOneURational, m_oOneURational);

    // Set scale to square pixel
    // Remarks: Tag [BestQualityScale] / [50780]
    oNegative->SetBestQualityScale(m_oOneURational);

    // Set pixel area
    // Remarks: Tag [DefaultCropOrigin] / [50719]
    oNegative->SetDefaultCropOrigin(0, 0);

    // Set pixel area
    // Remarks: Tag [DefaultCropSize] / [50720]
    oNegative->SetDefaultCropSize(oCamProfile->m_ulWidth, oCamProfile->m_ulHeight);

    // Set base orientation
    // Remarks: See Restriction / Extension tags chapter
    oNegative->SetBaseOrientation(m_oOrientation);

    const dng_vector *pNeutral;
    if (m_oConfig.m_bNoCalibration) {
      pNeutral = &m_oNeutralWB;
    } else {
      pNeutral = &oCamProfile->m_oNeutralWB;
    }
    // Set camera neutral coordinates
    // Remarks: Tag [AsShotNeutral] / [50728]
    oNegative->SetCameraNeutral(*pNeutral);

    // Set baseline exposure
    // Remarks: Tag [BaselineExposure] / [50730]
    oNegative->SetBaselineExposure(0);

    // Set if noise reduction is already applied on RAW data
    // Remarks: Tag [NoiseReductionApplied] / [50935]
    oNegative->SetNoiseReductionApplied(m_oZeroURational);

    // Set baseline noise
    // Remarks: Tag [BaselineNoise] / [50731]
    oNegative->SetBaselineNoise(oCamProfile->m_fBaselineNoise);

    // Set baseline sharpness
    // Remarks: Tag [BaselineSharpness] / [50732]
    oNegative->SetBaselineSharpness(1);

    // Set anti-alias filter strength
    // Remarks: Tag [AntiAliasStrength] / [50738]
    oNegative->SetAntiAliasStrength(m_oZeroURational);

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
    poExif->fModel.Set_ASCII(oCamProfile->m_szCameraModel.c_str());

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
      oProfile->SetColorMatrix1(oCameraRGB_to_XYZ_D65);
    }

    // Set second illuminant color calibration if available
    if (ulCalibrationIlluminant2 != 0) {
      // Set calibration illuminant 2
      // Remarks: Tag [CalibrationIlluminant2] / [50779]
      oProfile->SetCalibrationIlluminant2(ulCalibrationIlluminant2);

      // Set color matrix 1
      // Remarks: Tag [ColorMatrix2] / [50722]
      oProfile->SetColorMatrix2(oCameraRGB_to_XYZ_A);
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

#if 0
    AutoPtr<dng_opcode> oFixVignetteOpcode;
    if (m_oConfig.m_bLensCorrections) {
      dng_xmp *oXMP = oNegative->Metadata().GetXMP();
      oXMP->SetBoolean(kXMP_NS_CameraRaw, "AutoLateralCA", true);

#if 0
      if (oCamProfile->m_oCalib) {
        oFixVignetteOpcode.Reset(
          new dng_opcode_FixVignetteRadial(oCamProfile->m_oCalib->m_oVignetteParams, dng_opcode::kFlag_None));
        oNegative->OpcodeList3().Append(oFixVignetteOpcode);
      }
#endif
    }
#endif

    // -------------------------------------------------------------
    // Write DNG file
    // -------------------------------------------------------------

    // Assign Raw image data.
    oNegative->SetStage1Image(oImage);

#ifdef TIME_PROFILE
    oProfiler.reset();
    oProfiler.run();
#endif
    // Compute linearized and range mapped image
    oNegative->BuildStage2Image(oDNGHost);
#ifdef TIME_PROFILE
    oProfiler.stop();
    printf("dng_negative::BuildStage2Image() time: %lu usec\n", oProfiler.elapsed_usec());
#endif

#ifdef TIME_PROFILE
    oProfiler.reset();
    oProfiler.run();
#endif
    // Compute demosaiced image (used by preview and thumbnail)
    oNegative->BuildStage3Image(oDNGHost);
#ifdef TIME_PROFILE
    oProfiler.stop();
    printf("dng_negative::BuildStage3Image() time: %lu usec\n", oProfiler.elapsed_usec());
#endif

    // Update XMP / EXIF
    oNegative->SynchronizeMetadata();

    // Update IPTC
    oNegative->RebuildIPTC(true);

    dng_preview_list *oPreviewList = NULL;
    dng_jpeg_preview *oJpegPreview = NULL;

    if (m_oConfig.m_bGenPreview) {
#ifdef TIME_PROFILE
      oProfiler.reset();
      oProfiler.run();
#endif

      dng_render oNegRender(oDNGHost, *oNegative.Get());

      oJpegPreview = new dng_jpeg_preview();
      oJpegPreview->fInfo.fColorSpace = previewColorSpace_sRGB;

      oNegRender.SetMaximumSize(1024);
      AutoPtr<dng_image> negImage(oNegRender.Render());
      dng_image_writer oJpegWriter;
      oJpegWriter.EncodeJPEGPreview(oDNGHost, *negImage.Get(), *oJpegPreview, 4);
      AutoPtr<dng_preview> oPreview(dynamic_cast<dng_preview *>(oJpegPreview));

      dng_image_preview *oThumbnailPreview = new dng_image_preview();
      oThumbnailPreview->fInfo.fColorSpace = previewColorSpace_sRGB;

      oNegRender.SetMaximumSize(256);
      oThumbnailPreview->fImage.Reset(oNegRender.Render());
      AutoPtr<dng_preview> oThumbnail(dynamic_cast<dng_preview *>(oThumbnailPreview));

      oPreviewList = new dng_preview_list();
      oPreviewList->Append(oPreview);
      oPreviewList->Append(oThumbnail);

#ifdef TIME_PROFILE
      oProfiler.stop();
      printf("Generate thumbnails and preview time: %lu usec\n", oProfiler.elapsed_usec());
#endif
    }

    AutoPtr<dng_preview_list> oPreviews(oPreviewList);

    AutoPtr<dng_image_writer> oWriter(new dng_image_writer());

    if (m_oConfig.m_bDng) {
      // Create stream writer for output file
      dng_file_stream oDNGStream(m_szOutputFile.c_str(), true);

#ifdef TIME_PROFILE
      oProfiler.reset();
      oProfiler.run();
#endif
      // Write DNG file to disk
      oWriter->WriteDNG(oDNGHost, oDNGStream, *oNegative.Get(), oPreviews.Get());
#ifdef TIME_PROFILE
      oProfiler.stop();
      printf("dng_image_writer::WriteDNG() time: %lu usec\n", oProfiler.elapsed_usec());
#endif
    }

    if (m_oConfig.m_bTiff) {
      // -------------------------------------------------------------
      // Write TIFF file
      // -------------------------------------------------------------

      std::string m_szRenderFile;
      m_szRenderFile.reserve(m_oConfig.m_szPathPrefixOutput.size() + szBaseFilename.size() + tiff_suffix.size());
      m_szRenderFile = m_oConfig.m_szPathPrefixOutput;
      m_szRenderFile += szBaseFilename;
      m_szRenderFile += tiff_suffix;

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

#ifdef TIME_PROFILE
      oProfiler.reset();
      oProfiler.run();
#endif
      // Write TIFF file to disk
      oWriter->WriteTIFF(oDNGHost,
                         oTIFFStream,
                         *oFinalImage.Get(),
                         piRGB,
                         ccUncompressed,
                         oNegative.Get(),
                         &oRender.FinalSpace(),
                         NULL,
                         oJpegPreview);
#ifdef TIME_PROFILE
      oProfiler.stop();
      printf("dng_image_writer::WriteTIFF() time: %lu usec\n", oProfiler.elapsed_usec());
#endif
    }
  } catch (const dng_exception &except) {
    return except.ErrorCode();
  } catch (...) {
    return dng_error_unknown;
  }

#ifdef TIME_PROFILE
  oProfilerTotal.stop();
  printf("DNGConverter::ConvertToDNG() time: %lu usec\n", oProfilerTotal.elapsed_usec());
#endif

  return dng_error_none;
}
