#! /usr/bin/perl
# This file checks to see if there are textures in the images directory
# that aren't being used by any program files

@img = `ls ./images/*.png`;
chomp(@img);

for($x=0;$x<=$#img;$x++)
{
	$img[$x] =~ s/\.\/images\/(.*)\.png/$1/;
	$inf = `grep "TEX_$img[$x]" *.c; grep "T_$img[$x]" *.c`;
	if(length($inf) == 0) {print "$img[$x] not used\n";}
	else {print "$img[$x]\n";}
}
