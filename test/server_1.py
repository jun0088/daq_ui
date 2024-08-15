# 导入必要的库
import socket

# 创建一个socket对象，选择AF_INET，表示IPv4地址，和SOCK_STREAM，表示TCP协议
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 绑定到一个地址和端口
server_address = ('localhost', 12345)
server_socket.bind(server_address)

# 监听连接
server_socket.listen(1)
print("服务器已启动，等待连接...")

# 服务器循环
while True:
    # 接受客户端连接
    client_socket, client_address = server_socket.accept()
    print("客户端已连接:", client_address)

    # 接收客户端发送的数据
    data = client_socket.recv(1024)
    if data:
        print("接收到的数据:", data.decode())

        # 向客户端发送数据
        response = "abcccccccccc"
        client_socket.send(response.encode())
        print("发送的数据:", response)

    # 关闭连接
    client_socket.close()
    print("客户端连接已关闭。")

# 关闭服务器套接字
server_socket.close()
print("服务器已关闭。")
