#include "ncwrapper.h"

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

size_t YEAR;
char YEAR_S[5];

size_t time_x, time_y, lvl_x, lvl_y, lat_x, lat_y, lon_x, lon_y;



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
			snprintf(YEAR_S, 5, "%lu", YEAR);
			printf("'YEAR_S' := %s\n", YEAR_S);
		}
		printf("'YEAR' := %lu\n", YEAR); 
		return 1;
	}

	/* TODO: Output Time length and some values */
	if (str_eq(input, "time\n")) {
		printf("'TIME' := %lu length\n", 999);
		return 1;
	}

	/* TODO: Output Level length and [index] -> [value] table */
	if (str_eq(input, "level\n")) {
		printf("'LEVEL' := %lu length\n", 999);
		return 1;
	}

	return 0;
}

int extract_data(char* filename) {
	int file_id, num_vars, num_dims;

	Variable *vars;
	Dimension *dims;


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

    for (size_t time = time_x; time < time_y; time++) {
    	for (size_t lvl = lvl_x; lvl < lvl_y; lvl++) {
    		for (size_t lat = lat_x; lat < lat_y; lat++) {
    			for (size_t lon = lon_x; lon < lon_y; lon++) {
    				size_t index = ___access_nc_array(time, lvl, lat, lon);
    				printf("file[%lu][%lu][%lu][%lu] = [%lu] = %f\n", time, lvl, lat, lon, index, 
    					((float *)vars[VAR_ID_SPECIAL].data)[index]);
    			}
    		}
    	}
    }

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
    	printf("YEAR: %lu, YEAR_S: %s\n", YEAR, YEAR_S);
    	printf("In directory loop: year: %s nc: %s\n", strstr(dir_list[i]->d_name, YEAR_S), strstr(dir_list[i]->d_name, ".nc"));
    	if (strstr(dir_list[i]->d_name, YEAR_S) && strstr(dir_list[i]->d_name, ".nc")) {
    		
    		strcpy(cur_file_path, input_dir);
    		strcat(cur_file_path, dir_list[i]->d_name);

    		extract_data(cur_file_path);
    	}
    }
}

int process_request() {
	if (input_dir_flag && YEAR) {
		process_directory();
	}
}

int main(int argc, char* argv[]) {
	int opt;
	char* input = NULL;
	size_t buffer_size = 0;
	ssize_t char_read;

	int converted;

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

    printf("i: %s, f: %s, o: %s, p: %s, s: %s\n", input_dir, input_file, output_dir, prefix, suffix);

    if (!(input_dir_flag ^ input_file_flag)) {
    	printf("Must set one input directory OR input file\n");
    	exit(1);
    }

	printf("Input desired indice ranges {Time, Level, Lat, Lon} as: a-b c-d e-f g-h\n> ");
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
		process_request();

		/* Reset for next cycle */
		RESET:
		memset(input, 0, buffer_size);
		printf("\n> ");
	}


    free(input);

    return 0;
}