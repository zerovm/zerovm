import socket
import sys

server_address = sys.argv[1]
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
try:
    sock.connect(server_address)
    data = sys.stdin.read()
    sock.sendall(data)
    resp = sock.makefile()
    print "sent"
    data = resp.read(8)
    size = int(data, 0)
    data = resp.read(size)
    print data
finally:
    sock.close()

