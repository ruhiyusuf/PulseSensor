import random
import socket
import time

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
# server_socket.bind(('', 12000))
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
#   server_socket.setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST
#		broadcastEnable, sizeof(broadcastEnable))
server_socket.settimeout(0.2)
while True:
    # message, address = server_socket.recvfrom(1024)
    # message = message.upper()
    message = b"hello!!"
    server_socket.sendto(message, ('<broadcast>', 37020))
    # server_socket.sendto(message, address)
    time.sleep(1)
