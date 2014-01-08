/* $Header: /data/zender/nco_20150216/nco/src/nco/nco_aux.h,v 1.26 2014-01-08 22:10:56 pvicente Exp $ */

/* Purpose: Sub-set cell-based grids using auxiliary coordinate variable */

/* Copyright (C) 1995--2014 Charlie Zender
   License: GNU General Public License (GPL) Version 3
   See http://www.gnu.org/copyleft/gpl.html for full license text */

/* Usage:
   #include "nco_aux.h" *//* Auxiliary coordinates */

#ifndef NCO_AUX_H
#define NCO_AUX_H

/* Standard header files */
#include <math.h> /* sin cos cos sin 3.14159 */
#include <stdio.h> /* stderr, FILE, NULL, printf */

/* 3rd party vendors */
#include <netcdf.h> /* netCDF definitions and C library */
#include "nco_netcdf.h" /* NCO wrappers for netCDF C library */

/* Personal headers */
#include "nco.h" /* netCDF Operator (NCO) definitions */
#include "nco_lmt.h" /* Hyperslab limits */
#include "nco_sng_utl.h" /* String utilities */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

lmt_sct **
nco_aux_evl
(int in_id, 
 int aux_nbr, 
 char *aux_arg[],
 int *lmt_nbr,
 char *nm_dmn);                 /* O [sng] Dimension name */ 

nco_bool
nco_find_lat_lon
(int ncid, 
 char var_nm_lat[], 
 char var_nm_lon[], 
 char **units,
 int *lat_id,
 int *lon_id,
 nc_type *crd_typ);

int
nco_get_dmn_info
(int ncid,
 int varid,
 char dimname[],
 int *dimid,
 long *dmn_sz);

void 
nco_aux_prs
(const char *bnd_bx_sng, 
 const char *units, 
 float *lon_min, 
 float *lon_max, 
 float *lat_min, 
 float *lat_max);

lmt_sct **                           /* O [lst] Auxiliary coordinate limits */
nco_aux_evl_trv
(const int nc_id,                    /* I [ID] netCDF file ID */
 const trv_sct * const var_trv,      /* I [sct] Variable object */
 int aux_nbr,                        /* I [sng] Number of auxiliary coordinates */
 char *aux_arg[],                    /* I [sng] Auxiliary coordinates */
 const char * const lat_nm_fll,      /* I [sng] "latitude" full name */
 const char * const lon_nm_fll,      /* I [sng] "longitude" full name */
 const nc_type crd_typ,              /* I [nbr] netCDF type of both "latitude" and "longitude" */
 const char * const units,           /* I [sng] Units of both "latitude" and "longitude" */
 const trv_tbl_sct * const trv_tbl,  /* I [sct] GTT (Group Traversal Table) */
 int *aux_lmt_nbr);                  /* I/O [nbr] Number of coordinate limits */

nco_bool 
nco_find_lat_lon_trv
(const int nc_id,                    /* I [ID] netCDF file ID */
 const trv_sct * const var_trv,      /* I [sct] Variable object that contains "standard_name" attribute */
 const char * const attr_val,        /* I [sng] Attribute value to find ( "latitude" or "longitude" ) */
 char **var_nm_fll,                  /* I/O [sng] Full name of variable that has "latitude" or "longitude" attributes */
 nc_type *crd_typ,                   /* I/O [nbr] netCDF type of both "latitude" and "longitude" */
 char **units);                      /* I/O [sng] Units of both "latitude" and "longitude" */

#ifdef __cplusplus
} /* end extern "C" */
#endif /* __cplusplus */

#endif /* NCO_AUX_H */
