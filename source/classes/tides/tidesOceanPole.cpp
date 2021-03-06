/***********************************************/
/**
* @file tidesOceanPole.cpp
*
* @brief Ocean pole tide.
* Pole tides is generated by the centrifugal effect of polar motion on the oceans.
* @see Tides
*
* @author Torsten Mayer-Guerr
* @date 2014-05-23
*
*/
/***********************************************/

#include "base/import.h"
#include "base/sphericalHarmonics.h"
#include "config/config.h"
#include "files/fileMeanPolarMotion.h"
#include "files/fileOceanPoleTide.h"
#include "classes/earthRotation/earthRotation.h"
#include "classes/tides/tides.h"
#include "classes/tides/tidesOceanPole.h"

/***********************************************/

TidesOceanPole::TidesOceanPole(Config &config)
{
  try
  {
    FileName oceanPoleName, fileNameMeanPole;
    UInt     minDegree;
    UInt     maxDegree  = INFINITYDEGREE;
    Double   factor;

    readConfig(config, "inputfileOceanPole", oceanPoleName,    Config::MUSTSET,  "{groopsDataDir}/tides/oceanPoleTide_desai2004.dat", "");
    readConfig(config, "minDegree",          minDegree,        Config::DEFAULT,  "2",      "");
    readConfig(config, "maxDegree",          maxDegree,        Config::OPTIONAL, "",       "");
    readConfig(config, "gammaReal",          gammaR,           Config::DEFAULT,  "0.6870", "");
    readConfig(config, "gammaImaginary",     gammaI,           Config::DEFAULT,  "0.0036", "");
    readConfig(config, "inputfileMeanPole",  fileNameMeanPole, Config::MUSTSET,  "{groopsDataDir}/tides/secularPole2018.xml", "");
    readConfig(config, "factor",             factor,           Config::DEFAULT,  "1.0", "the result is multiplied by this factor, set -1 to subtract the field");
    if(isCreateSchema(config)) return;

    // read ocean spherical harmonics
    // ------------------------------
    readFileOceanPoleTide(oceanPoleName, harmReal, harmImag);
    harmReal = factor * harmReal.get(maxDegree, minDegree);
    harmImag = factor * harmImag.get(maxDegree, minDegree);

    // read mean pole tide model
    // -------------------------
    readFileMeanPolarMotion(fileNameMeanPole, meanPole);
  }
  catch(std::exception &e)
  {
    GROOPS_RETHROW(e)
  }
}

/***********************************************/

void TidesOceanPole::pole(const Time &time, EarthRotationPtr earthRotation, Double &m1, Double &m2) const
{
  try
  {
    Double xBar, yBar;
    meanPole.compute(time, xBar, yBar);

    // pole (IERS2010, eq. (7.24))
    Double xp, yp, sp, deltaUT, LOD, X, Y, S;
    earthRotation->earthOrientationParameter(time, xp, yp, sp, deltaUT, LOD, X, Y, S);
    m1 =  (xp*RAD2DEG*3600 - xBar);
    m2 = -(yp*RAD2DEG*3600 - yBar);
  }
  catch(std::exception &e)
  {
    GROOPS_RETHROW(e)
  }
}


/***********************************************/

SphericalHarmonics TidesOceanPole::sphericalHarmonics(const Time &time, const Rotary3d &/*rotEarth*/, EarthRotationPtr earthRotation, EphemeridesPtr /*ephemerides*/, UInt maxDegree, UInt minDegree, Double GM, Double R) const
{
  try
  {
    Double m1, m2;
    pole(time, earthRotation, m1, m2);

    Matrix cnm  = harmReal.cnm() * ((m1*gammaR+m2*gammaI)*DEG2RAD/3600);
    Matrix snm  = harmReal.snm() * ((m1*gammaR+m2*gammaI)*DEG2RAD/3600);
           cnm += harmImag.cnm() * ((m2*gammaR-m1*gammaI)*DEG2RAD/3600);
           snm += harmImag.snm() * ((m2*gammaR-m1*gammaI)*DEG2RAD/3600);

    return SphericalHarmonics(harmReal.GM(), harmReal.R(), cnm, snm).get(maxDegree, minDegree, GM, R);
  }
  catch(std::exception &e)
  {
    GROOPS_RETHROW(e)
  }
}

/***********************************************/

void TidesOceanPole::deformation(const std::vector<Time> &time, const std::vector<Vector3d> &point, const std::vector<Rotary3d> &/*rotEarth*/,
                                 EarthRotationPtr rotation, EphemeridesPtr /*ephemerides*/, const std::vector<Double> &gravity, const Vector &hn, const Vector &ln,
                                 std::vector< std::vector<Vector3d> > &disp) const
{
  try
  {
    if((time.size()==0) || (point.size()==0))
      return;

    Matrix A = deformationMatrix(point, gravity, hn, ln, harmReal.GM(), harmReal.R(), harmReal.maxDegree());
    Vector xReal = A * harmReal.x();
    Vector xImag = A * harmImag.x();

    for(UInt idEpoch=0; idEpoch<time.size(); idEpoch++)
    {
      Double m1, m2;
      pole(time.at(idEpoch), rotation, m1, m2);
      Vector x =  xReal * ((m1*gammaR+m2*gammaI)*DEG2RAD/3600) + xImag * ((m2*gammaR-m1*gammaI)*DEG2RAD/3600);

      for(UInt k=0; k<point.size(); k++)
      {
        disp.at(k).at(idEpoch).x() += x(3*k+0);
        disp.at(k).at(idEpoch).y() += x(3*k+1);
        disp.at(k).at(idEpoch).z() += x(3*k+2);
      }
    } // for(idEpoch)
  }
  catch(std::exception &e)
  {
    GROOPS_RETHROW(e)
  }
}

/***********************************************/
/***********************************************/
