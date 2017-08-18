### Query Program Usage:

```
./query -i [input_directory containing files for one variable]
```

While the program is running, these inputs are expected/acceptable:

- ```year [desired year]``` - Output the currently selected year (no arg), or set the year and load a file
- ```time``` - Output information regarding the Time dimension
- ```level``` - Output information regarding the Level dimension
- ```lat``` - Output information regarding the Latitude dimension
- ```lon``` - Output information regarding the Longitude dimension
- ```[time_i]-[time_f] [lvl_i]-[lvl_f] [lat_i]-[lat_f] [lon_i]-[lon_f]``` - Query a 4D range of values for the variable of the selected file and year
- ``?``` || ```help``` - Outputs additional instructions / assistance
- ```:q``` || ```quit``` - Exits the program gracefully


