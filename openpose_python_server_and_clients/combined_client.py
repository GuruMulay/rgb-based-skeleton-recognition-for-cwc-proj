#!/usr/bin/env python

import socket, sys, struct
import time
import numpy as np
import matplotlib.pyplot as plt

src_addr = '129.82.45.252'
src_port = 9009

stream_id1 = 512  # skeleton color stream
stream_id2 = 1024  # lh color stream
stream_id3 = 2048  # rh color stream


def connect(stream_id):
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


def decode_skeleton_frame_openpose(raw_frame):
    # The format is given according to the following assumption of network data

    # Expect little endian byte order
    endianness = "<"

    # [ commonTimestamp | frame type | Tracked body count | Engaged
    header_format = "qhHf"

    timestamp, frame_type, tracked_body_count, engaged = struct.unpack(endianness + header_format,
                                                                       raw_frame[:struct.calcsize(header_format)])

    # For each of the 18 joints, the following info is transmitted
    # [ Position.X | Position.Y | Confidence ]
    joint_format = "3f"

    frame_format = (joint_format * 18)

    print "engaged:", engaged
    # Unpack the raw frame into individual pieces of data as a tuple
    frame_pieces = struct.unpack(endianness + (frame_format),  # * (0 if abs(engaged) < 0.0001 else 1)),
                                 raw_frame[struct.calcsize(header_format):])

    decoded = (timestamp, frame_type, tracked_body_count, engaged) + frame_pieces
    print "decoded:", decoded
    return decoded


def decode_color_frame_openpose(raw_frame):
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


def recv_skeleton_frame(sock):
    """
    To read each stream frame from the server
    """
    (load_size,) = struct.unpack("<i", recv_all(sock, struct.calcsize("<i")))
    print "load_size = ", load_size
    return recv_all(sock, load_size)


if __name__ == '__main__':

    s1 = connect(stream_id1)  # skeleton
    s2 = connect(stream_id2)  # LH
    s3 = connect(stream_id3)  # RH

    if s1 is None:
        sys.exit(0)
    if s2 is None:
        sys.exit(0)
    if s3 is None:
        sys.exit(0)
    
    i = 0
    avg_frame_time = 0.0
    do_plot = True if len(sys.argv) > 1 and sys.argv[1] == '--plot' else False
    
    while True:
        print "======================================================================"
        try:
            t_begin = time.time()
            f1 = recv_skeleton_frame(s1)
            f2 = recv_color_frame(s2)
            f3 = recv_color_frame(s3)
            t_end = time.time()
        except:
            break

        print "Time taken for this frame: {}".format(t_end - t_begin)
        avg_frame_time += (t_end - t_begin)

        # skeleton
        timestamp1, frame_type1, tracked_body_count1, engaged1 = decode_skeleton_frame_openpose(f1)[:4]
        print timestamp1, frame_type1, tracked_body_count1, 'Engaged' if engaged1 == 1.0 else 'Not Engaged'

        # LH
        timestamp2, frame_type, width, height, color_data = decode_color_frame_openpose(f2)
        print timestamp2, frame_type, width, height

        # RH
        timestamp3, frame_type, width, height, color_data = decode_color_frame_openpose(f3)
        print timestamp3, frame_type, width, height

        print "ASSERTION:", (timestamp1 == timestamp2 == timestamp3)
        assert((timestamp1 == timestamp2) and (timestamp2 == timestamp3))

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

    s1.close()
    s2.close()
    s3.close()
    sys.exit(0)
