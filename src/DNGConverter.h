/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include <string>
#include <dng_exceptions.h>
#include <dng_rational.h>
#include <dng_matrix.h>
#include <dng_tag_values.h>
#include <dng_date_time.h>
#include <dng_xy_coord.h>
#include <dng_orientation.h>

struct Config {
  Config() : m_bTiff(false), m_bLensCorrections(true), m_bNoCalibration(false)
  {
  }

  bool m_bTiff;
  bool m_bLensCorrections;
  bool m_bNoCalibration;
  std::string m_szPathPrefixOutput;
};

struct Exif {
  Exif()
          : m_unISO(100), m_oExposureTime(1, 16), m_dLensAperture(1.7), m_dFocalLength(2.99), m_uLightSource(lsUnknown),
            m_oExposureBias(0, 0)
  {
  }

  uint16 m_unISO;
  dng_urational m_oExposureTime;
  double m_dLensAperture;
  double m_dFocalLength;
  uint32 m_uLightSource;
  dng_srational m_oExposureBias;
  std::string m_szCreatorTool;
  dng_date_time_info m_oOrigDate;
};

class DNGConverter
{
  public:
  DNGConverter(Config &config);
  ~DNGConverter();

  dng_error_code ConvertToDNG(const std::string &m_szInputFile, const std::string &m_szMetadataFile);

  int ParseMetadata(const std::string &metadata, Exif &oExif);

  protected:
  Config m_oConfig;

  dng_orientation m_oOrientation;

  static uint8 m_unColorPlanes;
  static uint16 m_unBayerType;

  static const std::string m_szMake;

  static uint32 m_unBitLimit;

  static const dng_urational m_oZeroURational;
  static const dng_matrix_3by3 m_oIdentityMatrix;
  static const dng_matrix_3by3 m_olsD65Matrix;
  static const dng_matrix_3by3 m_olsAMatrix;
};

#endif // __CONVERTER_H__
