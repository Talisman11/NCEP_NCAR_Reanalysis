#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <netcdf.h>

#include "ncwrapper.h"

extern char input_dir[];
extern char output_dir[];
extern char prefix[];
extern char suffix[];

extern char FILE_CUR[];
extern char FILE_NEXT[];
extern char COPY[];

extern int NUM_GRAINS;
extern int TEMPORAL_GRANULARITY;

/* Special indexing variables */
extern int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
extern int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

/* Stride lengths for array access */
extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;

extern size_t SPECIAL_CHUNKS[4];
size_t TIME_CHUNK[] = {1};

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

    /* Now we can set stride length global vars */
    TIME_STRIDE = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length * dims[DIM_ID_LVL].length;
    LVL_STRIDE  = dims[DIM_ID_LON].length * dims[DIM_ID_LAT].length;
    LAT_STRIDE  = dims[DIM_ID_LON].length;

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
    configure_special_chunks(dims, SPECIAL_CHUNKS);

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
        printf("Program usage: %s\n", argv[0]); 
        printf("\t-t [temporal granularity] -i [input directory] -d [delete flag; no args]\n");
        printf("\t-o [output directory] -p [output FILE_CUR prefix] -s [output FILE_CUR suffix] -v [verbose; no args]\n");
        exit(1);
    }

    if (-1 == (count = scandir(input_dir, &dir_list, NULL, alphasort))) {
        printf("Failed to open directory.\n");
        exit(1);
    }

    /* Loop through and grab pairs of files. I.e. A1 A2 A3 B1 B2 goes:
     * [A1 A2] -> [A2 A3] -> [A3 B1] --SKIP!--> [B1 B2] <END> */
    for (int i = 0; i < count - 1; i++) {
        if (invalid_file(dir_list[i]->d_name)) {
            printf("Warning: '%s' invalid name, file type, or no pairable file. Examining next pair.\n", dir_list[i]->d_name);
            continue;
        }

        if (invalid_file(dir_list[i + 1]->d_name)) {
            printf("Warning: '%s' invalid name, file type, or no pairable file. Skipping pair entries.\n", dir_list[i++]->d_name);
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

    printf("Reanalysis program completed successfully.\n");

    return 0;
}