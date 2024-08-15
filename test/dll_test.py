import ctypes as C
import numpy as np
import time
import matplotlib.pyplot as plt
from nptdms import TdmsWriter, ChannelObject, TdmsFile
import struct

tdms_file = "C:/Users/prm/Desktop/cdaq/test/read_data.tdms"
dll = C.CDLL('C:/Users/prm/Desktop/cdaq/test/libdaq.dll')


def run(filename, config, channel_size):

    # print(config, end = "")
    dll.init()
    key = dll.board_init(config.encode('utf-8'))
    if key < 0:
        print("board init failed")
        return -1
    
    channel_count = dll.get_channel_count(key)
    if channel_count < 0:
        print("get channel count failed")
        return -2
    print("channel count: ", channel_count)

    if dll.sample_rate(key, 25000) < 0:
        print("set sample rate failed")
        return -3

    if dll.sample_enable(key, 1) < 0:
        print("set sample enable failed")
        return -4

    data_size = channel_count*channel_size
    c_data = (C.c_float * data_size)()

    if dll.read(key, c_data, data_size, channel_size) < 0:
        print("read failed")
        return -5
    print("read success")

    with open('output.txt', 'w') as f:
        for val in c_data:
            f.write(str(val) + '\n')

    # write_tdms(filename, channel_count, c_data)
    print("write success")

    if dll.board_free(key) < 0:
        print("board free failed")
        return -6
    print("board free success")    

    dll.rst()
    return 0


def write_tdms(filename, channel_count, data):
    data = np.frombuffer(data, dtype=np.byte)
    data = data.reshape(channel_count, channel_size)
    with TdmsWriter(filename) as writer:
        channel = []
        for i in range(channel_count):
            channel.append(ChannelObject('Group1', 'Channel' + str(i+1), data[i]))
        writer.write_segment(channel)
    return 0


def loop_read(key, c_data, data_size, channel_size):
    count = 1
    while True:
        result2 = dll.read(key, c_data, data_size, channel_size)
        if result2 < 0:
            return -1  
        count += 1
        print("count:", count)
    return 0


def draw_tdms(filename, sample_rate):
    print("tdms_file")
    tdms_file = TdmsFile(filename)
    
    group_name = "Group1"

    channel1_name = "Channel1"
    channel2_name = "Channel2"
    channel1_data = tdms_file[group_name][channel1_name].data
    channel2_data = tdms_file[group_name][channel2_name].data
    time = np.linspace(0, len(channel1_data) / sample_rate, len(channel1_data))

    plt.figure(figsize=(12, 6))
    plt.plot(time, channel1_data, label="Channel 1")
    plt.plot(time, channel2_data, label="Channel 2")
    plt.xlabel("Time (s)")
    plt.ylabel("Value")
    plt.title("TDMS File Data")
    plt.legend()
    plt.grid()
    plt.show()
    return 0

def darw(channel_count, channel_size):
    float_data = []
    with open('output.txt', 'r') as f:
        for line in f:
            float_data.append(float(line.strip()))
    # 将数据划分为不同的通道
    channels = [float_data[i:i+channel_size] for i in range(0, len(float_data), channel_size)]

    # 绘制波形图
    fig, ax = plt.subplots(channel_count, 1, figsize=(12, 8))
    if channel_count == 1:
        ax = [ax]

    for i, channel in enumerate(channels):
        ax[i].plot(channel)
        ax[i].set_title(f'Channel {i+1}')
        ax[i].set_xlabel('Sample')
        ax[i].set_ylabel('Amplitude')

    plt.tight_layout()
    plt.savefig('waveform.png')

if __name__ == '__main__':
    config = """
192.168.0.173/ai0?ctrl=1
192.168.0.173/ai1?ctrl=1
"""
    channel_size = 10000
    tdms_file = "C:/Users/prm/Desktop/cdaq/test/read_data.tdms"
    rc = run(tdms_file, config, channel_size)
    if rc < 0:
        print("run failed", rc)
    darw(2, channel_size)
    # print("run success")
    # draw_tdms(tdms_file, 250000)
    # print("end")