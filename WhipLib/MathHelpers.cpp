#include "MathHelpers.h"
#include <cmath>
//-------------------------------------------------------------------------------------------------

double MathHelpers::ConstrainAngle(double dAngle)
{
  dAngle = std::fmod(dAngle, 360);
  if (dAngle < 0)
    dAngle += 360;
  return dAngle;
}

//-------------------------------------------------------------------------------------------------
