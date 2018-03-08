#ifndef __NC_WRAPPER_H_
#define __NC_WRAPPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <netcdf.h>

#define ATTR_PAD 30 // Placeholder number for printing out attributes
#define DAILY_4X 6.0 // Number of hours in each interval per day

#define NC_ERR(err_msg) { printf("File: %s Line: %d - NetCDF Error: %s\n", __FILE__, __LINE__, nc_strerror(err_msg)); exit(EXIT_FAILURE); }
#define ERR(err_no) { printf("File: %s Line: %d - C Error: %s\n", __FILE__, __LINE__, strerror(err_no)); exit(EXIT_FAILURE); }

/* Custom struct based off of NetCDF Variable. Added an internal length parameter */
typedef struct Variable {
	char name[NC_MAX_NAME + 1];		// Variable name
	int id;							// Variable ID
	nc_type type;					// ENUM value from NetCDF - https://www.unidata.ucar.edu/software/netcdf/netcdf-4/newdocs/netcdf-c/NetCDF_002d3-Variable-Types.html
	int num_attrs;					// Number of attributes. Currently unused
	int num_dims;					// Number of dimensions
	int dim_ids[NC_MAX_VAR_DIMS];	// Array of Dimension IDs

	size_t length;					// Length := dim_id[0].length * dim_id[1].length * ... * dim_id[n-1].length
	void* data;						// Corresponding array of aforementioned length
} Variable;

/* Custom struct based off of NetCDF Dimension. No additions */
typedef struct Dimension {
	char name[NC_MAX_NAME + 1];		// Dimension name
	int id;							// Dimension ID. Referenced by Variables
	size_t length;					// Dimension length. Determines size of associated Variables
} Dimension;

/* Determine type of nc_type variable */
char*  ___nc_type(int nc_type);

/* Opens NetCDF file in read-only mode*/
void ___nc_open(const char* file_name, int* ncid);

/* Creates NetCDF file with certain flags set already */
void ___nc_create(char* file_name, int* ncid);

/* Define a NetCDF dimension using struct Dimension */
void ___nc_def_dim(int ncid, Dimension dim);

/* Define a NetCDF variable using struct Variable */
void ___nc_def_var(int ncid, Variable var);

/* Sets variable chunking (size_t array must have size for each dimension) */
void ___nc_def_var_chunking(int ncid, int varid, const size_t* chunksizesp);

/* Enables shuffling and deflation at level 2 */
void ___nc_def_var_deflate(int ncid, int varid);

/* Print info about the dimension and store into struct Dimension */
void ___nc_inq_dim(int ncid, int id, Dimension* dim);

/* Display info about all attributes for the given variable. Does not store attributes */
void ___nc_inq_att(int ncid, int var_id, int num_attrs);

/* Print info about the variable */
void ___nc_inq_var(int ncid, int id, Variable* var, Dimension* dims);

/* Grab Variable data from the .nc file for the struct Variable */
void ___nc_get_var_array(int ncid, int id, Variable* var, Dimension* dims);

/* Calculate a 1D index to access the 1D representation of the 4D array, but requires dimensions in expected order */
size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx);

/* Shorthand string equality function */
int str_eq(char* test, const char* target);

/* Read in command flags and output info to confirm intended operations */
int process_arguments();

/* Verify file is not . or .. directory, does not contain suffix or prefix, and is indeed a .nc file */
int invalid_file(char* name);

/* Build the names for reading and writing the files */
void concat_names(struct dirent* dir_entry_cur, struct dirent* dir_entry_next);

/* Ensure the Variable names of the files are the same to avoid contaminating interpolation data */
int verify_next_file_variable(int next, Variable* var, Variable* var_next, Dimension* dims);

/* Abstraction function for configuring the chunk sizes in the special variable */
void configure_special_chunks(Dimension* dims, size_t* special_chunks, int time_grains);

/* Passes desired variables to compression functions */
void variable_compression(int ncid, int timeid, const size_t* time_chunks, int specialid, const size_t* special_chunks);

/* Abstracted calculation to account for new Time dimension length */
size_t time_dimension_adjust(size_t original_length);

/* Write Time, Level, Lat, Lon values for new file (same values except for Time dim) */
void skeleton_variable_fill(int copy, int num_vars, Variable* vars, Dimension time_interp);

/* Perform interpolation for the desired variable
 * copy 	- file handle of new interpolated file
 * next 	- file handle of NetCDF file that is (intended to be) the following year
 * var 		- pointer to struct holding data of our current input file
 * interp 	- pointer to data for our interpolated Variable
 * var_next - pointer to data of the next input file
 * dims 	- pointer to array of struct Dimensions */
void temporal_interpolate(int copy, int next, Variable* var, Variable* interp, Variable* var_next, Dimension* dims);

/* Frees memory for Variables and Dimensions allocated of input file, and the memory for the interpolated variable*/
void clean_up(int um_vars, Variable* vars, Variable interp, Dimension* dims);

#endif
