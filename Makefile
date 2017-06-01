OBJS = netCDF_driver.o
CC = h4cc
DEBUG = -g
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS = -Wall $(DEBUG)

program : $(OBJS)
	$(CC) $(DEBUG) $(NETCDF_FLAGS) $(LFLAGS) $(OBJS) -o program
clean:
	rm -f *.o *~ program