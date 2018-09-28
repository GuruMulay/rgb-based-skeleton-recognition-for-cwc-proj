import sys
import socket
import select
import struct
import numpy as np
import binascii

import datetime
import time

import subprocess
# USAGE:
# cd C:\openpose-master\openpose-master\windows\x64\Release
# python openpose_server_02.py


HOST = ''
PORT = 9009

stream_id_dict = {
    'ClosestBody': '512',
    'HandColorLH': '1024',
    'HandColorRH': '2048',
    'HeadColor': '4096'
}

# variables dependent on openpose output
n_keypoints = 18
values_per_keypoint = 3
img_width = 64
img_height = 64
head_img_height = 64
head_img_width = 64
n_img_channel = 3


# # dummy data
# np.random.seed(10)
# # kp_arr = np.random.rand(54,)
# kp_arr = np.random.randint(10, size=n_keypoints*values_per_keypoint)
# hand_arr = np.random.randint(10, size=2*img_width*img_height*3)  # 2(images)*img_width*img_height*3(channels)
#
# # print "kr_arr:", kp_arr
# # print "hand_arr:", hand_arr
# kp_tuple = tuple(kp_arr)
# hand_tuple = tuple(hand_arr)


def is_valid_stream_id(stream_id):
    if str(stream_id) in stream_id_dict.values():
        return True
    else:
        print "Client has an invalid stream id"
        return False


def get_stream_type(stream_id):
    for strm_type, strm_id in stream_id_dict.iteritems():
        if strm_id == str(stream_id):
            print "stream id found: ", strm_id, strm_type
            return strm_type


class OpenposeServer():
    """

    :return:
    """
    _number_of_clients = 2
    _connected_clients = {}

    def __init__(self):
        # threading.Thread.__init__(self)
        self._data_received = {}
        self._stopped = False

    def _recv_all(self, sock, size):
        result = b''
        while len(result) < size:
            data = sock.recv(size - len(result))
            if not data:
                raise EOFError("Error: Received only {} bytes into {} byte message".format(len(data), size))
            result += data
        return result

    def is_stopped(self):
        return self._stopped

    def did_all_clients_connect(self):
        if len(self._connected_clients) == self._number_of_clients:
            return True
        else:
            return False

    def run(self):

        # opening up a cpp process
        proc = subprocess.Popen('1_user_asynchronous_output.exe -model_folder C:\\openpose-master\\openpose-master\\models',
            bufsize=4096, stdout=subprocess.PIPE, shell=True)
        person_new_frame_detected = False
        person_keypoints_detected = False
        person_lh_detected = False
        person_rh_detected = False
        # send_all_zeroes = True
        keypoint_array = np.zeros(n_keypoints * values_per_keypoint)
        keypoint_tuple_empty = tuple(keypoint_array)
        image_array = np.zeros(img_width * img_height * n_img_channel)
        image_tuple_empty = tuple(image_array)


        # setting up sockets
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((HOST, PORT))
        server_socket.listen(5)
        print("Server socket:", server_socket)

        # add server socket object to the list of readable connections (inputs)
        inputs = [server_socket]
        outputs = []
        excepts = []
        print("Waiting for clients to connect ...")

        while not self.is_stopped():  # use while for standalone run function; use if for using inside another while  # if not self.is_stopped():

            line = proc.stdout.readline()
            # print "line is -----------", line

            if "Person new frame:" in line:
                # print "new frame is: ", line
                person_new_frame_detected = True
                # keypoint_tuple = ()
                continue

            if "(x, y, score):" in line:
                # print "keypoints are: ", line
                person_keypoints_detected = True
                continue

            if "ImageLeftHand:" in line:
                # print "ImageLeftHand are: ", line
                person_lh_detected = True
                person_keypoints_detected = False  # set to False after the lh line comes
                continue

            if "ImageRightHand:" in line:
                # print "ImageRightHand are: ", line
                person_rh_detected = True
                person_lh_detected = False  # set to False after the rh line comes
                continue

            if "ImageHead:" in line:
                person_head_detected = True
                person_rh_detected = False  # set to False after the head line comes
                continue

            if "[End]" in line:
                person_new_frame_detected = False
                person_keypoints_detected = False
                person_lh_detected = False
                person_rh_detected = False
                person_head_detected = False
                continue

            if person_new_frame_detected:  # useless condition

                try:
                    # returns sockets that are ready to read, write, or except
                    read_socks, write_socks, except_socks = select.select(inputs, outputs, excepts, 0.01)
                    # print("select:", read_socks, write_socks, except_socks)
                except socket.error:
                    for sock in inputs:
                        try:
                            select.select([sock], [], [], 0)  # 4th arg, time_out  if == 0 => poll and never block
                        except:
                            print "Client disconnected. Removing from the connected clients dictionary"
                            inputs.remove(sock)
                            self._connected_clients.pop(sock)
                            # self._unset_sync()  ###
                            # continue  # while -> if: solved SyntaxError: 'continue' not properly in loop

                for sock in read_socks:
                    # a new connection request received on the server_socket
                    if sock == server_socket:
                        client_sock, client_addr = server_socket.accept()
                        print "Client (%s, %s) connected" % client_addr

                        try:
                            stream_id_bytes = self._recv_all(client_sock, 4)
                            stream_id = struct.unpack('<i', stream_id_bytes)[0]
                            print "stream_id = ", stream_id  # , "stream_type = ".format(stream_id_dict[str(stream_id)])
                        except:
                            print "Unable to receive complete stream id. Ignoring the client"
                            client_sock.close()

                        print "Received stream id. Verifying ..."
                        if is_valid_stream_id(stream_id):
                            stream_str = get_stream_type(stream_id)
                            print "Stream is valid: ", stream_str
                            print "Checking if stream is already connected..."
                            if stream_str not in self._connected_clients.values():
                                print "New stream. Accepting the connection {}:{}".format(client_addr[0], client_addr[1])
                                # client_sock.shutdown(socket.SHUT_WR) #o
                                client_sock.shutdown(socket.SHUT_RD)  # shut down further reading on client socket
                                # inputs += [client_sock] #o
                                outputs += [client_sock]  # add the socket in output list
                                self._connected_clients[client_sock] = stream_str
                            else:
                                print "Stream already exists. Rejecting the connection."
                                client_sock.close()
                        else:
                            print "Rejecting invalid stream with stream id: {}".format(stream_id)
                            client_sock.close()

                    # a message from/to a client on new socket (sock), not a new connection on server_socket
                    else:
                        print "non-server-connect socket detected in the inputs"
                        break

                t_now = datetime.datetime.now()
                t_now_abs = time.mktime(t_now.timetuple())

                # send the data to the clients or process the data received from the clients
                # send only if all the clients connected? No
                for send_sock in write_socks:
                    if send_sock in self._connected_clients:
                        # print "send socket is present in the connected clients dictionary"

                        if "ClosestBody" in self._connected_clients[send_sock]:
                            # print "line", line
                            frame_type = int(stream_id_dict["ClosestBody"])
                            tracked_body_count = 100
                            engaged = 0
                            load_size = struct.calcsize('qiHH54f')

                            if person_keypoints_detected:
                                if "Undefined" in line:
                                    print "No skeleton found! Sending all zeroes."
                                    packed_data = struct.pack('<iqiHH54f', load_size, t_now_abs, frame_type,
                                                              tracked_body_count,
                                                              engaged, *keypoint_tuple_empty)  # send empty tuple
                                else:
                                    assert (len([float(kp) for kp in line[:-3].split(' ')]) == 1 + (n_keypoints * values_per_keypoint))  # 1 + because of engagedBit
                                    packed_data = struct.pack('<iqhH55f', load_size, t_now_abs, frame_type,
                                                              tracked_body_count,
                                                              *[float(kp) for kp in line[:-3].split(' ')])  # engaged bit is included in the *[] list
                                    # print "keypoints in floats: ", [float(kp) for kp in line[:-3].split(' ')]  # -3 since it removes '\r', '\n', and ''
                                person_keypoints_detected = False

                                try:
                                    send_sock.sendall(packed_data)
                                except:
                                    # broken socket connection
                                    send_sock.close()
                                    print "broken connection, removing {}".format(send_sock)
                                    # broken socket, remove it from * *
                                    self._connected_clients.pop(send_sock)
                                    if send_sock in outputs:
                                        outputs.remove(send_sock)

                        if "HandColorLH" in self._connected_clients[send_sock]:
                            # print "line", line
                            frame_type = int(stream_id_dict["HandColorLH"])
                            load_size = struct.calcsize('qiHH' + str(img_width * img_height * n_img_channel) + 'H')  # 1(images)*img_width*img_height*3(channels)

                            frame_type = 0   # => left hand

                            if person_lh_detected:
                                if "left hand unknown" in line:
                                    print "No LH found! Sending all zeroes."
                                    packed_data = struct.pack('<iqiHH' + str(img_width * img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, img_width, img_height, *image_tuple_empty)

                                else:
                                    assert (len([int(px) for px in line[:-3].split(' ')]) == img_width * img_height * n_img_channel)
                                    print "Sending lh image with length ---------- len(LH) = ", len([int(px) for px in line[:-3].split(' ')])
                                    packed_data = struct.pack('<iqiHH' + str(img_width * img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, img_width, img_height, *[int(px) for px in line[:-3].split(' ')])

                                person_lh_detected = False

                                try:
                                    send_sock.sendall(packed_data)
                                except:
                                    # broken socket connection
                                    send_sock.close()
                                    print "broken connection, removing {}".format(send_sock)
                                    # broken socket, remove it from * *
                                    self._connected_clients.pop(send_sock)
                                    if send_sock in outputs:
                                        outputs.remove(send_sock)

                            frame_type = 1  # => right hand

                            if person_rh_detected:
                                if "right hand unknown" in line:
                                    print "No RH found! Sending all zeroes."
                                    packed_data = struct.pack('<iqiHH' + str(img_width * img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, img_width, img_height, *image_tuple_empty)

                                else:
                                    assert (len([int(px) for px in line[:-3].split(' ')]) == img_width * img_height * n_img_channel)
                                    print "Sending rh image with length ---------- len(RH) = ", len([int(px) for px in line[:-3].split(' ')])
                                    packed_data = struct.pack('<iqiHH' + str(img_width * img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, img_width, img_height, *[int(px) for px in line[:-3].split(' ')])

                                person_rh_detected = False

                                try:
                                    send_sock.sendall(packed_data)
                                except:
                                    # broken socket connection
                                    send_sock.close()
                                    print "broken connection, removing {}".format(send_sock)
                                    # broken socket, remove it from * *
                                    self._connected_clients.pop(send_sock)
                                    if send_sock in outputs:
                                        outputs.remove(send_sock)

                        if "HeadColor" in self._connected_clients[send_sock]:
                            frame_type = int(stream_id_dict["HeadColor"])
                            load_size = struct.calcsize('qiHH' + str(head_img_width * head_img_height * n_img_channel) + 'H')

                            if person_head_detected:
                                if "head unknown" in line:
                                    print "No Head found! Sending all zeroes."
                                    packed_data = struct.pack('<iqiHH' + str(head_img_width * head_img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, head_img_width, head_img_height, *image_tuple_empty)

                                else:
                                    assert (len([int(px) for px in line[:-3].split(' ')]) == head_img_width * head_img_height * n_img_channel)
                                    print "Sending head image with length ---------- len(head) = ", len([int(px) for px in line[:-3].split(' ')])
                                    packed_data = struct.pack('<iqiHH' + str(head_img_width * head_img_height * n_img_channel) + 'H', load_size,
                                                              t_now_abs, frame_type, head_img_width, head_img_height, *[int(px) for px in line[:-3].split(' ')])

                                person_head_detected = False

                                try:
                                    send_sock.sendall(packed_data)
                                except:
                                    # broken socket connection
                                    send_sock.close()
                                    print "broken connection, removing {}".format(send_sock)
                                    # broken socket, remove it from * *
                                    self._connected_clients.pop(send_sock)
                                    if send_sock in outputs:
                                        outputs.remove(send_sock)


                person_new_frame_detected = True

        server_socket.close()


if __name__ == "__main__":
    print ""
    opserver = OpenposeServer()
    sys.exit(opserver.run())

    # run_openpose()



#
# //POSE_COCO_BODY_PARTS{
# //	{ 0,  "Nose" },
# //	{ 1,  "Neck" },
# //	{ 2,  "RShoulder" },
# //	{ 3,  "RElbow" },
# //	{ 4,  "RWrist" },
# //	{ 5,  "LShoulder" },
# //	{ 6,  "LElbow" },
# //	{ 7,  "LWrist" },
# //	{ 8,  "RHip" },
# //	{ 9,  "RKnee" },
# //	{ 10, "RAnkle" },
# //	{ 11, "LHip" },
# //	{ 12, "LKnee" },
# //	{ 13, "LAnkle" },
# //	{ 14, "REye" },
# //	{ 15, "LEye" },
# //	{ 16, "REar" },
# //	{ 17, "LEar" },
# //	{ 18, "Bkg" },
# //}


