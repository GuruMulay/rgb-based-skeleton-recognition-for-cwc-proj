#!/usr/bin/env python

import socket, sys, struct
import time
'''import numpy as np
import matplotlib.pyplot as plt
from realtime_hand_recognition import RealTimeHandRecognition
import os
import cv2'''
import random

src_addr = '129.82.45.252'
src_port = 9009


# lh and rh color stream


def connect_rgb(hand):
    """
    Connect to a specific port
    """
    if hand == "LH":
        stream_id = 1024
    elif hand == "RH":
        stream_id = 2048

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect((src_addr, src_port))
    except:
        print "Error connecting to {}:{}".format(src_addr, src_port)
        return None

    try:
        print "Sending stream info"
        sock.sendall(struct.pack('<i', stream_id))
    except:
        print "Error: Stream rejected"
        return None

    print "Successfully connected to host"
    return sock


def decode_frame_openpose(raw_frame):
    # The format is given according to the following assumption of network data

    # Expect little endian byte order
    endianness = "<"

    # [ commonTimestamp | frame type (0 => LH; 1 => RH)| img_width | img_height ]
    header_format = "qiHH"
    header_size = struct.calcsize(endianness + header_format)
    timestamp, frame_type, width, height = struct.unpack(endianness + header_format,
                                                         raw_frame[:struct.calcsize(header_format)])

    color_data_format = str(width * height * 3) + "H"  # 1(image)*img_width*img_height*3(channels)

    color_data = struct.unpack_from(endianness + color_data_format, raw_frame, header_size)

    decoded = (timestamp, frame_type, width, height, list(color_data))

    # print "decoded:", decoded
    return decoded


def recv_all(sock, size):
    result = b''
    while len(result) < size:
        data = sock.recv(size - len(result))
        if not data:
            raise EOFError("Error: Received only {} bytes into {} byte message".format(len(data), size))
        result += data
    return result


def recv_color_frame(sock):
    """
    Experimental function to read each stream frame from the server
    """
    (frame_size,) = struct.unpack("<i", recv_all(sock, 4))
    return recv_all(sock, frame_size)


if __name__ == '__main__':

    '''if hand == "RH":
        FRAME_TYPE = 1
    elif hand == "LH":
        FRAME_TYPE = 0'''


    s_rh = connect_rgb("RH")
    s_lh = connect_rgb("LH")

    if s_rh is None or s_lh is None:
        sys.exit(0)

    i = 0
    j = 0
    avg_frame_time = 0.0

    start_time = time.time()

    lh_list = []
    rh_list = []
    synchronized = False
    while True:
        for s, FRAME_TYPE in zip([s_lh, s_rh],[0,1]):
            try:
                f = recv_color_frame(s)
            except KeyboardInterrupt:
                sys.exit(0)
            except:
                break

            timestamp, frame_type, width, height, color_data = decode_frame_openpose(f)
            print j, timestamp, frame_type, width, height

            if FRAME_TYPE == 0:
                lh_list.append(timestamp)
            else:
                rh_list.append(timestamp)

            time.sleep(random.uniform(0,0.15))
            '''if height * width > 0 and frame_type == FRAME_TYPE:

                image_rgb = np.array(color_data[0:len(color_data)], dtype='uint8').reshape((height, width, 3))


                i += 1

                if i % 100 == 0:
                    print "=" * 100, "FPS", 100 / (time.time() - start_time)
                    start_time = time.time()'''

        j += 1

        if not synchronized:
            common_timestamp = list(set(lh_list).intersection(set(rh_list)))
            if len(common_timestamp) == 1:
                print "Synchronized at timestamp ", common_timestamp
                lh_index = lh_list.index(common_timestamp[0])
                rh_index = rh_list.index(common_timestamp[0])
                print "LH Index",lh_index,"RH Index",rh_index
                synchronized = True

        print '='*30

        if len(lh_list) % 100 == 0:
            for l, r in zip(lh_list[lh_index:], rh_list[rh_index:]):
                print l, r
                if l!=r:
                    print "Missing Frame ",l,r
                    raw_input()




    s_rh.close()
    s_lh.close()
    sys.exit(0)
