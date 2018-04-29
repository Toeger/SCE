find . -name "lib*" | while read lib
do
	printf " $(realpath $lib)"
done
