#! /usr/bin/perl

open(HEADER, ">sounds.h") or die "Can't write to sounds.h!";
open(LIST, ">sndlist.h") or die "Can't write to sndlist.h!";
@snd = `ls ./sounds/*.wav`;
chomp(@snd);

print "Generating sndlist.h...\n";
print LIST "/* This file is automatically generated by gensounds.pl */\n";
print LIST "char *SND_LIST[] = {	";
for($x=0;$x<=$#snd;$x++)
{
	print LIST "\"$snd[$x]\",\\\n			";
}
print LIST "NULL};\n";

print "Generating sounds.h...\n";
print HEADER "/* This file is automatically generated by gensounds.pl */\n";
print HEADER "#ifndef _MIXER_H_\n";
print HEADER "	#include \"SDL_mixer.h\"\n";
print HEADER "#endif\n\n";
print HEADER "void PlaySound(Mix_Chunk *snd);\n";
print HEADER "int InitSounds(void);\n";
print HEADER "void QuitSounds(void);\n";
print HEADER "\n#define NUM_SOUNDS ", $#snd+1, "\n";
print HEADER "extern Mix_Chunk **Sounds;\n\n";

for($x=0;$x<=$#snd;$x++)
{
	$snd[$x] =~ s/\.\/sounds\/(.*)\.wav/$1/;
	print HEADER "#define SND_$snd[$x] Sounds[$x]\n";
}
