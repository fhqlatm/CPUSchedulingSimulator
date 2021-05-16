echo TEST SIMULATOR > ./output.txt
echo >> ./output.txt

for num in {1..10}

    do
        gcc -o generator os-gen-cpu.c

        ./generator > test.bin

        echo ===== TEST-${num} ===== >> ./output.txt

        cat ./test.bin | ./simulatorFIFO >> ./output.txt

        cat ./test.bin | ./simulatorSRTJ >> ./output.txt

        cat ./test.bin | ./simulatorRR >> ./output.txt

        sleep 1
    done