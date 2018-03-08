#include "nc_wrapper.h"

char input_dir[128] = "";
char output_dir[128] = "";
char prefix[64] = "";
char suffix[64] = ".copy";

char FILE_CUR[256] = "";
char FILE_NEXT[256] = "";
char COPY[256] = "";

int NUM_GRAINS = -1;
int TEMPORAL_GRANULARITY = -1;

size_t TIME_STRIDE;
size_t LVL_STRIDE;
size_t LAT_STRIDE;

int VAR_ID_TIME, VAR_ID_LVL, VAR_ID_LAT, VAR_ID_LON, VAR_ID_SPECIAL;
int DIM_ID_TIME, DIM_ID_LVL, DIM_ID_LAT, DIM_ID_LON;

int retval;
int disable_clobber = 0;
int enable_verbose = 0;
int create_mask = NC_SHARE|NC_NETCDF4|NC_CLASSIC_MODEL;

size_t SPECIAL_CHUNKS[4];
// const size_t CHUNK_STANDARD_YEAR[] = {1, 2, 4, 10}; // Multiples of 1460
// const size_t CHUNK_LEAP_YEAR[] = {1, 2, 4, 8}; // Multiples of 1464

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

void ___nc_open(const char* file_name, int* file_handle) {
	if ((retval = nc_open(file_name, NC_NOWRITE, file_handle)))
    	NC_ERR(retval);
}

void ___nc_create(char* file_name, int* file_handle) {
	if (disable_clobber)
		create_mask = create_mask | NC_NOCLOBBER;

	/* Create netCDF4-4 classic model to match original NCAR/NCEP format of .nc files */
	if ((retval = nc_create(file_name, create_mask, file_handle)))
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
}

void ___nc_def_var_chunking(int ncid, int varid, const size_t *chunksizesp) {
	if ((retval = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksizesp)))
		NC_ERR(retval);
}

void ___nc_def_var_deflate(int ncid, int varid) {
	/* Variable deflation with shuffle = 1, deflate = 1, at level 2.
	 * Other levels take longer for variable and insignificant degrees of compression */
	if ((retval = nc_def_var_deflate(ncid, varid, 1, 1, 2)))
		NC_ERR(retval)
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

	if (enable_verbose)
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

	}
}

void ___nc_inq_var(int file_handle, int id, Variable* var, Dimension* dims) {
	if((retval = nc_inq_var(file_handle, id, var->name, &var->type, &var->num_dims, var->dim_ids, &var->num_attrs)))
		NC_ERR(retval);

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

    /* Output attribute data */
	if (enable_verbose) {
	    printf("\tName: %s\t ID: %d\t netCDF_type: %s\t Num_dims: %d\t Num_attrs: %d\n",
	    	var->name, id, ___nc_type(var->type), var->num_dims, var->num_attrs);

	    for (int i = 0; i < var->num_dims; i++) {
	    	printf("\tDim_id[%d] = %d (%s)\n", i, var->dim_ids[i], dims[var->dim_ids[i]].name);
	    }

		___nc_inq_att(file_handle, id, var->num_attrs);
	}
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
	}

	/* Use appropriate nc_get_vara() functions to access*/
	switch(var->type) {
		case NC_FLOAT:
			var->data = malloc(sizeof(float) * var->length);
			retval = nc_get_vara_float(file_handle, id, starts, counts, (float *) var->data);
			break;
		default: // Likely NC_DOUBLE. If not NC_DOUBLE, then some other type.. but never happened yet.
			var->data = malloc(sizeof(double) * var->length);
			retval = nc_get_vara_double(file_handle, id, starts, counts,  (double *) var->data);
	}

	if (retval)	NC_ERR(retval);

	if (enable_verbose)
		printf("\tPopulated array - ID: %d Variable: %s Type: %s Size: %lu, \n", var->id, var->name, ___nc_type(var->type), var->length);
}

size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx) {
	return (TIME_STRIDE * time_idx) + (LVL_STRIDE * lvl_idx) + (LAT_STRIDE * lat_idx) + lon_idx;
}


int str_eq(char* test, const char* target) {
	return strcmp(test, target) == 0;
}

int process_arguments(int argc, char* argv[]) {
	int opt;

    while ((opt = getopt(argc, argv, "t:i:do:s:p:v")) != -1) {
        switch(opt) {
            case 't': /* Temporal Granularity (minutes). Affects NUM_GRAINS */
                TEMPORAL_GRANULARITY = atoi(optarg);
                NUM_GRAINS = 360 / TEMPORAL_GRANULARITY;
                break;
            case 'i': /* Input file directory */
                strcpy(input_dir, optarg);
                break;
            case 'd':
            	disable_clobber = 1;
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
            case 'v':
            	enable_verbose = 1;
            	break;
            default:
            	printf("Bad case!\n");
            	return 1;
        }
    }

    /* Ensure a time value was set */
    if (TEMPORAL_GRANULARITY == -1 || 360 % TEMPORAL_GRANULARITY != 0) {
    	printf("Error: Program requires a valid temporal granularity value that divides evenly into 360.\n");
    	return 1;
    }

    /* Ensure a directory was actually specified */
    if (strlen(input_dir) == 0) {
    	printf("Error: Program requires an input directory.\n");
    	return 1;
    }

    /* If no output directory specified, default to the input directory */
    if (strlen(output_dir) == 0) {
    	strcpy(output_dir, input_dir);
    }

    printf("Program proceeding with:\n");
    printf("\tTemporal Granularity = %d minute slices => %d grains\n", TEMPORAL_GRANULARITY, NUM_GRAINS);
    printf("\tDisable clobbering = %d\n", disable_clobber);
    printf("\tInput directory = %s\n", input_dir);
    printf("\tOutput directory = %s\n", output_dir);
    printf("\tOutput file prefix = %s\n", prefix);
    printf("\tOutput file suffix = %s\n", suffix);

    return 0;
}

int invalid_file(char* name) {
	if (!(strcmp(name, ".")) || !strcmp(name, "..")) {
        return 1;
    }

    // We do not want to read in interpolated files. This case would happen if the output dir == input dir.
    if (((strlen(suffix) > 0 && strstr(name, suffix)) || (strlen(prefix) > 0 && strstr(name, prefix)))) {
    	printf("Presence of prefix or suffix in file indicates interpolated file. Consider using -o to set output directory.\n");
        return 1;
    }

    if (!strstr(name, ".nc")) {
    	printf("Entry '%s' is not a NetCDF file\n", name);
        return 1;
    }

    return 0;
}

void concat_names(struct dirent *dir_entry_cur, struct dirent *dir_entry_next) {

        /* Setup string for FILE_CUR name: [input_dir] + [dir_entry_name] */
        strcpy(FILE_CUR, input_dir);
        strcat(FILE_CUR, dir_entry_cur->d_name);

        /* Same for FILE_NEXT */
        strcpy(FILE_NEXT, input_dir);
        strcat(FILE_NEXT, dir_entry_next->d_name);

        /* Setup string for COPY name [output_dir] + [prefix] + [dir_entry_name] + [suffix] */
        strcpy(COPY, output_dir);
        strcat(COPY, prefix);
        strncat(COPY, dir_entry_cur->d_name, strlen(dir_entry_cur->d_name) - 3);
        strcat(COPY, suffix);
        strcat(COPY, ".nc");

        printf("Input_1 FILE_CUR:  %s\n", FILE_CUR);
        printf("Input_2 FILE_NEXT: %s\n", FILE_NEXT);
        printf("Output  COPY:      %s\n", COPY);
}

int verify_next_file_variable(int next, Variable* var, Variable* var_next, Dimension* dims) {
	int num_dims, num_vars;
	size_t starts[var->num_dims];
	size_t counts[var->num_dims];

	/* Inquire next file to verify its variable matches the current variable */
	nc_inq(next, &num_dims, &num_vars, NULL, NULL);
	___nc_inq_var(next, VAR_ID_SPECIAL, var_next, dims);

	/* Check that the variable names match exactly. If not, break */
	if (strcmp(var->name, var_next->name) != 0) {
		return 1;
	}

	for (int i = 0; i < var->num_dims; i++) {
		starts[i] = 0;
		counts[i] = dims[var->dim_ids[i]].length;
	}
	counts[0] = 1; // We are only pulling the first Time cube of data from the next file for interp.

	/* Since the variables are the same, grab the same ID and put it into our var_next->data */
	if ((retval = nc_get_vara_float(next, var->id, starts, counts, (float *) var_next->data)))
		NC_ERR(retval);

	return 0;
}

void configure_special_chunks(Dimension* dims, size_t* special_chunks, int time_grains) {
    special_chunks[0] = time_grains; // i.e. 360 / 15 minutes = 24 cubes, so use 24 for chunking
    special_chunks[1] = dims[DIM_ID_LVL].length;
    special_chunks[2] = dims[DIM_ID_LAT].length;
    special_chunks[3] = dims[DIM_ID_LON].length;
}

void variable_compression(int ncid, int timeid, const size_t* time_chunks, int specialid, const size_t* special_chunks) {
	/* Set chunking for Time and Special variables in question. */
	___nc_def_var_chunking(ncid, timeid, time_chunks);
	___nc_def_var_chunking(ncid, specialid, special_chunks);

	/* Set deflation for compression on the special Varaible. */
	___nc_def_var_deflate(ncid, specialid);
}

size_t time_dimension_adjust(size_t original_length) {
	return original_length * NUM_GRAINS;
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

void temporal_interpolate(int copy, int next, Variable* orig, Variable* interp, Variable* var_next, Dimension* dims) {
	size_t time, grain, lvl, lat, lon, x_idx, y_idx, interp_idx;
	size_t starts[interp->num_dims];
	size_t counts[interp->num_dims];
	float m; // Slope variable. y = mx + b for linear interpolation

	/* All dimensions run from [0, dim_length) except for Time, which will be [0, 1) */
	for (int i = 0; i < interp->num_dims; i++) {
		starts[i] = 0;
		counts[i] = dims[interp->dim_ids[i]].length;
	}
	counts[0] = 1; // In the Time dimension, we only want to do ONE count at a time.

	float* src = orig->data;
	float* dst = (float *) interp->data;
	float* next_data = var_next->data;

	/* {2010} [Cube], [Cube], ..., [Cube_n] {2011} [Cube_1], [Cube_2], ..., [Cube_n] */
	for (time = 0; time < dims[DIM_ID_TIME].length; time++) {
		for (grain = 0; grain < NUM_GRAINS; grain++) {
			for (lvl = 0; lvl < dims[DIM_ID_LVL].length; lvl++) {
				for (lat = 0; lat < dims[DIM_ID_LAT].length; lat++) {
					for (lon = 0; lon < dims[DIM_ID_LON].length; lon++) {
						x_idx = ___access_nc_array(time, lvl, lat, lon);

						/* Standard case is else branch. if branch has redundant calculation, but maintained for clarity */
						if (time == dims[DIM_ID_TIME].length - 1) {
							y_idx = ___access_nc_array(0, lvl, lat, lon);
							m = next_data[y_idx] - src[x_idx];
						} else {
							y_idx = ___access_nc_array(time + 1, lvl, lat, lon);
							m = src[y_idx] - src[x_idx];
						}

						interp_idx = ___access_nc_array(0, lvl, lat, lon); // Only doing one cube at a time. Each new grain replaces the current cube.
						dst[interp_idx] = m*((float) grain / (float) NUM_GRAINS) + src[x_idx];
					}
				}
			}

			starts[0] = NUM_GRAINS * time + grain;

			if ((retval = nc_put_vara_float(copy, interp->id, starts, counts, dst)))
				NC_ERR(retval);
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
