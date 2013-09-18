import socket
import sys
import struct
import ctypes
from time import sleep

peers = int(sys.argv[1])
port = 0
if len(sys.argv) > 2 and sys.argv[2]:
    port = int(sys.argv[2])
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('', port))
print s.getsockname()[1]
bind_map = {}
conn_map = {}
peer_map = {}
while 1:
    try:
        message, address = s.recvfrom(65535)
        print [str(message), len(message)]
        offset = 0
        alias = struct.unpack_from('!I', message, offset)[0]
        print '%08x = %s:%d' % (alias, address[0], address[1])
        offset += 4
        bind_count = struct.unpack_from('!I', message, offset)[0]
        offset += 4
        connect_count = struct.unpack_from('!I', message, offset)[0]
        offset += 4
        for i in range(bind_count):
            h, port = struct.unpack_from('!IH', message, offset)[0:3]
            bind_map.setdefault(alias, {})[h] = port
            print '%08x:%d <- %08x' % (alias, port, h)
            offset += 6
        conn_map[alias] = (connect_count, offset, ctypes.create_string_buffer(message))
        peer_map.setdefault(alias, {})[0] = address[0]
        peer_map.setdefault(alias, {})[1] = address[1]

        if len(peer_map) == peers:
            for src in peer_map.iterkeys():
                (connect_count, offset, reply) = conn_map[src]
                #offset = 0
                #count = struct.unpack_from('!I', reply, offset)[0]
                #offset += 4
                print [connect_count, offset]
                for i in range(connect_count):
                    h = struct.unpack_from('!I', reply, offset)[0]
                    port = bind_map[h][src]
                    struct.pack_into('!4sH', reply, offset, socket.inet_pton(socket.AF_INET, peer_map[src][0]), port)
                    offset += 6
                s.sendto(reply, (peer_map[src][0], peer_map[src][1]))
                print ['sending to: ', peer_map[src][0], peer_map[src][1]]
            sleep(2)
            exit(0)
    except (KeyboardInterrupt):
        exit(1)

