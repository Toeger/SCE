#!/bin/sh -e
find . -iregex ".*/lib[^./]*.so" | while read lib
do
	printf " $(realpath $lib)"
done
find . -iregex ".*/lib[^./]*.a" | while read lib
do
	printf " $(realpath $lib)"
done

