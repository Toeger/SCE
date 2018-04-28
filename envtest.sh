export PRETEST=pretest
find . -name "lib*" | while read lib
do
    export TEST="$TEST $(realpath $lib)"
    echo $(realpath $lib)
done

