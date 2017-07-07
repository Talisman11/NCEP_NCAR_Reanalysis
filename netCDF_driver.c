#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <netcdf.h>

#include "ncwrapper.h"

#define FILE_NAME "./ncar_files/pressure/air.2001.nc"
#define COPY_NAME "./ncar_files/pressure/air.2001.copy.nc"

int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

int retval;

/* Stride lengths for array access */
size_t TIME_STRIDE;
size_t LVL_STRIDE;
size_t LAT_STRIDE;

// const char argv[] = {"cp"}
const size_t SPECIAL_CHUNKS[] = {1, 1, 73, 144};
const size_t TIME_CHUNK[] = {1};

int main() {
	/* Declare local variables */
    int file, copy; // nc_open

    int num_dims, num_vars, num_global_attrs, unlimdimidp; // nc_inq

    Variable *vars, var_interp;
    Dimension *dims, time_interp;

    /* Initialize our global variables */
    VAR_ID_TIME = VAR_ID_LVL = VAR_ID_LAT = VAR_ID_LON = VAR_ID_SPECIAL = -1;
	DIM_ID_TIME = DIM_ID_LVL = DIM_ID_LAT = DIM_ID_LON = -1;

    // char cwd[1024];
    // getcwd(cwd, sizeof(cwd));

    // printf("Current dir: %s\n", cwd);

    // strcat(cwd, FILE_NAME);

    // printf("File dir: %s\n", cwd);

    // Open the file 
    ___nc_open(FILE_NAME, &file);

    // File Info
    nc_inq(file, &num_dims, &num_vars, &num_global_attrs, &unlimdimidp);
    printf("Num dims: %d\t Num vars: %d\t Num global attr: %d\n", num_dims, num_vars, num_global_attrs);


    /* Output Dimension Information */
    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
	    ___nc_inq_dim(file, i, &dims[i]);
    }

    /* Output Variable Information */
    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
    	___nc_inq_var(file, i, &vars[i], dims);
    }

    /* Populate Variable struct arrays */
    for (int i = 0; i < num_vars; i++) {
    	___nc_get_var_array(file, i, &vars[i], dims);
    }

    /* Ensure the dimensions are as expected. Not sure what to do if not. Level will be '2' if present; else Time will be '2'. */
    assert( 0 == strcmp(dims[0].name,"lon") &&
			0 == strcmp(dims[1].name,"lat") &&
			0 == strcmp(dims[2].name,"level") &&
			0 == strcmp(dims[3].name,"time"));

    /* Now we can set stride length global vars */
    TIME_STRIDE = dims[0].length * dims[1].length * dims[2].length;
	LVL_STRIDE 	= dims[0].length * dims[1].length;
	LAT_STRIDE 	= dims[0].length;

	/**** Set up and perform temporal interpolation ****/
    ___nc_create(COPY_NAME, &copy);

    memcpy(&var_interp, &vars[VAR_ID_SPECIAL], sizeof(Variable));
    memcpy(&time_interp, &dims[DIM_ID_TIME], sizeof(Dimension));

    var_interp.data = malloc(sizeof(float) * (TIME_STRIDE));
    var_interp.length *= NUM_GRAINS; // (var_orig_length) * (NUM_GRAINS) length
    time_interp.length *= NUM_GRAINS; // (time_orig_length) * (NUM_GRAINS) length

    /* Define dimensions based off the input, except for Time (length depends on grains specified) */
    for (int i = 0; i < num_dims; i++) {
    	if (strcmp(dims[i].name, "time") == 0) {
    		___nc_def_dim(copy, time_interp);
    	} else {
    		___nc_def_dim(copy, dims[i]);
    	}
    }

    /* Define variables based off input. All the same, since just dimension length changes */
    for (int i = 0; i < num_vars; i++) {
    	___nc_def_var(copy, vars[i]);
    }

    /* Add chunking for time and special variable, and shuffle and deflate for the special */
    variable_compression(copy, VAR_ID_TIME, TIME_CHUNK, VAR_ID_SPECIAL, SPECIAL_CHUNKS);

    /* Declare we are done defining dims and vars */
    nc_enddef(copy);

    /* Fill out values for all variables asides from the special */
    skeleton_variable_fill(copy, num_vars, vars, time_interp);


	temporal_interpolate(copy, &vars[VAR_ID_SPECIAL], &var_interp, dims);

	clean_up(num_vars, vars, var_interp, dims);	

    nc_close(file);
    nc_close(copy); // There is a memory leak of 64B from creating and closing the copy. NetCDF problems...

    return 0; 
}