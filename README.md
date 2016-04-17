# simple-webserver-c
Simple web server written in C

This is a simple webserver written in C for the Linux environment.  Originally written with CentOS 7.  The program reads the 
command line options and executes a simple webserver.  The webserver listens for a client request, accepts the request, and then 
sends the content of index.htm.


cc simple-webserver.c -o simple-webserver


Sample run: ./simple-webserver --port 80 --listenip 127.0.0.1 --indexfile /path/index.htm --debug
Sample run: ./simple-webserver --port 80 --listenip 127.0.0.1 --indexfile ./index.htm --debug
Sample run: ./simple-webserver --port 10000 --listenip 127.0.0.1 --indexfile /path/index.htm

Only --debug is optional, all other parameters are required.


Once the server is running, open a browser and enter the following URL:
http://127.0.0.1
or
http://127.0.0.1:10000 (if not running on port 80)


To stop the webserver CTRL-C or kill -9.  This may hold the port open for a few minutes.
