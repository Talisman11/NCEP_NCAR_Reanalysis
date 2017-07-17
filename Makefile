OBJS = netCDF_driver.o ncwrapper.o
CRAY_CC = cc
CC = h5cc
DEBUG = -g
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS += -std=c99 -Wall -O3
NAME = reanalysis

# program_cray: $(OBJS)
# 	$(CRAY_CC) $(OBJS) -o $(NAME) $(NETCDF_FLAGS)

program: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG) $(OBJS) -o $(NAME) $(NETCDF_FLAGS) 

clean:
	rm -f *.o *~ program
