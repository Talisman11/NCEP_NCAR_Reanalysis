REANALYSIS = netCDF_driver.o ncwrapper.o
QUERY = ncquery.o ncwrapper.o
AVERAGE = avg_reanalysis.o ncwrapper.o
CRAY_CC = cc
NETCDF_FLAGS = -lm -lnetcdf
CFLAGS += -g -O3
R_NAME = reanalysis
Q_NAME = query
average = average

reanalysis: $(REANALYSIS)
	$(CRAY_CC) $(CFLAGS) $(REANALYSIS) -o $(R_NAME) $(NETCDF_FLAGS) 

query: $(QUERY)
	$(CRAY_CC) $(CFLAGS) $(QUERY) -o $(Q_NAME) $(NETCDF_FLAGS)

average: $(AVERAGE)
	$(CRAY_CC) $(CFLAGS) $(AVERAGE) -o $(A_NAME) $(NETCDF_FLAGS)

clean:
	rm -f *.o *~ $(R_NAME) $(Q_NAME) $(A_NAME)
