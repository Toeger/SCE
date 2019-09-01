#!/bin/bash -e
cd `dirname $0`
virtualenv venv -p $1
. venv/bin/activate
$1 -m pip install --upgrade pip
$1 -m pip install protobuf grpcio
