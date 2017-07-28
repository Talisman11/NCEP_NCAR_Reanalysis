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
granularity, optionally labeled with a prefix and suffix for clarity/stockkeeping. Files preserve only the necessary data from the input file, i.e. values for each variable (time, lvl, lat, lon), but no attribute information.

Execution Setup:
1) SSH into Blue Waters
2) git clone
3) module load cray-hdf5
4) module load cray-netcdf/4.4.1.1
5) make
5) View/Modify and qsub curl.pbs to download data // modify range or path on NCAR/NCEP FTP server to download different files
6) View/Modify and qsub reanalysis.pbs with desired arguments

Example execution:
./reanalysis -t 90 -i ./ncar\_files/pressure/ -o ./ncar\_files/pressure\_interpolated/ -p interp -s \_90 -v -d
Runs the reanalysis program found in the current directory for granularity of 90 minutes from the pressure/ sub-subdirectory, outputting the files into the pressure\_interpolated/ sub-subdirectory, prefixing the files with string "interp" and suffixing them with "\_90", generating verbose output and disabling the clobbering (prevents program from overwriting any previously generated files with same prefix or suffix). Program usage requires -t and -i to be set; default output directory is the input directory (this can get messy after a few iterations; recommend to use GLOBUS or other file transfer to move the files out), with suffix ".copy".

HDFView Setup (Optional - Requires HDFView to be installed on local machine):
1) SFTP an HDFView directory or .tar file from local machine to BW directory
  - "put -r HDFView/" command fails if the directory does not exist on your BW directory
  - "mkdir HDFView", then run the "put" command again. Generates some recursion errors but can be cleaned up and used
2) Run .sh file, follow prompts
3) Verify X11 DISPLAY variable has been set:
  - "echo $DISPLAY"
  - Lack of output indicates it has not been set
4) "export DISPLAY=:0.0" to set DISPLAY var
5) "ssh -X" into Blue Waters
6) "sh hdfview.sh" to confirm it is working


