#./hera host port threads connections path filesize files endfile gzip timeout 
while :
do
	timelimit -t 10 ./hera 192.168.0.25 80 50 1 /test.php 0.001 20 0 0 2
done
