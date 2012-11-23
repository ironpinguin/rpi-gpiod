SOCKET=test.sock
REPORT=gpiod.testreport
NC="nc -U $SOCKET"

rm -f $SOCKET
./gpiod -d -s $SOCKET > $REPORT &
GPIOD_PID=$!

TESTCASE[0]="UNKNOWNCOMMAND"
EXPECTED[0]="ERROR - unkown command"
TESTCASE[1]="READALL"
EXPECTED[1]="OK
0   0 GPIO 0 0
1   1 GPIO 1 1
2   2 GPIO 2 0
3   3 GPIO 3 1
4   4 GPIO 4 0
5   5 GPIO 5 1
6   6 GPIO 6 0
7   7 GPIO 7 1
8   8 SDA    0
9   9 SCL    1
10  10 CE0    0
11  11 CE1    1
12  12 MOSI   0
13  13 MISO   1
14  14 SCLK   0
15  15 TxD    1
16  16 RxD    0"
TESTCASE[2]="READ"
EXPECTED[2]="ERROR - parameter of type integer expected"
TESTCASE[3]="READ 1"
EXPECTED[3]="OK - 1"
TESTCASE[4]="READ 2"
EXPECTED[4]="OK - 0"
TESTCASE[5]="READ A"
EXPECTED[5]="ERROR - parameter of type integer expected"
TESTCASE[5]="READ 999"
EXPECTED[5]="ERROR - unknown port number"
TESTCASE[6]="WRITE"
EXPECTED[6]="ERROR - expected WRITE <#pin> <0|1>"
TESTCASE[7]="WRITE 1"
EXPECTED[7]="ERROR - expected WRITE <#pin> <0|1>"
TESTCASE[8]="WRITE 999 0"
EXPECTED[8]="ERROR - unknown port number"
TESTCASE[9]="WRITE a 0"
EXPECTED[9]="ERROR - expected WRITE <#pin> <0|1>"
TESTCASE[9]="WRITE 10 o"
EXPECTED[9]="ERROR - expected WRITE <#pin> <0|1>"
TESTCASE[10]="WRITE 10 0"
EXPECTED[10]="OK - operation performed"
TESTCASE[11]="WRITE 11 1"
EXPECTED[11]="OK - operation performed"
TESTCASE[12]="WRITE 11 3"
EXPECTED[12]="ERROR - value must be 0 or 1"
TESTCASE[13]="MODE"
EXPECTED[13]="ERROR - expected MODE <#pin> <IN|OUT>"
TESTCASE[14]="MODE 1"
EXPECTED[14]="ERROR - expected MODE <#pin> <IN|OUT>"
TESTCASE[15]="MODE 999 0"
EXPECTED[15]="ERROR - unknown port number"
TESTCASE[16]="MODE a 0"
EXPECTED[16]="ERROR - expected MODE <#pin> <IN|OUT>"
TESTCASE[17]="MODE 10 o"
EXPECTED[17]="ERROR - mode must be IN or OUT"
TESTCASE[18]="MODE 10 IN"
EXPECTED[18]="OK - operation performed"
TESTCASE[19]="MODE 11 OUT"
EXPECTED[19]="OK - operation performed"

failcount=0
for((i=0; $i <= 19; i=$i + 1))
do
    TESTCASE="${TESTCASE[$i]}"
    EXPECTED="${EXPECTED[$i]}"
    printf "Test Case %4d :  %-30s " "$i" "$TESTCASE"
    ACTUAL=`echo "$TESTCASE" |$NC`

    if ! kill -0 $GPIOD_PID > /dev/null 2> /dev/null
    then
	printf " FAIL\n\n"
	printf "Expected: $EXPECTED\n\n"
	printf "gpiod died!\nABORT!\n"
	exit
    fi

    if [ "$ACTUAL" == "$EXPECTED" ]
    then
	printf " PASS\n"
    else
	printf " FAIL\n\n"
	printf "Actual:   $ACTUAL\n"
	printf "Expected: $EXPECTED\n\n"
	failcount=$(($failcount + 1))
    fi
done

TESTCASE="Socket permissions"
printf "Test Case %4d :  %-30s " "$i" "$TESTCASE"
ACTUAL=$(ls -l $SOCKET | awk '{ print $1 }')
EXPECTED='srwxrwxrwx'
if [ "$ACTUAL" == "$EXPECTED" ]
then
    printf " PASS\n"
else
    printf " FAIL\n\n"
    printf "Actual:   $ACTUAL\n"
    printf "Expected: $EXPECTED\n\n"
    failcount=$(($failcount + 1))
fi


kill $GPIOD_PID

if [ $failcount -gt 0 ]
then
    printf "\nMore than one test failed\n"
else
    printf "\nAll tests passed\n"
fi

