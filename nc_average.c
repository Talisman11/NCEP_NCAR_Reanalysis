#include "ncwrapper.h"

/* Special indexing variables */
extern int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
extern int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

extern char input_dir[];
extern char output_dir[];
extern char prefix[];
extern char suffix[];

extern char FILE_CUR[];
extern char FILE_NEXT[];
extern char COPY[];

/*
Task  		- Average over each day for January 2013 file
Approach	- Loop through file, gather times relevant to January 1 - 31
			- Average Daily 4x values.
Output 		- New file with 31 values for January

Future 		- Loop through all days, finer granularity
			- TERRA Overpass synchronization
*/

/* Return days in particular month (1-12) for that year. */
int days_in_month(int y, int m) {
	if (m == 2) {
		if ((y % 4 == 0 && y % 4 != 0) || y % 400 == 0)
			return 29;
		return 28;
	}

	if (m == 4 || m == 6 || m == 9 || m == 11)
		return 30;
	return 31;
}

/*
 * [start_m, end_m] - start to end month range, specified with values 1-12. 
 * if start_m == 0: process whole year
 */
int collect_range(int year, int start_m, int end_m) {
	if (start_m == 0) {
		start_m = 1;
		end_m = 12;
	}

	// Collect number of time units (4 * total_days) for indexing into other files (1460, 1464 remember)
	// Need 'start_offset_idx' and either 'count' or 'end' idx 
	for (int i = start_m; i <= end_m; i++) {

	}
}

int main(int argc, char* argv[]) {
    int count;
    struct dirent **dir_list;

	/* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;



    
    printf("arg 0 %s arg 1 %s\n", argv[0], argv[1]);

    printf("Reanalysis program completed successfully.\n");

}

