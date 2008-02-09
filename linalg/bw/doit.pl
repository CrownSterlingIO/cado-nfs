#!/usr/bin/perl -w

# Wrapper script for block wiedemann.
#
# This is an evolution of the previous shell script. The latter grew a
# bit cumbersome in the end, so it seems to become more manageable in
# perl now.

use strict;
use warnings;

use Time::HiRes qw(gettimeofday);
# For seed values. 1 sec granularity is too coarse for repeated
# experiments.

use Data::Dumper;

print "$0 @ARGV\n";

my $param = {
	# example data
	modulus=>"531137992816767098689588206552468627329593117727031923199444138200403559860852242739162502265229285668889329486246501015346579337652707239409519978766587351943831270835393219031728127",
	dens => 10,
	# everything can be overriden from the command line with switches
	# of the form ``param=xxx''
	m=>4,
	n=>4,
	vectoring=>4,
	method=>'ifft',
	multisols=>0,
	maxload=>1, # How many simultaneous jobs on one machine.
	threshold=>64,	# A good starting value.
	dimk=>10,
        solution=>'W',  # Name of the solution file.
};


my $favorite_tmp = "$ENV{HOME}/Local";
$favorite_tmp = "/tmp" if ! -d $favorite_tmp;

# Parameters that will be used only if nothing sets them earlier.
my $weak = {
	tidy=>1,
	wdir=> "$favorite_tmp/testmat",
};

sub dirname  { $_[0] =~ m{^(.*)/[^/]*$}; return $1 || '.'; };
sub basename { $_[0] =~ m{([^/]*)$}; return $1; };

my $srcdir = dirname $0;
my $bindir;

{
	my $cmd="make -s -C \"$srcdir\" variables";
	open F, "$cmd |";
	while (<F>) {
		/^BINARY_DIR=(.*)$/ && do { $bindir=$1; };
	}
	close F;

	if ($bindir eq '') {
		$bindir="$srcdir/";
	}
}

my $krylovbindir=$bindir;

my @args = @ARGV;
while (defined(my $x = shift(@args))) {
	if ($x =~ m/^mn=(.*)$/) {
		# special case.
		$param->{'m'} = $1;
		$param->{'n'} = $1;
	} elsif ($x =~ m/^(.*)=(.*)$/) {
		$param->{$1} = $2;
	} elsif ($x =~ /\.cfg$/) {
		# read a possible config file, and apply options right
		# now.
		open my $xh, $x or die "$x: $!";
		unshift @args, <$xh>;
		close $xh;
	} else {
		# Then it should be a matrix file.
		$param->{'matrix'} = $x;
	}
}

sub action { print join(@_,' '), "\n"; system @_; }

# We could possibly do cp --symlink just as well, or maybe ln (cp --link
# is a gnu-ism). Not ln -s without caution, because this would require
# some knowledge about the right path back, which could be quite a bit of
# a hack. I'm happy with cp --link at the moment.
sub do_cp { my @x = @_; unshift @x, "cp", "--link"; action @x; }

# If we already have a matrix, parse its header to obtain some
# information.
sub parse_matrix_header {
	my $f = shift @_;
	open my $mh, $f or die "$f: $!";
	my @header = ();
	my $h = {};
	my $hline;
	HEADER: {
		do {
			last HEADER unless defined(my $x = <$mh>);
			$hline = $x if $x =~ /ROWS/;
			if (!defined($hline)) {
				$hline = $x if $x =~ /^\d+\s\d+$/;
			}
			push @header, $x;
		} until (scalar @header == 10);
	}
	if (!defined($hline)) {
		die "No header line in first ten lines of $f";
	}
	my $fmt;
	if ($hline =~ /^(\d+)\s(\d+)$/) {
		$fmt = 'CADO';
		$h->{'nrows'} = $1;
		$h->{'ncols'} = $2;
		$h->{'modulus'} = 2;
	} else {
		$fmt = 'BW-LEGACY';
		$hline =~ /(\d+)\s+ROWS/i
			or die "bad header line in $f : $hline";
		$h->{'nrows'}=$1;
		$hline =~ /(\d+)\s+COLUMNS/i
			or die "bad header line in $f : $hline";
		$h->{'ncols'}=$1;
		$hline =~ /MODULUS\s+(\d+)/i
			or die "bad header line in $f : $hline";
		$h->{'modulus'} = $1;
	}
	print "$fmt format $h->{'nrows'} rows $h->{'ncols'} columns\n";
	die "Must have ncols >= nrows for a column dependency\n"
		if $h->{'ncols'} < $h->{'nrows'};
	my $hash = `head -c 2048 $f | md5sum | cut -c1-8`;
	chomp($hash);
	$h->{'hash'} = $hash;
	return $h;
}

if ($param->{'matrix'}) {
	my $h = parse_matrix_header $param->{'matrix'};
	$param->{'ncols'} = $h->{'ncols'};
	$param->{'nrows'} = $h->{'nrows'};
	$param->{'modulus'} = $h->{'modulus'};
	$weak->{'wdir'} .= ".$h->{'hash'}";
}

for my $k (keys %$weak) {
	next if exists $param->{$k};
	$param->{$k}=$weak->{$k};
}

# ok -- now we're fed up with all of the $param stuff, so short-circuit
# all this, and import the names in the main namespace.

my $dumped = '';
sub dumpvar {
	my $x = shift @_;
	if (defined(my $y = $param->{$x})) {
		$dumped .= "$x=$y\n";
	}
}

my $matrix =	$param->{'matrix'};	dumpvar 'matrix';
my $solution =	$param->{'solution'};	dumpvar 'solution';
my $modulus =	$param->{'modulus'};	dumpvar 'modulus';
my $msize =	$param->{'msize'};	dumpvar 'msize';
my $nrows =	$param->{'nrows'};	dumpvar 'nrows';
my $ncols =	$param->{'ncols'};	dumpvar 'ncols';
my $m =		$param->{'m'};		dumpvar 'm';
my $n =		$param->{'n'};		dumpvar 'n';
my $wdir =	$param->{'wdir'};	dumpvar 'wdir';
my $resume =	$param->{'resume'};	dumpvar 'resume';
my $tidy =	$param->{'tidy'};	dumpvar 'tidy';
my $vectoring =	$param->{'vectoring'};	dumpvar 'vectoring';
my $dens =	$param->{'dens'};	dumpvar 'dens';
my $dump =	$param->{'dump'};	dumpvar 'dump';
my $mt =	$param->{'mt'};		dumpvar 'mt';
my $machines =	$param->{'machines'};	dumpvar 'machines';
my $rsync =	$param->{'rsync'};	dumpvar 'rsync';
my $method =	$param->{'method'};	dumpvar 'method';
my $threshold =	$param->{'threshold'};	dumpvar 'threshold';
my $multisols =	$param->{'multisols'};	dumpvar 'multisols';
my $maxload =	$param->{'maxload'};	dumpvar 'maxload';
my $seed =	$param->{'seed'};	dumpvar 'seed';
my $precond =	$param->{'precond'};	dumpvar 'precond';
my $dimk =	$param->{'dimk'};	dumpvar 'dimk';

if ($param->{'dumpcfg'}) {
	print $dumped;
	exit 0;
}

my $seeding = '';
if ($seed) {
	$seeding = "--seed $seed";
} else {
	$seed = (gettimeofday)[1] % 999983;
	print "Selecting seed $seed\n";
	$seeding = "--seed $seed";
}



if ($vectoring && ($n % $vectoring != 0)) {
	die "vectoring parameter must divide n";
}

if ($msize) {
	die "msize conflicts with nrows/ncols" if ($nrows || $ncols);
	$nrows = $ncols = $msize;
} else {
	die "supply either msize or both nrows and ncols"
		unless ($nrows && $ncols);
}

if (defined($msize) && !defined($matrix) && $dimk > $msize / 2) {
    die "dimk $dimk seems unreasonably large for matrix size $msize";
}

# Check that we have the threshold argument if needed
my $exe_lingen = "${bindir}bw-lingen";
if ($modulus eq '2') {
	$exe_lingen = "${bindir}bw-lingen-binary";
	die "threshold needed" unless $threshold;
	$exe_lingen .= " -t $threshold";
} elsif ($method =~ /^(?:q(?:uadratic)?|old)$/) {
	$exe_lingen .= '-old';
} else {
	# TODO: allow runtime selection of method or (probably
	# smarter) runtime checking that the proper binary is
	# being used.
	die "threshold needed" unless $threshold;
	$exe_lingen .= "2 -t $threshold";
}

sub magmadump {
	if ($dump) {
		action "${bindir}bw-printmagma --subdir $wdir > $wdir/m.m";
	}
}

# Prepare the directory.
system "rm -rf $wdir" unless $resume;
if (!-d $wdir) {
	mkdir $wdir or die
		"Cannot create $wdir: $! -- select something non-default with wdir=";
}


open STDOUT, "| tee -a $wdir/solve.out";
open STDERR, "| tee -a $wdir/solve.err";


print "Solver started ", scalar localtime, "\nargs:\n$dumped\n";
print STDERR "Solver started ", scalar localtime, "\nargs:\n$dumped\n";
my $version=`$srcdir/util/version.sh`;
print "version info:\n$version";
print STDERR "version info:\n$version";

my $tstart = gettimeofday;
my $tlast = $tstart;

my $times = { };

sub account {
    my $t = gettimeofday;
    my $k = $_[0];
    if (!exists($times->{$k})) {
        $times->{$k} = 0.0;
    }
    $times->{$_[0]} += $t - $tlast;
    $tlast = $t;
}



if ($resume) {
	if ($precond && -f "$wdir/precond.txt") {
		print "There is already a $wdir/precond.txt file; let's hope it is the same as $precond !\n"
	} elsif ($precond && ! -f "$wdir/precond.txt")  {
		die "$wdir/precond.txt is missing";
	}
	# Having a precond file present but not required by cmdline is
	# conceivable if I introduce rhs someday.
	#
	# Having none is ok of course.
} else {
	if ($precond) {
		do_cp $precond, "$wdir/precond.txt";
	}
        
        open CFG, ">$wdir/bw.cfg";
        print CFG <<EOF;
n: $n
m: $m
EOF
        close CFG;
}


# In case of multiple machines, make sure that all machines see the
# directory.
my @mlist = split ' ', $machines || '';

my $shared=1;

if (@mlist) {
	my $tms = time;
	system "touch $wdir/$tms";
	for my $m (@mlist) {
		open my $xh, "ssh -n $m ls $wdir/$tms 2>&1 |";
		while (defined(my $x=<$xh>)) {
			print "($m)\t$x";
			chomp($x);
			if ($x ne "$wdir/$tms") {
				$shared=0;
			}
		}
		close $xh;
	}
	unlink "$wdir/$tms";
	if (!$shared) {
		print STDERR "Some machines can not see $wdir\n";
		if (!$rsync) {
			print STDERR "Either use rsync=1 or an nfs dir\n";
			exit 1;
		}
	}
	# TODO: what about inhomogeneous computations ?
        do_cp "${bindir}bw-krylov", "$wdir/";
        do_cp "${bindir}bw-krylov-mt", "$wdir/";
	$krylovbindir="$wdir/";
}

if ($resume) {
	die "Cannot resume: no matrix file" unless -f "$wdir/matrix.txt";
	my $h = parse_matrix_header "$wdir/matrix.txt";
	$param->{'ncols'} = $h->{'ncols'};
	$param->{'nrows'} = $h->{'nrows'};
	$param->{'modulus'} = $h->{'modulus'};
} else {
	if ($matrix) {
		do_cp $matrix, "$wdir/matrix.txt";
	} else {
		# If no matrix parameter is set at this moment, then surely it
		# means we're playing with a random sample: create it !
		action "${bindir}bw-random $seeding --dimk $dimk $nrows $ncols $modulus $dens > $wdir/matrix.txt";
	}
}

if ($modulus eq '2' && $multisols == 0) {
    print "Forcing multisols=1 for modulus 2\n";
    $multisols=1;
}

account 'io';

if ($mt) {
    action "${bindir}bw-balance --subdir $wdir --nbuckets $mt"
            unless ($resume && -f "$wdir/matrix.txt.old");
}
action "${bindir}bw-secure --subdir $wdir"
	unless ($resume && -f "$wdir/X0-vector");

if ($resume && -f "$wdir/X00") {
	# Make sure there's the proper number of vectors.
	my $dh;
	opendir $dh, $wdir;
	my $gx = scalar grep { /^X\d+$/ } readdir $dh;
	closedir $dh;
	if (scalar $gx != $m) { die "Bad m param ($m != $gx)\n"; }
	opendir $dh, $wdir;
	my $gy = scalar grep { /^Y\d+$/ } readdir $dh;
	closedir $dh;
	if (scalar $gy != $n) { die "Bad m param ($n != $gy)\n"; }
} else {
	action "${bindir}bw-prep $seeding --subdir $wdir $m $n"
}

account 'prep';

if ($dump) {
	action "${bindir}bw-printmagma --subdir $wdir > $wdir/m.m";
}

account 'io';

sub rsync_push {
	if (!$shared && $rsync) {
		for my $m (@mlist) {
			action "rsync -av @_ $wdir/ $m:$wdir/";
		}
	}
}

sub rsync_pull {
	if (!$shared && $rsync) {
		for my $m (@mlist) {
			action "rsync -av @_ $m:$wdir/ $wdir/";
		}
	}
}

rsync_push "--delete";

account 'io';

sub compute_spanned {
	my @jlist = @_;
	# for my $job (@jlist) { print "$job\n"; }
	if (@mlist) {
		my @kids=();
		my @assignments=();
		for my $j (1..($maxload * scalar @mlist)) {
			push @assignments, [];
		}
		for my $j (0..$#jlist) {
			my $job = $jlist[$j];
			push @{$assignments[$j % scalar @assignments]}, $j;
		}

		for my $i (0..$#assignments) {
			my $a = $assignments[$i];
			my $machine = $mlist[$i % scalar @mlist];
			my $pid=fork;
			if ($pid) {
				push @kids, $pid;
				next;
			}
			for my $j (@$a) {
				my $job = $jlist[$j];
				# kid code
				print "($machine/job$j)\t$job\n";
				open my $ch, "ssh -n $machine $job 2>&1 |";
				while (defined(my $x = <$ch>)) {
					print "($machine/job$j)\t$x";
				}
				close $ch;
			}
			# end of kid code.
			exit 0;
		}
		print "Child processes: ", join(' ', @kids), "\n";
		my @dead=();
		for my $i (0..$#assignments) { push @dead, wait; }
		print "Reaped processes: ", join(' ', @dead), "\n";
	} else {
		for my $job (@jlist) {
			action $job;
		}
	}
}

sub nlines {
	my $f = shift @_;
	open my $fh, "<$f" or return 0;
	my $l=0;
	while (<$fh>) { $l++; }
	close $fh;
	return $l;
}


my $m1=$m-1;
my $n1=$n-1;

# Gather the list of krylov commands.
KRYLOV : {
	my $exe="${krylovbindir}bw-krylov";
	if ($mt) {
		$exe .= "-mt --nthreads $mt";
	}
	$exe .= " --task krylov --subdir $wdir";


	my @krylovjobs = ();
	
	for my $vi (0..int($n/$vectoring) - 1) {
		my ($i,$ni1)=($vectoring*$vi,$vectoring*($vi+1)-1);
		# That's really a crude approximation.
		my $l_approx=$ncols/$m + $ncols/$n;
		my @linecounts=();
		for my $j (0..$m1) {
			my $x = nlines "$wdir/A-0$m1-00";
			if ($x < $l_approx) {
				# Then this one is not finished. We have
				# to go forward.
				last;
			}
			push @linecounts, $x;
		}
		if (scalar @linecounts == $m) {
			for my $j (0..$m1) {
				print "$wdir/A-0$m1-00 : $linecounts[$j] lines, over.\n";
			}
		} else {
			push @krylovjobs, "$exe 0,$i $m1,$ni1";
		}
	}

	if (scalar @krylovjobs) {
		print "--- krylov jobs: ---\n";
		for my $j (@krylovjobs) {
			print "// $j\n";
		}
		compute_spanned @krylovjobs;
	}
}

account 'krylov';

rsync_pull;

account 'io';

my @sols;

LINGEN : {
	RECYCLE_LINGEN: {
		if ($resume) {
			my $allfiles=-f "$wdir/lingen.log";
			for my $x (0..$m+$n-1) {
				my $p = sprintf "%02d", $x;
				$allfiles = $allfiles && -f "$wdir-$p";
			}
			if ($allfiles) {
				print "lingen: all files found, reusing\n";
			} else {
				last RECYCLE_LINGEN;
			}
			open my $ph, "tail -1 $wdir/lingen.log |";
			while (defined(my $x=<$ph>)) {
				if ($x =~ /LOOK \[\s*([\d\s]*)\]$/) {
					print "$x\n";
					@sols = split ' ',$1;
				}
			}
			if (!scalar @sols) {
				magmadump;
				die "No solution found";
			}
			last LINGEN;
		}
	}

	my $exe = $exe_lingen;
        if ($modulus == 2) {
            $exe .= " --subdir $wdir";
        } else {
            $exe .= " --subdir $wdir matrix.txt $m $n";
        }
	open my $mlog, ">$wdir/lingen.log";
	print "$exe\n";
	open my $mh, "$exe |";
	while (defined(my $x=<$mh>)) {
		print $mlog $x;
		print $x;
		if ($x =~ /LOOK \[\s*([\d\s]*)\]$/) {
			@sols = split ' ',$1;
		}
	}
	if (!scalar @sols) {
		magmadump;
		die "No solution found ; seed was $seed";
	}
}

account 'lingen';

rsync_push;
account 'io';


my @sols_found  = ();

# within the matrix source directory
my @solfiles=();

my $orig_nsols = scalar @sols;

MKSOL : {
	my $exe="${krylovbindir}bw-krylov";
	if ($mt) {
		$exe .= "-mt --nthreads $mt";
	}
	$exe .= " --task mksol --subdir $wdir";

        if (!$multisols) {
            # Aggressively select only one solution.
            @sols = ($sols[0]);
            print "Restricting to solution @sols\n";
        } elsif ($multisols > 1) {
            @sols = @sols[0..$multisols-1];
            print "Restricting to solutions $multisols first solutions\n";
        }

        my $common = $exe;
	for my $s (@sols) {
                $common .= " --sc $s";
	}

        my @mksoljobs = ( map
        {
            my ($i,$ni1)=($vectoring*$_,$vectoring*($_+1)-1);
            "$common 0,$i $m1,$ni1";
        }
        (0..int($n/$vectoring) - 1) );

	if (scalar @mksoljobs) {
		print "--- mksol jobs: ---\n";
		for my $t (@mksoljobs) {
			#for my $j (@$t) {
			#print "// $j\n";
			#}
			print "// $t\n";
		}
		compute_spanned @mksoljobs;
			# for my $t (@mksoljobs) {
			#	compute_spanned @$t;
			#	rsync_pull;
			# }
                account 'mksol';
		# Eh ? If we don't pull the results from our pals,  who's
		# gonna do it ?
		rsync_pull;
                account 'io';
	}

        # --nbys now defaults to auto-detect.
        action "${bindir}bw-gather --subdir $wdir";
        account 'gather';

        if ($matrix) {
            my $d = dirname $matrix;
            my $sol;
            # If there is no slash, assume it is a basename and put it
            # aside the matrix. Otherwise, trust the user.
            if ($solution =~ m{/}) {
                $sol = $solution;
            } else {
                $sol = "$d/$solution";
            }
            if (-f "$wdir/W") {
                do_cp "$wdir/W", $sol;
                push @solfiles, $sol;
            }
        }

# 	if ($multisols) {
# 		print scalar @sols_found, " solutions found ",
# 			"(from ", scalar @sols, " selected from ",
#                         "$orig_nsols out of lingen)\n";
# 		for my $f (@sols_found) { print "$f\n"; }
# 	}

        if (@solfiles) {
            print "solution files\n";
            for my $f (@solfiles) { print "$f\n"; }
        }

        account 'io';

#	if ($matrix) {
## 		if (scalar @solfiles != scalar @sols) {
## 			print "POSSIBLE BUG: not all wanted solutions were found\n";
## 			$tidy=0;
## 		}
        #
#		if (!$multisols && @solfiles) {
#			(my $sf = $matrix) =~ s/matrix/solution/;
#			if ($sf ne $matrix) {
#				my $s = $solfiles[0];
#				do_cp $s, $sf;
#				print "also: $sf\n";
#			}
#		}
#	}
    }

    if ($dump) {
        magmadump;
    }
    account 'io';

    if ($tidy) {
        system "rm -rf $wdir";
    }
    account 'io';

    print "-+" x 30, "\n";
    print "times (in seconds):\n";
    for my $k (keys %$times) {
        print "$k\t", sprintf("%.2f", $times->{$k}), "\n";
    }
    print "total\t", sprintf("%.2f",(gettimeofday-$tstart)), "\n";
    print "-+" x 30, "\n";
    print "Seed value for this run was $seed\n";

