PACKAGES = opengl sdl_image sdl
INCDIRS = gmae/sdl_mixer gmae/sdl_mixer/mikmod
EXTRALDFLAGS = -lm
LIBS = util/libmarf.a gmae/sdl_mixer/libsdl_mixer.a
TARGET = marfitude
EXEC = program
