import socket
import time
import random
import struct
def is_socket_connected(sock):
    try:
        remote_addr = sock.getpeername()
        return True
    except socket.error:
        return False

def server_init(ip, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', 7602)
    server_socket.bind(server_address)
    server_socket.listen(5)
    return server_socket



if __name__ == '__main__':
    server_socket = server_init('localhost', 12345)
    print("waiting connect...")
    
    while True:
        
        client_socket, client_address = server_socket.accept()
        print(f"client:{client_address} is connected!!!")
        try:
        # data = client_socket.recv(1024)
        # if data:
        #     print(f"receive data:{data}")
            flag = 0
            c = 0
            while 1:
                data = struct.pack('<4i',*list(range(c,c+4)))
                c+=1
                client_socket.send(data)
                time.sleep(.001)
        except Exception as e:
            print(e)
            # client_socket.close()
        # client_socket.close()
        print(f"client {client_address} is closed...")

    server_socket.close()
    print("server close ...")
 