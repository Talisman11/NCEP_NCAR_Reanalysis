#include "nc_wrapper.h"

#define DIM_PAD 30

int disable_clobber;
int enable_verbose;
int input_dir_flag;
int input_file_flag;
char input_file[256];

extern char input_dir[];
extern char output_dir[];
extern char prefix[];
extern char suffix[];

// write a parser to read the input
// time/[10-40], lvl/[3], lat/[0-73], lon/[0-144]

// write a logic function to binary search through the files to find the correct range

// handle ranges across files (or notify user to query separately)

// logging capability

// output function

/* Special indexing variables */
extern int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
extern int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;


Variable *vars;
Dimension *dims;

size_t YEAR;
char YEAR_S[5];

size_t time_x, time_y, lvl_x, lvl_y, lat_x, lat_y, lon_x, lon_y;


int file_id, num_vars, num_dims;

int digest_file(char* filename) {
    printf("Gathering data from: %s\n", filename);
	___nc_open(filename, &file_id);

	nc_inq(file_id, &num_dims, &num_vars, NULL, NULL);

	/* Output Dimension Information */
    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
        ___nc_inq_dim(file_id, i, &dims[i]);
    }

    /* Output Variable Information */
    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
        ___nc_inq_var(file_id, i, &vars[i], dims);
    }

    for (int i = 0; i < num_vars; i++) {
        ___nc_get_var_array(file_id, i, &vars[i], dims);
    }

	TIME_STRIDE = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length * dims[DIM_ID_LVL].length;
    LVL_STRIDE  = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length;
    LAT_STRIDE  = dims[DIM_ID_LON].length;
}

int process_directory() {
	int count;
	char cur_file_path[256];
    struct dirent **dir_list;

	if (-1 == (count = scandir(input_dir, &dir_list, NULL, alphasort))) {
        printf("Failed to open directory.\n");
        return 1;
    }	

    for (int i = 0; i < count; i++) {
    	if (strstr(dir_list[i]->d_name, YEAR_S) && strstr(dir_list[i]->d_name, ".nc")) {
    		
    		strcpy(cur_file_path, input_dir);
    		strcat(cur_file_path, dir_list[i]->d_name);

			digest_file(cur_file_path);
    		break; // only do the first file...
    	}
    }
}

int print_dimension_var(char* input, char* var_command, int var_id) {
	if (str_eq(input, var_command) && var_id != -1) {
		printf(":= %lu length\n", vars[var_id].length);

		for (size_t i = 0; i < vars[var_id].length; var_id == VAR_ID_TIME ? i += 100 : i++) {
			if (var_id == VAR_ID_TIME) {
				printf("\t[%lu] \t= %f\n", i, ((double *) vars[var_id].data)[i]);
			} else {
				printf("\t[%lu] \t= %f\n", i, ((float *) vars[var_id].data)[i]);
			}
		}
		return 1;
	}
	return 0;
}

int check_commands(char* input) {
	int match_count;

	/* Check for inputs */
	if (str_eq(input, ":q\n")) {
		printf("Received quit signal. Terminating.\n");
		exit(EXIT_SUCCESS);
	}

	/* TODO: Print dialog regarding possible inputs and options*/
	if (str_eq(input, "?\n") || str_eq(input, "help\n")) {
		printf("Help Information:\n");
		return 1;
	}

	// Print out the year, regardless of if sscanf() successful
	if (strstr(input, "year")) {
		if (1 == (match_count = sscanf(input, "year %lu", &YEAR))) {
			snprintf(YEAR_S, 5, "%lu", YEAR); // Read into a string version as well
			process_directory();
		}
		printf("'YEAR' := %lu\n", YEAR); 
		return 1;
	}

	// if (strstr(input, "dir")) {
	// 	if (1 == (match_count = sscanf(input, "dir %s", &input_dir))) {

	// 	}
	// }

	if (print_dimension_var(input, "time\n", VAR_ID_TIME) || print_dimension_var(input, "level\n", VAR_ID_LVL) || 
		print_dimension_var(input, "lat\n", VAR_ID_LAT) || print_dimension_var(input, "lon\n", VAR_ID_LON)) {
		return 1;
	}

	return 0;
}

int extract_data() {
	size_t time, lvl, lat, lon, index;

    float* var_data = vars[VAR_ID_SPECIAL].data;
    for (time = time_x; time < time_y; time++) {
    	for (lvl = lvl_x; lvl < lvl_y; lvl++) {
    		for (lat = lat_x; lat < lat_y; lat++) {
    			for (lon = lon_x; lon < lon_y; lon++) {
    				index = ___access_nc_array(time, lvl, lat, lon);
    				printf("%lu -> %s[%lu][%lu][%lu][%lu] = [%lu] = %f\n", 
    					YEAR, vars[VAR_ID_SPECIAL].name, time, lvl, lat, lon, index, var_data[index]);
    			}
    		}
    	}
    }

}


// int process_request() {
// 	if (input_dir_flag && YEAR) {
// 		process_directory();
// 	}
// }

int main(int argc, char* argv[]) {
	int opt;
	char* input = NULL;
	size_t buffer_size = 0;
	ssize_t char_read;

	int converted;

	/* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;

    while ((opt = getopt(argc, argv, "i:f:o:p:s:dv")) != -1) {
        switch(opt) {
            case 'i': /* Input file directory */
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

    // printf("i: %s, f: %s, o: %s, p: %s, s: %s\n", input_dir, input_file, output_dir, prefix, suffix);

    if (!(input_dir_flag ^ input_file_flag)) {
    	printf("Must set one input directory OR input file\n");
    	exit(1);
    }

	// printf("Input desired indice ranges {Time, Level, Lat, Lon} as: a-b c-d e-f g-h\n> ");
	printf("'year [value]' to select a file.\n> ");
	while((char_read = getline(&input, &buffer_size, stdin)) != -1) {
		if (check_commands(input)) {
			goto RESET;
		}

		converted = sscanf(input, "%lu-%lu %lu-%lu %lu-%lu %lu-%lu", &time_x, &time_y, &lvl_x, &lvl_y, &lat_x, &lat_y, &lon_x, &lon_y);
		if (converted  == EOF || converted < 8) {
			printf("Failed input conversion. Ensure 8 values are provided, paired by hyphens and separated by space.\n");
			goto RESET;
		}

		if (time_x > time_y || lvl_x > lvl_y || lat_x > lat_y || lon_x > lon_y) {
			printf("Ensure the 8 values provided for the ranges are non-zero and are [start, end) format\n");
			goto RESET;
		}

		printf("Found: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", time_x, time_y, lvl_x, lvl_y, lat_x, lat_y, lon_x, lon_y);
		// process_request();
		extract_data();

		/* Reset for next cycle */
		RESET:
		memset(input, 0, buffer_size);
		printf("\n> ");
	}


    free(input);

    return 0;
}