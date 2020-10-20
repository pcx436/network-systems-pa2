#!/usr/bin/env python3

from threading import Thread, Lock
from argparse import ArgumentParser
from pathlib import Path
from hashlib import md5
from random import shuffle
from requests import get

hashes = dict()

numError = 0
lock = Lock()

def handler(reqFile, url):
    r = get(url)
    hasher = md5(r.content)

    if r.ok:
        if hasher.hexdigest() != hashes[reqFile]:
            print('Failed to get file due to hash error', url)
            lock.acquire()
            numError += 1
            lock.release()
        else:
            pass
            # print('OK:', str(reqFile))
    else:
        print('Failed to get file due to 500', url)
        lock.acquire()
        numError += 1
        lock.release()

    r.close()


def main():
    parser = ArgumentParser(description='A simple python HTTP server stress tester')
    parser.add_argument('hostname', help="The hostname for the server.", type=str, default='localhost')
    parser.add_argument('port', help="The port the server will listen on.", type=int, default=8080)
    parser.add_argument('-r', '--requests', help='The number of times to request all files', type=int, default=3)
    parser.add_argument('-d', '--dir', help='The "www" directory that contains all the files', type=str, default='./www/')

    args = parser.parse_args()
    host = args.hostname
    port = args.port
    numRequests = args.requests
    file_dir = args.dir

    p = Path(file_dir)
    if not p.exists():
        print('{} does not exist, exiting...'.format(file_dir))

    files = [x for x in p.glob('**/*.*')]

    for fp in files:
        hasher = md5()

        with fp.open('rb') as f:
            hasher.update(f.read())

        hashes[fp.relative_to(p)] = hasher.hexdigest()

    files *= numRequests
    shuffle(files)

    threads = list()
    
    for fp in files:
        rela = fp.relative_to(p)
        url = 'http://{}:{}/{}'.format(host, port, str(rela))
        # print(url)
        # continue
        thread = Thread(target=handler, args=(rela, url))
        thread.start()
        threads.append(thread)

    for t in threads:
        t.join()

    print('{:.2f}% of requests failed.'.format(100.0 * (numError / len(files))))

if __name__ == '__main__':
    main()
