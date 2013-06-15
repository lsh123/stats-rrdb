<?php
include_once("../stats_rrdb.php");
date_default_timezone_set("UTC");

$display_modes = array('table' => 'Table', 'line-graph' => 'Line Graph' );
$columns       = array('all' => 'All', 'count' => 'Count', 'sum' => 'Sum', 'avg' => 'Avg', 'stddev' => 'StdDev', 'min' => 'Min', 'max' => 'Max');

$client = new StatsRRDB(StatsRRDB::Mode_Tcp);
$res = $client->send_command("show metrics like '.' ;");
$metrics      = preg_split('/;/', $res, -1, PREG_SPLIT_NO_EMPTY);

$now          = time();
$cur_metric   = isset($_GET['metric']) ? $_GET['metric'] : $metrics[0];
$column       = isset($_GET['column']) ? $_GET['column'] : "all";
$cur_ts1      = isset($_GET['ts1']) ? $_GET['ts1'] : date("r", $now - 24*60*60);
$cur_ts2      = isset($_GET['ts2']) ? $_GET['ts2'] : date("r", $now);
$cur_group_by = isset($_GET['group_by']) ? $_GET['group_by'] : "10 secs";
$mode         = isset($_GET['mode']) ? $_GET['mode'] : "table";

# query
$ts1 = strtotime($cur_ts1, $now);
$ts2 = strtotime($cur_ts2, $now);
$query = "select * from '{$cur_metric}' between {$ts1} and {$ts2} group by {$cur_group_by} ; ";

$res = $client->send_command($query);
$values = preg_split("/\n/", $res);

// separate header
$headers = array_shift($values);
$headers = preg_split('/,/', $headers, -1, PREG_SPLIT_NO_EMPTY);
$column_pos = ($column != 'all') ? array_search ($column, $headers) : 1;

// separate data
$data = array();
foreach($values as $value) {
	$data[] = preg_split('/,/', $value, -1, PREG_SPLIT_NO_EMPTY);
}
?>

<html>
<head>
    <title>Stats-RRDB Example Page</title>
</head>
<body>
    <h1>Stats-RRDB Example Page</h1>
    
	<section>                                                                                                                                                                                                                                               
	<h2>Query Builder</h2>   
   		<form>
		<table>
		<tr>
		    <td>Metric:</td>
		    <td>
		    <select name="metric" style="width: 300px">
		    <?php foreach($metrics as $metric): ?>
			<option name="<?php echo $metric; ?>" 
			    <?php echo ($cur_metric == $metric) ? "SELECTED" : ""?>
			><?php echo $metric; ?></option>
		    <?php endforeach; ?>
		    </select>
		    </td>
		</tr>
		<tr>
		    <td>Select Column:</td>
		    <td>
		    	<select name="column" style="width: 300px">
		    		<?php foreach($columns as $v => $n): ?>
		    		<option value="<?php echo $v; ?>" <?php echo ($column == $v ? "SELECTED='1'" : ""); ?> ><?php echo $n; ?></option>
		    		<?php endforeach; ?>
		    	</select>
		    </td>
		</tr>	
		<tr>
		    <td>Start Time:</td>
		    <td><input name="ts1" value="<?php echo $cur_ts1; ?>" style="width: 300px" /></td>
		</tr>
		<tr>
		    <td>End Time:</td>
		    <td><input name="ts2" value="<?php echo $cur_ts2; ?>" style="width: 300px" /></td>
		</tr>
		<tr>
		    <td>Aggregate:</td>
		    <td><input name="group_by" value="<?php echo $cur_group_by; ?>" style="width: 300px" /></td>
		</tr>
		<tr>
		    <td>Display Mode:</td>
		    <td>
		    	<select name="mode" style="width: 300px">
		    		<?php foreach($display_modes as $v => $n): ?>
		    		<option value="<?php echo $v; ?>" <?php echo ($mode == $v ? "SELECTED='1'" : ""); ?> ><?php echo $n; ?></option>
		    		<?php endforeach; ?>
		    	</select>
		    </td>
		</tr>
		<tr>
		    <td colspan="2">
		    <input type="submit" name="Submit" style="width: 200px; height: 30px;" />
		    </td>
		</tr>
		</table>
   		</form>
    </section>
    
    <hr/>
    
    <section>                                                                                                                                                                                                                                               
	<h2>Query: <?php echo $query;?></h2>

		<?php if($mode == 'table'):?>
	    <table>
			<!-- HEADER -->
			<thead>
			<tr align="left">
			    <?php for($ii = 0; $ii < count($headers); $ii++): ?>
				<th <?php echo ($ii != 0 ? 'width="75px"' : 'width="300px"'); ?> ><?php echo $headers[$ii] ; ?></th>
			    <?php endfor; ?>
			</tr>
			</thead>
			
			<!-- VALUES -->
			<tbody>
			<?php foreach($data as $row): ?>
			<tr>
			    <?php for($ii = 0; $ii < count($row); $ii++): ?>
				<td><?php echo $ii == 0 ? date("r", $row[$ii]) : $row[$ii] ; ?></td>
			    <?php endfor; ?>
			</tr>
			<?php endforeach; ?>
		    </tbody>
	    </table>
	    <?php elseif($mode == 'line-graph'):?>
	    	<script type="text/javascript">
			var line_graph_data = [
				<?php foreach($data as $row): ?>
					<?php if(!empty($row[0])): ?>
					    {"ts": "<?php echo date("r", $row[0]); ?>",	"value": <?php echo $row[$column_pos]; ?> },
					<?php endif ?>
				<?php endforeach; ?>
			];
			</script>
			<div id="line_graph"></div>                                                                                                                                                                                                                                                    
	    <?php else: ?>
	    <br>Invalid display mode '<?php echo $mode;?>'</br> 
	    <?php endif ?>
	</section>
	
    <script src="https://www.google.com/jsapi" type="text/javascript"></script>
    <script type="text/javascript">
	   	var bgColor  = '#222324';
	    var text_big = { color: '#BBB', fontSize: 24 }; // "big" text style
	    var text_med = { color: '#BBB', fontSize: 18 }; // "medium" text style

    	google.load('visualization', '1.0', {packages: ['corechart']});
    	google.setOnLoadCallback(on_charts_load);

    	function on_charts_load() {
    		if(document.getElementById('line_graph')) {
    			var data = new google.visualization.DataTable();
    			data.addColumn('datetime', 'ts');
    			data.addColumn('number', 'val');

    			for (var ii in line_graph_data) {
    				row = line_graph_data[ii];
    				data.addRow([ new Date(row.ts) , row.value ]);
    			}

    			var chart = new google.visualization.AreaChart(document.getElementById('line_graph'));
    			chart.draw(data, 
    				{ width:  800
    				, height: 400
    				, title: '<?php echo $metric; ?>: <?php echo $column; ?>'
    				, titleTextStyle: window.text_big
    				, backgroundColor: window.bgColor
    				, legend: 'none'
    				, chartArea: { width: '80%', height: '80%' }
    				}
    			);
    		}
    	}
    </script>	
</body>
</html>


