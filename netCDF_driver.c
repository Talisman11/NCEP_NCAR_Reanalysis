#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <netcdf.h>

#include "ncwrapper.h"

char input_dir[128] = "";
char output_dir[128] = "";
char prefix[64] = "";
char suffix[64] = ".copy";

char FILE_CUR[256] = "";
char FILE_NEXT[256] = "";
char COPY[256] = "";

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

int primary_function() {
    /* Declare local variables */
    int file_id_cur, file_id_next, copy_id; // nc_open
    int num_dims, num_vars, num_global_attrs, unlimdimidp; // nc_inq

    Variable *vars, var_interp, var_next;
    Dimension *dims, time_interp;

    /* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
    DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;


    /* Open the FILE_CUR */
    ___nc_open(FILE_CUR, &file_id_cur);

    /* FILE_CUR Info */
    nc_inq(file_id_cur, &num_dims, &num_vars, &num_global_attrs, &unlimdimidp);
    // printf("Num dims: %d\t Num vars: %d\t Num global attr: %d\n", num_dims, num_vars, num_global_attrs);


    /* Output Dimension Information */
    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
        ___nc_inq_dim(file_id_cur, i, &dims[i]);
    }

    /* Output Variable Information */
    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
        ___nc_inq_var(file_id_cur, i, &vars[i], dims);
    }

    /******VERIFY*****/

    /* Ensure the dimensions are as expected. Not sure what to do if not. Level will be '2' if present; else Time will be '2'. */
    assert( 0 == strcmp(dims[0].name,"lon") &&
            0 == strcmp(dims[1].name,"lat") &&
            0 == strcmp(dims[2].name,"level") &&
            0 == strcmp(dims[3].name,"time"));

    /* Now we can set stride length global vars */
    TIME_STRIDE = dims[0].length * dims[1].length * dims[2].length;
    LVL_STRIDE  = dims[0].length * dims[1].length;
    LAT_STRIDE  = dims[0].length;

    /* Determine if the next file is the correct file for interpolation */
    ___nc_open(FILE_NEXT, &file_id_next);
    var_next.data = malloc(sizeof(float) * (TIME_STRIDE));

    if (verify_next_file_variable(file_id_cur, file_id_next, &vars[VAR_ID_SPECIAL], &var_next, dims)) {
        printf("Warning: FILE_CUR var '%s' does not match FILE_NEXT var '%s'. Breaking out.\n", 
            vars[VAR_ID_SPECIAL].name, var_next.name);
        return 1;
    }

    /* Now that we know our files are (very likely) the same variable, populate the arrays */
    for (int i = 0; i < num_vars; i++) {
        ___nc_get_var_array(file_id_cur, i, &vars[i], dims);
    }

    /**** Set up and perform temporal interpolation ****/
    ___nc_create(COPY, &copy_id);

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

    temporal_interpolate(copy_id, file_id_next, &vars[VAR_ID_SPECIAL], &var_interp, &var_next, dims);

    clean_up(num_vars, vars, var_interp, dims); 

    nc_close(file_id_cur);
    nc_close(copy_id); // There is a memory leak of 64B from creating and closing the COPY. NetCDF problems...

    return 0; 
}

int main(int argc, char *argv[]) {
    int count;
    struct dirent **dir_list;

    if (process_arguments(argc, argv)) {
        printf("Program usage:\n %s -t [temporal granularity] -i [input directory] -d [delete flag; no args] -o [output directory] -p [output FILE_CUR prefix] -s [output FILE_CUR suffix]\n", argv[0]);
        exit(1);
    }

    // should do some error checking here
    count = scandir(input_dir, &dir_list, NULL, alphasort);

    /* Loop through and grab pairs of files. I.e. A1 A2 A3 B1 B2 goes:
     * [A1 A2] -> [A2 A3] -> [A3 B1] --SKIP!--> [B1 B2] <END> */
    for (int i = 0; i < count - 1; i++) {
        if (invalid_file(dir_list[i]->d_name)) {
            printf("Warning: '%s' invalid name or file type. Examining next pair.\n", dir_list[i]->d_name);
            continue;
        }

        if (invalid_file(dir_list[i + 1]->d_name)) {
            printf("Warning: '%s' invalid name or file type. Skipping pair entries.\n", dir_list[i++]->d_name);
            continue;
        }

        concat_names(dir_list[i], dir_list[i+1]);

        primary_function();

        memset(FILE_CUR, 0, 256);
        memset(FILE_NEXT, 0, 256);
        memset(COPY, 0, 256);
    }

    // Clean up the allocated array of names
    for (int i = 0; i < count; i++) {
        free(dir_list[i]);
    }
    free(dir_list);

    return 0;
}