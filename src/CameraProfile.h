/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __CAMERA_PROFILE_H__
#define __CAMERA_PROFILE_H__

#include <string>

#include <dng_lens_correction.h>

class LensCalibration
{
  protected:
  std::vector<real64> m_oVignetteGainParams;

  public:
  dng_vignette_radial_params m_oVignetteParams;

  LensCalibration(double val);
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
                double aperture,
                const LensCalibration *calib = NULL)
          : m_ulWidth(w), m_ulHeight(h), m_szCameraModel(name), m_ulBlackLevel(black_level), m_oNeutralWB(3),
            m_fAperture(aperture), m_oCalib(calib), m_fBaselineNoise(base_noise)
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
  double m_fAperture;
  const LensCalibration *m_oCalib;
};

struct SJ5000xProfile : public CameraProfile {
  SJ5000xProfile(uint32 w, uint32 h) : CameraProfile(w, h, 0, 0.87, 1.3, 0.72, 3, "SJ5000X", 2.8, &SJ5000xCalib)
  {
  }
};

struct M20Profile : public CameraProfile {
  M20Profile(uint32 w, uint32 h) : CameraProfile(w, h, 200, 0.46, 1, 0.7, 4, "M20", 2.8)
  {
  }
};

struct SJ6Profile : public CameraProfile {
  SJ6Profile(uint32 w, uint32 h) : CameraProfile(w, h, 0, 0.5, 1, 0.55, 4, "SJ6 LEGEND", 2.5)
  {
  }
};

#endif
