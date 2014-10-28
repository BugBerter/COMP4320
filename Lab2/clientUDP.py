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
    req_checksum = checksum(msg, 2)
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

            # check if it was an error case
            err_msg = struct.unpack('!5B', data[:5])
            err_checksum = checksum(err_msg, 0)
            if err_msg == (err_checksum, 24, requestID, 0, 0):
                print "Received bad checksum error from server..."
                continue
            if err_msg == (err_checksum, 127, 127, 0, 0):
                print "Received length mismatch error from server..."
                continue

            # happy case
            # grab msg_length
            msg_length_tuple = struct.unpack('!2B', data[:2])
            msg_length = (msg_length_tuple[0] << 8) + msg_length_tuple[1]
            print "received msg_length: %d" % msg_length

            # get rest of message
            try:
                resp = struct.unpack('!%dB' % msg_length, data[:msg_length])
            except struct.error:
                print "invalid message length..."
                print ' '.join(x.encode('hex') for x in data)
                continue

            resp_checksum = checksum(resp, 2)
            print "resp_checksum = %d\nrec checksum = %d" % (resp_checksum, resp[2])
            if resp_checksum != resp[2]:
                print "Bad checksum..."
                print ' '.join(x.encode('hex') for x in data)
                continue
            else:
                ip_addresses = resp[5:]
                for i, host in enumerate(hostnames):
                    print host + ": %d.%d.%d.%d" % (ip_addresses[i*4 + 3],ip_addresses[i*4+2],ip_addresses[i*4+1],ip_addresses[i*4])
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

def checksum(msg, checksum_idx):
    CARRY_MASK = 0xff00
    sum = 0
    for i, b in enumerate(msg):
        # skip checksum field
        if i != checksum_idx:
            sum += b
            sum += (sum & CARRY_MASK) >> 8
        # extract carry
    return ((~sum) & 0xff)

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print "Error: incorrect number of arguments"
        print "Usage: ./clientTCP.py <server> <port> <requestID> <n1> ... <nm>"

    else:
        udp_client(sys.argv)
