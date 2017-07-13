OBJS = netCDF_driver.o ncwrapper.o
CC = gcc
DEBUG = -g
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS += -std=c99 -Wall -O3
NAME = program

program : $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG) $(OBJS) -o $(NAME) $(NETCDF_FLAGS) 
clean:
	rm -f *.o *~ program