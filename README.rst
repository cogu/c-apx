APX for C
=========

This is the C implementation of `APX <http://apx.readthedocs.io/en/latest/>`_.

Current implementation status
-----------------------------

   +-----------------------------+---------------------+-------------------+
   |   Operating System          |     APX Server      |    APX Client     |
   +=============================+=====================+===================+
   |   Windows (native)          |   (testing)         |  (in development) |
   +-----------------------------+---------------------+-------------------+
   |   Windows (cygwin)          |   (testing)         |  (testing)        |
   +-----------------------------+---------------------+-------------------+
   |   Linux (native)            |   (testing)         |  (testing)        |
   +-----------------------------+---------------------+-------------------+
   |  APX for embedded systems   |   (not in scope)    |  (in development) |
   +-----------------------------+---------------------+-------------------+

APX server
----------
An APX server is a light-weight server application that clients connects to using sockets.
It's basically an APX router wrapped in a layer that provides socket communication. 
You can either use the default APX server (with sockets) or you can add your own communication adapters like for instance IPC.
   
APX router
----------

The APX router is the main (internal) component of an APX server. When started, it waits for connections from clients.
On connection, each client uploads an .apx file to the router then waits for port data.

An .apx file is an ASCII text file written in `APX Text <http://apx.readthedocs.io/en/latest/apx_text.html>`_
containing information about what port signatures it provides and what port signatures it requires.

The APX router uses this information to dynamically create data routes between provide and require ports
of all connected APX nodes (the clients). When a node/client writes/updates port data on one of its provide ports
the router automatically routes that data to all (connected) require ports of other APX nodes.


APX Client
----------
An APX client implements one or more APX nodes. It connects to an APX server in order to send/receive port data.

.. note:: A full presentation of what APX is and how it works is currently in the works.

Dependencies
------------

All dependencies needed to build APX for C are available in this git repository and its submodules. 
It is known to work for both Windows and Linux (it's all a single code base).

* For Linux: Build with gcc, both makefile(s) and eclipse projects are available in this git repository.
* For Windows: Build for cygwin using gcc or build for Windows using Visual Studio (2010 and 2015 has been verified to work so far).

Socket Support
~~~~~~~~~~~~~~

I'm using my own sockets library called `msocket <https://github.com/cogu/msocket>`_. It's a thin helper library for 
dealing with the sockets API in a portable manner (it uses BSD sockets API on Linux and Winsock2 API on Windows).

Abstract Data Structures
~~~~~~~~~~~~~~~~~~~~~~~~

For APX I'm using my own `implemenations <https://github.com/cogu/adt>`_ of commonly used `data structures <https://en.wikipedia.org/wiki/Data_structure>`_ .

* adt_ary: similar to `QList <http://doc.qt.io/qt-5/QList.html>`_
* adt_bytearray: similar to `QByteArray <http://doc.qt.io/qt-5/qbytearray.html>`_
* adt_hash: similar to `QHash <http://doc.qt.io/qt-5/qhash.html>`_
* adt_list: similar to `QLinkedList <http://doc.qt.io/qt-5/qlinkedlist.html>`_
* adt_str: similar to `QString <http://doc.qt.io/qt-5/qstring.html>`_

APX-ES (APX for embedded systems)
---------------------------------

A version of APX where all use of dynamic memory and underlying OS calls has been removed is currently being worked on.
The intent with this version is to enable APX clients to run on embedded devices such as IoT devices or automotive ECUs.
More information about this version will be provided once implementation progresses.
