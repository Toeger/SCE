find . -name "lib*.a" | while read lib
do
	export GRPC_LIBS="$GRPC_LIBS;$(realpath $lib)"
done
