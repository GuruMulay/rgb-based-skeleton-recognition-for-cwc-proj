// original_openpose_main
//// ------------------------- OpenPose Library Tutorial - Real Time Pose Estimation -------------------------
//// If the user wants to learn to use the OpenPose library, we highly recommend to start with the `examples/tutorial_*/` folders.
//// This example summarizes all the funcitonality of the OpenPose library:
//// 1. Read folder of images / video / webcam  (`producer` module)
//// 2. Extract and render body keypoint / heatmap / PAF of that image (`pose` module)
//// 3. Extract and render face keypoint / heatmap / PAF of that image (`face` module)
//// 4. Save the results on disk (`filestream` module)
//// 5. Display the rendered pose (`gui` module)
//// Everything in a multi-thread scenario (`thread` module)
//// Points 2 to 5 are included in the `wrapper` module
//// In addition to the previous OpenPose modules, we also need to use:
//// 1. `core` module:
//// For the Array<float> class that the `pose` module needs
//// For the Datum struct that the `thread` module sends between the queues
//// 2. `utilities` module: for the error & logging functions, i.e. op::error & op::log respectively
//// This file should only be used for the user to take specific examples.
//
//// C++ std library dependencies
//#include <chrono> // `std::chrono::` functions and classes, e.g. std::chrono::milliseconds
//#include <thread> // std::this_thread
//// Other 3rdparty dependencies
//// GFlags: DEFINE_bool, _int32, _int64, _uint64, _double, _string
//#include <gflags/gflags.h>
//// Allow Google Flags in Ubuntu 14
//#ifndef GFLAGS_GFLAGS_H_
//namespace gflags = google;
//#endif
//// OpenPose dependencies
//#include <openpose/headers.hpp>
//
//// See all the available parameter options withe the `--help` flag. E.g. `./build/examples/openpose/openpose.bin --help`.
//// Note: This command will show you flags for other unnecessary 3rdparty files. Check only the flags for the OpenPose
//// executable. E.g. for `openpose.bin`, look for `Flags from examples/openpose/openpose.cpp:`.
//// Debugging
//DEFINE_int32(logging_level, 3, "The logging level. Integer in the range [0, 255]. 0 will output any log() message, while"
//	" 255 will not output any. Current OpenPose library messages are in the range 0-4: 1 for"
//	" low priority messages and 4 for important ones.");
//// Producer
//DEFINE_int32(camera, -1, "The camera index for cv::VideoCapture. Integer in the range [0, 9]. Select a negative"
//	" number (by default), to auto-detect and open the first available camera.");
//DEFINE_string(camera_resolution, "1280x720", "Size of the camera frames to ask for.");
//DEFINE_double(camera_fps, 30.0, "Frame rate for the webcam (only used when saving video from webcam). Set this value to the"
//	" minimum value between the OpenPose displayed speed and the webcam real frame rate.");
//DEFINE_string(video, "", "Use a video file instead of the camera. Use `examples/media/video.avi` for our default"
//	" example video.");
//DEFINE_string(image_dir, "", "Process a directory of images. Use `examples/media/` for our default example folder with 20"
//	" images. Read all standard formats (jpg, png, bmp, etc.).");
//DEFINE_string(ip_camera, "", "String with the IP camera URL. It supports protocols like RTSP and HTTP.");
//DEFINE_uint64(frame_first, 0, "Start on desired frame number. Indexes are 0-based, i.e. the first frame has index 0.");
//DEFINE_uint64(frame_last, -1, "Finish on desired frame number. Select -1 to disable. Indexes are 0-based, e.g. if set to"
//	" 10, it will process 11 frames (0-10).");
//DEFINE_bool(frame_flip, false, "Flip/mirror each frame (e.g. for real time webcam demonstrations).");
//DEFINE_int32(frame_rotate, 0, "Rotate each frame, 4 possible values: 0, 90, 180, 270.");
//DEFINE_bool(frames_repeat, false, "Repeat frames when finished.");
//DEFINE_bool(process_real_time, false, "Enable to keep the original source frame rate (e.g. for video). If the processing time is"
//	" too long, it will skip frames. If it is too fast, it will slow it down.");
//// OpenPose
//DEFINE_string(model_folder, "models/", "Folder path (absolute or relative) where the models (pose, face, ...) are located.");
//DEFINE_string(output_resolution, "-1x-1", "The image resolution (display and output). Use \"-1x-1\" to force the program to use the"
//	" input image resolution.");
//DEFINE_int32(num_gpu, -1, "The number of GPU devices to use. If negative, it will use all the available GPUs in your"
//	" machine.");
//DEFINE_int32(num_gpu_start, 0, "GPU device start number.");
//DEFINE_int32(keypoint_scale, 0, "Scaling of the (x,y) coordinates of the final pose data array, i.e. the scale of the (x,y)"
//	" coordinates that will be saved with the `write_keypoint` & `write_keypoint_json` flags."
//	" Select `0` to scale it to the original source resolution, `1`to scale it to the net output"
//	" size (set with `net_resolution`), `2` to scale it to the final output size (set with"
//	" `resolution`), `3` to scale it in the range [0,1], and 4 for range [-1,1]. Non related"
//	" with `scale_number` and `scale_gap`.");
//// OpenPose Body Pose
//DEFINE_bool(body_disable, false, "Disable body keypoint detection. Option only possible for faster (but less accurate) face"
//	" keypoint detection.");
//DEFINE_string(model_pose, "COCO", "Model to be used. E.g. `COCO` (18 keypoints), `MPI` (15 keypoints, ~10% faster), "
//	"`MPI_4_layers` (15 keypoints, even faster but less accurate).");
//DEFINE_string(net_resolution, "656x368", "Multiples of 16. If it is increased, the accuracy potentially increases. If it is"
//	" decreased, the speed increases. For maximum speed-accuracy balance, it should keep the"
//	" closest aspect ratio possible to the images or videos to be processed. Using `-1` in"
//	" any of the dimensions, OP will choose the optimal aspect ratio depending on the user's"
//	" input value. E.g. the default `-1x368` is equivalent to `656x368` in 16:9 resolutions,"
//	" e.g. full HD (1980x1080) and HD (1280x720) resolutions.");
//DEFINE_int32(scale_number, 1, "Number of scales to average.");
//DEFINE_double(scale_gap, 0.3, "Scale gap between scales. No effect unless scale_number > 1. Initial scale is always 1."
//	" If you want to change the initial scale, you actually want to multiply the"
//	" `net_resolution` by your desired initial scale.");
//DEFINE_bool(heatmaps_add_parts, false, "If true, it will add the body part heatmaps to the final op::Datum::poseHeatMaps array,"
//	" and analogously face & hand heatmaps to op::Datum::faceHeatMaps & op::Datum::handHeatMaps"
//	" (program speed will decrease). Not required for our library, enable it only if you intend"
//	" to process this information later. If more than one `add_heatmaps_X` flag is enabled, it"
//	" will place then in sequential memory order: body parts + bkg + PAFs. It will follow the"
//	" order on POSE_BODY_PART_MAPPING in `include/openpose/pose/poseParameters.hpp`.");
//DEFINE_bool(heatmaps_add_bkg, false, "Same functionality as `add_heatmaps_parts`, but adding the heatmap corresponding to"
//	" background.");
//DEFINE_bool(heatmaps_add_PAFs, false, "Same functionality as `add_heatmaps_parts`, but adding the PAFs.");
//// OpenPose Face
//DEFINE_bool(face, false, "Enables face keypoint detection. It will share some parameters from the body pose, e.g."
//	" `model_folder`. Note that this will considerable slow down the performance and increse"
//	" the required GPU memory. In addition, the greater number of people on the image, the"
//	" slower OpenPose will be.");
//DEFINE_string(face_net_resolution, "368x368", "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the face keypoint"
//	" detector. 320x320 usually works fine while giving a substantial speed up when multiple"
//	" faces on the image.");
//// OpenPose Hand
//DEFINE_bool(hand, false, "Enables hand keypoint detection. It will share some parameters from the body pose, e.g."
//	" `model_folder`. Analogously to `--face`, it will also slow down the performance, increase"
//	" the required GPU memory and its speed depends on the number of people.");
//DEFINE_string(hand_net_resolution, "368x368", "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the hand keypoint"
//	" detector.");
//DEFINE_int32(hand_scale_number, 1, "Analogous to `scale_number` but applied to the hand keypoint detector. Our best results"
//	" were found with `hand_scale_number` = 6 and `hand_scale_range` = 0.4");
//DEFINE_double(hand_scale_range, 0.4, "Analogous purpose than `scale_gap` but applied to the hand keypoint detector. Total range"
//	" between smallest and biggest scale. The scales will be centered in ratio 1. E.g. if"
//	" scaleRange = 0.4 and scalesNumber = 2, then there will be 2 scales, 0.8 and 1.2.");
//DEFINE_bool(hand_tracking, false, "Adding hand tracking might improve hand keypoints detection for webcam (if the frame rate"
//	" is high enough, i.e. >7 FPS per GPU) and video. This is not person ID tracking, it"
//	" simply looks for hands in positions at which hands were located in previous frames, but"
//	" it does not guarantee the same person ID among frames");
//// OpenPose Rendering
//DEFINE_int32(part_to_show, 0, "Prediction channel to visualize (default: 0). 0 for all the body parts, 1-18 for each body"
//	" part heat map, 19 for the background heat map, 20 for all the body part heat maps"
//	" together, 21 for all the PAFs, 22-40 for each body part pair PAF");
//DEFINE_bool(disable_blending, false, "If enabled, it will render the results (keypoint skeletons or heatmaps) on a black"
//	" background, instead of being rendered into the original image. Related: `part_to_show`,"
//	" `alpha_pose`, and `alpha_pose`.");
//// OpenPose Rendering Pose
//DEFINE_double(render_threshold, 0.05, "Only estimated keypoints whose score confidences are higher than this threshold will be"
//	" rendered. Generally, a high threshold (> 0.5) will only render very clear body parts;"
//	" while small thresholds (~0.1) will also output guessed and occluded keypoints, but also"
//	" more false positives (i.e. wrong detections).");
//DEFINE_int32(render_pose, 2, "Set to 0 for no rendering, 1 for CPU rendering (slightly faster), and 2 for GPU rendering"
//	" (slower but greater functionality, e.g. `alpha_X` flags). If rendering is enabled, it will"
//	" render both `outputData` and `cvOutputData` with the original image and desired body part"
//	" to be shown (i.e. keypoints, heat maps or PAFs).");
//DEFINE_double(alpha_pose, 0.6, "Blending factor (range 0-1) for the body part rendering. 1 will show it completely, 0 will"
//	" hide it. Only valid for GPU rendering.");
//DEFINE_double(alpha_heatmap, 0.7, "Blending factor (range 0-1) between heatmap and original frame. 1 will only show the"
//	" heatmap, 0 will only show the frame. Only valid for GPU rendering.");
//// OpenPose Rendering Face
//DEFINE_double(face_render_threshold, 0.4, "Analogous to `render_threshold`, but applied to the face keypoints.");
//DEFINE_int32(face_render, -1, "Analogous to `render_pose` but applied to the face. Extra option: -1 to use the same"
//	" configuration that `render_pose` is using.");
//DEFINE_double(face_alpha_pose, 0.6, "Analogous to `alpha_pose` but applied to face.");
//DEFINE_double(face_alpha_heatmap, 0.7, "Analogous to `alpha_heatmap` but applied to face.");
//// OpenPose Rendering Hand
//DEFINE_double(hand_render_threshold, 0.2, "Analogous to `render_threshold`, but applied to the hand keypoints.");
//DEFINE_int32(hand_render, -1, "Analogous to `render_pose` but applied to the hand. Extra option: -1 to use the same"
//	" configuration that `render_pose` is using.");
//DEFINE_double(hand_alpha_pose, 0.6, "Analogous to `alpha_pose` but applied to hand.");
//DEFINE_double(hand_alpha_heatmap, 0.7, "Analogous to `alpha_heatmap` but applied to hand.");
//// Display
//DEFINE_bool(fullscreen, false, "Run in full-screen mode (press f during runtime to toggle).");
//DEFINE_bool(no_gui_verbose, false, "Do not write text on output images on GUI (e.g. number of current frame and people). It"
//	" does not affect the pose rendering.");
//DEFINE_bool(no_display, false, "Do not open a display window. Useful if there is no X server and/or to slightly speed up"
//	" the processing if visual output is not required.");
//// Result Saving
//DEFINE_string(write_images, "", "Directory to write rendered frames in `write_images_format` image format.");
//DEFINE_string(write_images_format, "png", "File extension and format for `write_images`, e.g. png, jpg or bmp. Check the OpenCV"
//	" function cv::imwrite for all compatible extensions.");
//DEFINE_string(write_video, "", "Full file path to write rendered frames in motion JPEG video format. It might fail if the"
//	" final path does not finish in `.avi`. It internally uses cv::VideoWriter.");
//DEFINE_string(write_keypoint, "", "Directory to write the people body pose keypoint data. Set format with `write_keypoint_format`.");
//DEFINE_string(write_keypoint_format, "yml", "File extension and format for `write_keypoint`: json, xml, yaml & yml. Json not available"
//	" for OpenCV < 3.0, use `write_keypoint_json` instead.");
//DEFINE_string(write_keypoint_json, "", "Directory to write people pose data in *.json format, compatible with any OpenCV version.");
//DEFINE_string(write_coco_json, "", "Full file path to write people pose data with *.json COCO validation format.");
//DEFINE_string(write_heatmaps, "", "Directory to write body pose heatmaps in *.png format. At least 1 `add_heatmaps_X` flag"
//	" must be enabled.");
//DEFINE_string(write_heatmaps_format, "png", "File extension and format for `write_heatmaps`, analogous to `write_images_format`."
//	" Recommended `png` or any compressed and lossless format.");
//
//int openPoseDemo()
//{
//	// logging_level
//	op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.", __LINE__, __FUNCTION__, __FILE__);
//	op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
//	// op::ConfigureLog::setPriorityThreshold(op::Priority::None); // To print all logging messages
//
//	op::log("Starting pose estimation demo!", op::Priority::High);
//	const auto timerBegin = std::chrono::high_resolution_clock::now();
//
//	// Applying user defined configuration - Google flags to program variables
//	// outputSize
//	const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
//	// netInputSize
//	const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
//	// faceNetInputSize
//	const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
//	// handNetInputSize
//	const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
//	// producerType
//	const auto producerSharedPtr = op::flagsToProducer(FLAGS_image_dir, FLAGS_video, FLAGS_ip_camera, FLAGS_camera,
//		FLAGS_camera_resolution, FLAGS_camera_fps);
//	// poseModel
//	const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
//	// keypointScale
//	const auto keypointScale = op::flagsToScaleMode(FLAGS_keypoint_scale);
//	// heatmaps to add
//	const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg, FLAGS_heatmaps_add_PAFs);
//	// Enabling Google Logging
//	const bool enableGoogleLogging = true;
//	// Logging
//	op::log("", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);
//
//	// OpenPose wrapper
//	op::log("Configuring OpenPose wrapper...........", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
//	op::Wrapper<std::vector<op::Datum>> opWrapper;
//	// Pose configuration (use WrapperStructPose{} for default and recommended configuration)
//	const op::WrapperStructPose wrapperStructPose{ !FLAGS_body_disable, netInputSize, outputSize, keypointScale, FLAGS_num_gpu,
//		FLAGS_num_gpu_start, FLAGS_scale_number, (float)FLAGS_scale_gap,
//		op::flagsToRenderMode(FLAGS_render_pose), poseModel,
//		!FLAGS_disable_blending, (float)FLAGS_alpha_pose,
//		(float)FLAGS_alpha_heatmap, FLAGS_part_to_show, FLAGS_model_folder,
//		heatMapTypes, op::ScaleMode::UnsignedChar, (float)FLAGS_render_threshold,
//		enableGoogleLogging };
//	// Face configuration (use op::WrapperStructFace{} to disable it)
//	const op::WrapperStructFace wrapperStructFace{ FLAGS_face, faceNetInputSize, op::flagsToRenderMode(FLAGS_face_render, FLAGS_render_pose),
//		(float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold };
//	// Hand configuration (use op::WrapperStructHand{} to disable it)
//	const op::WrapperStructHand wrapperStructHand{ FLAGS_hand, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
//		FLAGS_hand_tracking, op::flagsToRenderMode(FLAGS_hand_render, FLAGS_render_pose),
//		(float)FLAGS_hand_alpha_pose, (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold };
//	// Producer (use default to disable any input)
//	const op::WrapperStructInput wrapperStructInput{ producerSharedPtr, FLAGS_frame_first, FLAGS_frame_last, FLAGS_process_real_time,
//		FLAGS_frame_flip, FLAGS_frame_rotate, FLAGS_frames_repeat };
//	// Consumer (comment or use default argument to disable any output)
//	const op::WrapperStructOutput wrapperStructOutput{ !FLAGS_no_display, !FLAGS_no_gui_verbose, FLAGS_fullscreen, FLAGS_write_keypoint,
//		op::stringToDataFormat(FLAGS_write_keypoint_format), FLAGS_write_keypoint_json,
//		FLAGS_write_coco_json, FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video,
//		FLAGS_write_heatmaps, FLAGS_write_heatmaps_format };
//	// Configure wrapper
//	opWrapper.configure(wrapperStructPose, wrapperStructFace, wrapperStructHand, wrapperStructInput, wrapperStructOutput);
//	// Set to single-thread running (e.g. for debugging purposes)
//	opWrapper.disableMultiThreading();
//
//	// Start processing
//	// Two different ways of running the program on multithread environment
//	op::log("Starting thread(s) __", op::Priority::High);
//	// Option a) Recommended - Also using the main thread (this thread) for processing (it saves 1 thread)
//	// Start, run & stop threads
//	opWrapper.exec();  // It blocks this thread until all threads have finished
//
//					   // // Option b) Keeping this thread free in case you want to do something else meanwhile, e.g. profiling the GPU memory
//					   // // VERY IMPORTANT NOTE: if OpenCV is compiled with Qt support, this option will not work. Qt needs the main thread to
//					   // // plot visual results, so the final GUI (which uses OpenCV) would return an exception similar to:
//					   // // `QMetaMethod::invoke: Unable to invoke methods with return values in queued connections`
//					   // // Start threads
//					   // opWrapper.start();
//					   // // Profile used GPU memory
//					   //     // 1: wait ~10sec so the memory has been totally loaded on GPU
//					   //     // 2: profile the GPU memory
//					   // const auto sleepTimeMs = 10;
//					   // for (auto i = 0 ; i < 10000/sleepTimeMs && opWrapper.isRunning() ; i++)
//					   //     std::this_thread::sleep_for(std::chrono::milliseconds{sleepTimeMs});
//					   // op::Profiler::profileGpuMemory(__LINE__, __FUNCTION__, __FILE__);
//					   // // Keep program alive while running threads
//					   // while (opWrapper.isRunning())
//					   //     std::this_thread::sleep_for(std::chrono::milliseconds{sleepTimeMs});
//					   // // Stop and join threads
//					   // op::log("Stopping thread(s)", op::Priority::High);
//					   // opWrapper.stop();
//
//					   // Measuring total time
//	const auto now = std::chrono::high_resolution_clock::now();
//	const auto totalTimeSec = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(now - timerBegin).count() * 1e-9;
//	const auto message = "Real-time pose estimation demo successfully finished. Total time: " + std::to_string(totalTimeSec) + " seconds.";
//	op::log(message, op::Priority::High);
//
//	return 0;
//}
//
//int main(int argc, char *argv[])
//{
//	// Parsing command line flags
//	gflags::ParseCommandLineFlags(&argc, &argv, true);
//
//	// Running openPoseDemo
//	return openPoseDemo();
//}
//


//original 3_user_sync code cwc
// ------------------------- OpenPose Library Tutorial - Thread - Example 1 - Asynchronous -------------------------
// Asynchronous mode: ideal for fast prototyping when performance is not an issue. The user emplaces/pushes and pops frames from the OpenPose wrapper
// when he desires to.

// This example shows the user how to use the OpenPose wrapper class:
    // 1. User reads images
    // 2. Extract and render keypoint / heatmap / PAF of that image
    // 3. Save the results on disk
    // 4. User displays the rendered pose
    // Everything in a multi-thread scenario
// In addition to the previous OpenPose modules, we also need to use:
    // 1. `core` module:
        // For the Array<float> class that the `pose` module needs
        // For the Datum struct that the `thread` module sends between the queues
    // 2. `utilities` module: for the error & logging functions, i.e. op::error & op::log respectively
// This file should only be used for the user to take specific examples.

//cwc
#include <iostream>

// C++ std library dependencies
#include <chrono> // `std::chrono::` functions and classes, e.g. std::chrono::milliseconds
#include <thread> // std::this_thread
// Other 3rdparty dependencies
// GFlags: DEFINE_bool, _int32, _int64, _uint64, _double, _string
#include <gflags/gflags.h>
// Allow Google Flags in Ubuntu 14
#ifndef GFLAGS_GFLAGS_H_
    namespace gflags = google;
#endif
// OpenPose dependencies
#include <openpose/headers.hpp>

// See all the available parameter options withe the `--help` flag. E.g. `./build/examples/openpose/openpose.bin --help`.
// Note: This command will show you flags for other unnecessary 3rdparty files. Check only the flags for the OpenPose
// executable. E.g. for `openpose.bin`, look for `Flags from examples/openpose/openpose.cpp:`.
// Debugging
DEFINE_int32(logging_level,             3,              "The logging level. Integer in the range [0, 255]. 0 will output any log() message, while"
                                                        " 255 will not output any. Current OpenPose library messages are in the range 0-4: 1 for"
                                                        " low priority messages and 4 for important ones.");
// Producer
DEFINE_string(image_dir,                "examples/media/",      "Process a directory of images. Read all standard formats (jpg, png, bmp, etc.).");
// OpenPose
DEFINE_string(model_folder,             "models/",      "Folder path (absolute or relative) where the models (pose, face, ...) are located.");
DEFINE_string(output_resolution,        "-1x-1",        "The image resolution (display and output). Use \"-1x-1\" to force the program to use the"
                                                        " input image resolution.");
DEFINE_int32(num_gpu,                   -1,             "The number of GPU devices to use. If negative, it will use all the available GPUs in your"
                                                        " machine.");
DEFINE_int32(num_gpu_start,             0,              "GPU device start number.");
DEFINE_int32(keypoint_scale,            0,              "Scaling of the (x,y) coordinates of the final pose data array, i.e. the scale of the (x,y)"
                                                        " coordinates that will be saved with the `write_keypoint` & `write_keypoint_json` flags."
                                                        " Select `0` to scale it to the original source resolution, `1`to scale it to the net output"
                                                        " size (set with `net_resolution`), `2` to scale it to the final output size (set with"
                                                        " `resolution`), `3` to scale it in the range [0,1], and 4 for range [-1,1]. Non related"
                                                        " with `scale_number` and `scale_gap`.");
// OpenPose Body Pose
DEFINE_bool(body_disable,               false,          "Disable body keypoint detection. Option only possible for faster (but less accurate) face"
                                                        " keypoint detection.");
DEFINE_string(model_pose,               "COCO",         "Model to be used. E.g. `COCO` (18 keypoints), `MPI` (15 keypoints, ~10% faster), "
                                                        "`MPI_4_layers` (15 keypoints, even faster but less accurate).");
DEFINE_string(net_resolution,           "320x240",      "Multiples of 16. If it is increased, the accuracy potentially increases. If it is"
                                                        " decreased, the speed increases. For maximum speed-accuracy balance, it should keep the"
                                                        " closest aspect ratio possible to the images or videos to be processed. Using `-1` in"
                                                        " any of the dimensions, OP will choose the optimal aspect ratio depending on the user's"
                                                        " input value. E.g. the default `-1x368` is equivalent to `656x368` in 16:9 resolutions,"
                                                        " e.g. full HD (1980x1080) and HD (1280x720) resolutions.");
DEFINE_int32(scale_number,              1,              "Number of scales to average.");
DEFINE_double(scale_gap,                0.3,            "Scale gap between scales. No effect unless scale_number > 1. Initial scale is always 1."
                                                        " If you want to change the initial scale, you actually want to multiply the"
                                                        " `net_resolution` by your desired initial scale.");
DEFINE_bool(heatmaps_add_parts,         false,          "If true, it will add the body part heatmaps to the final op::Datum::poseHeatMaps array,"
                                                        " and analogously face & hand heatmaps to op::Datum::faceHeatMaps & op::Datum::handHeatMaps"
                                                        " (program speed will decrease). Not required for our library, enable it only if you intend"
                                                        " to process this information later. If more than one `add_heatmaps_X` flag is enabled, it"
                                                        " will place then in sequential memory order: body parts + bkg + PAFs. It will follow the"
                                                        " order on POSE_BODY_PART_MAPPING in `include/openpose/pose/poseParameters.hpp`.");
DEFINE_bool(heatmaps_add_bkg,           false,          "Same functionality as `add_heatmaps_parts`, but adding the heatmap corresponding to"
                                                        " background.");
DEFINE_bool(heatmaps_add_PAFs,          false,          "Same functionality as `add_heatmaps_parts`, but adding the PAFs.");
DEFINE_int32(heatmaps_scale,            2,              "Set 0 to scale op::Datum::poseHeatMaps in the range [0,1], 1 for [-1,1]; and 2 for integer"
                                                        " rounded [0,255].");
// OpenPose Face
DEFINE_bool(face,                       false,          "Enables face keypoint detection. It will share some parameters from the body pose, e.g."
                                                        " `model_folder`. Note that this will considerable slow down the performance and increse"
                                                        " the required GPU memory. In addition, the greater number of people on the image, the"
                                                        " slower OpenPose will be.");
DEFINE_string(face_net_resolution,      "368x368",      "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the face keypoint"
                                                        " detector. 320x320 usually works fine while giving a substantial speed up when multiple"
                                                        " faces on the image.");
// OpenPose Hand
DEFINE_bool(hand,                       false,          "Enables hand keypoint detection. It will share some parameters from the body pose, e.g."
                                                        " `model_folder`. Analogously to `--face`, it will also slow down the performance, increase"
                                                        " the required GPU memory and its speed depends on the number of people.");
DEFINE_string(hand_net_resolution,      "368x368",      "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the hand keypoint"
                                                        " detector.");
DEFINE_int32(hand_scale_number,         1,              "Analogous to `scale_number` but applied to the hand keypoint detector. Our best results"
                                                        " were found with `hand_scale_number` = 6 and `hand_scale_range` = 0.4");
DEFINE_double(hand_scale_range,         0.4,            "Analogous purpose than `scale_gap` but applied to the hand keypoint detector. Total range"
                                                        " between smallest and biggest scale. The scales will be centered in ratio 1. E.g. if"
                                                        " scaleRange = 0.4 and scalesNumber = 2, then there will be 2 scales, 0.8 and 1.2.");

DEFINE_bool(hand_tracking,              false,          "Adding hand tracking might improve hand keypoints detection for webcam (if the frame rate"
                                                        " is high enough, i.e. >7 FPS per GPU) and video. This is not person ID tracking, it"
                                                        " simply looks for hands in positions at which hands were located in previous frames, but"
                                                        " it does not guarantee the same person ID among frames");
// OpenPose Rendering
DEFINE_int32(part_to_show,              0,              "Prediction channel to visualize (default: 0). 0 for all the body parts, 1-18 for each body"
                                                        " part heat map, 19 for the background heat map, 20 for all the body part heat maps"
                                                        " together, 21 for all the PAFs, 22-40 for each body part pair PAF");
DEFINE_bool(disable_blending,           false,          "If enabled, it will render the results (keypoint skeletons or heatmaps) on a black"
                                                        " background, instead of being rendered into the original image. Related: `part_to_show`,"
                                                        " `alpha_pose`, and `alpha_pose`.");
// OpenPose Rendering Pose
DEFINE_double(render_threshold,         0.05,           "Only estimated keypoints whose score confidences are higher than this threshold will be"
                                                        " rendered. Generally, a high threshold (> 0.5) will only render very clear body parts;"
                                                        " while small thresholds (~0.1) will also output guessed and occluded keypoints, but also"
                                                        " more false positives (i.e. wrong detections).");
DEFINE_int32(render_pose,               2,              "Set to 0 for no rendering, 1 for CPU rendering (slightly faster), and 2 for GPU rendering"
                                                        " (slower but greater functionality, e.g. `alpha_X` flags). If rendering is enabled, it will"
                                                        " render both `outputData` and `cvOutputData` with the original image and desired body part"
                                                        " to be shown (i.e. keypoints, heat maps or PAFs).");
DEFINE_double(alpha_pose,               0.6,            "Blending factor (range 0-1) for the body part rendering. 1 will show it completely, 0 will"
                                                        " hide it. Only valid for GPU rendering.");
DEFINE_double(alpha_heatmap,            0.7,            "Blending factor (range 0-1) between heatmap and original frame. 1 will only show the"
                                                        " heatmap, 0 will only show the frame. Only valid for GPU rendering.");
// OpenPose Rendering Face
DEFINE_double(face_render_threshold,    0.4,            "Analogous to `render_threshold`, but applied to the face keypoints.");
DEFINE_int32(face_render,               -1,             "Analogous to `render_pose` but applied to the face. Extra option: -1 to use the same"
                                                        " configuration that `render_pose` is using.");
DEFINE_double(face_alpha_pose,          0.6,            "Analogous to `alpha_pose` but applied to face.");
DEFINE_double(face_alpha_heatmap,       0.7,            "Analogous to `alpha_heatmap` but applied to face.");
// OpenPose Rendering Hand
DEFINE_double(hand_render_threshold,    0.2,            "Analogous to `render_threshold`, but applied to the hand keypoints.");
DEFINE_int32(hand_render,               -1,             "Analogous to `render_pose` but applied to the hand. Extra option: -1 to use the same"
                                                        " configuration that `render_pose` is using.");
DEFINE_double(hand_alpha_pose,          0.6,            "Analogous to `alpha_pose` but applied to hand.");
DEFINE_double(hand_alpha_heatmap,       0.7,            "Analogous to `alpha_heatmap` but applied to hand.");
// Result Saving
DEFINE_string(write_images,             "",             "Directory to write rendered frames in `write_images_format` image format.");
DEFINE_string(write_images_format,      "png",          "File extension and format for `write_images`, e.g. png, jpg or bmp. Check the OpenCV"
                                                        " function cv::imwrite for all compatible extensions.");
DEFINE_string(write_video,              "",             "Full file path to write rendered frames in motion JPEG video format. It might fail if the"
                                                        " final path does not finish in `.avi`. It internally uses cv::VideoWriter.");
DEFINE_string(write_keypoint,           "",             "Directory to write the people body pose keypoint data. Set format with `write_keypoint_format`.");
DEFINE_string(write_keypoint_format,    "json",          "File extension and format for `write_keypoint`: json, xml, yaml & yml. Json not available"
                                                        " for OpenCV < 3.0, use `write_keypoint_json` instead.");
DEFINE_string(write_keypoint_json,      "examples/keypoints",             "Directory to write people pose data in *.json format, compatible with any OpenCV version.");
DEFINE_string(write_coco_json,          "",             "Full file path to write people pose data with *.json COCO validation format.");
DEFINE_string(write_heatmaps,           "",             "Directory to write body pose heatmaps in *.png format. At least 1 `add_heatmaps_X` flag"
                                                        " must be enabled.");
DEFINE_string(write_heatmaps_format,    "png",          "File extension and format for `write_heatmaps`, analogous to `write_images_format`."
                                                        " Recommended `png` or any compressed and lossless format.");


// If the user needs his own variables, he can inherit the op::Datum struct and add them
// UserDatum can be directly used by the OpenPose wrapper because it inherits from op::Datum, just define Wrapper<UserDatum> instead of
// Wrapper<op::Datum>
struct UserDatum : public op::Datum
{
    bool boolThatUserNeedsForSomeReason;

    UserDatum(const bool boolThatUserNeedsForSomeReason_ = false) :
        boolThatUserNeedsForSomeReason{boolThatUserNeedsForSomeReason_}
    {}
};

// The W-classes can be implemented either as a template or as simple classes given
// that the user usually knows which kind of data he will move between the queues,
// in this case we assume a std::shared_ptr of a std::vector of UserDatum

// This worker will just read and return all the jpg files in a directory
class UserInputClass
{
public:
    UserInputClass(const std::string& directoryPath) :
        mImageFiles{op::getFilesOnDirectory(directoryPath, "jpg")},
        // mImageFiles{op::getFilesOnDirectory(directoryPath, std::vector<std::string>{"jpg", "png"})}, // If we want "jpg" + "png" images
        mCounter{0},
        mClosed{false}
    {
        if (mImageFiles.empty())
            op::error("No images found on: " + directoryPath, __LINE__, __FUNCTION__, __FILE__);
    }

    std::shared_ptr<std::vector<UserDatum>> createDatum()
    {
        // Close program when empty frame
        if (mClosed || mImageFiles.size() <= mCounter)
        {
            op::log("Last frame read and added to queue. Closing program after it is processed.", op::Priority::High);
            // This funtion stops this worker, which will eventually stop the whole thread system once all the frames have been processed
            mClosed = true;
            return nullptr;
        }
        else // if (!mClosed)
        {
            // Create new datum
            auto datumsPtr = std::make_shared<std::vector<UserDatum>>();
            datumsPtr->emplace_back();
            auto& datum = datumsPtr->at(0);

            // Fill datum
            datum.cvInputData = cv::imread(mImageFiles.at(mCounter++));

            // If empty frame -> return nullptr
            if (datum.cvInputData.empty())
            {
                op::log("Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.", op::Priority::High);
                mClosed = true;
                datumsPtr = nullptr;
            }

            return datumsPtr;
        }
    }

    bool isFinished() const
    {
        return mClosed;
    }

private:
    const std::vector<std::string> mImageFiles;
    unsigned long long mCounter;
    bool mClosed;
};

// This worker will just read and return all the jpg files in a directory
class UserOutputClass
{
public:
    bool display(const std::shared_ptr<std::vector<UserDatum>>& datumsPtr)
    {
        // User's displaying/saving/other processing here
            // datum.cvOutputData: rendered frame with pose or heatmaps
            // datum.poseKeypoints: Array<float> with the estimated pose
        char key = ' ';
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
            cv::imshow("User worker GUI", datumsPtr->at(0).cvOutputData);
            // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
            key = (char)cv::waitKey(1);
        }
        else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
        return (key == 27);
    }
    void printKeypoints(const std::shared_ptr<std::vector<UserDatum>>& datumsPtr)
    {
        // Example: How to use the pose keypoints
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
            op::log("\nKeypoints:");
            // Accesing each element of the keypoints
            const auto& poseKeypoints = datumsPtr->at(0).poseKeypoints;
            op::log("Person pose keypoints:");
            for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
            {
                op::log("Person " + std::to_string(person) + " (x, y, score):");
                for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                {
                    std::string valueToPrint;
                    for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                        valueToPrint += std::to_string(   poseKeypoints[{person, bodyPart, xyscore}]   ) + " ";
                    op::log(valueToPrint);
                }
            }
            op::log(" ");
            
			//// Alternative: just getting std::string equivalent
   //         op::log("Face keypoints: " + datumsPtr->at(0).faceKeypoints.toString());
   //         op::log("Left hand keypoints: " + datumsPtr->at(0).handKeypoints[0].toString());
   //         op::log("Right hand keypoints: " + datumsPtr->at(0).handKeypoints[1].toString());
   //         // Heatmaps
   //         const auto& poseHeatMaps = datumsPtr->at(0).poseHeatMaps;
   //         if (!poseHeatMaps.empty())
   //         {
   //             op::log("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
   //                     + std::to_string(poseHeatMaps.getSize(1)) + ", "
   //                     + std::to_string(poseHeatMaps.getSize(2)) + "]");
   //             const auto& faceHeatMaps = datumsPtr->at(0).faceHeatMaps;
   //             op::log("Face heatmaps size: [" + std::to_string(faceHeatMaps.getSize(0)) + ", "
   //                     + std::to_string(faceHeatMaps.getSize(1)) + ", "
   //                     + std::to_string(faceHeatMaps.getSize(2)) + ", "
   //                     + std::to_string(faceHeatMaps.getSize(3)) + "]");
   //             const auto& handHeatMaps = datumsPtr->at(0).handHeatMaps;
   //             op::log("Left hand heatmaps size: [" + std::to_string(handHeatMaps[0].getSize(0)) + ", "
   //                     + std::to_string(handHeatMaps[0].getSize(1)) + ", "
   //                     + std::to_string(handHeatMaps[0].getSize(2)) + ", "
   //                     + std::to_string(handHeatMaps[0].getSize(3)) + "]");
   //             op::log("Right hand heatmaps size: [" + std::to_string(handHeatMaps[1].getSize(0)) + ", "
   //                     + std::to_string(handHeatMaps[1].getSize(1)) + ", "
   //                     + std::to_string(handHeatMaps[1].getSize(2)) + ", "
   //                     + std::to_string(handHeatMaps[1].getSize(3)) + "]");
   //         }
        }
        else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
    }
};

int openPoseTutorialWrapper1()
{
    // logging_level
    op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.", __LINE__, __FUNCTION__, __FILE__);
    op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
    // op::ConfigureLog::setPriorityThreshold(op::Priority::None); // To print all logging messages

    op::log("Starting pose estimation demo.", op::Priority::High);
    const auto timerBegin = std::chrono::high_resolution_clock::now();

    // Applying user defined configuration - Google flags to program variables
    // outputSize
    const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
    // netInputSize
    const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
    // faceNetInputSize
    const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
    // handNetInputSize
    const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
    // poseModel
    const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
    // keypointScale
    const auto keypointScale = op::flagsToScaleMode(FLAGS_keypoint_scale);
    // heatmaps to add
    const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg, FLAGS_heatmaps_add_PAFs);
    op::check(FLAGS_heatmaps_scale >= 0 && FLAGS_heatmaps_scale <= 2, "Non valid `heatmaps_scale`.", __LINE__, __FUNCTION__, __FILE__);
    const auto heatMapScale = (FLAGS_heatmaps_scale == 0 ? op::ScaleMode::PlusMinusOne
                               : (FLAGS_heatmaps_scale == 1 ? op::ScaleMode::ZeroToOne : op::ScaleMode::UnsignedChar ));
    // Enabling Google Logging
    const bool enableGoogleLogging = true;
    // Logging
    op::log("", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);

    // Configure OpenPose
    op::Wrapper<std::vector<UserDatum>> opWrapper{op::ThreadManagerMode::Asynchronous};
    // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
    const op::WrapperStructPose wrapperStructPose{!FLAGS_body_disable, netInputSize, outputSize, keypointScale, FLAGS_num_gpu,
                                                  FLAGS_num_gpu_start, FLAGS_scale_number, (float)FLAGS_scale_gap,
                                                  op::flagsToRenderMode(FLAGS_render_pose), poseModel,
                                                  !FLAGS_disable_blending, (float)FLAGS_alpha_pose,
                                                  (float)FLAGS_alpha_heatmap, FLAGS_part_to_show, FLAGS_model_folder,
                                                  heatMapTypes, heatMapScale, (float)FLAGS_render_threshold,
                                                  enableGoogleLogging};
    // Face configuration (use op::WrapperStructFace{} to disable it)
    const op::WrapperStructFace wrapperStructFace{FLAGS_face, faceNetInputSize, op::flagsToRenderMode(FLAGS_face_render, FLAGS_render_pose),
                                                  (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
    // Hand configuration (use op::WrapperStructHand{} to disable it)
    const op::WrapperStructHand wrapperStructHand{FLAGS_hand, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
                                                  FLAGS_hand_tracking, op::flagsToRenderMode(FLAGS_hand_render, FLAGS_render_pose),
                                                  (float)FLAGS_hand_alpha_pose, (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
    // Consumer (comment or use default argument to disable any output)
    const bool displayGui = false;
    const bool guiVerbose = false;
    const bool fullScreen = false;
    const op::WrapperStructOutput wrapperStructOutput{displayGui, guiVerbose, fullScreen, FLAGS_write_keypoint,
                                                      op::stringToDataFormat(FLAGS_write_keypoint_format), FLAGS_write_keypoint_json,
                                                      FLAGS_write_coco_json, FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video,
                                                      FLAGS_write_heatmaps, FLAGS_write_heatmaps_format};
    // Configure wrapper
    op::log("Configuring OpenPose wrapper.", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);
    opWrapper.configure(wrapperStructPose, wrapperStructFace, wrapperStructHand, op::WrapperStructInput{}, wrapperStructOutput);
    // Set to single-thread running (e.g. for debugging purposes)
    // opWrapper.disableMultiThreading();

    op::log("Starting thread(s)", op::Priority::High);
    opWrapper.start();

    // User processing
    UserInputClass userInputClass(FLAGS_image_dir);
    UserOutputClass userOutputClass;
    bool userWantsToExit = false;
    while (!userWantsToExit && !userInputClass.isFinished())
    {
        // Push frame
        auto datumToProcess = userInputClass.createDatum();
        if (datumToProcess != nullptr)
        {
            auto successfullyEmplaced = opWrapper.waitAndEmplace(datumToProcess);
            // Pop frame
            std::shared_ptr<std::vector<UserDatum>> datumProcessed;
            if (successfullyEmplaced && opWrapper.waitAndPop(datumProcessed))
            {
                //userWantsToExit = userOutputClass.display(datumProcessed);
                //userOutputClass.printKeypoints(datumProcessed);
            }
            else
                op::log("Processed datum could not be emplaced.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
        }
    }

    op::log("Stopping thread(s)", op::Priority::High);
    opWrapper.stop();

    // Measuring total time
    const auto now = std::chrono::high_resolution_clock::now();
    const auto totalTimeSec = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(now-timerBegin).count() * 1e-9;
    const auto message = "Real-time pose estimation demo successfully finished. Total time: " + std::to_string(totalTimeSec) + " seconds.";
    op::log(message, op::Priority::High);

    return 0;
}

int main(int argc, char *argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

	std::cout << "Wrapper ......................................................................." << std::endl;
    // Running openPoseTutorialWrapper1
    return openPoseTutorialWrapper1();
}



//// Another version of 1.user_async. This works with video instead of a camera
//
//// ------------------------- OpenPose Library Tutorial - Thread - Example 3 - Asynchronous Output -------------------------
//// Asynchronous output mode: ideal for fast prototyping when performance is not an issue and user wants to use the output OpenPose format. The user
//// simply gets the processed frames from the OpenPose wrapper when he desires to.
//
//// This example shows the user how to use the OpenPose wrapper class:
//// 1. Read folder of images / video / webcam
//// 2. Extract and render keypoint / heatmap / PAF of that image
//// 3. Save the results on disk
//// 4. User displays the rendered pose
//// Everything in a multi-thread scenario
//// In addition to the previous OpenPose modules, we also need to use:
//// 1. `core` module:
//// For the Array<float> class that the `pose` module needs
//// For the Datum struct that the `thread` module sends between the queues
//// 2. `utilities` module: for the error & logging functions, i.e. op::error & op::log respectively
//// This file should only be used for the user to take specific examples.
//
//// C++ std library dependencies
//#include <chrono> // `std::chrono::` functions and classes, e.g. std::chrono::milliseconds
//#include <thread> // std::this_thread
//// Other 3rdparty dependencies
//// GFlags: DEFINE_bool, _int32, _int64, _uint64, _double, _string
//#include <gflags/gflags.h>
//
//// cwc 
//#include <assert.h> 
//
//// Allow Google Flags in Ubuntu 14
//#ifndef GFLAGS_GFLAGS_H_
//namespace gflags = google;
//#endif
//// OpenPose dependencies
//#include <openpose/headers.hpp>
//
//
//// See all the available parameter options withe the `--help` flag. E.g. `./build/examples/openpose/openpose.bin --help`.
//// Note: This command will show you flags for other unnecessary 3rdparty files. Check only the flags for the OpenPose
//// executable. E.g. for `openpose.bin`, look for `Flags from examples/openpose/openpose.cpp:`.
//// Debugging
//DEFINE_int32(logging_level,             3,              "The logging level. Integer in the range [0, 255]. 0 will output any log() message, while"
//                                                        " 255 will not output any. Current OpenPose library messages are in the range 0-4: 1 for"
//                                                        " low priority messages and 4 for important ones.");
//// Producer
//DEFINE_int32(camera,                    -1,             "The camera index for cv::VideoCapture. Integer in the range [0, 9]. Select a negative"
//                                                        " number (by default), to auto-detect and open the first available camera.");
//DEFINE_string(camera_resolution,        "320x240",      "Size of the camera frames to ask for.");  // "1280x720" original
//DEFINE_double(camera_fps,               30.0,           "Frame rate for the webcam (only used when saving video from webcam). Set this value to the"
//                                                        " minimum value between the OpenPose displayed speed and the webcam real frame rate.");
//DEFINE_string(video,                    "",             "Use a video file instead of the camera. Use `examples/media/video.avi` for our default"
//                                                        " example video.");  // examples/media/Gesture1288_Session 18-Participant 36-Block 1_167783952.avi
//DEFINE_string(image_dir,                "examples/media",             "Process a directory of images. Use `examples/media/` for our default example folder with 20"
//                                                        " images. Read all standard formats (jpg, png, bmp, etc.).");
//DEFINE_string(ip_camera,                "",             "String with the IP camera URL. It supports protocols like RTSP and HTTP.");
//DEFINE_uint64(frame_first,              0,              "Start on desired frame number. Indexes are 0-based, i.e. the first frame has index 0.");
//DEFINE_uint64(frame_last,               -1,             "Finish on desired frame number. Select -1 to disable. Indexes are 0-based, e.g. if set to"
//                                                        " 10, it will process 11 frames (0-10).");
//DEFINE_bool(frame_flip,                 false,          "Flip/mirror each frame (e.g. for real time webcam demonstrations).");
//DEFINE_int32(frame_rotate,              0,              "Rotate each frame, 4 possible values: 0, 90, 180, 270.");
//DEFINE_bool(frames_repeat,              false,          "Repeat frames when finished.");
//DEFINE_bool(process_real_time,          false,          "Enable to keep the original source frame rate (e.g. for video). If the processing time is"
//                                                        " too long, it will skip frames. If it is too fast, it will slow it down.");
//// OpenPose
//DEFINE_string(model_folder,             "models/",      "Folder path (absolute or relative) where the models (pose, face, ...) are located.");
//DEFINE_string(output_resolution,        "-1x-1",        "The image resolution (display and output). Use \"-1x-1\" to force the program to use the"
//                                                        " input image resolution.");
//DEFINE_int32(num_gpu,                   -1,             "The number of GPU devices to use. If negative, it will use all the available GPUs in your"
//                                                        " machine.");
//DEFINE_int32(num_gpu_start,             0,              "GPU device start number.");
//DEFINE_int32(keypoint_scale,            0,              "Scaling of the (x,y) coordinates of the final pose data array, i.e. the scale of the (x,y)"
//                                                        " coordinates that will be saved with the `write_keypoint` & `write_keypoint_json` flags."
//                                                        " Select `0` to scale it to the original source resolution, `1`to scale it to the net output"
//                                                        " size (set with `net_resolution`), `2` to scale it to the final output size (set with"
//                                                        " `resolution`), `3` to scale it in the range [0,1], and 4 for range [-1,1]. Non related"
//                                                        " with `scale_number` and `scale_gap`.");
//// OpenPose Body Pose
//DEFINE_bool(body_disable,               false,          "Disable body keypoint detection. Option only possible for faster (but less accurate) face"
//                                                        " keypoint detection.");
//DEFINE_string(model_pose,               "COCO",         "Model to be used. E.g. `COCO` (18 keypoints), `MPI` (15 keypoints, ~10% faster), "
//                                                        "`MPI_4_layers` (15 keypoints, even faster but less accurate).");
//DEFINE_string(net_resolution,           "320x240",       "Multiples of 16. If it is increased, the accuracy potentially increases. If it is"
//                                                        " decreased, the speed increases. For maximum speed-accuracy balance, it should keep the"
//                                                        " closest aspect ratio possible to the images or videos to be processed. Using `-1` in"
//                                                        " any of the dimensions, OP will choose the optimal aspect ratio depending on the user's"
//                                                        " input value. E.g. the default `-1x368` is equivalent to `656x368` in 16:9 resolutions,"
//                                                        " e.g. full HD (1980x1080) and HD (1280x720) resolutions.");  // it doesn't stick with 16:9 always! e.g., 320x240
//DEFINE_int32(scale_number,              1,              "Number of scales to average.");
//DEFINE_double(scale_gap,                0.3,            "Scale gap between scales. No effect unless scale_number > 1. Initial scale is always 1."
//                                                        " If you want to change the initial scale, you actually want to multiply the"
//                                                        " `net_resolution` by your desired initial scale.");
//DEFINE_bool(heatmaps_add_parts,         false,          "If true, it will add the body part heatmaps to the final op::Datum::poseHeatMaps array,"
//                                                        " and analogously face & hand heatmaps to op::Datum::faceHeatMaps & op::Datum::handHeatMaps"
//                                                        " (program speed will decrease). Not required for our library, enable it only if you intend"
//                                                        " to process this information later. If more than one `add_heatmaps_X` flag is enabled, it"
//                                                        " will place then in sequential memory order: body parts + bkg + PAFs. It will follow the"
//                                                        " order on POSE_BODY_PART_MAPPING in `include/openpose/pose/poseParameters.hpp`.");
//DEFINE_bool(heatmaps_add_bkg,           false,          "Same functionality as `add_heatmaps_parts`, but adding the heatmap corresponding to"
//                                                        " background.");
//DEFINE_bool(heatmaps_add_PAFs,          false,          "Same functionality as `add_heatmaps_parts`, but adding the PAFs.");
//DEFINE_int32(heatmaps_scale,            2,              "Set 0 to scale op::Datum::poseHeatMaps in the range [0,1], 1 for [-1,1]; and 2 for integer"
//                                                        " rounded [0,255].");
//// OpenPose Face
//DEFINE_bool(face,                       false,          "Enables face keypoint detection. It will share some parameters from the body pose, e.g."
//                                                        " `model_folder`. Note that this will considerable slow down the performance and increse"
//                                                        " the required GPU memory. In addition, the greater number of people on the image, the"
//                                                        " slower OpenPose will be.");
//DEFINE_string(face_net_resolution,      "368x368",      "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the face keypoint"
//                                                        " detector. 320x320 usually works fine while giving a substantial speed up when multiple"
//                                                        " faces on the image.");
//// OpenPose Hand
//DEFINE_bool(hand,                       false,          "Enables hand keypoint detection. It will share some parameters from the body pose, e.g."
//                                                        " `model_folder`. Analogously to `--face`, it will also slow down the performance, increase"
//                                                        " the required GPU memory and its speed depends on the number of people.");
//DEFINE_string(hand_net_resolution,      "368x368",      "Multiples of 16 and squared. Analogous to `net_resolution` but applied to the hand keypoint"
//                                                        " detector.");
//DEFINE_int32(hand_scale_number,         1,              "Analogous to `scale_number` but applied to the hand keypoint detector. Our best results"
//                                                        " were found with `hand_scale_number` = 6 and `hand_scale_range` = 0.4");
//DEFINE_double(hand_scale_range,         0.4,            "Analogous purpose than `scale_gap` but applied to the hand keypoint detector. Total range"
//                                                        " between smallest and biggest scale. The scales will be centered in ratio 1. E.g. if"
//                                                        " scaleRange = 0.4 and scalesNumber = 2, then there will be 2 scales, 0.8 and 1.2.");
//
//DEFINE_bool(hand_tracking,              false,          "Adding hand tracking might improve hand keypoints detection for webcam (if the frame rate"
//                                                        " is high enough, i.e. >7 FPS per GPU) and video. This is not person ID tracking, it"
//                                                        " simply looks for hands in positions at which hands were located in previous frames, but"
//                                                        " it does not guarantee the same person ID among frames");  // originally false but does it matter if hand flag is set to false?
//// OpenPose Rendering
//DEFINE_int32(part_to_show,              0,              "Prediction channel to visualize (default: 0). 0 for all the body parts, 1-18 for each body"
//                                                        " part heat map, 19 for the background heat map, 20 for all the body part heat maps"
//                                                        " together, 21 for all the PAFs, 22-40 for each body part pair PAF");
//DEFINE_bool(disable_blending,           false,          "If enabled, it will render the results (keypoint skeletons or heatmaps) on a black"
//                                                        " background, instead of being rendered into the original image. Related: `part_to_show`,"
//                                                        " `alpha_pose`, and `alpha_pose`.");
//// OpenPose Rendering Pose
//DEFINE_double(render_threshold,         0.05,           "Only estimated keypoints whose score confidences are higher than this threshold will be"
//                                                        " rendered. Generally, a high threshold (> 0.5) will only render very clear body parts;"
//                                                        " while small thresholds (~0.1) will also output guessed and occluded keypoints, but also"
//                                                        " more false positives (i.e. wrong detections).");
//DEFINE_int32(render_pose,               2,              "Set to 0 for no rendering, 1 for CPU rendering (slightly faster), and 2 for GPU rendering"
//                                                        " (slower but greater functionality, e.g. `alpha_X` flags). If rendering is enabled, it will"
//                                                        " render both `outputData` and `cvOutputData` with the original image and desired body part"
//                                                        " to be shown (i.e. keypoints, heat maps or PAFs).");
//DEFINE_double(alpha_pose,               0.6,            "Blending factor (range 0-1) for the body part rendering. 1 will show it completely, 0 will"
//                                                        " hide it. Only valid for GPU rendering.");
//DEFINE_double(alpha_heatmap,            0.7,            "Blending factor (range 0-1) between heatmap and original frame. 1 will only show the"
//                                                        " heatmap, 0 will only show the frame. Only valid for GPU rendering.");
//// OpenPose Rendering Face
//DEFINE_double(face_render_threshold,    0.4,            "Analogous to `render_threshold`, but applied to the face keypoints.");
//DEFINE_int32(face_render,               -1,             "Analogous to `render_pose` but applied to the face. Extra option: -1 to use the same"
//                                                        " configuration that `render_pose` is using.");
//DEFINE_double(face_alpha_pose,          0.6,            "Analogous to `alpha_pose` but applied to face.");
//DEFINE_double(face_alpha_heatmap,       0.7,            "Analogous to `alpha_heatmap` but applied to face.");
//// OpenPose Rendering Hand
//DEFINE_double(hand_render_threshold,    0.2,            "Analogous to `render_threshold`, but applied to the hand keypoints.");
//DEFINE_int32(hand_render,               -1,             "Analogous to `render_pose` but applied to the hand. Extra option: -1 to use the same"
//                                                        " configuration that `render_pose` is using.");
//DEFINE_double(hand_alpha_pose,          0.6,            "Analogous to `alpha_pose` but applied to hand.");
//DEFINE_double(hand_alpha_heatmap,       0.7,            "Analogous to `alpha_heatmap` but applied to hand.");
//// Result Saving
//DEFINE_string(write_images,             "",             "Directory to write rendered frames in `write_images_format` image format.");
//DEFINE_string(write_images_format,      "png",          "File extension and format for `write_images`, e.g. png, jpg or bmp. Check the OpenCV"
//                                                        " function cv::imwrite for all compatible extensions.");
//DEFINE_string(write_video,              "",             "Full file path to write rendered frames in motion JPEG video format. It might fail if the"
//                                                        " final path does not finish in `.avi`. It internally uses cv::VideoWriter.");
//DEFINE_string(write_keypoint,           "",             "Directory to write the people body pose keypoint data. Set format with `write_keypoint_format`.");
//DEFINE_string(write_keypoint_format,    "json",          "File extension and format for `write_keypoint`: json, xml, yaml & yml. Json not available"
//                                                        " for OpenCV < 3.0, use `write_keypoint_json` instead.");
//DEFINE_string(write_keypoint_json,      "examples/keypoints",             "Directory to write people pose data in *.json format, compatible with any OpenCV version.");
//DEFINE_string(write_coco_json,          "",             "Full file path to write people pose data with *.json COCO validation format.");
//DEFINE_string(write_heatmaps,           "",             "Directory to write body pose heatmaps in *.png format. At least 1 `add_heatmaps_X` flag"
//                                                        " must be enabled.");
//DEFINE_string(write_heatmaps_format,    "png",          "File extension and format for `write_heatmaps`, analogous to `write_images_format`."
//                                                        " Recommended `png` or any compressed and lossless format.");
//
//
//// If the user needs his own variables, he can inherit the op::Datum struct and add them
//// UserDatum can be directly used by the OpenPose wrapper because it inherits from op::Datum, just define Wrapper<UserDatum> instead of
//// Wrapper<op::Datum>
//struct UserDatum : public op::Datum
//{
//	bool boolThatUserNeedsForSomeReason;
//
//	UserDatum(const bool boolThatUserNeedsForSomeReason_ = false) :
//		boolThatUserNeedsForSomeReason{ boolThatUserNeedsForSomeReason_ }
//	{}
//};
//
//// The W-classes can be implemented either as a template or as simple classes given
//// that the user usually knows which kind of data he will move between the queues,
//// in this case we assume a std::shared_ptr of a std::vector of UserDatum
//
//// This worker will just read and return all the jpg files in a directory
//class UserOutputClass
//{
//public:
//	bool display(const std::shared_ptr<std::vector<op::Datum>>& datumsPtr)
//	{
//		// User's displaying/saving/other processing here
//		// datum.cvOutputData: rendered frame with pose or heatmaps
//		// datum.poseKeypoints: Array<float> with the estimated pose
//		char key = ' ';
//		//op::log();
//		if (datumsPtr != nullptr && !datumsPtr->empty())
//		{
//			cv::imshow("User worker GUI", datumsPtr->at(0).cvOutputData);  // (cv::Rect(1, 1, 200, 200))
//																		   // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
//			key = (char)cv::waitKey(1);
//		}
//		else
//			op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
//		return (key == 27);
//	}
//
//
//	bool printKeypoints(const std::shared_ptr<std::vector<op::Datum>>& datumsPtr)
//	{
//		char key = ' ';
//		// Example: How to use the pose keypoints
//		if (datumsPtr != nullptr && !datumsPtr->empty())
//		{
//			// cwc // for LH RH image cropping
//			unsigned int hand_img_width = 64, hand_img_height = 64;
//			unsigned int res_x, res_y;
//			int left_hand_img_x_start = 0, left_hand_img_y_start = 0, left_hand_img_x_end = 0, left_hand_img_y_end = 0;  // LH: upper left x and y; lower right x and y
//			int right_hand_img_x_start = 0, right_hand_img_y_start = 0, right_hand_img_x_end = 0, right_hand_img_y_end = 0;  // RH: upper left x and y; lower right x and y
//
//			// for palm keypoint calculations
//			float lhForearmLength = 0, rhForearmLength = 0;
//			cv::Vec2f lhForearmVect, rhForearmVect;
//			float palmRatio = 0.195;  // ideally 0.167
//
//			// { 4,  "RWrist" }, { 7,  "LWrist" }, 
//			cv::Vec2f lhWrist, rhWrist;
//			// { 3,  "RElbow" }, { 6,  "LElbow" },
//			cv::Vec2f lhElbow, rhElbow;
//			cv::Vec2f lhPalm, rhPalm;
//
//
//			// Accessing the image pixels (RGB)
//			//const auto& rgbImage = datumsPtr->at(0).cvOutputData;
//			const auto& rgbImage = datumsPtr->at(0).cvInputData;
//			//op::log("rgbImage dimesions:" + std::to_string(rgbImage.size[0]) + " " + std::to_string(rgbImage.size[1]) + " " + std::to_string(rgbImage.size[2]));  // 240 x 320
//			res_x = rgbImage.size[1];
//			res_y = rgbImage.size[0];
//
//
//			// op::log("\nKeypoints:");
//			// Accesing each element of the keypoints 
//			const auto& poseKeypoints = datumsPtr->at(0).poseKeypoints;
//			op::log("Person new frame:");
//
//			// currently sending only one person Person 0 (Person 0 is (most probably) on the left of a image).
//			int sendOnlyOnePerson = 1;
//			int bestPerson = 0;  // cwc // change this later after finding the best person to send information about
//
//			for (auto person = bestPerson; person < sendOnlyOnePerson; person++)
//				//for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
//			{
//				op::log("Person " + std::to_string(person) + " (x, y, score):");
//				std::string valueToPrint;
//
//				for (auto bodyPart = 0; bodyPart < poseKeypoints.getSize(1); bodyPart++)
//				{
//					//std::string valueToPrint; // to print 3 at a time on a line
//					for (auto xyscore = 0; xyscore < poseKeypoints.getSize(2); xyscore++)
//					{
//						valueToPrint += std::to_string(poseKeypoints[{person, bodyPart, xyscore}]) + " ";
//
//						// find LH and RH x, y locations
//						if (bodyPart == 4) {
//							rhWrist[0] = poseKeypoints[{person, bodyPart, 0}];
//							rhWrist[1] = poseKeypoints[{person, bodyPart, 1}];
//						}
//						if (bodyPart == 7) {
//							lhWrist[0] = poseKeypoints[{person, bodyPart, 0}];
//							lhWrist[1] = poseKeypoints[{person, bodyPart, 1}];
//						}
//
//						if (bodyPart == 3) {
//							rhElbow[0] = poseKeypoints[{person, bodyPart, 0}];
//							rhElbow[1] = poseKeypoints[{person, bodyPart, 1}];
//						}
//						if (bodyPart == 6) {
//							lhElbow[0] = poseKeypoints[{person, bodyPart, 0}];
//							lhElbow[1] = poseKeypoints[{person, bodyPart, 1}];
//						}
//					}
//					//op::log(valueToPrint);
//				}
//				op::log(valueToPrint);
//
//				// palm keypoints calculations
//				lhForearmVect = lhWrist - lhElbow;
//				rhForearmVect = rhWrist - rhElbow;
//				lhForearmLength = norm(lhForearmVect);
//				rhForearmLength = norm(rhForearmVect);
//				lhPalm[0] = lhWrist[0] + lhForearmVect[0] * (palmRatio);  // no need for lhForearmLength since it is to be divided for norm and multiplied for palm 
//				lhPalm[1] = lhWrist[1] + lhForearmVect[1] * (palmRatio);  // 
//				rhPalm[0] = rhWrist[0] + rhForearmVect[0] * (palmRatio);
//				rhPalm[1] = rhWrist[1] + rhForearmVect[1] * (palmRatio);
//
//				//op::log("Wrist points: " + std::to_string(lhWrist[0]) + " " + std::to_string(lhWrist[1]) + " " + std::to_string(rhWrist[0]) + " " + std::to_string(rhWrist[1]));
//				//op::log("Elbow points: " + std::to_string(lhElbow[0]) + " " + std::to_string(lhElbow[1]) + " " + std::to_string(rhElbow[0]) + " " + std::to_string(rhElbow[1]));
//				//op::log("Forearm vectors: " + std::to_string(lhForearmVect[0]) + " " + std::to_string(lhForearmVect[1]) + " " + std::to_string(rhForearmVect[0]) + " " + std::to_string(rhForearmVect[1]));
//				//op::log("Forearm length: " + std::to_string(lhForearmLength) + " " + std::to_string(rhForearmLength));
//				//op::log("Palm points: " + std::to_string(lhPalm[0]) + " " + std::to_string(lhPalm[1]) + " " + std::to_string(rhPalm[0]) + " " + std::to_string(rhPalm[1]));
//
//
//				// display
//				// cv::imshow("User worker GUI", datumsPtr->at(0).cvOutputData);  // (cv::Rect(1, 1, 200, 200))
//				// Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
//				key = (char)cv::waitKey(1);
//			}  // for
//
//			op::log("[End]");
//
//			// Alternative: just getting std::string equivalent
//			/*op::log("Pose keypoints:" + datumsPtr->at(0).poseKeypoints.toString());
//			op::log("Face keypoints: " + datumsPtr->at(0).faceKeypoints.toString());
//			op::log("Left hand keypoints: " + datumsPtr->at(0).handKeypoints[0].toString());
//			op::log("Right hand keypoints: " + datumsPtr->at(0).handKeypoints[1].toString());*/
//
//			//        // Heatmaps
//			//        const auto& poseHeatMaps = datumsPtr->at(0).poseHeatMaps;
//			//        if (!poseHeatMaps.empty())
//			//op::log("------------------------------- ");
//			//        {
//			//            op::log("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
//			//                    + std::to_string(poseHeatMaps.getSize(1)) + ", "
//			//                    + std::to_string(poseHeatMaps.getSize(2)) + "]");
//			//            const auto& faceHeatMaps = datumsPtr->at(0).faceHeatMaps;
//			//            op::log("Face heatmaps size: [" + std::to_string(faceHeatMaps.getSize(0)) + ", "
//			//                    + std::to_string(faceHeatMaps.getSize(1)) + ", "
//			//                    + std::to_string(faceHeatMaps.getSize(2)) + ", "
//			//                    + std::to_string(faceHeatMaps.getSize(3)) + "]");
//			//            const auto& handHeatMaps = datumsPtr->at(0).handHeatMaps;
//			//            op::log("Left hand heatmaps size: [" + std::to_string(handHeatMaps[0].getSize(0)) + ", "
//			//                    + std::to_string(handHeatMaps[0].getSize(1)) + ", "
//			//                    + std::to_string(handHeatMaps[0].getSize(2)) + ", "
//			//                    + std::to_string(handHeatMaps[0].getSize(3)) + "]");
//			//            op::log("Right hand heatmaps size: [" + std::to_string(handHeatMaps[1].getSize(0)) + ", "
//			//                    + std::to_string(handHeatMaps[1].getSize(1)) + ", "
//			//                    + std::to_string(handHeatMaps[1].getSize(2)) + ", "
//			//                    + std::to_string(handHeatMaps[1].getSize(3)) + "]");
//			//        }
//
//		}  // if (datumsPtr != nullptr && !datumsPtr->empty())
//
//		else
//			op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
//		return (key == 27);
//	}
//
//};
//
//int openPoseTutorialWrapper3()
//{
//	// logging_level
//	op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.", __LINE__, __FUNCTION__, __FILE__);
//	op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
//	// op::ConfigureLog::setPriorityThreshold(op::Priority::None); // To print all logging messages
//
//	op::log("Starting pose estimation demo.", op::Priority::High);
//	const auto timerBegin = std::chrono::high_resolution_clock::now();
//
//	// Applying user defined configuration - Google flags to program variables
//	// outputSize
//	const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
//	// netInputSize
//	//const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");  // original
//	const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x-1");
//	// faceNetInputSize
//	const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
//	// handNetInputSize
//	const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
//	// producerType
//	const auto producerSharedPtr = op::flagsToProducer(FLAGS_image_dir, FLAGS_video, FLAGS_ip_camera, FLAGS_camera,
//		FLAGS_camera_resolution, FLAGS_camera_fps);
//	// poseModel
//	const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
//	// keypointScale
//	const auto keypointScale = op::flagsToScaleMode(FLAGS_keypoint_scale);
//	// heatmaps to add
//	const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg, FLAGS_heatmaps_add_PAFs);
//	op::check(FLAGS_heatmaps_scale >= 0 && FLAGS_heatmaps_scale <= 2, "Non valid `heatmaps_scale`.", __LINE__, __FUNCTION__, __FILE__);
//	const auto heatMapScale = (FLAGS_heatmaps_scale == 0 ? op::ScaleMode::PlusMinusOne
//		: (FLAGS_heatmaps_scale == 1 ? op::ScaleMode::ZeroToOne : op::ScaleMode::UnsignedChar));
//
//
//	// Variables for LH and RH cropping
//	//const auto outputHandSize = op::flagsToPoint(FLAGS_output_hand_resolution, "-1x-1");
//
//
//	// 
//
//	// Enabling Google Logging
//	const bool enableGoogleLogging = true;
//	// Logging
//	op::log("", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);
//
//	// Configure OpenPose
//	// op::log("Configuring OpenPose wrapper.", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);
//	op::Wrapper<std::vector<op::Datum>> opWrapper{ op::ThreadManagerMode::AsynchronousOut };
//	// Pose configuration (use WrapperStructPose{} for default and recommended configuration)
//	const op::WrapperStructPose wrapperStructPose{ !FLAGS_body_disable, netInputSize, outputSize, keypointScale, FLAGS_num_gpu,
//		FLAGS_num_gpu_start, FLAGS_scale_number, (float)FLAGS_scale_gap,
//		op::flagsToRenderMode(FLAGS_render_pose), poseModel,
//		!FLAGS_disable_blending, (float)FLAGS_alpha_pose,
//		(float)FLAGS_alpha_heatmap, FLAGS_part_to_show, FLAGS_model_folder,
//		heatMapTypes, heatMapScale, (float)FLAGS_render_threshold,
//		enableGoogleLogging };
//	// Face configuration (use op::WrapperStructFace{} to disable it)
//	const op::WrapperStructFace wrapperStructFace{ FLAGS_face, faceNetInputSize, op::flagsToRenderMode(FLAGS_face_render, FLAGS_render_pose),
//		(float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold };
//	// Hand configuration (use op::WrapperStructHand{} to disable it)
//	const op::WrapperStructHand wrapperStructHand{ FLAGS_hand, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
//		FLAGS_hand_tracking, op::flagsToRenderMode(FLAGS_hand_render, FLAGS_render_pose),
//		(float)FLAGS_hand_alpha_pose, (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold };
//	// Producer (use default to disable any input)
//	const op::WrapperStructInput wrapperStructInput{ producerSharedPtr, FLAGS_frame_first, FLAGS_frame_last, FLAGS_process_real_time,
//		FLAGS_frame_flip, FLAGS_frame_rotate, FLAGS_frames_repeat };
//	// Consumer (comment or use default argument to disable any output)
//	const bool displayGui = true;  // cwc // diplays the same gui as the one displayed by openpose main?
//	const bool guiVerbose = true;
//	const bool fullScreen = false;
//	const op::WrapperStructOutput wrapperStructOutput{ displayGui, guiVerbose, fullScreen, FLAGS_write_keypoint,
//		op::stringToDataFormat(FLAGS_write_keypoint_format), FLAGS_write_keypoint_json,
//		FLAGS_write_coco_json, FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video,
//		FLAGS_write_heatmaps, FLAGS_write_heatmaps_format };
//	// Configure wrapper
//	opWrapper.configure(wrapperStructPose, wrapperStructFace, wrapperStructHand, wrapperStructInput, wrapperStructOutput);
//	// Set to single-thread running (e.g. for debugging purposes)
//	// opWrapper.disableMultiThreading();
//
//	// op::log("Starting thread(s)", op::Priority::High);
//	opWrapper.start();
//
//	// User processing
//	UserOutputClass userOutputClass;
//	bool userWantsToExit = false;
//	while (!userWantsToExit)
//	{
//		// Pop frame
//		std::shared_ptr<std::vector<op::Datum>> datumProcessed;
//		if (opWrapper.waitAndPop(datumProcessed))
//		{
//			//userWantsToExit = userOutputClass.display(datumProcessed);
//			//userWantsToExit = userOutputClass.printKeypoints(datumProcessed);
//		}
//		else
//			op::log("Processed datum could not be emplaced.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
//	}
//
//	op::log("Stopping thread(s)", op::Priority::High);
//	opWrapper.stop();
//
//	// Measuring total time
//	const auto now = std::chrono::high_resolution_clock::now();
//	const auto totalTimeSec = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(now - timerBegin).count() * 1e-9;
//	const auto message = "Real-time pose estimation demo successfully finished. Total time: " + std::to_string(totalTimeSec) + " seconds.";
//	op::log(message, op::Priority::High);
//
//	return 0;
//}
//
//int main(int argc, char *argv[])
//{
//	// Parsing command line flags
//	gflags::ParseCommandLineFlags(&argc, &argv, true);
//
//	// Running openPoseTutorialWrapper3
//	return openPoseTutorialWrapper3();
//}
//
//
//
////POSE_COCO_BODY_PARTS{
////	{ 0,  "Nose" },
////	{ 1,  "Neck" },
////	{ 2,  "RShoulder" },
////	{ 3,  "RElbow" },
////	{ 4,  "RWrist" },
////	{ 5,  "LShoulder" },
////	{ 6,  "LElbow" },
////	{ 7,  "LWrist" },
////	{ 8,  "RHip" },
////	{ 9,  "RKnee" },
////	{ 10, "RAnkle" },
////	{ 11, "LHip" },
////	{ 12, "LKnee" },
////	{ 13, "LAnkle" },
////	{ 14, "REye" },
////	{ 15, "LEye" },
////	{ 16, "REar" },
////	{ 17, "LEar" },
////	{ 18, "Bkg" },
////}  