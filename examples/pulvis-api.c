/* This file is part of FES library.

   FES is free software: you can redistribute it and/or modify
   it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   FES is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU LESSER GENERAL PUBLIC LICENSE for more details.

   You should have received a copy of the GNU LESSER GENERAL PUBLIC LICENSE
   along with FES.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

#include "fes.h"

// Path to the configuration file and data used to test the library
// Change these settings to your liking.
#ifndef OCEAN_INI
#define OCEAN_INI "../test/fes.ini"
#endif

#ifndef LOAD_INI
#define LOAD_INI "../test/fes.ini"
#endif

#ifndef FES_DATA
#define FES_DATA "../test/data"
#endif

int main(int argc, char *argv[])
{
  // The return code
  int rc = 0;
  // The hour of the estimate.
  int hour;
  // Latitude and longitude of the point where the ocean tide will be
  // evaluated.
  double lon = 0.0;
  double lat = 0.0;

  // Short tides (semi_diurnal and diurnal tides)
  double tide;
  // Time in CNES Julian days, defined as Modified Julian Day minus 33282.
  // Thus CNES 0 is at midnight between the 31 December and 01 January 1950
  // AD Gregorian.
  double time;
  // Long period tides
  double lp;
  // Loading effects for short tide
  double load;
  // Loading effects for long period tides (is always equal to zero)
  double loadlp;
  // FES handlers
  FES short_tide;
  FES radial_tide = NULL;

#ifdef _WIN32
  _putenv_s("FES_DATA", FES_DATA);
#else
  setenv("FES_DATA", FES_DATA, 1);
#endif

  if (argc != 4)
  {
    printf("Usage: ./puvlis-api <timestamp> <lat> <lon>");
    return EXIT_FAILURE; // timestamp, lat, lon
  }
  double epochSeconds = atof(argv[1]);
  lat = atof(argv[2]);
  lon = atof(argv[3]);

  // Creating the FES handler to calculate the ocean tide
  if (fes_new(&short_tide, FES_TIDE, FES_IO, OCEAN_INI))
  {
    printf("fes error : %s\n", fes_error(short_tide));
    goto error;
  }

  // Creating the FES handler to calculate the loading tide
  if (fes_new(&radial_tide, FES_RADIAL, FES_IO, LOAD_INI))
  {
    printf("fes error : %s\n", fes_error(radial_tide));
    goto error;
  }
  unsigned int twentyYearsInDays = 7305;
  unsigned int secondsInDay = 86400;
  time = (epochSeconds / 86400) + twentyYearsInDays;

  // Compute ocean tide
  if (fes_core(short_tide, lat, lon, time, &tide, &lp))
  {
    // If the current point is undefined (i.e. the point is on land), the
    // tide is not defined.
    if (fes_errno(short_tide) == FES_NO_DATA)
    {
      fprintf(stderr, "short tide no data");
      goto error;
    }

    else
    {
      fprintf(stderr, "%s\n", fes_error(short_tide));
      goto error;
    }
  }

  // Compute loading tide
  if (fes_core(radial_tide, lat, lon, time, &load, &loadlp))
  {
    // If the current point is undefined (i.e. the point is on land), the
    // loading tide is not defined.
    if (fes_errno(radial_tide) == FES_NO_DATA)
    {
      fprintf(stderr, "radial tide no data");
      goto error;
    }
    else
    {
      fprintf(stderr, "%s\n", fes_error(radial_tide));
      goto error;
    }
  }

  // tide + lp        = pure tide (as seen by a tide gauge)
  // tide + lp + load = geocentric tide (as seen by a satellite)
  double geocentric_tide = tide + lp + load;
  printf("%.1f\n", geocentric_tide);

  goto finish;

error:
  rc = 1;

finish:
  // Release the memory used by the FES handlers.
  fes_delete(short_tide);
  fes_delete(radial_tide);

  return rc;
}
