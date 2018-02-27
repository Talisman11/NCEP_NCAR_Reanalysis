#include "nc_wrapper.h"
#include <math.h>
#include <string.h>

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

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;


double INPUT_LAT, INPUT_LON;
int INPUT_YEAR, INPUT_MONTH, INPUT_HOUR;

int LEAP_YEAR;
size_t DAY_STRIDE;

Variable *vars;
Dimension *dims;

int file_id, num_vars, num_dims;

int process_average_arguments(int argc, char* argv[]) {
    int opt;

    INPUT_LAT = INPUT_LON = -1000.0;
    INPUT_YEAR = INPUT_MONTH = INPUT_HOUR = -1;
    while ((opt = getopt(argc, argv, "r:c:y:m:h:i:f:o:p:s:dv")) != -1) {
        switch(opt) {
            /* Primary user arguments prefixed with 'INPUT_' */
            case 'r': /* Latitude */
                INPUT_LAT = atof(optarg);
                break;
            case 'c': /* Longitude */
                INPUT_LON = atof(optarg);
                break;
            case 'm': /* Month */
                INPUT_MONTH = atoi(optarg);
                break;
            case 'h': /* Hour on 24-Hour system */
                INPUT_HOUR = atoi(optarg);
                break;
            /* Additional program args */
            // case 'i':
            //     strcpy(input_dir, optarg);
            //     input_dir_flag = 1;
            //     break;
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

    /* NCAR/NCEP Reanalysis - Latitude: [-90.0, 90.0]; Longitude: [0, 357.5] degrees */
    if (INPUT_LAT < -90.0 || INPUT_LAT > 90.0 || fmod(INPUT_LAT, 2.5) != 0.0) {
        fprintf(stderr, "Latitude invalid (%f). \
            Accepts values in [-90.0, 90.0] range as multiples of 2.5 degrees\n", INPUT_LAT);
        return 1;
    }

    if (INPUT_LON < 0.0 || INPUT_LON > 357.5 || fmod(INPUT_LON, 2.5) != 0.0) {
        fprintf(stderr, "Longitude invalid (%f). \
            Accepts values in [0, 357.5] range as multiples of 2.5 degrees\n", INPUT_LON);
        return 1;
    }

    /* 13 values supported; 1-12 corresponding to the months, and 0 to process the whole year */
    if (INPUT_MONTH < 0 || INPUT_MONTH > 12) {
        fprintf(stderr, "Month invalid (%d). \
            Must be within [0, 12], where 1:January, 2:February, ..., and 0 for entire year\n", INPUT_MONTH);
        return 1;
    }

    /* 24 hour clock system */
    if (INPUT_HOUR < 0 || INPUT_HOUR > 24) {
        fprintf(stderr, "Hour invalid (%d). Must be within [0, 24] on 24-hour clock system\n", INPUT_HOUR);
        return 1;
    }

    if (input_file_flag != 1 || extract_year(&INPUT_YEAR, input_file)) {
        fprintf(stderr, "Failed to extract year from input file\n");
        return 1;
    }

    /* Ensure a 4 digit realistic number */
    if (INPUT_YEAR < 1800 || INPUT_YEAR > 2050) {
        fprintf(stderr, "Year invalid (%d). Accepts values in [1800, 2050] (arbitrary end year)\n", INPUT_YEAR);
        return 1;
    }

    printf("Program proceeding with:\n");
    printf("\tLatitutde: %f\n", INPUT_LAT);
    printf("\tLongitude: %f\n", INPUT_LON);
    printf("\tYear | Month | Hour: %d | %d | %d\n", INPUT_YEAR, INPUT_MONTH, INPUT_HOUR);
    printf("\tDisable clobbering = %d\n", disable_clobber);
    printf("\tInput directory = %s\n", input_dir);
    printf("\tOutput directory = %s\n", output_dir);
    printf("\tOutput file prefix = %s\n", prefix);
    printf("\tOutput file suffix = %s\n", suffix);

    return 0;
}

/*
 * Find idx of last period, NULL terminate, then find second to last and return the year
 * year - extracted value from file_name
 * file_name - input filename
 */
int extract_year(int* year, const char* file_name) {
    int i = 0;
    const char period = '.';
    char *copy, *last_per, *sec_per;

    // Create copy of file_name for extraction
    copy = (char *) malloc(sizeof(char) * (strlen(file_name) + 1));
    strcpy(copy, file_name);

    if (strlen(copy) > 0) {
        last_per = strrchr(copy, period);
        if ((last_per != NULL) && (last_per > 0)) {
            copy[last_per - copy] = '\0';
            sec_per = strrchr(copy, period);

            if ((sec_per != NULL) && (sec_per > 0)) {
                *year = atoi(sec_per + 1);
                return 0;
            }
        }
    }
    return 1;
}

/* Return days in particular month (1-12) for that year. */
int days_in_month(int y, int m) {
	if (m == 2) {
		if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
			return 29;
		return 28;
	}

	if (m == 4 || m == 6 || m == 9 || m == 11)
		return 30;
	return 31;
}

/* Prepares for processing */
int calibrate(int *preceding_days, int *total_days, int year, int month) {
    int start_m, end_m, days;

    if (month == 0) {
        start_m = 1;
        end_m = 12;
    } else {
        start_m = month;
        end_m = month + 1; // ?? why is there a 2 here?
    }
    printf("start: %d, end: %d\n", start_m, end_m);

    // Set leap year flag
    LEAP_YEAR = days_in_month(year, 2) == 29 ? 1 : 0;

    *total_days = 0; // zero the input value before adding to it
    *preceding_days = 0;
    for (int i = 1; i < end_m; i++) {
        days = days_in_month(year, i);
        if (i >= start_m) {
            *total_days += days;
        } else {
            *preceding_days += days;
        }
    }
    printf("total_days: %d, preceding_days: %d\n", *total_days, *preceding_days);

    return 0;
}

int gather(double tgt_lon, int preceding_days, int total_days, const char* input_file) {
    size_t time, lvl, lat, lon, index, time_start, time_end, tgt_lon_idx;

    ___nc_open(input_file, &file_id);
    nc_inq(file_id, &num_dims, &num_vars, NULL, NULL);

    /* Output Dimension Information */
    printf("DIMENSION INFO:\n");
    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
        ___nc_inq_dim(file_id, i, &dims[i]);
    }

    /* Output Variable Information */
    printf("VARIABLE INFO:\n");
    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
        ___nc_inq_var(file_id, i, &vars[i], dims);
    }

    printf("Loading variable data\n");
    for (int i = 0; i < num_vars; i++) {
        ___nc_get_var_array(file_id, i, &vars[i], dims);
    }

    TIME_STRIDE = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length * dims[DIM_ID_LVL].length;
    LVL_STRIDE  = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length;
    LAT_STRIDE  = dims[DIM_ID_LON].length;
    DAY_STRIDE  = LEAP_YEAR ? dims[DIM_ID_TIME].length / 366 : dims[DIM_ID_TIME].length / 365;

    time_start = DAY_STRIDE * preceding_days;
    time_end   = DAY_STRIDE * (preceding_days + total_days);

    for (lon = 0; lon < dims[DIM_ID_LON].length; lon++) {
        printf("lon: %f\n", dims[DIM_ID_LON][lon]);
    }


    printf("start: %lu, end: %lu, lon: %f\n", time_start, time_end);

    float* var_data = vars[VAR_ID_SPECIAL].data;
    for (time = time_start; time < time_end && time < dims[DIM_ID_TIME].length; time++) {
        for (lvl = 0; lvl < dims[DIM_ID_LVL].length; lvl++) {
            for (lat = 0; lat < dims[DIM_ID_LAT].length; lat++) {
                // for (lon = 0; lon < dims[DIM_ID_LON].length; lon++) {
                    index = ___access_nc_array(time, lvl, lat, LON);
                // }
            }


        }
        printf("%lu -> %s[%lu][%lu][%lu][%lu] = [%lu] = %f\n",
            YEAR, vars[VAR_ID_SPECIAL].name, time, lvl, lat, lon, index, var_data[index]);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int preceding_days, total_days;
    struct dirent **dir_list;

	/* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;

    if (process_average_arguments(argc, argv)) {
        printf("Program usage: %s\n", argv[0]);
        printf("\t-r [latitude] -c [longitude] -y [year] -m [month] -h [hour (24hr)]\n");
        printf("\t-i [input directory] -f [input file] -d [delete flag; no args]\n");
        printf("\t-o [output directory] -p [output FILE_CUR prefix] -s [output FILE_CUR suffix] -v [verbose; no args]\n");
        exit(1);
    }

    if (calibrate(&preceding_days, &total_days, INPUT_YEAR, INPUT_MONTH)) {
        printf("Calibrate function failed\n");
        exit(1);
    }

    if (gather(INPUT_LON, preceding_days, total_days, input_file)) {
        printf("Gather function failed\n");
        exit(1);
    }

    printf("Averaging program completed successfully.\n");
    return 0;
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
