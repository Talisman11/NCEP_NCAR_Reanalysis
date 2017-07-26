OBJS = netCDF_driver.o ncwrapper.o
CRAY_CC = cc
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS += -g -O3
NAME = reanalysis

program: $(OBJS)
	$(CRAY_CC) $(CFLAGS) $(DEBUG) $(OBJS) -o $(NAME) $(NETCDF_FLAGS) 

clean:
	rm -f *.o *~ program
