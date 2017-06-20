#ifndef __NCWRAPPER_H_
#define __NCWRAPPER_H_

/* Determine type of nc_type variable */
char*  ___nc_type(int nc_type);

void ___nc_open(char* file_name, int* file_handlle);

/* Print info about the dimension */
void ___nc_inq_dim(int file_handle, int id, char* name, size_t* length);

/* Display info about all attributes for the given variable*/
void ___nc_inq_att(int file_handle, int var_id, int num_attrs);

/* Print info  about the variable. nc_type is ENUM - type details at: 
 * https://www.unidata.ucar.edu/software/netcdf/netcdf-4/newdocs/netcdf-c/NetCDF_002d3-Variable-Types.html 
 * Returns: 
 * name - variable name
 * type - NC_TYPE value
 * num_dims -*/
void ___nc_inq_var(int file_handle, int id, char* name, nc_type* type, int* num_dims, int* dim_ids, int* num_attrs, char** dim_names);

/* file_handle - integer representing the .nc file 
 * var_id - the variable identifier
 * var_type - NC_TYPE value
 * var_dims - number of dimensions used by variable 
 * var_dim_ids - array of the dimension IDs used 
 * */
void ___nc_get_var_array(int file_handle, int id, char* name, int type, int num_dims, int* dim_ids, size_t* dim_lengths, void** dest, size_t* var_size);

/* Calculate a 1D index to access the 1D representation of the 4D array, but requires dimensions in expected order */
size_t ___access_nc_array(size_t time_idx, size_t lvl_idx, size_t lat_idx, size_t lon_idx);

/* Tests the 1D access function. Loops through 17 x 17 (lat and lon) and a few levels and time slices. No checking atm */
int ___test_access_nc_array();

#endif