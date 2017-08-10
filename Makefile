OBJS = netCDF_driver.o ncwrapper.o
QUERY = ncquery.o
CRAY_CC = cc
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS += -g -O3
NAME = reanalysis

program: $(OBJS)
	$(CRAY_CC) $(CFLAGS) $(OBJS) -o $(NAME) $(NETCDF_FLAGS) 

query: $(QUERY)
	$(CRAY_CC) $(CFLAGS) $(QUERY) -o query $(NETCDF_FLAGS)

clean:
	rm -f *.o *~ program
