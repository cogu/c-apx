# APX for C

This is the C implementation of [APX](https://github.com/cogu/apx-doc).

## Stable code base

The c-apx code base has gone through a major refactoring phase. If you are looking for a stable release, 
please use any of the 0.2.x releases (latest is [v0.2.5](https://github.com/cogu/c-apx/releases/tag/v0.2.5)).

## New experimental code base

The master branch should be considered highly experimental for a time and cannot be used for anything useful except 
runnning the unit test suite.

### What internal features was added by the refactoring?

- New event loop and event listener API (Server+Client)
- Support for extensions (Server)
	- First extension will be the socket extension.
- Preparations for APX 1.3 (Server+Client)
	- Boolean type
	- byte array type
	- UTF-8 string support
	- Dynamically sized arrays
	- Queued signals
	- JSON data import/export


## Planned features in next release (v0.3.0)

- Socket extension (TCP+Unix) (Server).
- JSON configuration file (Server).
- New data routing engine (Server).
- Minimal support for APX VM 2 (Client).


## What is APX?

APX consists of a set of clients (called nodes) and a server. The server acts as a message router between the clients so 
that signal data can flow between them. APX is free and open source.

## Where can APX be used?

APX is used in the automotive industry to quickly transfer automotive signals (primarily AUTOSAR signals) in real-time.
It is designed to be used in small, local networks such as an intranet or on localhost.
APX can scale from being used in a simple protype setup to full production solutions.

## What is APX-ES?

APX for embedded systems (APX-ES) is client source code written for very small devices.

- It does not require an operating system (Using a small RTOS is recomended)
- It does not require any heap memory.
- It is intended to be MISRA-compliant (at some point).




