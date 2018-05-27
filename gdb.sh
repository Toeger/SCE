#!/bin/sh -e
gdb -q --return-child-result -ex "set confirm off" -ex "set pagination off" -ex "handle SIG40 nostop" -ex "run" -ex "bt" -ex "quit" --args $*
