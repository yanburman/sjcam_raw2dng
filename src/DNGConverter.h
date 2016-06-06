/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include <string>
#include <dng_exceptions.h>
#include <dng_rational.h>
#include <dng_tag_values.h>

struct Config {
  Config() : m_bTiff(false), m_bLensCorrections(true)
  {
  }

  bool m_bTiff;
  bool m_bLensCorrections;
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
};

class DNGConverter
{
  public:
  DNGConverter(Config &config);
  ~DNGConverter();

  dng_error_code ConvertToDNG(std::string &m_szInputFile, const Exif &exif = m_oDefaultExif);

  protected:
  Config m_oConfig;
  static const dng_urational m_oZeroURational;
  static const Exif m_oDefaultExif;
};

#endif // __CONVERTER_H__
