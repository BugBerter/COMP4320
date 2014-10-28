#!/usr/bin/env python
import binascii
import socket
import struct
import sys
import time
import datetime


def udp_client(args):
    # vars
    server = args[1]
    port = int(args[2])
    requestID = int(args[3])
    hostnames = args[4:]
    GID = 24

    # compute message length
    msg_length = 5
    for host in hostnames:
        # add 1 for ~ and the hostname length to the msg length
        msg_length = msg_length + 1 + len(host)

    # compose msg
    msg = form_message(msg_length, GID, requestID, hostnames)

    # compute checksum
    req_checksum = checksum(msg)
    msg[2] = req_checksum

    # format request
    req = struct.pack('!%dB' % len(msg), *msg)

    conn = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = (server, port)

    try:
        for x in range(0, 7):
            print "Attempt #%d: sending..." % (x + 1)
            conn.sendto(req, server_address)
            # get message length
            data = conn.recv(1024)

            # grab msg_length
            msg_length_tuple = struct.unpack('!2B', data[:2])
            msg_length = (msg_length_tuple[1] << 8) + msg_length_tuple[0]

            # get rest of message
            try:
                resp = struct.unpack('!%dB' % msg_length, data)
            except struct.error:
                print "invalid message length..."
                print ''.join(x.encode('hex') for x in data)
                continue

            resp_checksum = checksum(resp)
            if resp_checksum != resp[2]:
                print "Bad checksum..."
                continue
            else:
                ip_addresses = resp[5:]
                for i, host in enumerate(hostnames):
                    print host + ": %d.%d.%d.%d" % (ip_addresses[i*4],ip_addresses[i*4+1],ip_addresses[i*4+2],ip_addresses[i*4+3])
                break

    finally:
        conn.close()

def form_message(msg_length, gid, rid, hostnames):
    # compose msg
    msg = []

    # add msg_length
    msg.append(msg_length >> 8)
    msg.append(msg_length & 0x00ff)

    # add checksum, 0 by default for computation purposes (3rd byte)
    msg.append(0)

    # add GID (4th byte)
    msg.append(gid)

    # add request ID (5th byte)
    msg.append(rid)

    for host in hostnames:
        msg.append(ord('~'))
        for c in host:
            msg.append(ord(c))

    return msg

def checksum(msg):
    temp = 0
    for i, b in enumerate(msg):
        # skip checksum field
        if i != 2:
            temp += b
            temp += (temp & 0xFF) >> 8
    return (~temp) & 0xFF

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print "Error: incorrect number of arguments"
        print "Usage: ./clientTCP.py <server> <port> <requestID> <n1> ... <nm>"

    else:
        udp_client(sys.argv)
