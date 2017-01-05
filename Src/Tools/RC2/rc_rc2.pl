sub MakeLayout;
sub MakeType;

$maxY = 23;

browser: 
while (<>) {
	chop $_;
#	(print $_,"\n") && next if (/^#/);
	next if (/^#/);
	next if (/^\/\//);
	if(/(.*)\/\*(.*)/) {
		$beforecomment=$1;
		while (<>) {
			if(/(.*)\*\/(.*)/) {
				$_=$beforecomment . $2;
				last;
			}
		}
	}
	next if /^$/;


	/TOOLBAR_(\w+) +(\w+) +\{/ && do {
		$toolbar_name = $1;
		$is_export = ($2 eq 'TV_EXPTOOLBAR' ? 1 : 0) ;
		$_=<>; chop $_;
		/\s*(\w+)/;
		if($is_export) {
			print 'toolbar export '.$toolbar_name.' '.$1." {\n";
		}
		else {
			print 'toolbar '.$toolbar_name.' '.$1." {\n";
		}
		while(<>) {
			chop $_;
			/\s*TV_MENUSEPARATOR/ && do {
				print "\t\tseparator\n";
				next;
			};
			/\s*TV_END/ && do {
				print "}\n\n";
				last;
			};
			print $_."\n";
		};
	};
	/BROWSER_(\w*) +TV_BROWSER +\{/ && do {  
		$browser_name = $1;
		$_ = <>;
		chop $_;
		($ax,$ay,$bx,$by,$hight, $freeze, $header, $browser_flags, $help_id)=split /,\s*/;
		$browser_flags=~s/BRO_//g;
		$layout = MakeLayout($ax,$ay,$bx,$by);
		print 'browser '.$browser_name.' '.$layout.', '.$hight.', '.$freeze.', '.$header.', '.$browser_flags.', '.$help_id."\n";
		print "{\n";
	
		$tab = "";

		while(<>) {
			chop $_;
			if(/^\s*\}/) {
				print "}\n\n";
				next browser;
			}
			/\s*TV_BROCOLUMN\s+(.*)$/ && do {
				$tail = $1;
				($name, $reqnumber, $dont_know, $stype, $size, $prec, $flags, $width, $width_prec)=split /,\s+/, $tail;
				$type = MakeType($stype, $size, $prec);
				if($width_prec != 0) {
					print $tab."\t".$name.", ".$reqnumber.", ".$type.", ".$flags.", ".$width.".".$width_prec."\n";
				}
				else {
					print $tab."\t".$name.", ".$reqnumber.", ".$type.", ".$flags.", ".$width."\n";
				}
				next;
			};
			/\s*TV_IMPTOOLBAR\s+TOOLBAR_(.*)$/ && do {
				print "\ttoolbar ".$1."\n";
				next;
			};
			/\s*TV_BROGROUP\s+\"(.+)\".*/ && do {
				print "\tgroup \"".$1."\" {\n";
				$tab = "\t";
				next;
			};
			/\s*TV_END/ && do {
				print "\t}\n";
				$tab = "";
				next;
			};
			/\s*TV_TOOLBAR\s+(.*)$/ && do {
				print "\ttoolbar ".$1." {\n";
				while(<>) {
					chop $_;
					/\s*TV_END/ && do {
						print "\t}\n";
						$tab = "";
						last;
					};
					/\s*TV_MENUSEPARATOR/ && do {
						print "\t\tseparator\n";
						next;
					};
					print $_."\n";
				}
				next;
			};
		}
	}
}

sub MakeLayout
{
	$ax = @_[0]; $ay = @_[1]; $bx =  @_[2]; $by = @_[3];
	$percent = int((($by - $ay)*1000/$maxY/10));
	if($ay == 0) {
		return "north(".$percent.")";
	}
	else {
		if($by == $maxY) {
			return "south(".$percent.")";
		}
		else {
			return "center(".$percent.")";
		}
	}
}

sub MakeType
{
	$_ = @_[0]; $size = @_[1]; $prec = @_[2];
	return 'zstring('.$size.')' if(/S_ZSTRING/);
	return ( $prec == 4 ? 'float' : 'double' ) if(/S_FLOAT/);
	return 'acct' if(/S_ACCT/);
	return 'date' if(/S_DATE/);
	return 'char['.$size.']' if(/S_CHAR/);
	return 'notype';
}

