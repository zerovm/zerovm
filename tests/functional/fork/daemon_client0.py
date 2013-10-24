import socket
import sys

server_address = sys.argv[1]
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
try:
    print "\tSENDING MANIFEST.."
    sock.connect(server_address)
    data = sys.stdin.read()
    sock.sendall(data)
    resp = sock.makefile()
    print "\tMANIFEST SENT"
    print "\tRECEIVING REPORT.."
    data = resp.read(1)
    while data:
      sys.stdout.write(data)
      data = resp.read(1)
    print "\tREPORT RECEIVED\n\n"
finally:
    sock.close()

