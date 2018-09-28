import socket
import struct
import sys

src_addr = '129.82.45.252'
src_port = 9009

stream_id = 512


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


def decode_frame(raw_frame):
    # The format is given according to the following assumption of network data

    # Expect little endian byte order
    endianness = "<"

    # [ commonTimestamp | frame type | Tracked body count | Engaged
    header_format = "qiBB"

    timestamp, frame_type, tracked_body_count, engaged = struct.unpack(endianness + header_format, raw_frame[:struct.calcsize(header_format)])

    # For each body, a header is transmitted
    # TrackingId | HandLeftConfidence | HandLeftState | HandRightConfidence | HandRightState ]
    body_format = "Q4B"

    # For each of the 25 joints, the following info is transmitted
    # [ JointType | TrackingState | Position.X | Position.Y | Position.Z | Orientation.W | Orientation.X | Orientation.Y | Orientation.Z ]
    joint_format = "BB7f"

    frame_format = body_format + (joint_format * 25)

    # Unpack the raw frame into individual pieces of data as a tuple
    frame_pieces = struct.unpack(endianness + (frame_format * (1 if engaged else 0)), raw_frame[struct.calcsize(header_format):])
    
    decoded = (timestamp, frame_type, tracked_body_count, engaged) + frame_pieces

    return decoded


def decode_frame_openpose(raw_frame):
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


def decode_frame_2(raw_frame):
    # The format is given according to the following assumption of network data

    # Expect little endian byte order
    endianness = "<"

    # [ commonTimestamp | frame type | Tracked body count | Engaged
    header_format = "<54f"

    header_int = struct.unpack(endianness + header_format, raw_frame[:struct.calcsize(header_format)])

    # For each body, a header is transmitted
    # TrackingId | HandLeftConfidence | HandLeftState | HandRightConfidence | HandRightState ]
    # body_format = "Q4B"

    # For each of the 25 joints, the following info is transmitted
    # [ JointType | TrackingState | Position.X | Position.Y | Position.Z | Orientation.W | Orientation.X | Orientation.Y | Orientation.Z ]
    joint_format = "3f"

    frame_format = joint_format * 18

    # Unpack the raw frame into individual pieces of data as a tuple
    frame_pieces = struct.unpack(endianness + (frame_format),
                                 raw_frame[struct.calcsize(header_format):])

    decoded = header_format + frame_pieces

    print "decoded", decoded

    return decoded


def recv_all(sock, size):
    result = b''
    while len(result) < size:
        data = sock.recv(size - len(result))
        if not data:
            raise EOFError("Error: Received only {} bytes into {} byte message".format(len(data), size))
        result += data
    return result


def recv_skeleton_frame(sock):
    """
    To read each stream frame from the server
    """
    (load_size,) = struct.unpack("<i", recv_all(sock, struct.calcsize("<i")))
    print "load_size = ", load_size
    return recv_all(sock, load_size)


if __name__ == '__main__':
    s = connect()
    if s is None:
        sys.exit(0)
        
    while True:
        # try:
        f = recv_skeleton_frame(s)
        timestamp, frame_type, tracked_body_count, engaged = decode_frame_openpose(f)[:4]
        print timestamp, frame_type, tracked_body_count, 'Engaged' if engaged == 1.0 else 'Not Engaged'
        # decode_frame_1(f)
        # except:
        #     s.close()
        #     break
        print "\n\n"

