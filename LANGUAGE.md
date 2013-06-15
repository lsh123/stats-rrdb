STATS-RRDB LANGUAGES
=========

Query Language (TCP connections)
---------

The query language used for TCP connections is based on SQL and provides full
control over the Stats-RRDB server:
- Create/delete/view metadata for metrics collected by the server;
- Update metrics data;
- Query metrics data.

### Token Types

The following are the tokens types used in the language statements:

* 	The metric name "&lt;name&gt;" is a quoted string that starts with a letter 
	(a-zA-Z) and contains only letters, digits or "._-" characters (a-zA-Z0-9._-)

* 	The interval &lt;interval&gt; is a time interval (duration) in the folowing format:

		<integer-value> <units>
		
	where &lt;units&rt; is one of the following:
	
		sec secs min mins hour hours day days week weeks month months year years

	For example, the "15 secs" interval defines a 15 seconds interval :)

* 	The data retention policy &lt;policy&gt; is defined as follows:

		<frequency-interval> FOR <duration-interval>, <frequency-interval> FOR <duration-interval>, ...
		
	For example, the following data retention policy:

		1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year
	
	instructs stats-rrdb to keep 1 sec aggregated data for the previous 1 minute;
	then 10 sec aggregated data for the previous 1 hour; then 10 mins aggregated 
	data for the previous 1 month; and then finaly 30 mins aggregated data 
	for the previous 1 year. Total, stats-rrdb will keep:
		
		(1 min + 1 hour + 1 month + 1 year) 
	
	worth of data with different granularity.

*	The timestamp &lt;timestamp&gt; is the number of seconds from Jan 1st, 1970 00:00:00 UTC (Unix epoch time). 

*	The metric value &lt;value&gt; is just a *double* value.	
	

### Statements

The query language statements are always close with a semi-colon (;). The statements
allow user to create/view/delete metrics, update metrics with data, and query metrics data.

#### Create/view/delete statements

* 	The CREATE METRIC statement defines a new metric with the given name and 
	the given data retention policy (see above):

		CREATE METRIC "<name>" KEEP <policy> ;

	This statement will generate an error if the metric with the given name 
	already exists.

	For example, the following CREATE METRIC statement will create metric "system.cpu.load" 
	with the data retention policy described above:

		CREATE METRIC "system.cpu.load" KEEP 1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year ;

* 	The DROP METRIC statement deletes the given metric (including all the data files):

		DROP METRIC "<name>" ;

	This statement will generate an error if the metric with the given name 
	does not exist.

	For example, the following DROP METRIC statement will erase metric "system.cpu.load":

		DROP METRIC "system.cpu.load" ;


* 	The SHOW METRIC POLICY statement returns the data retention policy for the 
	specified metric:

		SHOW METRIC POLICY "<name>" ;

	If the metric with the given name does not exist then an error is generated.

	For example, the following SHOW METRIC POLICY statement will return the data 
	retention policy for metric "system.cpu.load":

		SHOW METRIC POLICY "system.cpu.load" ;

	If the "system.cpu.load" metric was defined as above, then the returned string
	will look as follows:

		1 sec FOR 1 min, 10 sec FOR 1 hour, 10 mins FOR 1 month, 30 mins FOR 1 year
		

* 	The SHOW METRICS statement prints the semi-colon (';') separated list of all 
	the metrics' names that match the given pattern:

		SHOW METRICS LIKE "<pattern>";

	For example, the following SHOW METRICS statement will return all the metrics' 
	names that have "system" as part of their name:

		SHOW METRICS LIKE "system" ;
 
	The returned values might look as follows:

		system.cpu.load;system.memory.usage;system.memory.swap;system.io.reads;system.io.writes;
 
#### Update statements

* 	The UPDATE METRIC statement adds the given double value to the metric at 
	the given timestamp

		UPDATE METRIC "<name>" ADD <value> AT <timestamp> ;

	If the specified metric doesn't exist then it will be automatically created
	using the default retention policy specified in the configuration file. 
	The &lt;timestamp&gt; value can be in the past.

	For example, the following UPDATE METRIC statement will update metric 
	"system.cpu.load" with value 0.34 recorded at 1371278126 (Sat, 15 Jun 
	2013 06:35:26 GMT):

		UPDATE METRIC "system.cpu.load" ADD 0.34 AT 1371278126 ;

#### Query statements

* 	The SELECT * FROM METRIC statement returns the stored data between two 
	timestamps, aggregated with the given precision:

		SELECT * FROM [METRIC] "<name>" BETWEEN <timestamp1> AND <timestamp2> GROUP BY <interval> ;

	The data is returned in CSV format with a header line describing the returned 
	columns and the most recent data first. If the metric with the given name does
	not exist then an error is generated.

	For example, the following SELECT FROM METRIC statement will return metric 
	"system.cpu.load" between 1371270000 (Sat, 15 Jun 2013 04:20:00 GMT) and 
	1371279000 (Sat, 15 Jun 2013 06:50:00 GMT) aggregated in 10 secs "buckets":

		SELECT * FROM METRIC "system.cpu.load" BETWEEN 1371270000 AND 1371279000 GROUP BY 10 secs ;

	The returned CSV data will look as follows:

		ts,count,sum,avg,stddev,min,max\n
		1371279000,1000,1023,1.023,0.5,0.1,1.6\n
		1371278990,1000,1436,1.436,0.35,0.4,1.9\n
		....



Update Language (UDP connections)
---------
