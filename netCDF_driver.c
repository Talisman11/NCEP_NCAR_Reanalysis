#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <netcdf.h>

#include "ncwrapper.h"

#define FILE_NAME "./ncar_files/pressure/air.2001.nc"
#define COPY "./ncar_files/pressure/air.2001.copy.nc"

/* Used by main() and wrapper functions */
int retval;

/* Stride lengths for array access */
size_t TIME_STRIDE;
size_t LVL_STRIDE;
size_t LAT_STRIDE;

/* Print error code and message */
// #define NC_ERR(func, err_msg) { printf("NetCDF Error: %s: %s\n", func, nc_strerror(err_msg)); return 1; }
// #define ERR(func, err_no) { printf("C Error: %s: %s\n", func, strerror(err_no)); return 1; }

// #define TYPE(type) { printf("%s\n", type); return type; }
#define TYPE(type) { return type; }


int main() {
    int file, copy; // nc_open

    int num_dims, num_vars, num_global_attrs, unlimdimidp; // nc_inq

	// nc_inq_dim
    // char** dim_names; 
    // size_t* dim_lengths;

    // nc_inq_var
    // char** var_names; // array of names
    // nc_type* var_types;
    // int* var_num_dims; // each var may have dif num of dimensions
    // int* var_attrs;
    // int** var_dim_ids; // use 2D array so each var (row) can store its dim_ids (columns)

    Variable* vars;
    Dimension* dims;

    // char cwd[1024];
    // getcwd(cwd, sizeof(cwd));

    // printf("Current dir: %s\n", cwd);

    // strcat(cwd, FILE_NAME);

    // printf("File dir: %s\n", cwd);

    // Open the file 
    ___nc_open(FILE_NAME, &file);

    ___nc_create(COPY, &copy);

    // File Info
    nc_inq(file, &num_dims, &num_vars, &num_global_attrs, &unlimdimidp);
    printf("Num dims: %d\t Num vars: %d\t Num global attr: %d\n", num_dims, num_vars, num_global_attrs);

    printf("Dimension Information:\n");
    // dim_names 	= (char **) malloc(num_dims * sizeof(char *));
    // dim_lengths = (size_t *) malloc(num_dims * sizeof(size_t));

    dims = (Dimension *) malloc(num_dims * sizeof(Dimension));
    for (int i = 0; i < num_dims; i++) {
    	// dim_names[i] = (char *) malloc((NC_MAX_NAME + 1) * sizeof(char)); // allocate space for names
	    // ___nc_inq_dim(file, i, dim_names[i], &dim_lengths[i]); // returns dimension name and length

	    ___nc_inq_dim(file, i, &dims[i]);
    }

    printf("Variable Information:\n");
    // var_names 		= (char **) 	malloc(num_vars * sizeof(char *));
    // var_types 		= (nc_type *) 	malloc(num_vars * sizeof(nc_type));
    // var_num_dims 	= (int *) 		malloc(num_vars * sizeof(int *));
    // var_attrs 		= (int *) 		malloc(num_vars * sizeof(int *));
    // var_dim_ids 	= (int **) 		malloc(num_vars * sizeof(int *));

    vars = (Variable *) malloc(num_vars * sizeof(Variable));
    for (int i = 0; i < num_vars; i++) {
    	// var_names[i] = (char *) malloc((NC_MAX_NAME + 1) * sizeof(char));
    	// var_dim_ids[i] = (int *) malloc(NC_MAX_VAR_DIMS * sizeof(int));
    	// ___nc_inq_var(file, i, var_names[i], &var_types[i], &var_num_dims[i], var_dim_ids[i], var_attrs, dim_names);

    	___nc_inq_var(file, i, &vars[i], dims);

    }

    // void* var_data[num_vars];
    // size_t var_size[num_vars];
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

	/* Can start accessing wherever in the 1D array now */
	___test_access_nc_array(&vars[4]);

	// temporal_interpolate(var_data[4], var_num_dims[4], var_dim_ids[4], dim_lengths);

	nc_close(file);
	nc_close(copy);

    return 0; 
}