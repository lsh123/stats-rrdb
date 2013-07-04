<?php
include_once("sdk/stats_rrdb.php");

//
// helpers
//
function query_get($name, $default = false)
{
	return isset($_GET[$name]) ? $_GET[$name] : $default;
}

//
// main code
//
date_default_timezone_set("UTC");
$error = "";

// use localhost
$stats_rrdb   = new StatsRRDB('127.0.0.1', 9876);

// fetch metrics
$metrics = array();
try {
	$metrics  = $stats_rrdb->show_metrics();
} catch(Exception $e) {
	$error = $e->getMessage();	
}

// get request data
$now          = $_SERVER['REQUEST_TIME'];
$the_metric   = query_get('metric', $metrics[0]);
$the_column   = query_get('column', '*');
$the_ts1      = query_get('ts1', '1 day ago');
$the_ts2      = query_get('ts2', 'now');
$the_group_by = query_get('group_by', '1 min');
$the_mode     = query_get('mode', 'table');

// query RRDB data
$headers = array();
$values = array();
try {
	$ts1 = strtotime($the_ts1, $now);
	$ts2 = strtotime($the_ts2, $now);
	$values = $stats_rrdb->select($the_metric, $the_column, $ts1, $ts2, $the_group_by);
	
	// separate header
	$headers = !empty($values) ? array_shift($values) : array();
} catch(Exception $e) {
	$error = $e->getMessage();
}

// prep select options
$display_modes = array('table' => 'Table', 'line-graph' => 'Line Graph', 'column-chart' => 'Column Chart', 'bar-chart' => 'Bar Chart' );
$columns       = array('*' => 'All', 'count' => 'Count', 'sum' => 'Sum', 'avg' => 'Avg', 'stddev' => 'StdDev', 'min' => 'Min', 'max' => 'Max');
?>
<!DOCTYPE html>
<html>
<head>
    <title>Stats-RRDB Example Page</title>
	<link href="//netdna.bootstrapcdn.com/twitter-bootstrap/2.3.2/css/bootstrap-combined.min.css" rel="stylesheet"></link>
</head>
<body>
<h1 class="offset1">Stats-RRDB Example Page</h1>

<div class="container-fluid fill offset1">
<?php if(!empty($error)):?>
<div class="row-fluid">
<p class="text-error"><?php echo htmlspecialchars($error); ?></p>
</div>
<?php endif?>

<br/><br/>
<div class="row-fluid">
	<div class="span4">
		<form class="form-horizontal" method="GET">
			<div class="control-group">
				<label class="control-label" for="metric">Metric</label>
				<div class="controls">
					<select name="metric">
						<?php foreach($metrics as $metric):?>
						<option value="<?php echo $metric?>" <?php echo ($the_metric == $metric) ? 'SELECTED' : ''?>><?php echo $metric?></option>
						<?php  endforeach?>
					</select>
				</div>
			</div>
			<div class="control-group">
				<label class="control-label" for="column">Select Column</label>
				<div class="controls">
					<select name="column">
						<?php foreach($columns as $v => $n):?>
						<option value="<?php echo $v?>" <?php echo ($the_column == $v) ? 'SELECTED' : ''?>><?php echo $n?></option>
						<?php  endforeach?>
					</select>
				</div>
			</div>
			
			<div class="control-group">
				<label class="control-label" for="ts1">From</label>
				<div class="controls">
					<input name="ts1" type="text" value="<?php echo  $the_ts1; ?>"/>
				</div>
			</div>
			
			<div class="control-group">
				<label class="control-label" for="ts2">To</label>
				<div class="controls">
					<input name="ts2" type="text" value="<?php echo  $the_ts2; ?>"/>
				</div>
			</div>  
			
			<div class="control-group">
				<label class="control-label" for="group_by">Aggregate</label>
				<div class="controls">
					<input name="group_by" type="text" value="<?php echo  $the_group_by; ?>"/>
				</div>
			</div> 

			<div class="control-group">
				<label class="control-label" for="mode">Display Mode</label>
				<div class="controls">
					<select name="mode">
						<?php foreach($display_modes as $v => $n):?>
						<option value="<?php echo $v?>" <?php echo ($the_mode == $v) ? 'SELECTED' : ''?>><?php echo $n?></option>
						<?php endforeach?>
					</select>
				</div>
			</div> 
			
			 <div class="control-group">
				<div class="controls">
					<button class="btn btn-success span9" type="submit" name="query" value="1">Query</button>
				</div>
			</div>
		</form>
	</div>
	
	<?php if(!empty($headers)):?>
	<div class="span8">                                                                                                                                                                                                                                               
		<?php 
		switch($the_mode):
		case "table":
		?>
		    <table class="table table-striped">
			<!-- HEADER -->
			<thead>
			<tr align="left">
			    <?php  for($ii = 0; $ii < count($headers); $ii++): ?>
				<th <?php echo  ($ii != 0 ? 'width="75px"' : 'width="300px"'); ?>>
					<?php echo  $headers[$ii] ; ?>
				</th>
			    <?php  endfor; ?>
			</tr>
			</thead>
			
			<!-- VALUES -->
			<tbody>
			<?php foreach($values as $row): ?>
			<tr>
			    <?php for($ii = 0; $ii < count($row); $ii++): ?>
				<td><?php echo  ($ii == 0 ? date("m/d/Y h:i:s e", $row[$ii]) : $row[$ii]); ?></td>
			    <?php endfor; ?>
			</tr>
			<?php endforeach; ?>
		    </tbody>
	    	</table>
	    	
	    	<?php break;?>
	    <?php case "line-graph"?>
	    <?php case "column-chart"?>
	   	<?php case "bar-chart"?>
	   		<!-- Common data -->
			<script type="text/javascript">
			var graph_data = [
				<?php foreach($values as $row): ?>
					<?php if(!empty($row[0])): ?>
					    {"ts": "<?php echo  date("r", $row[0]); ?>", "value": <?php echo  $row[1]; ?> },
					<?php  endif ?>
				<?php endforeach; ?>
			];
			var graph_title = '<?php echo $the_metric?>:<?php echo $headers[1]?> (<?php echo $the_ts1?> to <?php echo $the_ts2?>)';
			</script>
			
			<!-- Div for the chart -->
			<div id="<?php echo $the_mode?>"></div>
			
	    	<?php break;?>
	    <?php default:?>
			<span>Invalid display mode '<?php echo $the_mode?>'</span> 
	    	<?php break;?>
	    <?php  endswitch?>
	</div>
	<?php endif?>
</div>
</div>
<script src="https://www.google.com/jsapi" type="text/javascript"></script>		
<script src="stats-rrdb-graph.js" type="text/javascript"></script>		

</body>
</html>


