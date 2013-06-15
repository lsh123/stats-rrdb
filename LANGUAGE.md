STATS-RRDB LANGUAGE
=========

DDL and Query Language (TCP connections)
---------

* Common types:

** "&lt;name&gt;" - the name of the metric: quoted string that matches the following regexp (a-zA-Z0-9._-)*
** &lt;policy&gt; - the data retention policy (see below)
** &lt;value&gt; - the value (double number)
** &lt;timestamp&gt; - the timestamp (UNIX epoch)
** &lt;interval&gt; - the human readable interval (e.g. "1 sec", "10 mins", etc.)

* The data retention policy is defined as follows:

		&lt;frequency-interval&gt; FOR &lt;duration-interval&gt;, &lt;frequency-interval&gt; FOR &lt;duration-interval&gt;, ...
		
For example, the following data retention policy:

		1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year
	
instructs stats-rrdb to keep 1 sec aggregated data for the previous 1 minute; then 10 sec aggregated data for 
the previous 1 hour; then 10 mins aggregated data for the previous 1 month; and then finaly 30 mins aggregated data 
for the previous 1 year. Total, stats-rrdb will keep (1 min + 1 hour + 1 month + 1 year) worth of data with different
granularity.


* The CREATE METRIC statement defines a new metric with the given name and the given 
data retention policy (see above):

		CREATE METRIC "&lt;name&gt;" KEEP &lt;policy&gt; ;

This statement will generate an error if the metric with the given name already exists.

For example, the following CREATE METRIC statement will create metric "system.cpu.load" with the data retention policy
described above:

		CREATE METRIC "system.cpu.load" KEEP 1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year ;

* The DROP METRIC statement deletes the given metric (including all the data files):

		DROP METRIC "&lt;name&gt;" ;

This statement will generate an error if the metric with the given name does not exist.

For example, the following DROP METRIC statement will erase metric "system.cpu.load":

		DROP METRIC "system.cpu.load" ;

* The UPDATE METRIC statement adds the given double value to the metric at the given timestamp


		UPDATE METRIC "&lt;name&gt;" ADD &lt;value&gt; AT &lt;timestamp&gt; ;

If the specified metric doesn't exist then it will be automatically created using the default 
retention policy specified in the configuration file. The &lt;timestamp&gt; value can be in
the past.

For example, the following UPDATE METRIC statement will update metric "system.cpu.load" with 
value 0.34 recorded at 1371278126 (Sat, 15 Jun 2013 06:35:26 GMT)

		UPDATE METRIC "system.cpu.load" ADD 0.34 AT 1371278126 ;

* The SELECT * FROM METRIC statement returns the stored data between two timestamps, aggregated 
with the given precision:

		SELECT * FROM [METRIC] "&lt;name&gt;" BETWEEN &lt;timestamp1&gt; AND &lt;timestamp2&gt; GROUP BY &lt;interval&gt; ;

The data is returned in CSV format with a header line describing the returned columns and the most 
recent data first. If the metric with the given name does not exist then an error is generated.

For example, the following SELECT FROM METRIC statement will return metric "system.cpu.load" between 1371270000 
(Sat, 15 Jun 2013 04:20:00 GMT) and 1371279000 (Sat, 15 Jun 2013 06:50:00 GMT) aggregated
in 10 secs "buckets":

		SELECT * FROM METRIC "system.cpu.load" BETWEEN 1371270000 AND 1371279000 GROUP BY 10 secs ;

The returned CSV data will look as follows:

		ts,count,sum,avg,stddev,min,max\n
		1371279000,1000,1023,1.023,0.5,0.1,1.6\n
		1371278990,1000,1436,1.436,0.35,0.4,1.9\n
		....

* The SHOW METRIC POLICY statement returns the data retention policy for the specified metric:

		SHOW METRIC POLICY "&lt;name&gt;" ;

If the metric with the given name does not exist then an error is generated.

For example, the following SHOW METRIC POLICY statement will return the data retention policy
for metric "system.cpu.load":

		SHOW METRIC POLICY "system.cpu.load" ;

If the "system.cpu.load" metric was defined as above, then the returned string will look as follows:

		1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year

* The SHOW METRICS statement prints the semi-colon (';') separated list of all the metrics' names 
that match the given pattern:

		SHOW METRICS LIKE "&lt;pattern&gt;";

For example, the following SHOW METRICS statement will return all the metrics' names that have 
"system" as part of their name:

		SHOW METRICS LIKE "system" ;
 
The returned values might look as follows:

		system.cpu.load;system.memory.usage;system.memory.swap;system.io.reads;system.io.writes;
 
 
 
Optimized Language (UDP connections)
---------

