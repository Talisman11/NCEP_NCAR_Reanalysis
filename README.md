# Reanalysis

A C program using HDF5/NetCDF4-C libraries to read in NCAR/NCEP Reanalysis data and perform temporal interpolation to output NetCDF files of finer granularity to better match the Reanalysis data with other NASA instruments and data. The NCAR/NCEP data is located at: https://www.esrl.noaa.gov/psd/data/gridded/data.ncep.reanalysis.html . 

Input:
The program requires n+1 NetCDF input files for interpolation to correctly output n files. This is because interpolation of the data
between the final time unit of one year requires the data of the following year's first time unit. These files are also expected to be
multilevel files from NCAR/NCEP, and of the finest granularity available (Daily 4x = 6 hour interval data sampling). Note that this means
surface-level files will not work (single level), as these files are 3D whereas multilevel are 4D. Also ensure that there are no unrelated
files in the directory, such as .txt or .log, or other variables that require separate interpolation. 

Output:
Given a combination of arguments through command line flags, the program will output n interpolated files of the specified temporal
granularity, optionally labeled with a prefix and suffix for clarity/stockkeeping  
