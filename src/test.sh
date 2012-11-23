SOCKET=test.sock
NC="nc -U $SOCKET"

rm -f $SOCKET
./gpiod -d -s $SOCKET > /dev/null &
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

for((i=0; $i <= 5; i=$i + 1))
do
    TESTCASE="${TESTCASE[$i]}"
    EXPECTED="${EXPECTED[$i]}"
    printf "Test Case %4d :  %-30s " "$i" "$TESTCASE"
    ACTUAL=`echo "$TESTCASE" |$NC`
    if [ "$ACTUAL" == "${EXPECTED[$i]}" ]
    then
	 printf " PASS\n"
     else
	 printf " FAIL\n\n"
	 printf "Actual: $ACTUAL\n"
	 printf "Expected: ${EXPECTED[$i]}\n\n"
    fi
done

kill $GPIOD_PID

