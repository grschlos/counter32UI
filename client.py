#!/usr/bin/env python3
# encoding: utf-8

import socket
from multiprocessing import Process
from time import sleep

def main(args=None):
    mode = 'text'
    if (args): mode = args.mode
    soc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    print('Socket created')

    try:
        if (mode=='bin'): soc.bind(('', 1200))
        else: soc.bind(('', 1400))
        print('Socket bind complete')
    except socket.error as msg:
        import sys
        print('Bind failed. Error: '+str(sys.exc_inf()))
        sys.exit()
    ##end try

    t = Process(target=listen, args=(soc, mode))
    t.start()
    try:
        while 1:
            cmd = input()
            if (mode=='bin'):
                a = bytearray()
                for v in cmd.split(): a.append(int(v, base=16))
                soc.sendto(a, ('192.168.1.3', 1200))
            else:
                soc.sendto(cmd.encode(), ('127.0.0.1', 1300))
        ##end while
    except (KeyboardInterrupt, SystemExit):
        t.join()
    ##end try
##end def main

def listen(sock, mode='text'):
    while True:
        if (mode!='text'):
            data = sock.recv(1024)
            for b in data: print("{:02x} ".format(b), end='')
            print()
        else:
            data = sock.recv(1024).decode().rstrip()
            print(data)
##end def listen

if __name__=='__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument('--mode', type=str, choices=["text", "bin"], dest='mode')
    args = parser.parse_args()
    main(args)
##end ifmain
