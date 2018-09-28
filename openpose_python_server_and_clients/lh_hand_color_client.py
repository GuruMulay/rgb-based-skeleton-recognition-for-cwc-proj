#!/usr/bin/env python

import socket, sys, struct
import time
import numpy as np
import matplotlib.pyplot as plt

src_addr = '129.82.45.252'
src_port = 9009

stream_id = 1024  # lh color stream


def connect():
    """
    Connect to a specific port
    """

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

    color_data_format = str(width*height*3) + "H"  # 1(image)*img_width*img_height*3(channels)

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

    s = connect()
    if s is None:
        sys.exit(0)
    
    i = 0
    avg_frame_time = 0.0
    do_plot = True if len(sys.argv) > 1 and sys.argv[1] == '--plot' else False
    
    while True:
        try:
            t_begin = time.time()
            f = recv_color_frame(s)
            t_end = time.time()
        except:
            break
        print "Time taken for this frame: {}".format(t_end - t_begin)
        avg_frame_time += (t_end - t_begin)
        timestamp, frame_type, width, height, color_data = decode_frame_openpose(f)
        print timestamp, frame_type, width, height

        print "len of color image: ", len(color_data)

        if do_plot and i % 160 == 0 and height*width > 0:
            fig = plt.figure()

            image_rgb = np.array(color_data[0:len(color_data)], dtype='uint8').reshape((height, width, 3))
            image = np.zeros((height, width, 3))
            im = plt.imshow(image_rgb, cmap='gray')

            plt.title("Left hand" if frame_type == 0 else "Right hand")
            plt.show()

        if do_plot and i % 161 == 0 and height * width > 0:
            fig = plt.figure()

            image_rgb = np.array(color_data[0:len(color_data)],  dtype='uint8').reshape((height, width, 3))
            im = plt.imshow(image_rgb, cmap='gray')

            plt.title("Left hand" if frame_type == 1 else "Right hand")
            plt.show()

        print "\n\n"
        i += 1

    if i != 0:
        print "Total frame time: {}".format(avg_frame_time)
        avg_frame_time /= i
        print "Average frame time over {} frames: {}".format(i, avg_frame_time)

    s.close()
    sys.exit(0)
