#include "nc_wrapper.h"
#include "math.h"

int disable_clobber;
int enable_verbose;
int input_dir_flag;
int input_file_flag;
char input_file[256];

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

double INPUT_LAT, INPUT_LON;
int INPUT_YEAR, INPUT_MONTH, INPUT_HOUR;

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
		printf("i %d\n", i);
	}
}

int process_average_arguments(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "r:c:y:m:h:i:f:o:p:s:dv")) != -1) {
        switch(opt) {
            /* Primary user arguments prefixed with 'INPUT_' */
            case 'r': /* Latitude */
                INPUT_LAT = atof(optarg);
                break;
            case 'c': /* Longitude */
                INPUT_LON = atof(optarg);
                break;
            case 'y': /* Year */
                INPUT_YEAR = atoi(optarg);
                break;
            case 'm': /* Month */
                INPUT_MONTH = atoi(optarg);
                break;
            case 'h': /* Hour on 24-Hour system */
                INPUT_HOUR = atoi(optarg);
                break;
            /* Additional program args */
            case 'i':
                strcpy(input_dir, optarg);
                input_dir_flag = 1;
                break;
            case 'f':
                strcpy(input_file, optarg);
                input_file_flag = 1;
                break;
            case 'o': /* Output file directory (default is same as input) */
                strcpy(output_dir, optarg);
                break;
            case 'p': /* Output file prefix */
                strcpy(prefix, optarg);
                break;
            case 's': /* Output file suffix (default is '.copy'[.nc]) */
                strcpy(suffix, optarg);
                break;
            case 'd':
                disable_clobber = 1;
                break;
            case 'v':
                enable_verbose = 1;
                break;
            default:
                printf("Bad case!\n");
                return 1;
        }
    }

    /* NCAR/NCEP Reanalysis  is [-90.0, 90.0] degrees */
    if (INPUT_LAT < -90.0 || INPUT_LAT > 90.0 || fmod(INPUT_LAT, 2.5) != 0.0) {
        fprintf(stderr, "Latitude not valid\n");
    }
}

int main(int argc, char* argv[]) {
    int opt, count;
    struct dirent **dir_list;

	/* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;

    if (process_average_arguments(argc, argv)) {
        printf("Program usage: %s\n", argv[0]); 
        printf("\t-r [latitude] -c [longitude] -y [year] -m [month] -h [hour (24hr)]\n");
        printf("\t-i [input directory] -d [delete flag; no args]\n");
        printf("\t-o [output directory] -p [output FILE_CUR prefix] -s [output FILE_CUR suffix] -v [verbose; no args]\n");
        exit(1);
    }

    printf("arg 0 %s arg 1 %s\n", argv[0], argv[1]);

    collect_range(2013, 0, 0);


    printf("Averaging program completed successfully.\n");

}


/*
main(int argc, char* argv[]) {
	- declare and initalize flags/counters
	- process command-line arguments (model after 'process_arguments' in ncwrapper)
	- parse an input date and time -> potentially convert to UTC
	- parse the input locaiton -> primarily the east/west (longitude?) matters
	- find the matching interpoalted file (year), data segment (time x lat x lon)
	- grab particular "column" of data, then progress eastward, accounting for the local time
	- 	in that region. I.e. 10AM in UTC -6 -> 9AM in UTC-7, but +1 (next data segment) for 10AM in UTC-7
	-> Remember the huge temporal diagonal cut.

	-
}

*/