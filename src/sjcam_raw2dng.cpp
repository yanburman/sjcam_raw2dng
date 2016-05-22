#include <stdio.h>
#include <string>

#include <dng_exceptions.h>
#include <dng_orientation.h>
#include <dng_xy_coord.h>
#include <dng_camera_profile.h>
#include <dng_image.h>
#include <dng_negative.h>
#include <dng_rational.h>
#include <dng_exif.h>
#include <dng_image_writer.h>
#include <dng_host.h>
#include <dng_file_stream.h>
#include <dng_render.h>
#include <dng_xmp_sdk.h>
#include <dng_globals.h>

#include "CFAReader.h"

static dng_error_code ConvertToDNG(std::string m_szInputFile)
{
  // Sample BAYER image at ISO100 and tEXP 1/10 on f/1.7 and focal length 2.99mm
  uint16 m_unISO = 100;
  double m_dExposureTime = 0.1;
  double m_dLensAperture = 1.7;
  double m_dFocalLength = 2.99;

  // SETTINGS: 12-Bit RGGB BAYER PATTERN
  uint8 m_unColorPlanes = 3;
  uint16 m_unBayerType = 1; // RGGB
  uint32 m_ulWidth = 4000;
  uint32 m_ulHeight = 3000;
  uint32 m_ulBlackLevel = 0;

  // SETTINGS: Whitebalance D65, Orientation "normal"
  dng_orientation m_oOrientation = dng_orientation::Normal();
  dng_xy_coord m_oWhitebalanceDetectedXY = D65_xy_coord();

  // SETTINGS: Names
  std::string m_szMake = "SJCAM";
  std::string m_szCameraModel = "SJ5000X";
  std::string szProfileName = m_szCameraModel;
  std::string szProfileCopyright = m_szMake;

  // Calculate bit limit
  uint32 m_unBitLimit = 0x01 << 12;

  // Form output filenames
  std::string szBaseFilename = "";
  std::string m_szOutputFile = "";
  std::string m_szRenderFile = "";
  std::string m_szPathPrefixInput = "";
  std::string m_szPathPrefixOutput = "";
  std::string m_szPathPrefixProfiles = "";
  size_t unIndex = m_szInputFile.find_last_of(".");
  if (unIndex == std::string::npos) {
    szBaseFilename = m_szInputFile;
  } else {
    szBaseFilename = m_szInputFile.substr(0, unIndex);
  }
  m_szInputFile = m_szPathPrefixInput + m_szInputFile;
  m_szOutputFile = m_szPathPrefixOutput + szBaseFilename + ".dng";
  m_szRenderFile = m_szPathPrefixOutput + szBaseFilename + ".tiff";

  // Create DNG
  try {
    dng_host oDNGHost;

    // -------------------------------------------------------------
    // Print settings
    // -------------------------------------------------------------

    printf("\n");
    printf("==================================================================="
           "============\n");
    printf("Simple DNG converter\n");
    printf("==================================================================="
           "============\n\n");
    printf("\n");
    printf("BAYER:\n");
    printf("%s\n", m_szInputFile.c_str());

    printf("\nConverting...\n");

    // -------------------------------------------------------------
    // BAYER input file settings
    // -------------------------------------------------------------

    CFAReader reader;
    int ret = reader.open(m_szInputFile.c_str(), (m_ulWidth * m_ulHeight * 12) / 8);
    if (ret)
      return dng_error_unknown;

    AutoPtr<dng_memory_block> oBayerData(oDNGHost.Allocate(m_ulWidth * m_ulHeight * TagTypeSize(ttShort)));

    uint8_t *buf = (uint8_t*)oBayerData->Buffer();
    for (unsigned int i = 0; i < m_ulWidth * m_ulHeight; ++i, buf += 2) {
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

    dng_rect vImageBounds(m_ulHeight, m_ulWidth);

    AutoPtr<dng_image> oImage(
        oDNGHost.Make_dng_image(vImageBounds, m_unColorPlanes, ttShort));

    dng_pixel_buffer oBuffer;

    oBuffer.fArea = vImageBounds;
    oBuffer.fPlane = 0;
    oBuffer.fPlanes = 1;
    oBuffer.fRowStep = oBuffer.fPlanes * m_ulWidth;
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
    oNegative->SetModelName(m_szCameraModel.c_str());

    // Set localized camera model
    // Remarks: Tag [UniqueCameraModel] / [50709]
    oNegative->SetLocalName(m_szCameraModel.c_str());

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetColorKeys(colorKeyRed, colorKeyGreen, colorKeyBlue);

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetBayerMosaic(m_unBayerType);

    // Set bayer pattern information
    // Remarks: Tag [CFAPlaneColor] / [50710] and [CFALayout] / [50711]
    oNegative->SetColorChannels(m_unColorPlanes);

    // Set linearization table
    // Remarks: Tag [LinearizationTable] / [50712]
    AutoPtr<dng_memory_block> oCurve(
        oDNGHost.Allocate(sizeof(uint16) * m_unBitLimit));
    for (int64 i = 0; i < m_unBitLimit; i++) {
      uint16 *pulItem = oCurve->Buffer_uint16() + i;
      *pulItem = (uint16)(i);
    }
    oNegative->SetLinearization(oCurve);

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
    oNegative->SetDefaultCropSize(m_ulWidth - 4, m_ulHeight - 4);

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
    oNegative->SetNoiseReductionApplied(dng_urational(0, 1));

    // Set baseline sharpness
    // Remarks: Tag [BaselineSharpness] / [50732]
    oNegative->SetBaselineSharpness(1);

    // -------------------------------------------------------------
    // DNG EXIF Settings
    // -------------------------------------------------------------

    dng_exif *poExif = oNegative->GetExif();

    // Set Camera Make
    // Remarks: Tag [Make] / [EXIF]
    poExif->fMake.Set_ASCII(m_szMake.c_str());

    // Set Camera Model
    // Remarks: Tag [Model] / [EXIF]
    poExif->fModel.Set_ASCII(m_szCameraModel.c_str());

    // Set Lens Model
    // Remarks: Tag [LensName] / [EXIF]
    poExif->fLensName.Set_ASCII("GP43520");

    // Set Lens Make
    // Remarks: Tag [LensMake] / [EXIF]
    poExif->fLensMake.Set_ASCII(m_szMake.c_str());

    // Set ISO speed
    // Remarks: Tag [ISOSpeed] / [EXIF]
    poExif->fISOSpeedRatings[0] = m_unISO;
    poExif->fISOSpeedRatings[1] = 0;
    poExif->fISOSpeedRatings[2] = 0;

    // Set WB mode
    // Remarks: Tag [WhiteBalance] / [EXIF]
    poExif->fWhiteBalance = 0;

    // Set metering mode
    // Remarks: Tag [MeteringMode] / [EXIF]
    poExif->fMeteringMode = 2;

    // Set metering mode
    // Remarks: Tag [ExposureBiasValue] / [EXIF]
    poExif->fExposureBiasValue = dng_srational(0, 0);

    // Set aperture value
    // Remarks: Tag [ApertureValue] / [EXIF]
    poExif->SetFNumber(m_dLensAperture);

    // Set exposure time
    // Remarks: Tag [ExposureTime] / [EXIF]
    poExif->SetExposureTime(m_dExposureTime);

    // Set focal length
    // Remarks: Tag [FocalLength] / [EXIF]
    poExif->fFocalLength.Set_real64(m_dFocalLength, 1000);

    // Set lens info
    // Remarks: Tag [LensInfo] / [EXIF]
    poExif->fLensInfo[0].Set_real64(m_dFocalLength, 10);
    poExif->fLensInfo[1].Set_real64(m_dFocalLength, 10);
    poExif->fLensInfo[2].Set_real64(m_dLensAperture, 10);
    poExif->fLensInfo[3].Set_real64(m_dLensAperture, 10);

    // -------------------------------------------------------------
    // DNG Profile Settings: Simple color calibration
    // -------------------------------------------------------------

    // Camera space RGB to XYZ matrix with D65 illumination
    // Remarks: Derived from MATLAB using least square linear regression with
    //          MacBeth ColorChecker classic
    dng_matrix_3by3 oCameraRGB_to_XYZ_D65 =
        dng_matrix_3by3(2.3150, 0.0711, 0.1455, 0.9861, 0.7815, -0.2192, 0.2082,
                        -0.3349, 1.6432);
    uint32 ulCalibrationIlluminant1 = lsD65;

    // Camera space RGB to XYZ matrix with StdA illumination
    // Remarks: Derived from MATLAB using least square linear regression with
    //          MacBeth ColorChecker classic
    dng_matrix_3by3 oCameraRGB_to_XYZ_A =
        dng_matrix_3by3(1.6335, 0.4718, -0.0656, 0.5227, 1.0298, -0.3416,
                        0.0475, -0.2020, 1.2522);

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
    oWriter->WriteDNG(oDNGHost, oDNGStream, *oNegative.Get(), NULL,
                      ccUncompressed);
#if 0
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
    oWriter->WriteTIFF(oDNGHost, oTIFFStream, *oFinalImage.Get(), piRGB,
                       ccUncompressed, oNegative.Get(), &oRender.FinalSpace());
#endif
  }
  catch (const dng_exception &except) {
    return except.ErrorCode();
  }
  catch (...) {
    return dng_error_unknown;
  }

  printf("Conversion complete\n");

  return dng_error_none;
}

static void usage(const char* app)
{
	fprintf(stderr, "Usage: %s <input file>\n", app);
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
                usage(argv[0]);
		return EXIT_FAILURE;
	}

	dng_xmp_sdk::InitializeSDK();
        dng_error_code rc = ConvertToDNG(argv[1]);
	dng_xmp_sdk::TerminateSDK();
        if (rc != dng_error_none)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

