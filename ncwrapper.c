#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#include "ncwrapper.h"




#define NC_ERR(err_msg) { printf("File: %s Line: %d - NetCDF Error: %s\n", __FILE__, __LINE__, nc_strerror(err_msg)); exit(EXIT_FAILURE); }
#define ERR(err_no) { printf("File: %s Line: %d - C Error: %s\n", __FILE__, __LINE__, strerror(err_no)); exit(EXIT_FAILURE); }

#define ATTR_PAD 30
#define TEMPORAL_GRANULARITY 15 // new "sample rate" in minutes of output reanalysis file. 
#define NUM_GRAINS (360 / TEMPORAL_GRANULARITY)

extern int retval;

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;

char*  ___nc_type(int nc_type) {
	switch (nc_type) {
		case NC_BYTE:
			return "NC_BYTE";
		case NC_UBYTE:
			return "NC_UBYTE";
		case NC_CHAR:
			return "NC_CHAR";
		case NC_SHORT:
			return "NC_SHORT";
		case NC_USHORT:
			return "NC_USHORT";
		case NC_INT:
			return "NC_INT";
		case NC_UINT:
			return "NC_UINT";
		case NC_INT64:
			return "NC_INT64";
		case NC_UINT64:
			return "NC_UINT64";
		case NC_FLOAT:
			return "NC_FLOAT";
		case NC_DOUBLE:
			return "NC_DOUBLE";
		case NC_STRING:
			return "NC_STRING";
		default:
			return "TYPE_ERR";
	}
}

void ___nc_open(char* file_name, int* file_handle) {
	if ((retval = nc_open(file_name, NC_NOWRITE, file_handle)))
    	NC_ERR(retval);
}

void ___nc_create(char* file_name, int* file_handle) {
	// if ((retval = nc_create(file_name, NC_NOCLOBBER|NC_SHARE, file_handle)))
	if ((retval = nc_create(file_name, NC_SHARE, file_handle)))
		NC_ERR(retval);
}

// void ___nc_inq_dim(int file_handle, int id, char* name, size_t* length) {
void ___nc_inq_dim(int file_handle, int id, Dimension* dim) {
	if ((retval = nc_inq_dim(file_handle, id, dim->name, &dim->length))) 
		NC_ERR(retval);

	dim->id = id;
    printf("\tName: %s\t ID: %d\t Length: %lu\n", dim->name, dim->id, dim->length);
}

void ___nc_inq_att(int file_handle, int var_id, int num_attrs) {
	char att_name[NC_MAX_NAME + 1];
	nc_type att_type;
	size_t att_length; 

	printf("\tAttribute Information: (name, att_num, nc_type, length)\n");
	for (int att_num = 0; att_num < num_attrs; att_num++) {
		if ((retval = nc_inq_attname(file_handle, var_id, att_num, att_name)))
			NC_ERR(retval);

		if ((retval = nc_inq_att(file_handle, var_id, att_name, &att_type, &att_length)))
			NC_ERR(retval);	

		/* Double indent, with fixed length padding, in format of: (att_name, att_num, att_type, att_length)
		 * %-*s -> string followed by * spaces for left align
		 * %*s 	-> string preceded by * spaces for right align */
		printf("\t\t %-*s %d\t %d\t %lu\n", ATTR_PAD, att_name, att_num, att_type, att_length);

		// Can get Attribute values.. but does not seem very necessary
	}
	
}

// void ___nc_inq_var(int file_handle, int id, char* name, nc_type* type, int* num_dims, int* dim_ids, int* num_attrs, char** dim_names) {
// 	if((retval = nc_inq_var(file_handle, id, name, type, num_dims, dim_ids, num_attrs)))
// 		NC_ERR(retval);

//     printf("\tName: %s\t ID: %d\t netCDF_type: %s\t Num_dims: %d\t Num_attrs: %d\n", 
//     	name, id, ___nc_type(*type), *num_dims, *num_attrs);

//     for (int i = 0; i < *num_dims; i++) {
//     	printf("\tDim_id[%d] = %d (%s)\n", i, dim_ids[i], dim_names[dim_ids[i]]);
//     }

//     // Display attribute data
// 	___nc_inq_att(file_handle, id, *num_attrs);
// }


void ___nc_inq_var(int file_handle, int id, Variable* var, Dimension* dims) {
	if((retval = nc_inq_var(file_handle, id, var->name, &var->type, &var->num_dims, var->dim_ids, &var->num_attrs)))
		NC_ERR(retval);

	var->id = id;

	for (int i = 0; i < var->num_dims; i++) {
		printf("DIMENSION IDS: %d\n", var->dim_ids[i]);
	}

    printf("\tName: %s\t ID: %d\t netCDF_type: %s\t Num_dims: %d\t Num_attrs: %d\n", 
    	var->name, id, ___nc_type(var->type), var->num_dims, var->num_attrs);

    for (int i = 0; i < var->num_dims; i++) {
    	printf("\tDim_id[%d] = %d (%s)\n", i, var->dim_ids[i], dims[var->dim_ids[i]].name);
    }

    // Display attribute data
	___nc_inq_att(file_handle, id, var->num_attrs);
}

void ___nc_get_var_array(int file_handle, int id, Variable* var, Dimension* dims) {
	size_t starts[var->num_dims];
	size_t ends[var->num_dims];
	var->length = 1;

	/* We want to grab all the data in each dimension from [0 - dim_length). Number of elements to read == max_size (hence multiplication) */
	for (int i = 0; i < var->num_dims; i++) {
		starts[i] 	= 0; 
		ends[i] 	= dims[var->dim_ids[i]].length; 
		var->length 	*= dims[var->dim_ids[i]].length;
	}

	var->data 	= malloc(sizeof(void *) * var->length); // now we can allocate the space for our NC array

	/* Make sure memory allocated */
	if (starts == NULL || ends == NULL || var->data == NULL)
		ERR(errno);

	/* Use appropriate nc_get_vara() functions to access*/
	switch(var->type) {
		case NC_FLOAT:
			retval = nc_get_vara_float(file_handle, id, starts, ends, (float *)var->data);
			break;
		case NC_DOUBLE:
			retval = nc_get_vara_double(file_handle, id, starts, ends,  (double *)var->data);
			break;
		default:
			retval = nc_get_vara(file_handle, id, starts, ends, var->data);
	}

	if (retval)	NC_ERR(retval);

	printf("Populated array - ID: %d Variable: %s Type: %s Size: %lu, \n", var->id, var->name, ___nc_type(var->type), var->length);

	for (int i = 0; i < 10; i++) {
		printf("var[%d] = %f\n", i, ((float *) var->data)[i]);
	}

}

size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx) {
	return (TIME_STRIDE * time_idx) + (LVL_STRIDE * lvl_idx) + (LAT_STRIDE * lat_idx) + lon_idx;
}

void temporal_interpolate(Variable* var, int num_dims, int* dim_ids, size_t* dim_lengths) {
	// Assume 4D for now

	float* dest = var->data;
	// loop through all 1460 indices, linear interpolate (slope),
	// for (size_t time = 0; time < dim_lengths[dim_ids[0]]; time++) {
	for (size_t time = 0; time < 1; time++) {
		for (size_t grain = 0; grain < NUM_GRAINS; grain++) {
			for (size_t lvl = 0; lvl < dim_lengths[dim_ids[1]]; lvl++) {
				for (size_t lat = 0; lat < dim_lengths[dim_ids[2]]; lat++) {
					for (size_t lon = 0; lon < dim_lengths[dim_ids[3]]; lon++) {
						size_t x_idx = ___access_nc_array(time, lvl, lat, lon);
						size_t y_idx = ___access_nc_array(time + 1, lvl, lat, lon);

						float m = dest[y_idx] - dest[x_idx];
						float interp = m*((float) grain / (float) NUM_GRAINS) + dest[x_idx];

						printf("[%lu][%lu][%lu][%lu][%lu] \t data[%lu] = %f \t interp = %f \t data[%lu] = %f \n",
							time, lvl, lat, lon, grain, x_idx, (float) dest[x_idx], interp, y_idx, (float) dest[y_idx]);
					}
				}
			}
		}
	}


	// need to use nc_put_var to write data

	// make a pseudo copy: open original in read-only mode, and then read its metadata to create a skeleton copy, where the new data 
	// will be written.


	// write the file.
}

int ___test_access_nc_array(Variable* var) {
	// float* destination = dest; // argument is void* since wasn't sure what type the data would be but it will usually be float*
	float* destination = var->data;
	printf("Var: %s, %d, %d, %d, %lu\n", var->name, var->id, var->num_dims, var->num_attrs, var->length);
	for (size_t time_idx = 0; time_idx < 3; time_idx++) {
		for (size_t lvl_idx = 0; lvl_idx < 3; lvl_idx++) {
			for (size_t lat_idx = 0; lat_idx < 17; lat_idx++) {
				for (size_t lon_idx = 0; lon_idx < 17; lon_idx++) {
					size_t idx = ___access_nc_array(time_idx, lvl_idx, lat_idx, lon_idx);
					printf("var[%lu][%lu][%lu][%lu] = var[%lu] = %f \n", 
						time_idx, lvl_idx, lat_idx, lon_idx, idx, destination[idx]);
				}
			}
		}
	}   

	return 0;
}