/* $Header: /data/zender/nco_20150216/nco/src/nco/ncwa.c,v 1.35 2000-06-21 00:42:41 zender Exp $ */

/* ncwa -- netCDF weighted averager */

/* Purpose: Compute averages of specified hyperslabs of specfied variables
   in a single input netCDF file and output them to a single file. */

/* Copyright (C) 1995--2000 Charlie Zender

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   The file LICENSE contains the GNU General Public License, version 2
   It may be viewed interactively by typing, e.g., ncks -L

   The author of this software, Charlie Zender, would like to receive
   your suggestions, improvements, bug-reports, and patches for NCO.
   Please contact me via e-mail at zender@uci.edu or by writing

   Charlie Zender
   Department of Earth System Science
   University of California at Irvine
   Irvine, CA 92697-3100
 */

/* fxm: As of 1998/12/02, -n and -W switches were deactivated but code left in place
   while I rethink the normalization switches */ 

/* Usage:
   ncwa -O -a lon /home/zender/nco/data/in.nc foo.nc
   ncwa -O -R -p /ZENDER/tmp -l /home/zender/nco/data in.nc foo.nc
   ncwa -O -a lat -w gw -d lev,17 -v T -p /fs/cgd/csm/input/atm SEP1.T42.0596.nc foo.nc
   ncwa -O -C -a lat,lon,time -w gw -v PS -p /fs/cgd/csm/input/atm SEP1.T42.0596.nc foo.nc;ncks -H foo.nc
 */ 

/* Standard header files */
#include <math.h>               /* sin cos cos sin 3.14159 */
#include <netcdf.h>             /* netCDF def'ns */
#include <stdio.h>              /* stderr, FILE, NULL, etc. */
#include <stdlib.h>             /* atof, atoi, malloc, getopt */ 
#include <string.h>             /* strcmp. . . */
#include <sys/stat.h>           /* stat() */
#include <time.h>               /* machine time */
#include <unistd.h>             /* all sorts of POSIX stuff */ 
/* #include <assert.h> */            /* assert() debugging macro */ 
/* #include <errno.h> */             /* errno */
/* #include <malloc.h>    */         /* malloc() stuff */

/* 3rd party vendors */
#ifdef OMP /* OpenMP */
#include <omp.h> /* OpenMP pragmas */
#endif /* not OpenMP */

/* #define MAIN_PROGRAM_FILE MUST precede #include nc.h */
#define MAIN_PROGRAM_FILE
#include "nc.h"                 /* global definitions */

int 
main(int argc,char **argv)
{
  bool EXCLUDE_INPUT_LIST=False; /* Option c */ 
  bool FILE_RETRIEVED_FROM_REMOTE_LOCATION;
  bool FORCE_APPEND=False; /* Option A */ 
  bool FORCE_OVERWRITE=False; /* Option O */ 
  bool FORTRAN_STYLE=False; /* Option F */
  bool HISTORY_APPEND=True; /* Option h */
  bool MUST_CONFORM=False; /* Must var_conform_dim() find truly conforming variables? */ 
  bool DO_CONFORM_MSK; /* Did var_conform_dim() find truly conforming variables? */ 
  bool DO_CONFORM_WGT; /* Did var_conform_dim() find truly conforming variables? */ 
  bool NCAR_CSM_FORMAT;
  bool PROCESS_ALL_COORDINATES=False; /* Option c */
  bool PROCESS_ASSOCIATED_COORDINATES=True; /* Option C */
  bool REMOVE_REMOTE_FILES_AFTER_PROCESSING=True; /* Option R */ 
  bool NRM_BY_DNM=True; /* Option N */ 
  bool MULTIPLY_BY_TALLY=False; /* Not currently implemented */ 
  bool NORMALIZE_BY_TALLY=True; /* Not currently implemented */ 
  bool NORMALIZE_BY_WEIGHT=True; /* Not currently implemented */ 
  bool WGT_MSK_CRD_VAR=True; /* Option I */ 
  bool opt_a_flg=False; /* Option a */

  char **dmn_avg_lst_in=NULL_CEWI; /* Option a */ 
  char **var_lst_in=NULL_CEWI;
  char **fl_lst_abb=NULL; /* Option n */ 
  char **fl_lst_in;
  char *fl_in=NULL;
  char *fl_pth_lcl=NULL; /* Option l */ 
  char *lmt_arg[MAX_NC_DIMS];
  char *opt_sng;
  char *fl_out;
  char *fl_out_tmp;
  char *fl_pth=NULL; /* Option p */ 
  char *time_bfr_srt;
  char *msk_nm=NULL;
  char *wgt_nm=NULL;
  char *cmd_ln;
  char CVS_Id[]="$Id: ncwa.c,v 1.35 2000-06-21 00:42:41 zender Exp $"; 
  char CVS_Revision[]="$Revision: 1.35 $";
  
  dmn_sct **dim;
  dmn_sct **dmn_out;
  dmn_sct **dmn_avg=NULL_CEWI;
  
  double msk_val=1.0; /* Option M */ 

  extern char *optarg;
  extern int ncopts;
  extern int optind;
  
  int arg_cnt=0; /* arg_cnt get incremented */
  int idx;
  int idx_avg;
  int idx_fl;
  int in_id;  
  int out_id;  
  int nbr_abb_arg=0;
  int nbr_dmn_fl;
  int nbr_dmn_avg=0;
  int lmt_nbr=0; /* Option d. NB: lmt_nbr gets incremented */
  int nbr_var_fl;
  int nbr_var_fix; /* nbr_var_fix gets incremented */ 
  int nbr_var_prc; /* nbr_var_prc gets incremented */ 
  int nbr_xtr=0; /* nbr_xtr won't otherwise be set for -c with no -v */ 
  int nbr_dmn_out;
  int nbr_dmn_xtr;
  int nbr_fl=0;
  int opt;
  int op_type=0; /* Option o */ 
  int rec_dmn_id=-1;
  
  lmt_sct *lmt;
  
  nm_id_sct *dmn_lst;
  nm_id_sct *xtr_lst=NULL; /* xtr_lst can get realloc()'d from NULL with -c option */ 
  nm_id_sct *dmn_avg_lst;
  
  time_t clock;
  
  var_sct **var;
  var_sct **var_fix;
  var_sct **var_fix_out;
  var_sct **var_out;
  var_sct **var_prc;
  var_sct **var_prc_out;
  var_sct *msk=NULL;
  var_sct *msk_out=NULL;
  var_sct *wgt=NULL;
  var_sct *wgt_avg=NULL;
  var_sct *wgt_out=NULL;
  
  /* Start the clock and save the command line */  
  cmd_ln=cmd_ln_sng(argc,argv);
  clock=time((time_t *)NULL);
  time_bfr_srt=ctime(&clock);
  
  /* Get program name and set program enum (e.g., prg=ncra) */
  prg_nm=prg_prs(argv[0],&prg);

  /* Parse command line arguments */
  opt_sng="Aa:CcD:d:FhIl:M:m:nNo:Op:rRv:xWw:";
  while((opt = getopt(argc,argv,opt_sng)) != EOF){
    arg_cnt++;
    switch(opt){
    case 'A': /* Toggle FORCE_APPEND */
      FORCE_APPEND=!FORCE_APPEND;
      break;
    case 'a':
      /* Dimensions over which to average hyperslab */
      if(opt_a_flg){
	(void)fprintf(stdout,"%s: ERROR Option -a appears more than once\n",prg_nm);
	(void)fprintf(stdout,"%s: HINT Use -a dim1,dim2,... not -a dim1 -a dim2 ...\n",prg_nm);
	(void)usg_prn();
	exit(EXIT_FAILURE);
      } /* endif */
      dmn_avg_lst_in=lst_prs(optarg,",",&nbr_dmn_avg);
      opt_a_flg=True;
      break;
    case 'C': /* Extraction list should include all coordinates associated with extracted variables? */ 
      PROCESS_ASSOCIATED_COORDINATES=False;
      break;
    case 'c':
      PROCESS_ALL_COORDINATES=True;
      break;
    case 'D':
      /* Debugging level. Default is 0. */
      dbg_lvl=atoi(optarg);
      break;
    case 'd':
      /* Copy argument for later processing */ 
      lmt_arg[lmt_nbr]=(char *)strdup(optarg);
      lmt_nbr++;
      break;
    case 'F':
      /* Toggle index convention. Default is 0-based arrays (C-style). */
      FORTRAN_STYLE=!FORTRAN_STYLE;
      break;
    case 'h': /* Toggle appending to history global attribute */
      HISTORY_APPEND=!HISTORY_APPEND;
      break;
    case 'I':
      WGT_MSK_CRD_VAR=!WGT_MSK_CRD_VAR;
      break;
    case 'l':
      /* Local path prefix for files retrieved from remote file system */
      fl_pth_lcl=optarg;
      break;
    case 'm':
      /* Name of variable to use as mask in averaging.  Default is none */
      msk_nm=optarg;
      break;
    case 'M':
      /* Good data defined by relation to mask value. Default is 1. */
      msk_val=strtod(optarg,(char **)NULL);
      break;
    case 'N':
      NRM_BY_DNM=False;
      NORMALIZE_BY_TALLY=False;
      NORMALIZE_BY_WEIGHT=False;
      break;
    case 'n':
      NORMALIZE_BY_WEIGHT=False;
      (void)fprintf(stdout,"%s: ERROR This option has been disabled while I rethink its implementation\n",prg_nm);
      exit(EXIT_FAILURE);
      break;
    case 'O': /* Toggle FORCE_OVERWRITE */
      FORCE_OVERWRITE=!FORCE_OVERWRITE;
      break;
    case 'o':
      /* The relational operator type.  Default is 0, eq, equality */
      op_type=op_prs(optarg);
      break;
    case 'p':
      /* Common file path */
      fl_pth=optarg;
      break;
    case 'R':
      /* Toggle removal of remotely-retrieved-files. Default is True. */
      REMOVE_REMOTE_FILES_AFTER_PROCESSING=!REMOVE_REMOTE_FILES_AFTER_PROCESSING;
      break;
    case 'r':
      /* Print CVS program information and copyright notice */
      (void)copyright_prn(CVS_Id,CVS_Revision);
      (void)nc_lib_vrs_prn();
       exit(EXIT_SUCCESS);
      break;
    case 'v':
      /* Variables to extract/exclude */ 
      var_lst_in=lst_prs(optarg,",",&nbr_xtr);
      break;
    case 'W':
      NORMALIZE_BY_TALLY=False;
      (void)fprintf(stdout,"%s: ERROR This option has been disabled while I rethink its implementation\n",prg_nm);
      exit(EXIT_FAILURE);
      break;
    case 'w':
      /* Variable to use as weight in averaging.  Default is none */
      wgt_nm=optarg;
      break;
    case 'x':
      /* Exclude rather than extract variables specified with -v */
      EXCLUDE_INPUT_LIST=True;
      break;
    case '?':
      /* Print proper usage */
      (void)usg_prn();
      exit(EXIT_FAILURE);
      break;
    } /* end switch */
  } /* end while loop */

  /* If called without arguments, print usage and exit successfully */ 
  if(arg_cnt == 0){
    (void)usg_prn();
    exit(EXIT_SUCCESS);
  } /* endif */
  
  /* Ensure we do not attempt to normalize by non-existent weight */ 
  if(wgt_nm == NULL) NORMALIZE_BY_WEIGHT=False;

  /* Process positional arguments and fill in filenames */
  fl_lst_in=fl_lst_mk(argv,argc,optind,&nbr_fl,&fl_out);

  /* Make uniform list of user-specified dimension limits */ 
  lmt=lmt_prs(lmt_nbr,lmt_arg);
  
  /* Make netCDF errors fatal and print the diagnostic */   
  ncopts=NC_VERBOSE | NC_FATAL; 
  
  /* Parse filename */ 
  fl_in=fl_nm_prs(fl_in,0,&nbr_fl,fl_lst_in,nbr_abb_arg,fl_lst_abb,fl_pth);
  /* Make sure file is on local system and is readable or die trying */ 
  fl_in=fl_mk_lcl(fl_in,fl_pth_lcl,&FILE_RETRIEVED_FROM_REMOTE_LOCATION);
  /* Open the file for reading */ 
  in_id=ncopen(fl_in,NC_NOWRITE);
  
  /* Get number of variables, dimensions, and record dimension ID of input file */
  (void)ncinquire(in_id,&nbr_dmn_fl,&nbr_var_fl,(int *)NULL,&rec_dmn_id);
  
  /* Form initial extraction list from user input */ 
  xtr_lst=var_lst_mk(in_id,nbr_var_fl,var_lst_in,PROCESS_ALL_COORDINATES,&nbr_xtr);

  /* Change included variables to excluded variables */ 
  if(EXCLUDE_INPUT_LIST) xtr_lst=var_lst_xcl(in_id,nbr_var_fl,xtr_lst,&nbr_xtr);

  /* Add all coordinate variables to extraction list */ 
  if(PROCESS_ALL_COORDINATES) xtr_lst=var_lst_add_crd(in_id,nbr_var_fl,nbr_dmn_fl,xtr_lst,&nbr_xtr);

  /* Make sure coordinates associated extracted variables are also on extraction list */ 
  if(PROCESS_ASSOCIATED_COORDINATES) xtr_lst=var_lst_ass_crd_add(in_id,xtr_lst,&nbr_xtr);

  /* Remove record coordinate, if any, from extraction list */ 
  if(False) xtr_lst=var_lst_crd_xcl(in_id,rec_dmn_id,xtr_lst,&nbr_xtr);

  /* Finally, heapsort the extraction list by variable ID for fastest I/O */ 
  if(nbr_xtr > 1) xtr_lst=lst_heapsort(xtr_lst,nbr_xtr,False);
    
  /* Find coordinate/dimension values associated with user-specified limits */ 
  for(idx=0;idx<lmt_nbr;idx++) (void)lmt_evl(in_id,lmt+idx,0L,FORTRAN_STYLE);
  
  /* Find dimensions associated with variables to be extracted */ 
  dmn_lst=dmn_lst_ass_var(in_id,xtr_lst,nbr_xtr,&nbr_dmn_xtr);

  /* Fill in dimension structure for all extracted dimensions */ 
  dim=(dmn_sct **)malloc(nbr_dmn_xtr*sizeof(dmn_sct *));
  for(idx=0;idx<nbr_dmn_xtr;idx++){
    dim[idx]=dmn_fll(in_id,dmn_lst[idx].id,dmn_lst[idx].nm);
  } /* end loop over idx */
  
  /* Merge hyperslab limit information into dimension structures */ 
  if(lmt_nbr > 0) (void)dmn_lmt_mrg(dim,nbr_dmn_xtr,lmt,lmt_nbr);

  /* Not specifying any dimensions is interpreted as specifying all dimensions */
  if (nbr_dmn_avg == 0){
    nbr_dmn_avg=nbr_dmn_xtr;
    dmn_avg_lst_in=(char **)malloc(nbr_dmn_avg*sizeof(char *));
    for(idx=0;idx<nbr_dmn_avg;idx++){
      dmn_avg_lst_in[idx]=(char *)strdup(dmn_lst[idx].nm);
    } /* end loop over idx */
    (void)fprintf(stdout,"%s: WARNING No dimensions specified with -a, therefore averaging over all dimensions\n",prg_nm);
  } /* end if nbr_dmn_avg == 0 */

  if (nbr_dmn_avg > 0){
    /* Form list of averaging dimensions */ 
    dmn_avg_lst=dmn_lst_mk(in_id,dmn_avg_lst_in,nbr_dmn_avg);

    if(nbr_dmn_avg > nbr_dmn_xtr){
      (void)fprintf(stdout,"%s: ERROR More averaging dimensions than extracted dimensions\n",prg_nm);
      exit(EXIT_FAILURE);
    } /* end if */

    /* Form list of averaging dimensions from extracted input dimensions */ 
    dmn_avg=(dmn_sct **)malloc(nbr_dmn_avg*sizeof(dmn_sct *));
    for(idx_avg=0;idx_avg<nbr_dmn_avg;idx_avg++){
      for(idx=0;idx<nbr_dmn_xtr;idx++){
	if(!strcmp(dmn_avg_lst[idx_avg].nm,dim[idx]->nm)) break;
      } /* end loop over idx_avg */
      if(idx != nbr_dmn_xtr){
	dmn_avg[idx_avg]=dim[idx];
      }else{
	(void)fprintf(stderr,"%s: WARNING averaging dimension \"%s\" is not contained in any variable in extraction list\n",prg_nm,dmn_avg_lst[idx_avg].nm);
	/* Collapse dimension average list by omitting irrelevent dimension */ 
	(void)memmove(dmn_avg_lst,dmn_avg_lst,idx_avg*sizeof(nm_id_sct));
	(void)memmove(dmn_avg_lst+idx_avg*sizeof(nm_id_sct),dmn_avg_lst+(idx_avg+1)*sizeof(nm_id_sct),(nbr_dmn_avg-idx_avg+1)*sizeof(nm_id_sct));
	--nbr_dmn_avg;
	dmn_avg_lst=(nm_id_sct *)realloc(dmn_avg_lst,nbr_dmn_avg*sizeof(nm_id_sct));
      } /* end else */ 
    } /* end loop over idx_avg */

    /* Make sure no averaging dimension is specified more than once */ 
    for(idx=0;idx<nbr_dmn_avg;idx++){
      for(idx_avg=0;idx_avg<nbr_dmn_avg;idx_avg++){
	if(idx_avg != idx){
	  if(dmn_avg[idx]->id == dmn_avg[idx_avg]->id){
	    (void)fprintf(stdout,"%s: ERROR %s specified more than once in averaging list\n",prg_nm,dmn_avg[idx]->nm);
	    exit(EXIT_FAILURE);
	  } /* end if */
	} /* end if */
      } /* end loop over idx_avg */
    } /* end loop over idx */

    /* Dimensions to be averaged will not appear in output file */ 
    dmn_out=(dmn_sct **)malloc((nbr_dmn_xtr-nbr_dmn_avg)*sizeof(dmn_sct *));
    nbr_dmn_out=0;
    for(idx=0;idx<nbr_dmn_xtr;idx++){
      for(idx_avg=0;idx_avg<nbr_dmn_avg;idx_avg++){
	if(!strcmp(dmn_avg_lst[idx_avg].nm,dim[idx]->nm)) break;
      } /* end loop over idx_avg */
      if(idx_avg == nbr_dmn_avg){
	dmn_out[nbr_dmn_out]=dmn_dup(dim[idx]);
	(void)dmn_xrf(dim[idx],dmn_out[nbr_dmn_out]);
	nbr_dmn_out++;
      } /* end if */
    } /* end loop over idx_avg */

    if(nbr_dmn_out != nbr_dmn_xtr-nbr_dmn_avg){
      (void)fprintf(stdout,"%s: ERROR nbr_dmn_out != nbr_dmn_xtr-nbr_dmn_avg\n",prg_nm);
      exit(EXIT_FAILURE);
    } /* end if */
    
  }else{

    /* Duplicate input dimension structures for output dimension structures */ 
    nbr_dmn_out=nbr_dmn_xtr;
    dmn_out=(dmn_sct **)malloc(nbr_dmn_out*sizeof(dmn_sct *));
    for(idx=0;idx<nbr_dmn_out;idx++){
      dmn_out[idx]=dmn_dup(dim[idx]);
      (void)dmn_xrf(dim[idx],dmn_out[idx]);
    } /* end loop over idx */

  } /* end if */ 

  /* Is this an NCAR CSM-format history tape? */
  NCAR_CSM_FORMAT=ncar_csm_inq(in_id);

  /* Fill in variable structure list for all extracted variables */ 
  var=(var_sct **)malloc(nbr_xtr*sizeof(var_sct *));
  var_out=(var_sct **)malloc(nbr_xtr*sizeof(var_sct *));
  for(idx=0;idx<nbr_xtr;idx++){
    var[idx]=var_fll(in_id,xtr_lst[idx].id,xtr_lst[idx].nm,dim,nbr_dmn_xtr);
    var_out[idx]=var_dup(var[idx]);
    (void)var_xrf(var[idx],var_out[idx]);
    (void)var_dmn_xrf(var_out[idx]);
  } /* end loop over idx */

  /* Divide variable lists into lists of fixed variables and variables to be processed */ 
  (void)var_lst_divide(var,var_out,nbr_xtr,NCAR_CSM_FORMAT,dmn_avg,nbr_dmn_avg,&var_fix,&var_fix_out,&nbr_var_fix,&var_prc,&var_prc_out,&nbr_var_prc);

  /* We now have final list of variables to extract. Phew. */
  if(dbg_lvl > 0){
    for(idx=0;idx<nbr_xtr;idx++) (void)fprintf(stderr,"var[%d]->nm = %s, ->id=[%d]\n",idx,var[idx]->nm,var[idx]->id);
    for(idx=0;idx<nbr_var_fix;idx++) (void)fprintf(stderr,"var_fix[%d]->nm = %s, ->id=[%d]\n",idx,var_fix[idx]->nm,var_fix[idx]->id);
    for(idx=0;idx<nbr_var_prc;idx++) (void)fprintf(stderr,"var_prc[%d]->nm = %s, ->id=[%d]\n",idx,var_prc[idx]->nm,var_prc[idx]->id);
  } /* end if */
  
  /* Open output file */ 
  fl_out_tmp=fl_out_open(fl_out,FORCE_APPEND,FORCE_OVERWRITE,&out_id);
  if(dbg_lvl > 4) (void)fprintf(stderr,"Input, output file IDs = %d, %d\n",in_id,out_id);

  /* Copy all global attributes */ 
  (void)att_cpy(in_id,out_id,NC_GLOBAL,NC_GLOBAL);
  
  /* Catenate time-stamped command line to "history" global attribute */ 
  if(HISTORY_APPEND) (void)hst_att_cat(out_id,cmd_ln);

  /* Define dimensions in output file */ 
  (void)dmn_def(fl_out,out_id,dmn_out,nbr_dmn_out);

  /* Define variables in output file, and copy their attributes */ 
  (void)var_def(in_id,fl_out,out_id,var_out,nbr_xtr,dmn_out,nbr_dmn_out);

  /* New missing values must be added to the output file in define mode */
  if(msk_nm != NULL){
    for(idx=0;idx<nbr_var_prc;idx++){
      /* Define for var_prc_out because mss_val for var_prc will be overwritten in var_refresh */ 
      if(!var_prc_out[idx]->has_mss_val){
	var_prc_out[idx]->has_mss_val=True;
	var_prc_out[idx]->mss_val=mss_val_mk(var_prc[idx]->type);
	(void)ncattput(out_id,var_prc_out[idx]->id,"missing_value",var_prc_out[idx]->type,1,var_prc_out[idx]->mss_val.vp);
      } /* end if */
    } /* end for */ 
  } /* end if */

  /* Turn off default filling behavior to enhance efficiency */ 
#if ( ! defined SUN4 ) && ( ! defined SUN4SOL2 ) && ( ! defined SUNMP )
  (void)ncsetfill(out_id,NC_NOFILL);
#endif
  
  /* Take output file out of define mode */ 
  (void)ncendef(out_id);
  
  /* Zero start vectors for all output variables */ 
  (void)var_srt_zero(var_out,nbr_xtr);

  /* Copy variable data for non-processed variables */ 
  (void)var_val_cpy(in_id,out_id,var_fix,nbr_var_fix);

  /* Close first input netCDF file */ 
  ncclose(in_id);
  
  /* Loop over input files */ 
  for(idx_fl=0;idx_fl<nbr_fl;idx_fl++){
    /* Parse filename */ 
    if(idx_fl != 0) fl_in=fl_nm_prs(fl_in,idx_fl,&nbr_fl,fl_lst_in,nbr_abb_arg,fl_lst_abb,fl_pth);
    if(dbg_lvl > 0) (void)fprintf(stderr,"\nInput file %d is %s; ",idx_fl,fl_in);
    /* Make sure file is on local system and is readable or die trying */ 
    if(idx_fl != 0) fl_in=fl_mk_lcl(fl_in,fl_pth_lcl,&FILE_RETRIEVED_FROM_REMOTE_LOCATION);
    if(dbg_lvl > 0) (void)fprintf(stderr,"local file %s:\n",fl_in);
    in_id=ncopen(fl_in,NC_NOWRITE);
    
    /* Perform various error-checks on input file */ 
    if(False) (void)fl_cmp_err_chk();

    /* Find weighting variable in input file */
    if(wgt_nm != NULL){
      int wgt_id;
      
      wgt_id=ncvarid_or_die(in_id,wgt_nm);
      wgt=var_fll(in_id,wgt_id,wgt_nm,dim,nbr_dmn_fl);
      
      /* Retrieve weighting variable */ 
      (void)var_get(in_id,wgt);
      /* fxm: Perhaps should allocate default tally array for wgt here
       That way, when wgt conforms to the first var_prc_out and it therefore
       does not get a tally array copied by var_dup() in var_conform_dim(), 
       it will at least have space for a tally array. TODO #114. */ 

    } /* end if */

    /* Find mask variable in input file */
    if(msk_nm != NULL){
      int msk_id;
      
      msk_id=ncvarid_or_die(in_id,msk_nm);
      msk=var_fll(in_id,msk_id,msk_nm,dim,nbr_dmn_fl);
      
      /* Retrieve mask variable */ 
      (void)var_get(in_id,msk);
    } /* end if */

#ifdef OMP /* OpenMP */
    (void)fprintf(stderr,"%s: DEBUG Attempting OpenMP Parallelization...\n");
#pragma omp parallel for
#endif /* not OMP */
    /* Process all variables in current file */ 
    for(idx=0;idx<nbr_var_prc;idx++){
      if(dbg_lvl > 0) (void)fprintf(stderr,"%s, ",var_prc[idx]->nm);
      if(dbg_lvl > 0) (void)fflush(stderr);

      /* Allocate and, if necesssary, initialize accumulation space for all processed variables */ 
      var_prc_out[idx]->sz=var_prc[idx]->sz;
      if((var_prc_out[idx]->tally=var_prc[idx]->tally=(long *)malloc(var_prc_out[idx]->sz*sizeof(long))) == NULL){
	(void)fprintf(stdout,"%s: ERROR Unable to malloc() %ld*%ld bytes for tally buffer for variable %s in main()\n",prg_nm_get(),var_prc_out[idx]->sz,(long)sizeof(long),var_prc_out[idx]->nm);
	exit(EXIT_FAILURE); 
      } /* end if */ 
      (void)zero_long(var_prc_out[idx]->sz,var_prc_out[idx]->tally);
      if((var_prc_out[idx]->val.vp=(void *)malloc(var_prc_out[idx]->sz*nctypelen(var_prc_out[idx]->type))) == NULL){
	(void)fprintf(stdout,"%s: ERROR Unable to malloc() %ld*%d bytes for value buffer for variable %s in main()\n",prg_nm_get(),var_prc_out[idx]->sz,nctypelen(var_prc_out[idx]->type),var_prc_out[idx]->nm);
	exit(EXIT_FAILURE); 
      } /* end if */ 
      (void)var_zero(var_prc_out[idx]->type,var_prc_out[idx]->sz,var_prc_out[idx]->val);
      
      (void)var_refresh(in_id,var_prc[idx]);
      /* Retrieve variable from disk into memory */ 
      (void)var_get(in_id,var_prc[idx]);
      if(msk_nm != NULL && (!var_prc[idx]->is_crd_var || WGT_MSK_CRD_VAR)){
	msk_out=var_conform_dim(var_prc[idx],msk,msk_out,MUST_CONFORM,&DO_CONFORM_MSK);
	/* If msk and var did not conform then do not mask var! */ 
	if(DO_CONFORM_MSK){
	  msk_out=var_conform_type(var_prc[idx]->type,msk_out);
	  
	  /* mss_val for var_prc has been overwritten in var_refresh() */ 
	  if(!var_prc[idx]->has_mss_val){
	    var_prc[idx]->has_mss_val=True;
	    var_prc[idx]->mss_val=mss_val_mk(var_prc[idx]->type);
	  } /* end if */
	  
	  /* Mask by changing variable to missing value where condition is false */ 
	  (void)var_mask(var_prc[idx]->type,var_prc[idx]->sz,var_prc[idx]->has_mss_val,var_prc[idx]->mss_val,msk_val,op_type,msk_out->val,var_prc[idx]->val);
	} /* end if */
      } /* end if */
      if(wgt_nm != NULL && (!var_prc[idx]->is_crd_var || WGT_MSK_CRD_VAR)){
	/* fxm: var_conform_dim() has a bug where it does not allocate a tally array
	 for weights that do already conform to var_prc. TODO #114. */ 
	wgt_out=var_conform_dim(var_prc[idx],wgt,wgt_out,MUST_CONFORM,&DO_CONFORM_WGT);
	wgt_out=var_conform_type(var_prc[idx]->type,wgt_out);
	/* Weight variable by taking product of weight and variable */ 
	(void)var_multiply(var_prc[idx]->type,var_prc[idx]->sz,var_prc[idx]->has_mss_val,var_prc[idx]->mss_val,wgt_out->val,var_prc[idx]->val);
      } /* end if */
      /* Copy (masked) (weighted) values from var_prc to var_prc_out */ 
      (void)memcpy((void *)(var_prc_out[idx]->val.vp),(void *)(var_prc[idx]->val.vp),var_prc_out[idx]->sz*nctypelen(var_prc_out[idx]->type));
      /* Average variable over specified dimensions (tally array is set here) */
      var_prc_out[idx]=var_avg(var_prc_out[idx],dmn_avg,nbr_dmn_avg);
      /* var_prc_out[idx]->val holds numerator of averaging expression documented in NCO User's Guide 
	 Denominator is also tricky due to sundry normalization options 
	 These logical switches are VERY tricky---be careful modifying them */
      if(NRM_BY_DNM && wgt_nm != NULL && (!var_prc[idx]->is_crd_var || WGT_MSK_CRD_VAR)){
	/* Duplicate wgt_out as wgt_avg so that wgt_out is not contaminated by any
	   averaging operation and may be reused on next variable.
	   Be sure to free wgt_avg after each use */
	wgt_avg=var_dup(wgt_out);

	if(var_prc[idx]->has_mss_val){
	  double mss_val_dbl=double_CEWI;
	  /* The denominator must be set to the missing value at all locations 
	     where the variable is the missing value.
	     If this is accomplished by setting the weight to the missing value 
	     wherever the variable is the missing value, then the weight must not
	     be reused by the next variable (which might conform but have 
	     missing values in different locations). 
	     This is one of the reasons to copy wgt_out into the disposable wgt_avg 
	     for each new variable. */
	  /* First make sure wgt_avg has the same missing value as the variable */
	  (void)mss_val_cp(var_prc[idx],wgt_avg);
	  /* Copy the missing value into a double precision variable */
	  switch(wgt_avg->type){
	  case NC_FLOAT: mss_val_dbl=wgt_avg->mss_val.fp[0]; break; 
	  case NC_DOUBLE: mss_val_dbl=wgt_avg->mss_val.dp[0]; break; 
	  case NC_LONG: mss_val_dbl=wgt_avg->mss_val.lp[0]; break;
	  case NC_SHORT: mss_val_dbl=wgt_avg->mss_val.sp[0]; break;
	  case NC_CHAR: mss_val_dbl=wgt_avg->mss_val.cp[0]; break;
	  case NC_BYTE: mss_val_dbl=wgt_avg->mss_val.bp[0]; break;
	  } /* end switch */
	  /* Second mask wgt_avg where the variable is the missing value */
	  (void)var_mask(wgt_avg->type,wgt_avg->sz,var_prc[idx]->has_mss_val,var_prc[idx]->mss_val,mss_val_dbl,nc_op_ne,var_prc[idx]->val,wgt_avg->val);
	} /* endif weight must be checked for missing values */ 

	if(msk_nm != NULL && DO_CONFORM_MSK){
	  /* Must mask weight in same fashion as variable was masked */ 
	  /* If msk and var did not conform then do not mask wgt */ 
	  /* Ensure wgt_avg has a missing value */ 
	  if(!wgt_avg->has_mss_val){
	    wgt_avg->has_mss_val=True;
	    wgt_avg->mss_val=mss_val_mk(wgt_avg->type);
	  } /* end if */
	  /* Mask by changing weight to missing value where condition is false */ 
	  (void)var_mask(wgt_avg->type,wgt_avg->sz,wgt_avg->has_mss_val,wgt_avg->mss_val,msk_val,op_type,msk_out->val,wgt_avg->val);
	} /* endif weight must be masked */

	/* fxm: temporary kludge to make sure weight has tally space.
	   wgt_avg may occasionally lack a valid tally array in ncwa because
	   it is created, sometimes, before the tally array for var_prc_out[idx] is 
	   created, and thus the var_dup() call in var_conform_dim() does not copy
	   a tally array into wgt_avg. See related note about this above. TODO #114.*/ 
	if((wgt_avg->tally=(long *)realloc(wgt_avg->tally,wgt_avg->sz*sizeof(long))) == NULL){
	  (void)fprintf(stdout,"%s: ERROR Unable to realloc() %ld*%ld bytes for tally buffer for weight %s in main()\n",prg_nm_get(),wgt_avg->sz,(long)sizeof(long),wgt_avg->nm);
	  exit(EXIT_FAILURE); 
	} /* end if */ 
	/* Average weight over specified dimensions (tally array is set here) */ 
	wgt_avg=var_avg(wgt_avg,dmn_avg,nbr_dmn_avg);
	if(MULTIPLY_BY_TALLY){
	  /* Currently this is not implemented */ 
	  /* Multiply numerator (weighted sum of variable) by tally 
	     We deviously accomplish this by dividing the denominator by tally */ 
	  (void)var_normalize(wgt_avg->type,wgt_avg->sz,wgt_avg->has_mss_val,wgt_avg->mss_val,wgt_avg->tally,wgt_avg->val);
	} /* endif */ 
	/* Divide numerator by denominator */ 
	/* Diagnose problem #116 before it core dumps */ 
	if(var_prc_out[idx]->sz == 1 && var_prc_out[idx]->type == NC_LONG && var_prc_out[idx]->val.lp[0] == 0){
	  (void)fprintf(stdout,"%s: ERROR Denominator weight = 0. Problem described in TODO #116\n%s: HINT A possible workaround is to remove variable \"%s\" from output file using \"%s -x -v %s ...\"\n%s: Expecting core dump...now!\n",prg_nm,prg_nm,var_prc_out[idx]->nm,prg_nm,var_prc_out[idx]->nm,prg_nm);
	} /* end if */ 
	/* This constructs default weighted average */ 
	(void)var_divide(var_prc_out[idx]->type,var_prc_out[idx]->sz,var_prc_out[idx]->has_mss_val,var_prc_out[idx]->mss_val,wgt_avg->val,var_prc_out[idx]->val);
	/* Free wgt_avg, but keep wgt_out, after each use */
	if(wgt_avg != NULL) wgt_avg=var_free(wgt_avg);
      }else if(NRM_BY_DNM){
	/* Normalize by tally only and forget about weights */ 
	(void)var_normalize(var_prc_out[idx]->type,var_prc_out[idx]->sz,var_prc_out[idx]->has_mss_val,var_prc_out[idx]->mss_val,var_prc_out[idx]->tally,var_prc_out[idx]->val);
      }else if(!NRM_BY_DNM){
	/* No normalization required, we are done */ 
	;
      }else{
	(void)fprintf(stdout,"%s: ERROR Unforeseen logical branch in main()\n",prg_nm);
	exit(EXIT_FAILURE);
      } /* end if */
      /* Free tallying buffer */
      (void)free(var_prc_out[idx]->tally); var_prc_out[idx]->tally=NULL;
      /* Free current input buffer */
      (void)free(var_prc[idx]->val.vp); var_prc[idx]->val.vp=NULL;

      /* Copy average to output file and free averaging buffer */ 
      if(var_prc_out[idx]->nbr_dim == 0){
	(void)ncvarput1(out_id,var_prc_out[idx]->id,var_prc_out[idx]->srt,var_prc_out[idx]->val.vp);
      }else{ /* end if variable is scalar */ 
	(void)ncvarput(out_id,var_prc_out[idx]->id,var_prc_out[idx]->srt,var_prc_out[idx]->cnt,var_prc_out[idx]->val.vp);
      } /* end if variable is array */ 
      (void)free(var_prc_out[idx]->val.vp);
      var_prc_out[idx]->val.vp=NULL;
      
    } /* end loop over idx */
#ifdef OMP /* OpenMP */
    (void)fprintf(stderr,"%s: DEBUG End of OpenMP Parallelization...\n");
#endif /* not OMP */
    
    /* Free weights and masks */ 
    if(wgt != NULL) wgt=var_free(wgt);
    if(wgt_out != NULL) wgt_out=var_free(wgt_out);
    if(wgt_avg != NULL) wgt_avg=var_free(wgt_avg);
    if(msk != NULL) msk=var_free(msk);
    if(msk_out != NULL) msk_out=var_free(msk_out);

    if(dbg_lvl > 0) (void)fprintf(stderr,"\n");
    
    /* Close input netCDF file */ 
    ncclose(in_id);
    
    /* Remove local copy of file */ 
    if(FILE_RETRIEVED_FROM_REMOTE_LOCATION && REMOVE_REMOTE_FILES_AFTER_PROCESSING) (void)fl_rm(fl_in);

  } /* end loop over idx_fl */
  
  /* Close output file and move it from temporary to permanent location */ 
  (void)fl_out_cls(fl_out,fl_out_tmp,out_id);
  
  Exit_gracefully();
  return EXIT_SUCCESS;
} /* end main() */
