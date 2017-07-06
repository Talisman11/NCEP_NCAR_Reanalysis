#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netcdf.h>

#include "ncwrapper.h"

extern int retval;

extern size_t TIME_STRIDE;
extern size_t LVL_STRIDE;
extern size_t LAT_STRIDE;

extern int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
extern int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;


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

/* TODO: add NC_NOCLOBBER once program is more finished so written files are not overwritten on accident */
void ___nc_create(char* file_name, int* file_handle) {
	/* Create netCDF4-4 classic model to match original NCAR/NCEP format of .nc files */
	if ((retval = nc_create(file_name, NC_SHARE|NC_NETCDF4|NC_CLASSIC_MODEL, file_handle)))
		NC_ERR(retval);

	/* Has no effect? */
	if ((retval = nc_set_fill(*file_handle, NC_NOFILL, NULL)))
		NC_ERR(retval);
}

void ___nc_def_dim(int file_handle, Dimension dim) {
	if ((retval = nc_def_dim(file_handle, dim.name, dim.length, &dim.id)))
		NC_ERR(retval);
}

void ___nc_def_var(int file_handle, Variable var) {
	if ((retval = nc_def_var(file_handle, var.name, var.type, var.num_dims, var.dim_ids, &var.id)))
		NC_ERR(retval);

	/* Set NO_FILL = True */
	if ((retval = nc_def_var_fill(file_handle, var.id, 1, NULL)))
		NC_ERR(retval);
}

void ___nc_put_var_array(int file_handle, Variable var) {
	switch(var.type) {
		case NC_FLOAT:
			retval = nc_put_var_float(file_handle, var.id, var.data);
			break;
		case NC_DOUBLE:
			retval = nc_put_var_double(file_handle, var.id, var.data);
			break;
		default:
			retval = nc_put_var(file_handle, var.id, var.data);
	}

	if (retval) NC_ERR(retval);

}

void ___nc_inq_dim(int file_handle, int id, Dimension* dim) {
	if ((retval = nc_inq_dim(file_handle, id, dim->name, &dim->length))) 
		NC_ERR(retval);

	dim->id = id;
	if (str_eq(dim->name, "time")) 
		DIM_ID_TIME = id;
	if (str_eq(dim->name, "level"))
		DIM_ID_LVL = id;
	if (str_eq(dim->name, "lat"))
		DIM_ID_LAT = id;
	if (str_eq(dim->name, "lon"))
		DIM_ID_LON = id;

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

void ___nc_inq_var(int file_handle, int id, Variable* var, Dimension* dims) {
	if((retval = nc_inq_var(file_handle, id, var->name, &var->type, &var->num_dims, var->dim_ids, &var->num_attrs)))
		NC_ERR(retval);

	// probably needs brackets lol
	var->id = id;
	if (str_eq(var->name, "time"))
		VAR_ID_TIME = id;
	else if (str_eq(var->name, "level"))
		VAR_ID_LVL = id;
	else if (str_eq(var->name, "lat"))
		VAR_ID_LAT = id;
	else if (str_eq(var->name, "lon"))
		VAR_ID_LON = id;
	else
		VAR_ID_SPECIAL = id;

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
	size_t counts[var->num_dims];
	var->length = 1;

	/* We want to grab all the data in each dimension from [0 - dim_length). Number of elements to read == max_size (hence multiplication) */
	for (int i = 0; i < var->num_dims; i++) {
		starts[i] 	= 0; 
		counts[i] 	= dims[var->dim_ids[i]].length; 
		var->length 	*= dims[var->dim_ids[i]].length;
		printf("setting length to : %lu\n", var->length);
	}


	/* Use appropriate nc_get_vara() functions to access*/
	switch(var->type) {
		case NC_FLOAT:
			var->data = malloc(sizeof(float) * var->length);
			retval = nc_get_vara_float(file_handle, id, starts, counts, (float *) var->data);
			break;
		default: // likely NC_DOUBLE. If not NC_DOUBLE, then some other type.. but never happened yet
			var->data = malloc(sizeof(double) * var->length);
			retval = nc_get_vara_double(file_handle, id, starts, counts,  (double *) var->data);
	}

	if (retval)	NC_ERR(retval);

	printf("Populated array - ID: %d Variable: %s Type: %s Size: %lu, \n", var->id, var->name, ___nc_type(var->type), var->length);
}

void skeleton_variable_fill(int copy, int num_vars, Variable* vars, Dimension time_interp) {

	for (int i = 0; i < num_vars; i++) {
		if (i != VAR_ID_TIME && i != VAR_ID_SPECIAL) {
			nc_put_var_float(copy, vars[i].id, vars[i].data);
		}
	}

	float min_elapsed, decimal_conversion;
	double* time_orig = vars[VAR_ID_TIME].data;
	double* time_new = malloc(sizeof(double) * time_interp.length);
	for (size_t i = 0; i < time_interp.length; i++) {

		// divide minutes by 360 (6 hours), then multiply by decimal increment (6.0 for 6 hours in time dimension in file)
		min_elapsed = (i % NUM_GRAINS) * TEMPORAL_GRANULARITY;
		decimal_conversion = DAILY_4X * (min_elapsed / 360.0); 
		time_new[i] = time_orig[i / NUM_GRAINS] +  decimal_conversion;
		// printf("time_orig[%lu / %d] = %f. time_new[%lu] = %f\n", i, NUM_GRAINS, time_orig[i / NUM_GRAINS], i, time_new[i]);
	}

	nc_put_var_double(copy, VAR_ID_TIME, time_new);


}

size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx) {
	return (TIME_STRIDE * time_idx) + (LVL_STRIDE * lvl_idx) + (LAT_STRIDE * lat_idx) + lon_idx;
}


void temporal_interpolate(int copy, Variable* orig, Variable* interp, Dimension* dims) {
	size_t time, grain, lvl, lat, lon, x_idx, y_idx, interp_idx;
	size_t starts[interp->num_dims];
	size_t counts[interp->num_dims];
	float m; // for slope

	// Debugging code
	// printf("var: name: %s id: %d num_dims %d length %lu\n", orig->name, orig->id, orig->num_dims, orig->length);
	// for (int i = 0; i < orig->num_dims; i++) {
	// 	printf("dim_ids[%d] = %d\n", i, orig->dim_ids[i]);
	// }
	// for (int i = 0; i < 4; i++) {
	// 	printf("dim name: %s id: %d length: %lu\n", dims[i].name, dims[i].id, dims[i].length);
	// }

	// all dimensions run from [0, dim_length) except for Time, which is [0, 1)
	for (int i = 0; i < interp->num_dims; i++) {
		starts[i] = 0; 
		counts[i] = dims[interp->dim_ids[i]].length;
	}
	counts[0] = 1; // Time dimension we only want to do ONE count at a time





	float* src = orig->data;
	float* dst = (float *) interp->data;

	// for (time = 0; time < 100; time++) {
	for (time = 0; time < dims[orig->dim_ids[0]].length; time++) {
		for (grain = 0; grain < NUM_GRAINS; grain++) {
			for (lvl = 0; lvl < dims[orig->dim_ids[1]].length; lvl++) {
				for (lat = 0; lat < dims[orig->dim_ids[2]].length; lat++) {
					for (lon = 0; lon < dims[orig->dim_ids[3]].length; lon++) {
						x_idx = ___access_nc_array(time, lvl, lat, lon);
						y_idx = ___access_nc_array(time + 1, lvl, lat, lon);

						interp_idx = ___access_nc_array(0, lvl, lat, lon); // only doing one cube at a time. Each new grain / time replaces the current cube;

						m = src[y_idx] - src[x_idx];
						dst[interp_idx] = m*((float) grain / (float) NUM_GRAINS) + src[x_idx];

						/* Print out debug level each grain */
						// if (grain == 0 && lvl == 0 && lat == 0 && lon == 0) {
						// 	printf("[%lu][%lu][%lu][%lu][%lu] \t data[%lu] = %f \t interp[%lu] = %f \t data[%lu] = %f \n",
						// 	time, grain, lvl, lat, lon, x_idx, src[x_idx], interp_idx, dst[interp_idx], y_idx, src[y_idx]);	
						// }
					}
				}
			}

			starts[0] = NUM_GRAINS * time + grain;

			if ((retval = nc_put_vara_float(copy, interp->id, starts, counts, dst)))
				NC_ERR(retval);

			// printf("starts: %lu %lu %lu %lu ends: %lu %lu %lu %lu\n", starts[0], starts[1], starts[2], starts[3],
			// 	ends[0], ends[1], ends[2], ends[3]);
		}
	}
}

size_t time_dimension_adjust(size_t original_length) {
	return original_length * NUM_GRAINS;
}

void ___test_access_nc_array(Variable* var) {
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
}

void clean_up(int num_vars, Variable* vars, Variable interp, Dimension* dims) {
    for (int i = 0; i < num_vars; i++) {
		free(vars[i].data);
	}
	free(vars);
	free(dims);

	free(interp.data);
}


void timer_start(clock_t* start) {
	*start = clock();
}

void timer_end(clock_t* start, clock_t* end) {
	*end = clock() - *start;

	int msec = *end * 1000 / CLOCKS_PER_SEC;
	printf("Time: %d seconds %d milliseconds", msec/1000, msec%1000);
}

int str_eq(char* test, const char* target) {
	return strcmp(test, target) == 0;
}
