#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

#include "ncwrapper.h"

extern int retval;

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;

#define NC_ERR(func, err_msg) { printf("NetCDF Error: %s: %s\n", func, nc_strerror(err_msg)); return 1; }
#define ERR(func, err_no) { printf("C Error: %s: %s\n", func, strerror(err_no)); return 1; }

#define ATTR_PAD 30

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

int ___nc_open(char* file_name, int* file_handle) {
	if ((retval = nc_open(file_name, NC_NOWRITE, file_handle)))
    	NC_ERR("nc_open", retval);
    return 0;
}

int ___nc_inq_dim(int file_handle, int id, char* name, size_t* length) {
	if ((retval = nc_inq_dim(file_handle, id, name, length))) 
		NC_ERR("nc_inq_dim", retval);

    printf("\tName: %s\t ID: %d\t Length: %lu\n", name, id, *length);
    return 0;
}

int ___nc_inq_att(int file_handle, int var_id, int num_attrs) {
	char att_name[NC_MAX_NAME + 1];
	nc_type att_type;
	size_t att_length; 

	printf("\tAttribute Information: (name, att_num, nc_type, length)\n");
	for (int att_num = 0; att_num < num_attrs; att_num++) {
		if ((retval = nc_inq_attname(file_handle, var_id, att_num, att_name)))
			NC_ERR("nc_inq_attname", retval);

		if ((retval = nc_inq_att(file_handle, var_id, att_name, &att_type, &att_length)))
			NC_ERR("nc_inq_att", retval);	

		/* Double indent, with fixed length padding, in format of: (att_name, att_num, att_type, att_length)
		 * %-*s -> string followed by * spaces for left align
		 * %*s 	-> string preceded by * spaces for right align */
		printf("\t\t %-*s %d\t %d\t %lu\n", ATTR_PAD, att_name, att_num, att_type, att_length);

		// Can get Attribute values.. but does not seem very necessary
	}
	
	return 0;
}

int ___nc_inq_var(int file_handle, int id, char* name, nc_type* type, int* num_dims, int* dim_ids, int* num_attrs, char** dim_names) {
	if((retval = nc_inq_var(file_handle, id, name, type, num_dims, dim_ids, num_attrs)))
		NC_ERR("nc_inq_var", retval);

	// char* type_name[NC_MAX_NAME + 1];
    printf("\tName: %s\t ID: %d\t netCDF_type: %s\t Num_dims: %d\t Num_attrs: %d\n", 
    	name, id, ___nc_type(*type), *num_dims, *num_attrs);

    for (int i = 0; i < *num_dims; i++) {
    	printf("\tDim_id[%d] = %d (%s)\n", i, dim_ids[i], dim_names[dim_ids[i]]);
    }

    // Display attribute data
	___nc_inq_att(file_handle, id, *num_attrs);

	return 0;
}

int ___nc_get_var_array(int file_handle, int id, char* name, int type, int num_dims, int* dim_ids, size_t* dim_lengths, void** dest) {
	size_t max_size = 1;
	size_t* starts;
	size_t* ends;

	for (int i = 0; i < num_dims; i++) {
		max_size *= dim_lengths[dim_ids[i]];
	}

	*dest 	= malloc(sizeof(void *) * max_size); // now we can allocate the space for it
	starts 	= malloc(sizeof(size_t) * num_dims); // start indices for each dim (want 0)
	ends 	= malloc(sizeof(size_t) * num_dims); // end indices for each dim (want all data, so the length)

	if (starts == NULL || ends == NULL || dest == NULL)
		ERR("___nc_get_var_array", errno);

	for (int i = 0; i < num_dims; i ++) {
		starts[i] = 0; 
		ends[i] = dim_lengths[dim_ids[i]]; 
	}

	if ((retval = nc_get_vara(file_handle, id, starts, ends, *dest))) 
		NC_ERR("nc_get_vara", retval);

	printf("Populated array - ID: %d Variable: %s Size: %lu \n", id, name, max_size);

	return 0;
}

size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx) {
	return (TIME_STRIDE * time_idx) + (LVL_STRIDE * lvl_idx) + (LAT_STRIDE * lat_idx) + lon_idx;
}

int ___test_access_nc_array(void* dest) {
	float* destination = dest;
	for (size_t time_idx = 0; time_idx < 3; time_idx++) {
		for (size_t lvl_idx = 0; lvl_idx < 3; lvl_idx++) {
			for (size_t lat_idx = 0; lat_idx < 17; lat_idx++) {
				for (size_t lon_idx = 0; lon_idx < 17; lon_idx++) {
					size_t idx = ___access_nc_array(time_idx, lvl_idx, lat_idx, lon_idx);
					printf("dest[%lu][%lu][%lu][%lu] = dest[%lu] = %f\n", 
						time_idx, lvl_idx, lat_idx, lon_idx, idx, destination[idx]);
				}
			}
		}
	}   

	return 0;
}