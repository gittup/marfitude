#define MAX_NOTE 4
#define MAX_COLS 8

// a column is one group of module channels in one pattern.
// There are a constant number of columns in each pattern, which is
// less than or equal to the number of module channels
// the Column specifies which module channels it contains
typedef struct {
	int numchn;	// number of channels in this column
	int *chan;	// list of numchn channels
	} Column;

// a pattern is a group of columns.  It is called a pattern because it is
// based on the module concept of a "pattern."  So for a single pattern in a
// mod we have a single pattern in the WAM file, which gives the column
// to channel relationship.  Therefore, the channels in a column are
// consistent throughout a single pattern.  Here, patterns are ever increasing,
// even if they are duplicates (unlike a mod, which can have a single pattern
// be played at multiple song positions)
typedef struct {
	Column columns[MAX_COLS];	// numCols columns
	Column unplayed;	// contains all the channels not represented
				// by the other columns
	} Pattern;

typedef struct {
	// the following are taken from the mod file itself
	// they are used for speed calculations and reference data
	int bpm;	// bpm for this row
	int sngspd;	// sngspd for this row
	int patpos;	// this row number (position in pattern)
	int sngpos;	// position in song

	// the following are generated by the program
	int ticpos;	// tick count up to this point
	int line;	// 0 = no line, 1 = regular line, 2 = start of new patt
	int patnum;	// the pattern this row is in
	char notes[MAX_COLS];	// 0 = no note, 1, 2, 4 = position of note
	} Row;

typedef struct {
	int numCols;		// number of columns in the song, <= mod->numchn
	int numTics;		// number of ticks in the song
	int numPats;		// number of patterns
	int numRows;		// number of rows
	Pattern *patterns;	// numPats Patterns
	Row *rowData;		// numRows Rows
	} Wam;

void FreeWam(Wam *wam);		// frees wam from LoadWam
int WriteWam(char *modFile);	// loads the modFile and creates and writes
				// the appropriate WamFile, returns 1 if
				// successful, 0 on failure
Wam *LoadWam(char *modFile);	// loads the wam related to the modFile,
				// and returns it.  Must be freed later by
				// FreeWam
