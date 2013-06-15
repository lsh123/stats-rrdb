STATS-RRDB
=========

What is it? 
---------
stats-rrdb is a high-performance server for collecting, aggregating and storing
various statistics. It's a combination of the famous [statsd](https://github.com/etsy/statsd/)
and [graphite](http://graphite.wikidot.com/) without the UI from the later.

stats-rrdb provides simple out-of-the box configuration; [DDL and query language](LANGUAGE.md#ddl-and-query-language-tcp-connections); 
optimized [UDP data format](LANGUAGE.md#optimized-language-udp-connections), 
support for current and historical data; and, of course, high-performance of C++ code.

stats-rrdb is focused on data and data only. It *does not* provide any graphical interface 
(though there are examples of how to build one quickly). 


Why do we need yet another statsd? 
---------
Well, fair question. 

First, I really dislike the architecture for [statsd](https://github.com/etsy/statsd/)
and [graphite](http://graphite.wikidot.com/) for two main reasons:

* The data storage is split between two systems (statsd and graphite's whisper)
* The graphite engine combines data storage and presentation layers in one package
* The [graphite architecture](http://graphite.wikidot.com/high-level-diagram) is overly complicated for the task

Second, I don't want yet another "framework" in the production environment. I like Python as much as the next
guy, but if you run XXX (insert your favorite non-Python here), then why do you need to install and maintain
Python environment? And even if you install Python, then don't forget about packages compatibilities (while
installing graphite, I was presented with a choice of either keeping yum or installing graphite... I decided
to go with yum).

Last but not the least, it is *fun* to write server side software :)


Installation
---------
For installation instructions see [Install](INSTALL.md).


Roadmap
---------
* 0.5.0 (ready) - complete language, storage, TCP/UDP servers, tests, rpms
* 1.0.0 - memory usage controls, dynamic blocks loading, journal file, graphing examples 
* 1.x.0 - more metric types (e.g. 95 percentile, etc.)
