#!/usr/bin/env perl
# Shebang line above may have to be set explicitly to /usr/local/bin/perl
# on ESMF when running in queue. Otherwise it may pick up older perl

# $Header: /data/zender/nco_20150216/nco/bm/nco_bm.pl,v 1.124 2006-05-01 03:51:25 zender Exp $

# Usage: usage(), below, has more information
# ~/nco/bm/nco_bm.pl # Tests all operators
# ~/nco/bm/nco_bm.pl ncra # Test one operator
# ~/nco/bm/nco_bm.pl --thr_nbr=2 --regress --udpreport # Test OpenMP
# ~/nco/bm/nco_bm.pl --mpi_prc=2 --regress --udpreport # Test MPI
# ~/nco/bm/nco_bm.pl --dap --regress --udpreport # Test OPeNDAP on sand
# ~/nco/bm/nco_bm.pl --dap=http://soot.ess.uci.edu/cgi-bin/dods/nph-dods/dodsdata --regress --udpreport # Test OPeNDAP on soot
# scp ~/nco/bm/nco_bm.pl esmf.ess.uci.edu:nco/bm

# NB: when adding debugging messages, use dgb_msg(nbr,message);, where
# nbr = debug level at which the message should be emitted
# message = valid Perl string to print. ie: "just before foo, \$blah = $blah"
# Subroutine prefixes message with DEBUG[#] and adds newline

require 5.6.1 or die "This script requires Perl version >= 5.6.1, stopped";
use Cwd 'abs_path';
use English; # WCS96 p. 403 makes incomprehensible Perl errors sort of comprehensible
use Getopt::Long; # GNU-style getopt #qw(:config no_ignore_case bundling);
use strict; # Protect all namespaces

# 'use' statements for NCO_rgr.pm and NCO_benchmarks.pm are later 
# to minimize chances shared variables will be contaminated.
# Unnecessary globals should continue to be hunted down and killed off

# Declare vars for strict
use vars qw(
	    $aix_mpi_nvr_prfx $aix_mpi_sgl_nvr_prfx $arg_nbr $bch_flg $bm
	    @bm_cmd_ary $bm_dir $caseid $cmd_ln $dbg_lvl $dodap $dot_fmt $dot_nbr
	    $dot_nbr_min $dot_sng $dsc_fmt $dsc_lng_max $dsc_sng $dta_dir
	    $dust_usr %failure $fl_cnt @fl_cr8_dat $fl_fmt $fl_pth @fl_tmg
	    $foo1_fl $foo2_fl $foo_avg_fl $foo_fl $foo_T42_fl $foo_tst $foo_x_fl
	    $foo_xy_fl $foo_xymyx_fl $foo_y_fl $foo_yx_fl $gnu_cut $hiresfound
	    @ifls $itmp $localhostname $md5 $md5found %MD5_tbl $mpi_fke $mpi_prc
	    $mpi_prfx $MY_BIN_DIR $nco_D_flg %NCO_RC $nco_vrsn_sng $ncwa_scl_tst $notbodi
	    $nsr_xpc $NUM_FLS $nvr_my_bin_dir $omp_flg $opr_fmt $opr_lng_max
	    @opr_lst @opr_lst_all @opr_lst_mpi $opr_nm $opr_rgr_mpi $opr_sng_mpi
	    $orig_outfile $os_sng $outfile $prfxd $prg_nm $prsrv_fl
	    $pth_rmt_scp_tst $pwd $que $rcd %real_tme $result $rgr $server_ip
	    $server_name $server_port $sock $spc_fmt $spc_nbr $spc_nbr_min
	    $spc_sng $srvr_sde %subbenchmarks %success %sym_link @sys_tim_arr
	    $sys_time %sys_tme $thr_nbr $timed $timestamp $tmp $tmr_app
	    %totbenchmarks @tst_cmd $tst_fl_cr8 $tst_fmt $tst_id_sng $tst_idx
	    %tst_nbr $tw_prt_bm  $udp_reprt $USER $usg %usr_tme %wc_tbl
	    $wnt_log $xdta_pth $xpt_dsc
	    $prefix
	    );
#$udp_reprt
# Initializations
# Re-constitute commandline
$prg_nm=$0; # $0 is program name Camel p. 136
$cmd_ln = "$0 "; $arg_nbr = @ARGV;
for (my $i=0; $i<$arg_nbr; $i++){ $cmd_ln .= "$ARGV[$i] ";}

# Set defaults for command line arguments
$aix_mpi_nvr_prfx = "";
$aix_mpi_sgl_nvr_prfx = "";
$bch_flg=0; # [flg] Batch behavior
$dbg_lvl = 0; # [enm] Print tests during execution for debugging
$nco_D_flg = "";
my $nvr_data=$ENV{'DATA'} ? $ENV{'DATA'} : '';
my $nvr_home=$ENV{'HOME'} ? $ENV{'HOME'} : '';
my $nvr_host=$ENV{'HOST'} ? $ENV{'HOST'} : '';
$USER = $ENV{'USER'};
$pwd = `pwd`; chomp $pwd;
$fl_pth = '';
$que = 0;
$thr_nbr = 0; # If not zero, pass explicit threading argument
$tst_fl_cr8 = "0";
$udp_reprt = 0;
$usg = 0;
$mpi_fke = 0;
$wnt_log = 0;
$md5 = 0;
$mpi_prc = 0; # by default, don't want no steekin MPI
$mpi_prfx = "";
$timestamp = `date -u "+%x %R"`; chomp $timestamp;
$dodap = "FALSE"; # Unless redefined by the command line, it does not get reset
$fl_cnt = 32; # nbr of files to process (reduced to 4 if using remote/dods files
$pth_rmt_scp_tst='dust.ess.uci.edu:/var/www/html/dodsdata';
$dust_usr = "";
$xdta_pth = ''; # explicit data path that user can set from cmdline; more powerful than $dta_dir
$os_sng = "";
$nco_vrsn_sng = "";
$gnu_cut = 1;
$fl_fmt = "classic"; # file format for wirting
$caseid = "";
$srvr_sde = "SSNOTSET";
$prsrv_fl = 1;
$prefix = "";

# other inits
$localhostname = `hostname`; chomp $localhostname;
$notbodi = 0; # specific for hjm's puny laptop
my $prfxd = 0;
if ($localhostname !~ "bodi") {$notbodi = 1} # spare the poor laptop
$ARGV = @ARGV;

my $iosockfound;

BEGIN{
    unshift @INC,$ENV{'HOME'}.'/nco/bm'; # Location of NCO_rgr.pm, NCO_bm.pm
    unshift @INC,'.';
} # end BEGIN

BEGIN {eval "use IO::Socket"; $iosockfound = $@ ? 0 : 1}
#$iosockfound = 0;  # uncomment to simulate not found
if ($iosockfound == 0) {
    print "\nOoops! IO::Socket module not found - continuing with no udp logging.\n\n";
} else {
    print "\tIO::Socket  ... found.\n";
}

$rcd=Getopt::Long::Configure('no_ignore_case'); # Turn on case-sensitivity
&GetOptions(
	    'bch_flg!'     => \$bch_flg,    # [flg] Batch behavior
	    'benchmark'    => \$bm,         # Run benchmarks
	    'bm'           => \$bm,         # Run benchmarks
	    'dbg_lvl=i'    => \$dbg_lvl,    # Debug level - # is now optional
	    'debug=i'      => \$dbg_lvl,    # Debug level
	    'dods:s'       => \$dodap,      # Optional string is URL to DAP data
	    'dap:s'        => \$dodap,      # Optional string is URL to DAP data
	    'fl_fmt=s'     => \$fl_fmt,     # Output format for writing netcdf files; one of:
	    # classic,64bit,netcdf4,netcdf4_classic
	    'opendap:s'    => \$dodap,      # Optional string is URL to DAP data
	    'dust_user=s'  => \$dust_usr,   #  #
	    'h'            => \$usg,        # Explain how to use this thang
	    'help'         => \$usg,        # Explain how to use this thang
	    'log'          => \$wnt_log,    # Log output
	    'mpi_prc=i'    => \$mpi_prc,    # Number MPI processes to use
	    'mpi_fake'	   => \$mpi_fke,    # Run SMP version of MPI code
	    'fake_mpi'	   => \$mpi_fke,    # Run SMP version of MPI code
	    'queue'        => \$que,        # Bypass all interactive stuff
	    'pth_rmt_scp_tst' => \$pth_rmt_scp_tst, # [drc] Path to scp regression test file
	    'regress'      => \$rgr,        # Perform regression tests
	    'rgr'          => \$rgr,        # Perform regression tests
	    'scaling'      => \$ncwa_scl_tst, # do scaling test on ncwa bench to see how dif var sizes change time.
	    'serverside:s' => \$srvr_sde,   # do benchmarks on server side (w/ ssdwrap)
	    'test_files=s' => \$tst_fl_cr8, # Create test files "134" does 1,3,4
	    'tst_fl=s'     => \$tst_fl_cr8, # Create test files "134" does 1,3,4
	    'thr_nbr=i'    => \$thr_nbr,    # Number of OMP threads to use
	    'udpreport'    => \$udp_reprt,    # punt the timing data back to udpserver on sand
	    'usage'        => \$usg,        # Explain how to use this thang
	    'caseid=s'     => \$caseid,     # short string to tag test dir and batch queue
	    'xdata=s'	=> \$xdta_pth,   # explicit data path set from cmdline
	    'xpt_dsc=s'    => \$xpt_dsc,    # Long string to describe experiment
#BROKEN - FXM hjm	'md5'          => \$md5,        # requests md5 checksumming results (longer but more exacting)
	    );

# kill all md5 stuff asap
# BEGIN {eval "use Digest::MD5"; $md5found = $@ ? 0 : 1}
# # $md5found = 0;  # uncomment to simulate no MD5
# if ($md5 == 1) {
# 	if ($md5found == 0) {print "\nOoops! Digest::MD5 module not found - continuing with simpler error checking\n\n" ;	}
# 	else                {print "\tDigest::MD5 ... found.\n";}
# } else {	print "\tMD5 NOT requested; continuing with ncks checking of single values.\n";}

$NUM_FLS = 4; # max number of files in the file creation series

if ($srvr_sde eq "") {$srvr_sde = 1;}

my $lcl_vars =  "\n\t \$cmd_ln = $cmd_ln\n";
$lcl_vars .=    "\t \$caseid = $caseid\n";
$lcl_vars .=    "\t \$rgr = $rgr\n" ;
$lcl_vars .=    "\t \$bm = $bm\n" ;
$lcl_vars .=    "\t \$bch_flg = $bch_flg\n";
$lcl_vars .=    "\t \$srvr_sde = [$srvr_sde]\n";
$lcl_vars .=    "\t \$nvr_data = $nvr_data\n";
$lcl_vars .=    "\t \$nvr_home = $nvr_home\n";
$lcl_vars .=    "\t \$nvr_my_bin_dir = $nvr_my_bin_dir\n";
$lcl_vars .=    "\t \$nvr_my_bin_dir = $nvr_my_bin_dir\n";
$lcl_vars .=    "\t \@ENV = @ENV\n";
$lcl_vars .=    "\t \@INC:\n";
foreach my $subpth (@INC) {$lcl_vars .= "\t   $subpth\n"}
dbg_msg(1,$lcl_vars); # spit the whole thing out.

if ($ARGV == 0) {	NCO_bm::usage();}

# Test file format
if ($fl_fmt eq "64bit" || $fl_fmt eq "netcdf4" || $fl_fmt eq "netcdf4_classic") {
    $fl_fmt = "--fl_fmt=" . $fl_fmt;
    dbg_msg(1,"File format set to [$fl_fmt]");
}elsif ($fl_fmt eq "classic"){
    $fl_fmt = " ";
} else {
    die "Your file format spec (--fl_fmt) isn't correct; it has to be one of:\n  classic,  64bit, netcdf4, or netcdf4_classic\nPlease choose one of these and repeat.\n\n";
}

# Read values from ~/.ncorc, if present, into global hash
if (-e "$ENV{'HOME'}/.ncorc" && -r "$ENV{'HOME'}/.ncorc" && !-z "$ENV{'HOME'}/.ncorc" ){ # if exists, readable, nonzero
    # read it it into the %NCO_RC hash (file format is "name" => "value"
    open(RC, "$ENV{'HOME'}/.ncorc") or die "Can't open user's ~/.ncorc file for reading.\n";
    my $ln_cnt = 0;
    while (<RC>){
	$ln_cnt++;
	if ($_ !~ /^#/) { # ignore comments
	    my $N = my @L = split('=');
	    chomp $L[1]; # if should have a \n on it so get rid of it.
	    if ($N != 2) {print "ERR: typo in ~/.ncorc file on line $ln_cnt.\nFormat is: 'Name=value'\nIgnoring error for now\n";}
	    $NCO_RC{$L[0]} = $L[1];
	    # print "DEBUG: NCO_RC{$L[0]} = $L[1] \n";
	}
    }
}

# Allow UDP reporting if perl has found IO::Socket and OK by ~/.ncorc
# If user sets it to 'no' then this test fails
if ($iosockfound && $NCO_RC{"udp_report"} =~ "yes"){$udp_reprt = 1;}

# this next commented block will soon disappear as we DO need to make
# BOTH benchmarks and regressions  run serverside
# check that if serverside has been requested, also benchmarks have been or emit errors
# if ($srvr_sde ne "SSNOTSET" && !$bm) {
# 	print "\nWARN: The only option allowed with '--serverside' is '--benchmark' - continue? [Ny] ";
# 	my $tmp = <STDIN>;
# 	if ($tmp !~ /[Yy]/) {die "OK - try again without the serverside option\n";}
# 	#else {$bm = 1;} #set $bm and continue
# }
# can't do both serverside and DAP - check that the options don't conflict
if ($srvr_sde ne "SSNOTSET" && $dodap ne "FALSE") {
    die "\nERR: Can't combine '--serverside' and '--dap' - choose one or the other.\n";
}
# if testing DAP, use $case_id to specify separate dir, so don't mess with current files
if ($dodap ne "FALSE") {$caseid = "DAP_DIR"; print "\nDAP_DIR set as caseid. \n";}

# set up some host-specific id's
$os_sng = `uname`; chomp $os_sng;

# check for user trying to run benchmarks on the UCI esmf interactive node:
if ($nvr_host =~ /esmf04m/ && $bm) {
    print "\n\nAre you sure you want to run the NCO benchmarks on the interactive node?\n";
    print "Enter 'y' to continue.  Anything else cancels. [Default No]: ";
    my $tmp = <STDIN>; chomp $tmp;
    if ($tmp !~ /[yY]/) {
	die "OK - Quitting now.  To run the benchmarks under AIX, modify <NCO_ROOT>/bm/nco_bm.sh (a POE script) \nand 'llsubmit' that script to the loadleveler.\n";
    }
}
if ($os_sng =~ /AIX/ && $rgr && $mpi_prc > 0) {
    # set env vars for MPI to run on AIX (not just esmf)
    $aix_mpi_nvr_prfx = "MP_PROCS=$mpi_prc MP_EUILIB='us' MP_NODES='1'  MP_TASKS_PER_NODE=$mpi_prc MP_RMPOOL='1' ";
    $aix_mpi_sgl_nvr_prfx = " MP_PROCS=1  MP_RMPOOL=1 ";
}

# Check for bad cut on MacOSX
if ($os_sng =~ /Darwin/){
    print "\nTesting for GNU cut on Darwin..\n";
    $tmp = `cut --version 2>&1 | grep 'Free Software Foundation'`;
    if ($tmp !~ /Free/) {
	print << 'BADCUT';
	
      WARN: You appear to be running this on MacOSX with the default wacko
	  'cut'. This will cause some of the regressions and benchmarks to fail as
	  well as contribute to the overall negative kharma of the universe.
	  If you want life to be better, consider installing the GNU coreutils
	  which will provide an acceptable 'cut'.
	  
	  Hit <Enter> to acknowledge your miserable state of cut kharma.
BADCUT
        $tmp = <STDIN>;
	$gnu_cut = 0;
    }
}

# do $mpi_prc and $mpi_fke conflict?
if ($mpi_prc > 0 && $mpi_fke) {
    die "\nERR: You requested both an MPI run (--mpi_prc) as well as a FAKE MPI run (--mpi_fake)\n\tMake up your mind!\n\n";
}

# if wanted an MPI run, figure out what MPI variant and check to see that the right MPI daemon is running
# FXM - hjm still need to figure this out for AIX.

if ($mpi_prc > 0 && $os_sng =~ /inux/) {
    my $lam_ok = 0;
    my $mpich_ok = 0;
    my $myhostname_ip = "";
    my $myif_ip = "";
    # have to check that hostname matches IP number in /etc/hosts for mpd to allow connections correctly;
    # maybe for LAM as well
    dbg_msg(2,"Determining IP and hostname info.\nMay timeout if /etc/hosts, ifconfig, and hostname disagree.");
    $myhostname_ip = `ping -c1 \`hostname\` |grep PING |cut -d' ' -f 3|cut -d'(' -f2 |cut -d')' -f1`; chomp $myhostname_ip;
    $myif_ip = `/sbin/ifconfig |grep 'inet addr' |cut -d':' -f2 |cut -d' ' -f1 |grep -v '127.0.0.1' `; chomp $myif_ip;
    dbg_msg(1,"\$localhostname = $localhostname\n\t     \$myhostname_ip = $myhostname_ip\n\t           \$myif_ip = $myif_ip ");
    if ($myif_ip ne $myhostname_ip) {
	print "WARN: Your interface IP # ($myif_ip) is different than your \nhostname IP number ($myhostname_ip) that is set in /etc/hosts.\nThe mpd (and maybe lamd) may timeout and fail unless they agree.\n"
	} else {dbg_msg(1,"Good!  Your interface IP # ($myif_ip) equals your \nhostname IP number ($myhostname_ip). mpd will be happy!")}
	
	if (-e '/etc/lam/conf.lamd' && -r '/etc/lam/conf.lamd') {# if you've got a conf.lamd, maybe you're runnning LAM?
								     my $lamd_usr = `ps aux |grep lamd | grep -v grep | cut -d' ' -f1`;  chomp $lamd_usr; $lamd_usr =~ s/\n/ /g;
								     dbg_msg(2,"Testing for a running lamd:USER = [$ENV{'USER'}] and \$lamd_usr = [$lamd_usr]");
								     if ( $lamd_usr !~ /$ENV{'USER'}/ )  {
									 print "\nWARN: You might be trying to run LAM_MPI without a running lamd.\nIf the run fails, try running 'lamboot'\n\n";
								     } else {
									 dbg_msg(1,"OK! You seem to be using LAM_MPI and at least one lamd seems to be owned by you");
									 $lam_ok = 1;
								     }
								 }
    my $mpd_usr = `ps aux |grep mpd | grep -v grep | cut -d' ' -f1`;
    $mpd_usr =~ s/\n/ /;
    if ($mpd_usr ne "") { # you might be using the mpich MPI system
#		my $mpd_usr = `ps aux |grep mpd | grep -v grep | cut -d' ' -f1`;
#		$mpd_usr =~ s/\n/ /;
#		print "\n\n__ $mpd_usr __\n\n";
	dbg_msg(2,"Testing for a correctly owned running mpd: USER = [$ENV{'USER'}] and \$mpd_usr = [$mpd_usr]");
	if ( $mpd_usr !~ /$ENV{'USER'}/ )  {
	    print "\nWARN: You might be trying to run MPICH without a running mpd.\nIf the run fails, and I can't start an mpd for you, try running 'mpd &' manually\n\n";
	    # try to start it automatically?
	} else {
	    dbg_msg(1,"OK! You seem to be using MPICH and at least one mpd seems to be owned by you.");
	    $mpich_ok = 1;
	}
    }
    
    if (!$lam_ok && !$mpich_ok) {
	print "\nWARN: you asked for an MPI run (--mpi_prc=$mpi_prc) but you don't seem to be running either LAM-MPI or MPICH (no running lamd or mpd).\nIf the run fails, you might try running one of those 2 MPI systems.\n";
    }
}

# Any irrationally exuberant values?
if ($mpi_prc > 16) {die "\nThe '--mpi_prc' value was set to an irrationally exuberant [$mpi_prc].  Try a lower value\n ";}
if ($thr_nbr > 16) {die "\nThe '--thr_nbr' value was set to an irrationally exuberant [$thr_nbr].  Try a lower value\n ";}
if (length($caseid) > 80) {die "\nThe caseid string is > 80 characters - please reduce it to less than 80 chars.\nIt's used to create file and directory names, so it has to be relatively short\n";}

# Slurp in data for the checksum hashes
if ($md5 == 1) {	do "nco_bm_md5wc_tbl.pl" or die "Can't find the validation data (nco_bm_md5wc_tbl.pl).\n";}

$nco_D_flg = "-D $dbg_lvl";
dbg_msg(1,"WARN: Using the --debug flag set to greater than 0 will cause the NCO\n  commandline -D flag to be set to the corresponding number as well, which will cause\n  some of the tests to fail, as the output will be different also.\n  It is currently set to \$nco_D_flg = $nco_D_flg");

# Determine where $DATA should be, prompt user if necessary
if ($xdta_pth eq '') {
    dbg_msg(2, "$prg_nm: Calling set_dat_dir()");
    set_dat_dir($caseid); # Set $dta_dir
} else { #validate $xdta_pth
    if (-e $xdta_pth && -w $xdta_pth){
	dbg_msg(1,"User-specified DATA path ($xdta_pth) exists and is writable.");
	$dta_dir = $xdta_pth; # and assign it to the previously coded variable.
    } else {
	die "FATAL(bm): The directory you specified on the commandline ($xdta_pth) doesn't exist or isn't writable by you.\n";
    }
}

# Set $fl_pth to reasonable defalt
$fl_pth = "$dta_dir";

# Initialize & set up some variables
dbg_msg(3, "Calling initialize().");
initialize($bch_flg,$dbg_lvl);

# Use variables for file names in regressions; some of these could be collapsed into
# fewer ones, no doubt, but keep them separate until whole shebang starts working correctly
# $outfile       = "$dta_dir/foo.nc"; # replaces outfile in tests, typically 'foo.nc'
# $orig_outfile  = "$dta_dir/foo.nc";
# $foo_fl        = "$dta_dir/foo";
# $foo_avg_fl    = "$dta_dir/foo_avg.nc";
# $foo_tst       = "$dta_dir/foo.tst";
# $foo1_fl       = "$dta_dir/foo1.nc";
# $foo2_fl       = "$dta_dir/foo2.nc";
# $foo_x_fl      = "$dta_dir/foo_x.nc";
# $foo_y_fl      = "$dta_dir/foo_y.nc";
# $foo_xy_fl     = "$dta_dir/foo_xy.nc";
# $foo_yx_fl     = "$dta_dir/foo_yx.nc";
# $foo_xymyx_fl  = "$dta_dir/foo_xymyx.nc";
# $foo_T42_fl    = "$dta_dir/foo_T42.nc";

# NCO_bm defined here to allow above variables to be defined for later use
use NCO_bm; # module that contains most of the functions.

# the real udping server
$server_name = "sand.ess.uci.edu";
$server_ip = "128.200.14.132";
$server_port = 29659;

if ($usg) { usage()};   # dump usage blurb
if (0) { tst_hirez(); } # tests the hires timer - needs explict code mod to do this

if ($iosockfound) {
    $sock = IO::Socket::INET->new (
				   Proto    => 'udp',
				   PeerAddr => $server_ip,
				   PeerPort => $server_port
				   ) or print "\nCan't get the socket - continuing anyway.\n"; # if off network..
} else {$udp_reprt = 0;}

if ($wnt_log) {
    open(LOG, ">$bm_dir/nctest.log") or die "\nUnable to open log file '$bm_dir/nctest.log' - check permissions on it\nor the directory you are in.\n stopped";
}

# Pass explicit threading argument
if ($thr_nbr > 0){$omp_flg="--thr_nbr=$thr_nbr";} else {$omp_flg=' ';}

# does dodap require that we ignore both MPI and OpenMP?  Let's leave it in for now.
# If dodap is not set then test with local files
# If dodap is set and string is NULL, then test with OPeNDAP files on sand.ess.uci.edu
# If string is NOT NULL, use URL to grab files

dbg_msg(4, "before dodap assignment, \$fl_pth = $fl_pth, \$dodap = $dodap");
# $dodap asks for and if defined, carries, the URL that's inserted in the '-p' place in nco cmdlines
if ($dodap ne "FALSE") {
    if ($dodap eq "") {
	$fl_pth = "http://sand.ess.uci.edu/cgi-bin/dods/nph-dods/dodsdata";
	$fl_cnt = 4;
    } elsif ($dodap =~ /http/) {
	$fl_pth = $dodap;
	$fl_cnt = 4;
    } else {
	die "\nThe URL specified with the --dods option:\n $dodap \ndoesn't look like a valid URL.\nTry again\n\n";
    }
}
dbg_msg(3, "after dodap assignment, \$fl_pth = $fl_pth, \$dodap = $dodap");

# Initialize & set up some variables
#if($dbg_lvl > 0){printf ("$prg_nm: Calling initialize()...\n");}
#initialize($bch_flg,$dbg_lvl);

# Grok /usr/bin/time, as in shell scripts
if (-e "/usr/bin/time" && -x "/usr/bin/time") {
    $tmr_app = "/usr/bin/time ";
    if (`uname` =~ "inux"){$tmr_app.="-p ";}
} else { # just use whatever the shell thinks is the time app
    $tmr_app = "time "; # bash builtin or other 'time'-like application (AIX)
} # endif time

if ($dbg_lvl > 1) {
    print "\nAbout to begin requested tests; waiting for keypress to proceed.\n";
    my $tmp = <STDIN>;
}


# Regression tests
if ($rgr){
    use NCO_rgr; # module that contains perform_tests()
    NCO_rgr::perform_tests();
    NCO_bm::smrz_rgr_rslt();
} # endif rgr

# Start real benchmark tests
# Test if necessary files are available - if so, may skip creation tests

# initialize filenames
if( $tst_fl_cr8 ne "0"  ||( $bm && $dodap eq "FALSE")){
    if($dbg_lvl > 1){printf ("\n$prg_nm: Calling fl_cr8_dat_init()...\n");}
    NCO_bm::fl_cr8_dat_init(@fl_cr8_dat); # Initialize data strings & timing array for files
}

# Check if files have already been created
# If so, skip file creation if not requested
if ($bm && $tst_fl_cr8 eq "0" && $dodap eq "FALSE" )  {
    if ($dbg_lvl> 0){print "\nINFO: File creation tests:\n";}
    for (my $i = 0; $i < $NUM_FLS; $i++) {
	my $fl = $fl_cr8_dat[$i][2] . ".nc"; # file root name stored in $fl_cr8_dat[$i][2]
	print "testing for $dta_dir/$fl...\n";
	if (-e "$dta_dir/$fl" && -r "$dta_dir/$fl") {
	    if ($dbg_lvl> 0){printf ("%50s exists - can skip creation\n", $dta_dir . "/" . $fl);}
	} else {
	    my $e = $i+1;
	    $tst_fl_cr8 .= "$e";
	}
    }
}

#	print "DEBUG:  in nco_bm.pl, \$fl_tmg[1][0] = $fl_tmg[1][0] & \$NUM_FLS = $NUM_FLS\n";

# file creation tests
if ($tst_fl_cr8 ne "0" || $srvr_sde ne "SSNOTSET"){
    my $fc = 0; $prsrv_fl = 1;
    if ($tst_fl_cr8 =~ "[Aa]") { $tst_fl_cr8 = "1234";}
    if ($tst_fl_cr8 =~ /1/){ @fl_tmg = NCO_bm::fl_cr8(0); $fc++; }
    if ($tst_fl_cr8 =~ /2/){ @fl_tmg = NCO_bm::fl_cr8(1); $fc++; }
    if ($tst_fl_cr8 =~ /3/){ @fl_tmg = NCO_bm::fl_cr8(2); $fc++; }
    if ($notbodi && $tst_fl_cr8 =~ /4/) { @fl_tmg = NCO_bm::fl_cr8(3); $fc++; }
    if ($fc >0) {NCO_bm::smrz_fl_cr8_rslt(@fl_tmg); } # prints and udpreports file creation timing
}

my $doit=1; # for skipping various tests
use NCO_benchmarks; #module that contains the actual benchmark code
# and now, the REAL benchmarks, set up as the regression tests below to use go() and smrz_rgr_rslt()
#print "DEBUG: prior to benchmark call, dodap = $dodap\n";
#print "in main(),just priior to the benchmarks \$mpi_prc=[$mpi_prc] \$mpi_prfx=[$mpi_prfx] \$mpi_fke=[$mpi_fke]\n";

if ($bm) { NCO_benchmarks::benchmarks(); }

