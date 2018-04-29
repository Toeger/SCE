find . -name "lib*.a" | while read lib
do
	printf "$(realpath $lib);"
done
find . -name "lib*.so" | while read lib
do
	printf "$(realpath $lib);"
done
