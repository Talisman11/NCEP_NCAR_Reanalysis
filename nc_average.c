#include "nc_wrapper.h"
#include <math.h>
#include <string.h>

int retval;
int input_dir_flag = 0;
int output_dir_flag = 0;
char input_file_name[256];
char input_file_path[256];
char output_file[256];

char prev_file_path[256];
char prev_file_name[256];
char next_file_path[256];
char next_file_name[256];

int INPUT_FILE_ID, OUTPUT_FILE_ID, PREV_FILE_ID, NEXT_FILE_ID;

extern int disable_clobber;
extern int enable_verbose;
extern char input_dir[];
extern char output_dir[];
extern char prefix[];
extern char suffix[];

/* Special indexing variables */
extern int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
extern int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;
extern size_t SPECIAL_CHUNKS[4];

/* Global variables specified by user */
double INPUT_LAT, INPUT_LON;
int INPUT_YEAR, INPUT_MONTH, INPUT_HOUR;

Variable *vars, *prev_vars;
Dimension *dims;
int NUM_VARS, NUM_DIMS;

int LEAP_YEAR;
size_t DAY_STRIDE;
size_t HOURS_IN_DAY[] = {24};
int TIME_ZONE[25] = {-12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, \
                    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
int TIME_ZONE_OFFSET[25] = { 0 };

int process_average_arguments(int argc, char* argv[]) {
    int opt;

    INPUT_LAT = INPUT_LON = -1000.0;
    INPUT_YEAR = INPUT_MONTH = INPUT_HOUR = -1;
    while ((opt = getopt(argc, argv, "r:c:y:m:h:i:f:n:o:p:s:dv")) != -1) {
        switch(opt) {
            /* Primary user arguments prefixed with 'INPUT_' */
            case 'r': /* Latitude */
                INPUT_LAT = atof(optarg);
                break;
            case 'c': /* Longitude */
                INPUT_LON = atof(optarg);
                break;
            case 'y':
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
            case 'o': /* Output file directory (default is same as input) */
                strcpy(output_dir, optarg);
                output_dir_flag = 1;
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

    /* Ensure a 4 digit realistic number */
    if (INPUT_YEAR < 1800 || INPUT_YEAR > 2050) {
        fprintf(stderr, "Year invalid (%d). Accepts values in [1800, 2050] (arbitrary end year)\n", INPUT_YEAR);
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

    if (input_dir_flag != 1 || find_target_file(input_file_path, input_file_name, input_dir, INPUT_YEAR)) {
        fprintf(stderr, "Failed to find target file in input directory\n");
        return 1;
    }

    if (output_dir_flag == 0) {
        printf("No output directory specified; defaulting to input directory\n");
        strcpy(output_dir, input_dir);
    }

    printf("Program proceeding with:\n");
    printf("\tLatitude: %f\n", INPUT_LAT);
    printf("\tLongitude: %f\n", INPUT_LON);
    printf("\tYear | Month | Hour: %d | %d | %d\n", INPUT_YEAR, INPUT_MONTH, INPUT_HOUR);
    printf("\tDisable clobbering = %d\n", disable_clobber);
    printf("\tInput directory = %s\n", input_dir);
    printf("\tInput file = %s\n", input_file_name);
    printf("\tOutput directory = %s\n", output_dir);
    printf("\tOutput file suffix = %s\n", suffix);

    return 0;
}

/* Returns the path and name of a .nc file matching the target year within the directory
 * Modeled off nc_wrapper.c -> process_directory() */
int find_target_file(char* tgt_path, char* tgt_name, const char* input_dir, int target_year) {
    int count;
    char tgt_year[5];
    struct dirent **dir_list;

    if (-1 == (count = scandir(input_dir, &dir_list, NULL, alphasort))) {
        fprintf(stderr, "Failed to open input directory.\n");
        return 1;
    }

    snprintf(tgt_year, 5, "%lu", target_year); // Convert to a string for strstr()

    for (int i = 0; i < count; i++) {
        if (strstr(dir_list[i]->d_name, tgt_year) && strstr(dir_list[i]->d_name, ".nc")) {
            /* TGT_PATH = input_dir + d_name; TGT_NAME = d_name */
            strcpy(tgt_path, input_dir);
            strcat(tgt_path, dir_list[i]->d_name);

            strcpy(tgt_name, dir_list[i]->d_name);
            printf("Found matching file for year: %d -> %s\n", target_year, tgt_path);
            return 0;
        }
    }
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

int find_longitude(size_t* lon_idx, size_t* lon_bin, double tgt_lon) {
    float *lon_data;

    lon_data = vars[VAR_ID_LON].data;

    *lon_bin = 0; // Start at UTC -12 for [0, 7.5] degrees. [352.5, 360.0] is UTC +12.
    for (int i = 0; i < vars[VAR_ID_LON].length; i++) {
        // Update which bin we are analyzing accordingly
        if (fmod(lon_data[i], 15.0) == 7.5) {
            *lon_bin += 1;
        }

        // Break out on match
        if (lon_data[i] == tgt_lon) {
            *lon_idx = i;
            return 0;
        }
    }

    return 1;
}

void load_nc_file(const char* file_path) {

    ___nc_open(file_path, &INPUT_FILE_ID);
    nc_inq(INPUT_FILE_ID, &NUM_DIMS, &NUM_VARS, NULL, NULL);

    /* Output Dimension Information */
    // printf("DIMENSION INFO:\n");
    dims = (Dimension *) malloc(NUM_DIMS * sizeof(Dimension));
    for (int i = 0; i < NUM_DIMS; i++) {
        ___nc_inq_dim(INPUT_FILE_ID, i, &dims[i]);
    }

    /* Output Variable Information */
    // printf("VARIABLE INFO:\n");
    vars = (Variable *) malloc(NUM_VARS * sizeof(Variable));
    for (int i = 0; i < NUM_VARS; i++) {
        ___nc_inq_var(INPUT_FILE_ID, i, &vars[i], dims);
    }

    printf("Loading variable data\n");
    for (int i = 0; i < NUM_VARS; i++) {
        ___nc_get_var_array(INPUT_FILE_ID, i, &vars[i], dims);
    }

    TIME_STRIDE = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length * dims[DIM_ID_LVL].length;
    LVL_STRIDE  = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length;
    LAT_STRIDE  = dims[DIM_ID_LON].length;
}

void prepare_output_file(const char* output_dir, const char* file_name, const char* suffix) {
    char output_file[256];

    strcpy(output_file, output_dir);
    strncat(output_file, file_name, strlen(file_name) - 3); // Copy filename without filetype
    strcat(output_file, suffix);
    strcat(output_file, ".nc");

    ___nc_create(output_file, &OUTPUT_FILE_ID);

    for (int i = 0; i < NUM_DIMS; i++) {
        ___nc_def_dim(OUTPUT_FILE_ID, dims[i]);
    }

    for (int i = 0; i < NUM_VARS; i++) {
        ___nc_def_var(OUTPUT_FILE_ID, vars[i]);
    }
    /* Add chunking for time and special variable, and shuffle and deflate for the special */
    configure_special_chunks(dims, SPECIAL_CHUNKS, HOURS_IN_DAY[0]);
    variable_compression(OUTPUT_FILE_ID, VAR_ID_TIME, HOURS_IN_DAY, VAR_ID_SPECIAL, SPECIAL_CHUNKS);

    nc_enddef(OUTPUT_FILE_ID);

    nc_put_var_double(OUTPUT_FILE_ID, VAR_ID_TIME, vars[VAR_ID_TIME].data);
    for (int i = 0; i < NUM_VARS; i++) {
        if (i != VAR_ID_SPECIAL && i != VAR_ID_TIME) {
            nc_put_var_float(OUTPUT_FILE_ID, vars[i].id, vars[i].data);
        }
    }

    printf("Created output file: %s with id %d (input_id: %d)\n", output_file, OUTPUT_FILE_ID, INPUT_FILE_ID);
}

// TODO: INIT the PREV and NEXT files correctly
int populate_buffer_file(int file_id, Variable** buffer_vars, Dimension** buffer_dims) {
    int num_dims, num_vars;

    nc_inq(file_id, &num_dims, &num_vars, NULL, NULL);

    *buffer_dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
        ___nc_inq_dim(file_id, i, buffer_dims[i]);
    }

    *buffer_vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
        ___nc_inq_var(file_id, i, buffer_vars[i], *buffer_dims);
    }

    printf("Loading variable data for buffer region\n");
    for (int i = 0; i < num_vars; i++) {
        ___nc_get_var_array(file_id, i, buffer_vars[i], *buffer_dims);
    }
}

int gather(double tgt_lon, int tgt_hour, int preceding_days, int total_days) {
    size_t time, lvl, lat, lon, src_idx, dst_idx;
    size_t tgt_lon_idx, tgt_lon_bin, cur_lon_bin;
    size_t starts[vars[VAR_ID_SPECIAL].num_dims];
	size_t counts[vars[VAR_ID_SPECIAL].num_dims];
    int time_start, time_end, pre_diff, post_diff, time_offset;
    float *lon_data, *var_data, *output_data, *prev_data, *next_data, tgt_data;
    Variable output, *next_vars;
    Dimension *prev_dims, *next_dims;

    /* Potentially necessary files if specifying Jan or Dec */
    char prev_file[256];
    char next_file[256];

    /* Check how many time indices equate to one day. Must be 24 */
    DAY_STRIDE  = LEAP_YEAR ? dims[DIM_ID_TIME].length / 366 : dims[DIM_ID_TIME].length / 365;
    if (DAY_STRIDE != 24) {
        fprintf(stderr, "NCAR/NCEP Reanalysis file '%s' variable '%s' must have 24 time indices per day.\
        Broader and finer granularities currently unsupported.\n", input_file_name, vars[VAR_ID_SPECIAL].name);
        return 1;
    }

    /* Calibrate [start, end] range */
    time_start = DAY_STRIDE * preceding_days + tgt_hour; // Find start day idx, adjust for target hour
    time_end   = DAY_STRIDE * (preceding_days + total_days) + tgt_hour; // Adjust end time as wells

    /* Find index and 15 degree bin (time zone) of the input longitude */
    if (find_longitude(&tgt_lon_idx, &tgt_lon_bin, tgt_lon)) {
        fprintf(stderr, "Longitude (%f) invalid or not found in reanalysis variable.\
        Cannot proceed; terminating.\n", tgt_lon);
        return 1;
    }

    /* Adjust the offset for each timezone */
    for (int i = 0; i < 25; i++) {
        TIME_ZONE_OFFSET[i] = TIME_ZONE[i] - TIME_ZONE[tgt_lon_bin];
        printf("tz[%d] = %d\n", i, TIME_ZONE_OFFSET[i]);
    }

    /* Check if the arguments require the preceding file */
    pre_diff = time_start - TIME_ZONE_OFFSET[24];
    post_diff = time_end - TIME_ZONE_OFFSET[0];
    printf("Time pre: %d\n", pre_diff);
    if (pre_diff < 0) {
        printf("Time dimension initial indices less than zero in Eastward direction (likely due to January selection).\n\
        Searching for previous year .nc file in directory\n");
        find_target_file(prev_file_path, prev_file_name, input_dir, INPUT_YEAR - 1);

        ___nc_open(prev_file_path, &PREV_FILE_ID);
        // TODO: prev_vars gets passed in and populated correctly within the function,
        // but on exit, accessing prev_vars we still have no data, hence the segfault

        populate_buffer_file(PREV_FILE_ID, &prev_vars, &prev_dims);
        prev_data = prev_vars[VAR_ID_SPECIAL].data;
    }

    /* Check if require superseding file */
    printf("Time post: %d\n", post_diff);
    if (post_diff > dims[DIM_ID_TIME].length) {
        printf("Time dimension final indices greater than length of .nc file in Westward direction (likely due to December selection).\n\
        Searching for next year .nc file in directory\n");
        find_target_file(next_file_path, next_file_name, input_dir, INPUT_YEAR + 1);

        // ___nc_open(next_file_path, &NEXT_FILE_ID);
        // populate_buffer_file(NEXT_FILE_ID, next_vars, next_dims);
        // next_data = next_vars[VAR_ID_SPECIAL].data;

    }

    // printf("start: %lu, end: %lu, day_stride: %lu, tgt_hour: %d\n", time_start, time_end, DAY_STRIDE, tgt_hour);
    // printf("tgt_lon: %f, tgt_lon_idx: %lu, tgt_lon_bin: %lu\n", tgt_lon, tgt_lon_idx, tgt_lon_bin);

    // Init 'output' Variable struct based off input variable.
    memcpy(&output, &vars[VAR_ID_SPECIAL], sizeof(Variable));
    if (NULL == (output.data = malloc(sizeof(float) * TIME_STRIDE))) {
        fprintf(stderr, "Failed to allocate memory for output data variable\n");
        return 1;
    }

    /* Initialize start indices and counters for each dimension for later call to nc_put_vara_float */
    for (int i = 0; i < vars[VAR_ID_SPECIAL].num_dims; i++) {
        starts[i] = 0;
        counts[i] = dims[vars[VAR_ID_SPECIAL].dim_ids[i]].length;
    }
    counts[0] = 1; // In the Time dimension, we are only doing 1 cube at a time

    /* Start processing */
    lon_data = vars[VAR_ID_LON].data;
    var_data = vars[VAR_ID_SPECIAL].data;
    output_data = output.data;
    for (time = time_start; time < time_end && time < dims[DIM_ID_TIME].length; time++) {
        printf("TIME: %lu\n", time);
        for (lvl = 0; lvl < dims[DIM_ID_LVL].length; lvl++) {
            for (lat = 0; lat < dims[DIM_ID_LAT].length; lat++) {
                for (lon = 0, cur_lon_bin = 0; lon < dims[DIM_ID_LON].length; lon++) {
                    if (fmod(lon_data[lon], 15.0) == 7.5) { // Find our current longitude bin to match with the corr. time zone
                        cur_lon_bin += 1;
                    }

                    time_offset = time - TIME_ZONE_OFFSET[cur_lon_bin];
                    if (time_offset < 0) {
                        time_offset = dims[DIM_ID_TIME].length + time_offset;
                        printf("Adjusted time_offset = %d\n", time_offset);

                        src_idx = ___access_nc_array(time_offset, lvl, lat, lon);
                        tgt_data = prev_data[src_idx];
                    } else if (time_offset > dims[DIM_ID_TIME].length) {
                        time_offset = -1 * time_offset;
                        printf("Adjusted time_offset = %d\n", time_offset);

                        src_idx = ___access_nc_array(time_offset, lvl, lat, lon);
                        tgt_data = next_data[src_idx];
                    } else {
                        src_idx = ___access_nc_array(time_offset, lvl, lat, lon);
                        tgt_data = var_data[src_idx];
                    }


                    // if (lvl == 0 && lat == 0) {
                    //     printf("%lu -> %s[%lu]...[%lu][%lu] = [%lu] = %f -->> [%lu]\n", INPUT_YEAR, vars[VAR_ID_SPECIAL].name,
                    //         time - TIME_ZONE_OFFSET[cur_lon_bin], lat, lon, src_idx, var_data[src_idx], dst_idx);
                    // }
                    dst_idx = ___access_nc_array(0, lvl, lat, lon);
                    output_data[dst_idx] = tgt_data;
                }
            }
        }

        starts[0] = time;

        if ((retval = nc_put_vara_float(OUTPUT_FILE_ID, output.id, starts, counts, output_data)))
            NC_ERR(retval);
    }

    nc_close(INPUT_FILE_ID);
    nc_close(OUTPUT_FILE_ID);

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

    load_nc_file(input_file_path);
    prepare_output_file(output_dir, input_file_name, suffix);



    if (gather(INPUT_LON, INPUT_HOUR, preceding_days, total_days)) {
        printf("Gather function failed\n");
        exit(1);
    }

    printf("Averaging program completed successfully.\n");
    return 0;
}


// TODO: average of each month -> 31 days of this hourly "average" should be converted to 1 value
// --- 744 x 17 x 73 x 144 -> 1 x 17 x 73 x 744.
// First give Yizhe the average for 2 months
// --- Ultimately want the average for each months
// --- Then the averrage of a year from these averages
