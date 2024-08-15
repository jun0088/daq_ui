import socket

SERVER_ADDRESS = '192.168.0.200'
SERVER_PORT = 7602
RECV_SIZE = 1000

def client():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((SERVER_ADDRESS, SERVER_PORT))
    print(f'connected to {SERVER_ADDRESS}:{SERVER_PORT}')

    data = client_socket.recv(RECV_SIZE*4)
    int_data = [int.from_bytes(data[i:i+4], byteorder='big') for i in range(0, len(data), 4)]
    # print(f'recv data: {data.decode()}')
    file = open("output.txt", "w")
    for d in int_data:
        file.write(d)
    file.close()

    client_socket.close()
    print('client closed')

if __name__ == '__main__':
    client()