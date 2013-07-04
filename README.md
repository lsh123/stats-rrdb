STATS-RRDB
=========

What is it? 
---------
stats-rrdb is a high-performance server for collecting, aggregating and storing
various statistics. It's a combination of the famous [statsd](https://github.com/etsy/statsd/)
and [graphite](http://graphite.wikidot.com/) without the UI from the later.

stats-rrdb provides simple out-of-the box configuration; [query language](LANGUAGE.md#query-language-tcp-connections)
(TCP connections); optimized [update language](LANGUAGE.md#update-language-udp-connections)
(UDP connections), support for current and historical data; and, of course, high-performance of C++ code.

stats-rrdb is focused on data and data only. It *does not* provide any graphical interface 
(though there are examples of how to build one quickly). However, it has a lot of advanced DB features
to make it run fast and efficient:
- TCP and UDP servers for queries returning data and fast updates
- Data blocks cache: only frequently accessed data blocks are loaded in memory
- Open file handles cache: set open files limit high in the OS and save time on re-opening files all the time
- Journal file: never lose your data
- Easy integration with your favorite charting package ([Google Charts example](examples/php/index.php))


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
* 	1.x.0 - query language joins to query multiple metrics in the same time
