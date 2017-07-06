#ifndef __NCWRAPPER_H_
#define __NCWRAPPER_H_


#define ATTR_PAD 30
#define TEMPORAL_GRANULARITY 15 // new "sample rate" in minutes of output reanalysis file. 
#define NUM_GRAINS (360 / TEMPORAL_GRANULARITY)
#define DAILY_4X 6.0 // hours between intervals in a day

#define NC_ERR(err_msg) { printf("File: %s Line: %d - NetCDF Error: %s\n", __FILE__, __LINE__, nc_strerror(err_msg)); exit(EXIT_FAILURE); }
#define ERR(err_no) { printf("File: %s Line: %d - C Error: %s\n", __FILE__, __LINE__, strerror(err_no)); exit(EXIT_FAILURE); }
typedef struct Variable {
	char name[NC_MAX_NAME + 1];		// Variable name
	int id;							// Variable ID
	nc_type type;					// Int value informing the type of this NetCDF var
	int num_attrs;					// Number of attributes. Currently unused
	int num_dims;					// Number of dimensions
	int dim_ids[NC_MAX_VAR_DIMS];	// Array of Dimension IDs

	size_t length;					// Number of values = dim_id[0].length * dim_id[1].length * ... * dim_id[n-1].length
	void* data;						// Corresponding array of aforementioned length
} Variable;

typedef struct Dimension {
	char name[NC_MAX_NAME + 1];		// Dimension name
	int id;							// Dimension ID. Referenced by Variables
	size_t length;					// Dimension length. Determines size of associated Variables

} Dimension;

/* Determine type of nc_type variable */
char*  ___nc_type(int nc_type);

void ___nc_open(char* file_name, int* file_handle);

void ___nc_create(char* file_name, int* file_handle);

void ___nc_def_dim(int file_handle, Dimension dim);

void ___nc_def_var(int file_handle, Variable var);

/* Print info about the dimension */
void ___nc_inq_dim(int file_handle, int id, Dimension* dim);

/* Display info about all attributes for the given variable*/
void ___nc_inq_att(int file_handle, int var_id, int num_attrs);

/* Print info  about the variable. nc_type is ENUM - type details at: 
 * https://www.unidata.ucar.edu/software/netcdf/netcdf-4/newdocs/netcdf-c/NetCDF_002d3-Variable-Types.html 
 * Returns: 
 * name - variable name
 * type - NC_TYPE value
 * num_dims -*/
// void ___nc_inq_var(int file_handle, int id, char* name, nc_type* type, int* num_dims, int* dim_ids, int* num_attrs, char** dim_names);
void ___nc_inq_var(int file_handle, int id, Variable* var, Dimension* dims);

/* file_handle - integer representing the .nc file 
 * var_id - the variable identifier
 * var_type - NC_TYPE value
 * var_dims - number of dimensions used by variable 	
 * var_dim_ids - array of the dimension IDs used 
 * */
void ___nc_get_var_array(int file_handle, int id, Variable* var, Dimension* dims);

/* Write Time, Level, Lat, Lon values for new file (same values except for Time dim) */
void skeleton_variable_fill(int copy, int num_vars, Variable* vars, Dimension time_interp);

/* Calculate a 1D index to access the 1D representation of the 4D array, but requires dimensions in expected order */
size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx);

/* Perform interpolation for the desired variable */
void temporal_interpolate(int copy, Variable* var, Variable* interp, Dimension* dims);

/* Tests the 1D access function. Loops through 17 x 17 (lat and lon) and a few levels and time slices. No checking atm */
void ___test_access_nc_array(Variable* var);

/* Abstracted calculation to account for new Time dimension length */
size_t time_dimension_adjust(size_t original_length);

void timer_start(clock_t* start);
void timer_end(clock_t* start, clock_t* end);

int str_eq(char* test, const char* target);

#endif