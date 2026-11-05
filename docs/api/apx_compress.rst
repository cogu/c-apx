Compression (apx_compress)
==========================

The ``apx_compress`` module provides internal compression and bounded decompression of zlib-wrapped
DEFLATE streams (RFC 1950 / RFC 1951) for transferring APX node definitions. It supports pluggable
backends (``zlib`` and ``miniz``) selected at build time via ``APX_COMPRESSION_BACKEND``.

API Reference
-------------

Backend Information
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: apx_compress_backend_name

Compression
~~~~~~~~~~~

.. doxygenfunction:: apx_compress_bound
.. doxygenfunction:: apx_compress
.. doxygenfunction:: apx_compress_level

Decompression
~~~~~~~~~~~~~

.. doxygenfunction:: apx_decompress

Both backends report an Adler-32 mismatch as ``APX_CHECKSUM_ERROR``.
Malformed DEFLATE data, incomplete streams, trailing bytes, and output-limit
failures remain distinct errors. No failed stream is retried as another encoding.
