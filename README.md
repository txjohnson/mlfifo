mlfFIFO
=======

Mostly Lock Free FIFO Queue

Overview
--------
mlfFIFO is a simple implementation of a lock free structure. It supports multiple readers and writers to a FIFO queue. 
The interface is very simple (ignoring construction and destruction):

+  put() puts an object into the queue. 
+  get() takes an object out.

Both operations are nonblocking and will return false if they fail.

Usage
-----
Include the header and instantiate the template.



fin