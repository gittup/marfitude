#! /usr/bin/perl
# This script checks to see if any sounds are listed in the sounds directory
# but aren't being used by any program files

@snd = `ls ./sounds/*.wav`;
chomp(@snd);

for($x=0;$x<=$#snd;$x++)
{
	$sndname = $snd[$x];
	$sndname =~ s/\.\/sounds\/(.*)\.wav/$1/;
	$inf = `grep "SND_$sndname" *.c`;
	if(length($inf) == 0) {print "$snd[$x] not used\n";}
	else {print "$snd[$x]\n";}
}
