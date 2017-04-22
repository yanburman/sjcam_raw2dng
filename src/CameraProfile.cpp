/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include "CameraProfile.h"

static const dng_point_real64 g_oCenter(0.5, 0.5);

LensCalibration::LensCalibration(double val)
        : m_oVignetteGainParams(dng_vignette_radial_params::kNumTerms),
          m_oVignetteParams(m_oVignetteGainParams, g_oCenter)
{
  m_oVignetteGainParams[0] = val;
  m_oVignetteGainParams[1] = val;
  m_oVignetteGainParams[2] = val;
  m_oVignetteGainParams[3] = val;
  m_oVignetteGainParams[4] = val;
}
