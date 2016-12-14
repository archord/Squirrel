/*
  This file is part of the Astrometry.net suite.
  Copyright 2010 Dustin Lang.

  The Astrometry.net suite is free software; you can redistribute
  it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, version 2.

  The Astrometry.net suite is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the Astrometry.net suite ; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#ifndef ANWCSLIB_H
#define ANWCSLIB_H

#include "sip.h"
#include "an-bool.h"
#include "qfits_header.h"
#include "bl.h"

/** Interface to Mark Calabretta's wcslib, if available, and
 Astrometry.net's TAN/SIP implementation. */

#define ANWCS_TYPE_WCSLIB 1
#define ANWCS_TYPE_SIP 2

struct anwcs_t {
	/**
	 If type == ANWCS_TYPE_WCSLIB:
	   data is a private struct containing a wcslib  "struct wcsprm*".
	 If type == ANWCS_TYPE_SIP:
	   data is a "sip_t*"
	 */
	int type;
	void* data;
};
typedef struct anwcs_t anwcs_t;


anwcs_t* anwcs_open(const char* filename, int ext);

anwcs_t* anwcs_open_wcslib(const char* filename, int ext);

anwcs_t* anwcs_open_sip(const char* filename, int ext);

anwcs_t* anwcs_open_tan(const char* filename, int ext);

anwcs_t* anwcs_new_sip(const sip_t* sip);

anwcs_t* anwcs_new_tan(const tan_t* tan);

int anwcs_write(const anwcs_t* wcs, const char* filename);

int anwcs_write_to(const anwcs_t* wcs, FILE* fid);

int anwcs_add_to_header(const anwcs_t* wcs, qfits_header* hdr);

int anwcs_radec2pixelxy(const anwcs_t* wcs, double ra, double dec, double* px, double* py);

int anwcs_pixelxy2radec(const anwcs_t* wcs, double px, double py, double* ra, double* dec);

int anwcs_pixelxy2xyz(const anwcs_t* wcs, double px, double py, double* xyz);

int anwcs_xyz2pixelxy(const anwcs_t* wcs, const double* xyz, double *px, double *py);

bool anwcs_radec_is_inside_image(const anwcs_t* wcs, double ra, double dec);

void anwcs_get_radec_bounds(const anwcs_t* wcs, int stepsize,
							double* pramin, double* pramax,
							double* pdecmin, double* pdecmax);

void anwcs_print(const anwcs_t* wcs, FILE* fid);

// Center and radius of the field.
// RA,Dec,radius in degrees.
int anwcs_get_radec_center_and_radius(const anwcs_t* anwcs,
									  double* p_ra, double* p_dec, double* p_radius);

void anwcs_walk_image_boundary(const anwcs_t* wcs, double stepsize,
							   void (*callback)(const anwcs_t* wcs, double x, double y, double ra, double dec, void* token),
							   void* token);

bool anwcs_is_discontinuous(const anwcs_t* wcs, double ra1, double dec1,
							double ra2, double dec2);

/*
 // Assuming there is a discontinuity between (ra1,dec1) and (ra2,dec2),
 // return 
 int anwcs_get_discontinuity(const anwcs_t* wcs, double ra1, double dec1,
 double ra2, double dec2,
 double* dra, double* ddec);
 */
dl* anwcs_walk_discontinuity(const anwcs_t* wcs,
							 double ra1, double dec1, double ra2, double dec2,
							 double ra3, double dec3, double ra4, double dec4,
							 double stepsize,
							 dl* radecs);

bool anwcs_overlaps(const anwcs_t* wcs1, const anwcs_t* wcs2, int stepsize);

double anwcs_imagew(const anwcs_t* anwcs);
double anwcs_imageh(const anwcs_t* anwcs);

void anwcs_set_size(anwcs_t* anwcs, int W, int H);

int anwcs_scale_wcs(anwcs_t* anwcs, double scale);

// angle in deg
int anwcs_rotate_wcs(anwcs_t* anwcs, double angle);

// Approximate pixel scale, in arcsec/pixel, at the reference point.
double anwcs_pixel_scale(const anwcs_t* anwcs);

void anwcs_free(anwcs_t* wcs);


#endif
