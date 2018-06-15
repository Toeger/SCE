#!/bin/sh -e
. $(dirname $0)/venv/bin/activate
PYTHONPATH=$(dirname $0) $1 $2
