#!/usr/bin/env python
import binascii
import socket
import struct
import sys
import time
import datetime


def tcp_client(server, port, operation, msg):
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (server, int(port))
    conn.connect(server_address)

    # format request
    req = struct.pack('!H H B %ds' %(len(msg),), len(msg)+5, 1, int(operation), msg)

    try:
        print "sending..."
        start_time = time.time()
        conn.sendall(req)
        # get message length
        msg_length = conn.recv(2)
        # get rest of message
        data = conn.recv(int(binascii.hexlify(msg_length), 16) - 2)

        end_time = time.time()
        uptime = end_time - start_time

        # format print results

        if int(operation) == 0x55:
            resp = struct.unpack('!H H', data)
            print "Request ID: %d" % resp[0]
            print "Answer: %d" % resp[1]
            print "Trip time: %s ms" % str(uptime * 1000)

        if int(operation) == 0xAA:
            resp = struct.unpack('!H %ds' % (int(binascii.hexlify(msg_length), 16) - 4,), data)
            print "Request ID: %d" % resp[0]
            print "Returned string: %s" % str(resp[1])
            print "Trip time: %s ms" % str(uptime * 1000)

    finally:
        conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print "Error: incorrect number of arguments"
        print "Usage: ./clientTCP.py <server> <port> <operation> <message>"
        
    else:
        tcp_client(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
