import socket
import time

SERVER_ADDRESS = '192.168.0.6'
READ_CHANNEL = 1
DATA_SIZE = 1000
RECV_SIZE = DATA_SIZE*4

def RST(s):
    msg = '>RST()\r\n'
    s.send(msg.encode())
    data = s.recv(1024).decode()
    print(data)
    return 0

def channel_ctrl(s, channel, ctrl):
    msg = f'>channel_ctrl({channel},{ctrl})\r\n'
    s.send(msg.encode())
    data = s.recv(1024).decode()
    print(data)
    return 0

def sample_rate(s, rate):
    msg = f'>sample_rate({rate})\r\n'
    s.send(msg.encode())
    data = s.recv(1024).decode()
    print(data)
    return 0

def sample_enable(s, enable):
    msg = f'>sample_enable({enable})\r\n'
    s.send(msg.encode())
    data = s.recv(1024).decode()
    print(data)
    return 0


if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((SERVER_ADDRESS, 7601))

    RST(s)

    channel_ctrl(s, READ_CHANNEL, 1)
    sample_rate(s, 200000)
    sample_enable(s, 1)
    s.close()

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.settimeout(3)
    client_socket.connect((SERVER_ADDRESS, 7602+READ_CHANNEL))
    print(f'connected to {SERVER_ADDRESS}:{7602+READ_CHANNEL}')
    # data = client_socket.recv(RECV_SIZE)
    # print(f'recv data: {data}')

    read_count = 0
    read_data = b''
    while True:
        data = client_socket.recv(RECV_SIZE)
        # print(f'recv data: {data}')
        if read_count + len(data) < RECV_SIZE:
            read_data += data
            read_count += len(data)
        else:
            read_data += data[:RECV_SIZE - read_count]
            read_count = RECV_SIZE
            break
    # for byte in data:
    #     print(f'{byte:02x}', end=' ')

    int_data = [int.from_bytes(read_data[i:i+4], byteorder='big', signed=True) for i in range(0, len(read_data), 4)]
    # print(f'recv data: {data.decode()}')
    file = open("output.txt", "w")
    for d in int_data:
        file.write(f"{d}\n") 
    # for i,d in enumerate(read_data):
        # file.write(f"{hex(d)[2:].zfill(2)}")
        # if (i+1)%4==0:
        #     file.write("\n")
        # else:
        #     file.write(" ")
    file.close()

    client_socket.close()
    print('client closed')
