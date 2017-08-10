#include "ncwrapper.h"

int disable_clobber;
int input_dir_flag;
int input_file_flag;

char input_dir[128] = "";
char input_file[128] = "";
char output_dir[128] = "";
char prefix[64] = "";
char suffix[64] = ".copy";

// write a parser to read the input
// time/[10-40], lvl/[3], lat/[0-73], lon/[0-144]

// write a logic function to binary search through the files to find the correct range

// handle ranges across files (or notify user to query separately)

// logging capability

// output function

int process_directory() {
	int count;
    struct dirent **dir_list;

	if (-1 == (count = scandir(input_dir, &dir_list, NULL, alphasort))) {
        printf("Failed to open directory.\n");
        exit(1);
    }	
}


int main(int argc, char* argv[]) {
	int opt;
	char* input = NULL;
	size_t buffer_size = 0;
	ssize_t char_read;

	int converted;
	size_t time_x, time_y, lvl_x, lvl_y, lat_x, lat_y, lon_x, lon_y;


    while ((opt = getopt(argc, argv, "i:f:o:p:s:d")) != -1) {
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
            default:
            	printf("Bad case!\n");
            	return 1;
        }
    }

    printf("i: %s, f: %s, o: %s, p: %s, s: %s\n", input_dir, input_file, output_dir, prefix, suffix);

    if (input_dir_flag ^ input_file_flag) {
    	printf("Must set one input directory OR input file\n");
    	exit(1);
    }

	printf("Input desired indice ranges {Time, Level, Lat, Lon} as: a-b c-d e-f g-h\n> ");
	while((char_read = getline(&input, &buffer_size, stdin)) != -1) {
		/* Check for inputs */
  		if (strcmp(input, ":q\n") == 0) {
  			printf("Received quit signal. Terminating.\n");
  			exit(EXIT_SUCCESS);
  		}

  		if (strcmp(input, "time\n") == 0) {
  		}

  		if (strstr(input, "year")) {
  			
  		}

		converted = sscanf(input, "%lu-%lu %lu-%lu %lu-%lu %lu-%lu", &time_x, &time_y, &lvl_x, &lvl_y, &lat_x, &lat_y, &lon_x, &lon_y);
		if (converted  == EOF || converted < 8) {
			printf("Failed input conversion. Ensure 8 values are provided, paired by hyphens and separated by space.\n\n> ");
			continue;
		}

		if (time_x > time_y || lvl_x > lvl_y || lat_x > lat_y || lon_x > lon_y) {
			printf("Ensure the 8 values provided for the ranges are non-zero and are [start, end) format\n");
			continue;
		}

		printf("Found: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", time_x, time_y, lvl_x, lvl_y, lat_x, lat_y, lon_x, lon_y);

		/* Reset for next cycle */
		memset(input, 0, buffer_size);
		printf("> ");
	}

    // printf("i XOR f: %d\n", input_dir[0] ^ input_file[0]);

    free(input);

    return 0;
}