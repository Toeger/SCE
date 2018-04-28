export PRETEST=pretest
find . -name "lib*" | while read lib
do
    export TEST="$TEST $(realpath $lib)"
    echo $(realpath $lib)
    echo $TEST
    echo "$TEST $(realpath $lib)"
done

