#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <netcdf.h>
#include <dirent.h>

#include "ncwrapper.h"


int retval;

char input_dir[128] = "";
char output_dir[128] = "";
char prefix[64] = "";
char suffix[64] = ".copy";

char FILE_PATH[256] = "";
char COPY_PATH[256] = "";

int NUM_GRAINS = -1;
int TEMPORAL_GRANULARITY = -1;

/* Special indexing variables */
int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

/* Stride lengths for array access */
size_t TIME_STRIDE;
size_t LVL_STRIDE;
size_t LAT_STRIDE;

const size_t SPECIAL_CHUNKS[] = {1, 1, 73, 144};
const size_t TIME_CHUNK[] = {1};


int primary_function(){
    /* Declare local variables */
    int file_id, copy_id; // nc_open

    Variable *vars, var_interp;
    Dimension *dims, time_interp;

    /* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;


    /* Open the FILE_PATH */
    ___nc_open(FILE_PATH, &file_id);

    /* FILE_PATH Info */
    int num_dims, num_vars, num_global_attrs, unlimdimidp; // nc_inq
    nc_inq(file_id, &num_dims, &num_vars, &num_global_attrs, &unlimdimidp);
    printf("Num dims: %d\t Num vars: %d\t Num global attr: %d\n", num_dims, num_vars, num_global_attrs);


    /* Output Dimension Information */
    printf("Dimension Information:\n");
    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
        ___nc_inq_dim(file_id, i, &dims[i]);
    }

    /* Output Variable Information */
    printf("Variable Information:\n");
    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
        ___nc_inq_var(file_id, i, &vars[i], dims);
    }

    /* Populate Variable struct arrays */
    for (int i = 0; i < num_vars; i++) {
        ___nc_get_var_array(file_id, i, &vars[i], dims);
    }

    /* Ensure the dimensions are as expected. Not sure what to do if not. Level will be '2' if present; else Time will be '2'. */
    assert( 0 == strcmp(dims[0].name,"lon") &&
            0 == strcmp(dims[1].name,"lat") &&
            0 == strcmp(dims[2].name,"level") &&
            0 == strcmp(dims[3].name,"time"));

    /* Now we can set stride length global vars */
    TIME_STRIDE = dims[0].length * dims[1].length * dims[2].length;
    LVL_STRIDE  = dims[0].length * dims[1].length;
    LAT_STRIDE  = dims[0].length;

    /**** Set up and perform temporal interpolation ****/
    ___nc_create(COPY_PATH, &copy_id);

    memcpy(&var_interp, &vars[VAR_ID_SPECIAL], sizeof(Variable));
    memcpy(&time_interp, &dims[DIM_ID_TIME], sizeof(Dimension));

    var_interp.data = malloc(sizeof(float) * (TIME_STRIDE));
    var_interp.length *= NUM_GRAINS; // (var_orig_length) * (NUM_GRAINS) length
    time_interp.length *= NUM_GRAINS; // (time_orig_length) * (NUM_GRAINS) length

    /* Define dimensions based off the input, except for Time (length depends on grains specified) */
    for (int i = 0; i < num_dims; i++) {
        if (strcmp(dims[i].name, "time") == 0) {
            ___nc_def_dim(copy_id, time_interp);
        } else {
            ___nc_def_dim(copy_id, dims[i]);
        }
    }

    /* Define variables based off input. All the same, since just dimension length changes */
    for (int i = 0; i < num_vars; i++) {
        ___nc_def_var(copy_id, vars[i]);
    }

    /* Add chunking for time and special variable, and shuffle and deflate for the special */
    variable_compression(copy_id, VAR_ID_TIME, TIME_CHUNK, VAR_ID_SPECIAL, SPECIAL_CHUNKS);

    /* Declare we are done defining dims and vars */
    nc_enddef(copy_id);

    /* Fill out values for all variables asides from the special */
    skeleton_variable_fill(copy_id, num_vars, vars, time_interp);

    temporal_interpolate(copy_id, &vars[VAR_ID_SPECIAL], &var_interp, dims);

    clean_up(num_vars, vars, var_interp, dims); 

    nc_close(file_id);
    nc_close(copy_id); // There is a memory leak of 64B from creating and closing the COPY_PATH. NetCDF problems...

    return 0; 
}

void concat_names(struct dirent *dir_entry) {

        /* Setup string for FILE_PATH name [input_dir] + [dir_entry_name] */
        strcpy(FILE_PATH, input_dir);
        strcat(FILE_PATH, dir_entry->d_name);

        /* Setup string for COPY_PATH name [output_dir] + [prefix] + [dir_entry_name] + [suffix] */
        strcpy(COPY_PATH, output_dir);
        strcat(COPY_PATH, prefix);
        strncat(COPY_PATH, dir_entry->d_name, strlen(dir_entry->d_name) - 3);
        strcat(COPY_PATH, suffix);
        strcat(COPY_PATH, ".nc");

        printf("Input FILE_PATH: %s\n", FILE_PATH);
        printf("Output FILE_PATH: %s\n", COPY_PATH);
           
}
int main(int argc, char *argv[]) {

    struct dirent *dir_entry;
    DIR *dir;

    if (process_arguments(argc, argv)) {
        printf("Program usage:\n %s -t [temporal granularity] -i [input directory] -d [delete flag; no args] -o [output directory] -p [output FILE_PATH prefix] -s [output FILE_PATH suffix]\n", argv[0]);
        exit(1);
    }

    dir = opendir(input_dir);
    if (dir == NULL) {
        printf("Error: Cannot open directory: '%s'\n", input_dir);
        return 1;
    }

    /* Loop through all valid files in the directory */
    while ((dir_entry = readdir(dir)) != NULL) {
        printf("Name: %s\n", dir_entry->d_name);
        /* Skip '.' and '..' directories, and files containing the suffix or prefix */
        if ((!strcmp(dir_entry->d_name, ".") || !strcmp(dir_entry->d_name, ".."))) continue;

        if (strlen(suffix) > 0 && strstr(dir_entry->d_name, suffix)) continue;
        if (strlen(prefix) > 0 && strstr(dir_entry->d_name, prefix)) continue;
        if (!strstr(dir_entry->d_name, ".nc")) continue;

        concat_names(dir_entry);

        primary_function();

        memset(FILE_PATH, 0, 256);
        memset(COPY_PATH, 0, 256);
    }

    closedir(dir);
    
    return 0;
}