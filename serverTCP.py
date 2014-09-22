#!/usr/bin/env python
import binascii
import socket
import struct
import sys


def tcp_listen(port):
    print("starting server on port %i" % port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', port)
    sock.bind(server_address)
    sock.listen(1)

    while True:
        conn, client_address = sock.accept()
        try:
            # get message length
            msg_length = conn.recv(2)

            # get rest of message
            data = conn.recv(int(binascii.hexlify(msg_length)) - 2)

            # unpack
            req = struct.unpack('!H B %ds' % (int(binascii.hexlify(msg_length)) - 5,), data)

            # check length
            if int(binascii.hexlify(msg_length)) != len(req[2]) + 5:
                print "wrong"

            # number of vowels
            elif req[1] == 0x55:
                x = num_vowels(req[2])
                conn.send(struct.pack('!H H H', 6, req[0], x))
            # disemvowel
            elif req[1] == 0xAA:
                x = disemvowel(req[2])
                y = len(x)
                conn.send(struct.pack('!HH%ds' % (y,), y+4, req[0], x))
            else:
                pass

        finally:
            conn.close()

def num_vowels(msg):
    vowels = "aeiou"
    count = 0
    for letter in msg:
        if letter.lower() in vowels:
            count = count + 1
    return count

def disemvowel(msg):
    vowels = "aeiou"
    for letter in msg:
        if letter.lower() in vowels:
            msg = msg.replace(letter, "")
    return msg

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Error: incorrect number of arguments"
        print "Usage: ./serverTCP.py <port>"
        
    else:
        tcp_listen(int(sys.argv[1]))

