APX for C
=========

This is the C implementation of `APX <http://apx.readthedocs.io/en/latest/`_.

Current implementation status
-----------------------------

   +-----------------------------+---------------------+-------------------+
   |   Operating System          |     APX Router      |    APX Client     |
   +=============================+=====================+===================+
   |   Windows (native)          |   (in development)  |  (in development) |
   +-----------------------------+---------------------+-------------------+
   |   Windows (cygwin)          |   (testing)         |  (testing)        |
   +-----------------------------+---------------------+-------------------+
   |   Linux (native)            |   (testing)         |  (testing)        |
   +-----------------------------+---------------------+-------------------+
   |  No OS or RTOS (APX-ES)     |   (not in scope)    |  (in development) |
   +-----------------------------+---------------------+-------------------+

   
APX Router
----------

An APX Router is a server application that clients connects to using IPC or sockets.
On connection, each client uploads an .apx file to the router then waits for port data.

An .apx file is an ASCII text file written in `APX Text <http://apx.readthedocs.io/en/latest/apx_text.html>`_
containing information about what port signatures it provides and what port signatures it requires.

The APX router uses this information to dynamically create data routes between provide and require ports
of all connected APX nodes (the clients). When a node/client writes/updates port data on one of its provide ports
the router automatically routes that data to all (connected) require ports of other APX nodes.

APX Client
----------
An APX client implements one or more APX nodes. It connects to an APX router in order to send/receive port data.

.. note:: A full presentation of what APX is and how it works is currently in the works.

Dependencies
------------

APX for C requires an operating system to run. Currently it supports Linux and Windows (using cygwin).

In addition, if you intend to use APX with sockets it also requires:

* `msocket <https://github.com/cogu/msocket>`_: A small wrapper layer around the native sockets API (uses BSD sockets on Linux, Winsock2 sockets on Windows).

Abstract Data Structures
~~~~~~~~~~~~~~~~~~~~~~~~

For APX I'm using my own `implemenations <https://github.com/cogu/adt>`_ of commonly used `data structures <https://en.wikipedia.org/wiki/Data_structure>`_ .

* adt_ary: similar (in concept) to `QList <http://doc.qt.io/qt-5/QList.html>`_
* adt_bytearray: similar to `QByteArray <http://doc.qt.io/qt-5/qbytearray.html>`_
* adt_hash: similar to `QHash <http://doc.qt.io/qt-5/qhash.html>`_
* adt_list: similar to `QLinkedList <http://doc.qt.io/qt-5/qlinkedlist.html>`_

APX-ES (APX for embedded systems)
---------------------------------

A version of APX where all use of dynamic memory and underlying OS calls has been removed is currently being worked on.
The intent with this version is to enable APX clients to run on embedded devices such as IoT devices or automotive ECUs.
More information about this version will be provided once implementation finishes.
