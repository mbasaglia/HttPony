HttPony
=======

A modern C++ HTTP library


Features
--------
* HTTP/1.x
* Optional SSL/HTTPS support (Requires to link against OpenSSL)
* Threaded server implementation
* Synchronous and asynchronous client implementation
* Abstraction over low-level aspects of the protocol
* Implementation of related RFCs (URI, base encoding and others)


License
-------

AGPLv3+, See COPYING


Author
------

Mattia Basaglia <mattia.basaglia@gmail.com>


Sources
-------

https://github.com/mbasaglia/HttPony


Installing
==========

Dependencies
------------

* [Melanolib](https://github.com/mbasaglia/Melanolib) (Included as a sub-module)
* [Boost ASIO](http://boost.org)


Building
--------

    mkdir build && cd build && cmake .. && make

Examples
--------

See example/ for example files, src/examples.dox for an explanation of those
examples.

You can also build the doxygen documentation with

    make doc

Which, of course, requires doxygen to be installed.
