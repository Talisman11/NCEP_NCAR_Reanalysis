#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#define FILE_NAME "air.2005.nc"
#define ATTR_PAD 30

/* Used by main() and wrapper functions */
int retval;

/* Print error code and message */
#define ERR(func, err_msg) { printf("Error: %s: %s\n", func, nc_strerror(err_msg)); return 1; }

/* Print info about the dimension */
int ___nc_inq_dim(int file_handle, int id, char* name, size_t* length) {
	if ((retval = nc_inq_dim(file_handle, id, name, length))) 
		ERR("nc_inq_dim", retval);

    printf("\tName: %s\t ID: %d\t Length: %lu\n", name, id, *length);
    return 0;
}


/* Display info about all attributes for the given variable*/
int ___nc_inq_att(int file_handle, int var_id, int num_attrs) {
	char att_name[NC_MAX_NAME + 1];
	nc_type att_type;
	size_t att_length; 

	printf("\tAttribute Information: (name, att_num, nc_type, length)\n");
	for (int att_num = 0; att_num < num_attrs; att_num++) {
		if ((retval = nc_inq_attname(file_handle, var_id, att_num, att_name)))
			ERR("nc_inq_attname", retval);

		if ((retval = nc_inq_att(file_handle, var_id, att_name, &att_type, &att_length)))
			ERR("nc_inq_att", retval);	

		
		/* Double indent, with fixed length padding, in format of: (att_name, att_num, att_type, att_length)
		 * %-*s -> string followed by * spaces for left align
		 * %*s 	-> string preceded by * spaces for right align */
		printf("\t\t %-*s %d\t %d\t %d\n", ATTR_PAD, att_name, att_num, att_type, att_length);
	}
	
	return 0;
}

/* Print info  about the variable. nc_type is ENUM - type details at: 
 * https://www.unidata.ucar.edu/software/netcdf/netcdf-4/newdocs/netcdf-c/NetCDF_002d3-Variable-Types.html */
int ___nc_inq_var(int file_handle, int id, char* name, nc_type* type, int* num_dims, int* dim_ids, int* num_attrs, char** dim_names) {
	if((retval = nc_inq_var(file_handle, id, name, type, num_dims, dim_ids, num_attrs)))
		ERR("nc_inq_var", retval);

    printf("\tName: %s\t ID: %d\t netCDF_type: %d\t Num_dims: %d\t Num_attrs: %d\n", name, id, *type, *num_dims, *num_attrs);
    for (int i = 0; i < *num_dims; i++) {
    	printf("\tDim_id[%d] = %d (%s)\n", i, dim_ids[i], dim_names[dim_ids[i]]);
    }

    // Display attribute data
	___nc_inq_att(file_handle, id, *num_attrs); // should do error checking

	return 0;
}

int main() {
    int nc_file_handle; // nc_open

    int num_dims, num_vars, num_global_attrs, unlimdimidp; // nc_inq

	// nc_inq_dim
    char** dim_names; 
    size_t* dim_lengths;

    // nc_inq_var
    char** var_names; // array of names
    nc_type* var_types;
    int* var_dims; // each var may have dif num of dimensions
    int* var_attrs;
    int** var_dim_ids; // use 2D array so each var (row) can store its dim_ids (columns)


    // Open the file 
    if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &nc_file_handle)))
    	ERR("nc_open", retval);

    // Get some info about the file 
    nc_inq(nc_file_handle, &num_dims, &num_vars, &num_global_attrs, &unlimdimidp);
    printf("Num dims: %d\t Num vars: %d\t Num global attr: %d\n", num_dims, num_vars, num_global_attrs);

    // Now we can loop through the dimension data and see what we have here
    printf("Dimension Information:\n");
    dim_names = (char **) malloc(num_dims * sizeof(char *));
    dim_lengths = (size_t *) malloc(num_dims * sizeof(size_t));
    for (int i = 0; i < num_dims; i++) {
    	dim_names[i] = (char *) malloc((NC_MAX_NAME + 1) * sizeof(char)); // allocate space for names
	    ___nc_inq_dim(nc_file_handle, i, dim_names[i], &dim_lengths[i]); // returns dimension name and length

	    // printf("Dim[%d]: %s, %lu\n", i, dim_names[i], dim_lengths[i]);
    }

    // Same thing for vars
    printf("Variable Information:\n");
    var_names 	= (char **) malloc(num_vars * sizeof(char *));
    var_types 	= (nc_type *) malloc(num_vars * sizeof(nc_type));
    var_dims 	= (int *) 	malloc(num_vars * sizeof(int *));
    var_attrs 	= (int *) 	malloc(num_vars * sizeof(int *));
    var_dim_ids = (int **) 	malloc(num_vars * sizeof(int *));
    for (int i = 0; i < num_vars; i++) {
    	var_names[i] = (char *) malloc((NC_MAX_NAME + 1) * sizeof(char));
    	var_dim_ids[i] = (int *) malloc(NC_MAX_VAR_DIMS * sizeof(int));
    	___nc_inq_var(nc_file_handle, i, var_names[i], &var_types[i], &var_dims[i], var_dim_ids[i], var_attrs, dim_names);
    }

    return 0; 
}