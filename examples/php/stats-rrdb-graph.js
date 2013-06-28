var bgColor    = '#222324';
var axisColor  = '#EEE';
var titleColor = '#EEE';

google.load('visualization', '1.0', {packages: ['corechart']});
google.setOnLoadCallback(on_charts_load);

Element.prototype.getElementWidth = function() {
	if (typeof this.clip !== "undefined") {
		return this.clip.width;
	} else if (this.style.pixelWidth) {
		return this.style.pixelWidth;
	} else {
		return this.offsetWidth;
	}
}

Element.prototype.getElementHeight = function() {
	if (typeof this.clip !== "undefined") {
		return this.clip.height;
	} else if (this.style.pixelHeight) {
		return this.style.pixelHeight;
	} else {
		return this.offsetHeight;
	}
}

function load_data()
{
	var data = new google.visualization.DataTable();
	data.addColumn('datetime', 'ts');
	data.addColumn('number', 'val');

	for (var ii in graph_data) {
		row = graph_data[ii];
		data.addRow([ new Date(row.ts) , row.value ]);
	}
	
	return data;
}

function render_line_graph(id)
{
	var data = load_data();
	var elem = document.getElementById(id);

	var chart = new google.visualization.AreaChart(elem);
	chart.draw(data, { 
		width:  elem.getElementWidth() * 0.8, 
		height: 0.5 * elem.getElementWidth(),
		chartArea: { width: '80%', height: '80%' },
		title:  graph_title,
		titleTextStyle: { color: window.titleColor },
		backgroundColor: window.bgColor,
		legend: 'none', 
		hAxis: { textStyle: { color: window.axisColor } },
		vAxis: { textStyle: { color: window.axisColor } }
	});
}

function render_column_chart(id)
{
	var data = load_data();
	var elem = document.getElementById(id);

	var chart = new google.visualization.ColumnChart(elem);
	chart.draw(data, { 
		width:  elem.getElementWidth() * 0.8, 
		height: 0.5 * elem.getElementWidth(),
		chartArea: { width: '80%', height: '80%' },
		title:  graph_title,
		titleTextStyle: { color: window.titleColor },
		backgroundColor: window.bgColor,
		legend: 'none', 
		hAxis: { textStyle: { color: window.axisColor } },
		vAxis: { textStyle: { color: window.axisColor } }
	});
}

function render_bar_chart(id)
{
	var data = load_data();
	var elem = document.getElementById(id);

	var chart = new google.visualization.BarChart(elem);
	chart.draw(data, { 
		width:  elem.getElementWidth() * 0.8, 
		height: 0.5 * elem.getElementWidth(),
		chartArea: { width: '80%', height: '80%' },
		title:  graph_title,
		titleTextStyle: { color: window.titleColor },
		backgroundColor: window.bgColor,
		legend: 'none', 
		hAxis: { textStyle: { color: window.axisColor } },
		vAxis: { textStyle: { color: window.axisColor } }
	});
}

function on_charts_load() {
	if(document.getElementById('line-graph')) {
		render_line_graph('line-graph');
	}
	if(document.getElementById('column-chart')) {
		render_column_chart('column-chart');
	}
	if(document.getElementById('bar-chart')) {
		render_bar_chart('bar-chart');
	}
}
