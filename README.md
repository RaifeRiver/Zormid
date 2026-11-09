# Zormid

Zormid is a web-based chat app designed for self-hosting with low-spec servers.

## Features
 - Low server requirements
 - Free and open source
 - Either self-hosting or use our server

## Limitations
 - Requires linux server
 - Only supports web clients
 - No encryption

**NOTE: Zormid is currently WIP and these limitations will likely be addressed soon.**

## Why did I make this?
I wanted to learn how online chat apps work and make something I could actually use (if I can convince anyone else to use it). Also, it is incredibly lightweight with the custom web server written in C++ using just 4MB of RAM thanks to using raw posix sockets instead of large bloated libraries.