// ------------------------- OpenPose Library Tutorial - Thread - Example 3 - Asynchronous Output -------------------------
// Asynchronous output mode: ideal for fast prototyping when performance is not an issue and user wants to use the output OpenPose format. The user
// simply gets the processed frames from the OpenPose wrapper when he desires to.

// This example shows the user how to use the OpenPose wrapper class:
    // 1. Read folder of images / video / webcam
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

// C++ std library dependencies
#include <chrono> // `std::chrono::` functions and classes, e.g. std::chrono::milliseconds
#include <thread> // std::this_thread
// Other 3rdparty dependencies
// GFlags: DEFINE_bool, _int32, _int64, _uint64, _double, _string
#include <gflags/gflags.h>

// cwc 
#include <assert.h>
#include <map>

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
DEFINE_int32(camera,                    -1,             "The camera index for cv::VideoCapture. Integer in the range [0, 9]. Select a negative"
                                                        " number (by default), to auto-detect and open the first available camera.");
DEFINE_string(camera_resolution,        "320x240",      "Size of the camera frames to ask for.");  // "1280x720" original  // cwc_data res = 1920x1080 (16:9)  // closest 320x176
DEFINE_double(camera_fps,               30.0,           "Frame rate for the webcam (only used when saving video from webcam). Set this value to the"
                                                        " minimum value between the OpenPose displayed speed and the webcam real frame rate.");
DEFINE_string(video,                    "",             "Use a video file instead of the camera. Use `examples/media/video.avi` for our default"
                                                        " example video.");
DEFINE_string(image_dir,                "",             "Process a directory of images. Use `examples/media/` for our default example folder with 20"
                                                        " images. Read all standard formats (jpg, png, bmp, etc.).");
DEFINE_string(ip_camera,                "",             "String with the IP camera URL. It supports protocols like RTSP and HTTP.");
DEFINE_uint64(frame_first,              0,              "Start on desired frame number. Indexes are 0-based, i.e. the first frame has index 0.");
DEFINE_uint64(frame_last,               -1,             "Finish on desired frame number. Select -1 to disable. Indexes are 0-based, e.g. if set to"
                                                        " 10, it will process 11 frames (0-10).");
DEFINE_bool(frame_flip,                 false,          "Flip/mirror each frame (e.g. for real time webcam demonstrations).");
DEFINE_int32(frame_rotate,              0,              "Rotate each frame, 4 possible values: 0, 90, 180, 270.");
DEFINE_bool(frames_repeat,              false,          "Repeat frames when finished.");
DEFINE_bool(process_real_time,          false,          "Enable to keep the original source frame rate (e.g. for video). If the processing time is"
                                                        " too long, it will skip frames. If it is too fast, it will slow it down.");
// OpenPose
DEFINE_string(model_folder,             "models/",      "Folder path (absolute or relative) where the models (pose, face, ...) are located.");
DEFINE_string(output_resolution,        "320x240",        "The image resolution (display and output). Use \"-1x-1\" to force the program to use the"
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
DEFINE_string(net_resolution,           "320x240",       "Multiples of 16. If it is increased, the accuracy potentially increases. If it is"
                                                        " decreased, the speed increases. For maximum speed-accuracy balance, it should keep the"
                                                        " closest aspect ratio possible to the images or videos to be processed. Using `-1` in"
                                                        " any of the dimensions, OP will choose the optimal aspect ratio depending on the user's"
                                                        " input value. E.g. the default `-1x368` is equivalent to `656x368` in 16:9 resolutions,"
                                                        " e.g. full HD (1980x1080) and HD (1280x720) resolutions.");  // it doesn't stick with 16:9 always! e.g., 320x240
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
                                                        " it does not guarantee the same person ID among frames");  // originally false but does it matter if hand flag is set to false?
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
DEFINE_string(write_keypoint_format,    "yml",          "File extension and format for `write_keypoint`: json, xml, yaml & yml. Json not available"
                                                        " for OpenCV < 3.0, use `write_keypoint_json` instead.");
DEFINE_string(write_keypoint_json,      "",             "Directory to write people pose data in *.json format, compatible with any OpenCV version.");
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
class UserOutputClass
{
public:

	struct DetectedPerson {
		bool isWithinCentralFrame;
		float averageLimbLength;
		float meanKeypointX;
		float meanKeypointY;
	};

	std::map<int, DetectedPerson> PersonMap;

	void populatePersonMap(const op::Array<float>& poseKeypoints, const unsigned res_x)
	{
		// {personId: InsideCentralFrame?, AverageLimbLength, CentroidOfKeypoints} 
		//std::map<int, DetectedPerson> PersonMap;

		// central window of the camera frame along x resolution
		unsigned central_window_x_start = (unsigned)(res_x / 3), central_window_x_end = (unsigned)(2 * res_x / 3);  // [--|--|--]
		//op::log("central_window_x_start and end: " + std::to_string(central_window_x_start) + " " + std::to_string(central_window_x_end));

		int middleJoints[] = {0, 1, 2, 5};  // joint to be considered to find the mean keypoint
		
		for (auto person = 0; person < poseKeypoints.getSize(0); person++) {
			float temp_meanKeypointX = 0;
			//float temp_meanKeypointY = 0;
			int kpCountX = 0, kpCountY = 0;
			// temp points for limb length calculations
			cv::Vec2f pointA, pointB;
			float temp_averageLimbLength = 0;
			int limbCount = 0;

			for (auto bodyPartIndex = 0; bodyPartIndex < sizeof(middleJoints)/sizeof(middleJoints[0]); bodyPartIndex++)
			{
				if (poseKeypoints[{person, middleJoints[bodyPartIndex], 0}] > 0.00001) {
					temp_meanKeypointX = temp_meanKeypointX + poseKeypoints[{person, middleJoints[bodyPartIndex], 0}];
					kpCountX++;
				}  // if X not zero

				//if (bodyPart == 0 || bodyPart == 1 || bodyPart == 2 || bodyPart == 5) {
				//	if (poseKeypoints[{person, bodyPart, 0}] > 0.00001) {
				//		temp_meanKeypointX = temp_meanKeypointX + poseKeypoints[{person, bodyPart, 0}];
				//		kpCountX++;
				//	}  // if X not zero

				//	//if (poseKeypoints[{person, bodyPart, 1}] > 0.00001) {
				//	//	temp_meanKeypointY = temp_meanKeypointY + poseKeypoints[{person, bodyPart, 1}];
				//	//	kpCountY++;
				//	//}  // if Y not zero
				//} // if the bodyPart is one of 0, 1, 2, 5

			}  // for body parts

			temp_meanKeypointX = temp_meanKeypointX / kpCountX;
			PersonMap[person].meanKeypointX = temp_meanKeypointX;
			//temp_meanKeypointY = temp_meanKeypointY / kpCountY;
			//PersonMap[person].meanKeypointY = temp_meanKeypointY;

			if (temp_meanKeypointX >= central_window_x_start && temp_meanKeypointX <= central_window_x_end) {
				PersonMap[person].isWithinCentralFrame = true;
			}
			else {
				PersonMap[person].isWithinCentralFrame = false;
			}

			// average limb length calculation per person
			// limbs are: | 0-1 | 2-1 | 5-1 | 8-1 | 11-1 | 3-2 | 6-5 | and not considered following: | 4-3 | 7-6 | and not considered following: | 14-0 | 15-0 | 16-14 | 17-15 |

			if (poseKeypoints[{person, 1, 0}] > 0.00001 && poseKeypoints[{person, 1, 1}] > 0.00001)  // if x and y both are not zero
			{
				pointA[0] = poseKeypoints[{person, 1, 0}];  // X1
				pointA[1] = poseKeypoints[{person, 1, 1}];  // Y1
				if (poseKeypoints[{person, 0, 0}] > 0.00001 && poseKeypoints[{person, 0, 1}] > 0.00001)  // | 0-1 |
				{
					pointB[0] = poseKeypoints[{person, 0, 0}];  // X0
					pointB[1] = poseKeypoints[{person, 0, 1}];  // Y0
					temp_averageLimbLength = temp_averageLimbLength + norm(pointB -pointA);  // | 0-1 |
					limbCount++;
				}
				if (poseKeypoints[{person, 2, 0}] > 0.00001 && poseKeypoints[{person, 2, 1}] > 0.00001)  // | 2-1 |
				{
					pointB[0] = poseKeypoints[{person, 2, 0}];  // X2
					pointB[1] = poseKeypoints[{person, 2, 1}];  // Y2
					temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 2-1 |
					limbCount++;
				}
				if (poseKeypoints[{person, 5, 0}] > 0.00001 && poseKeypoints[{person, 5, 1}] > 0.00001)  // | 5-1 |
				{
					pointB[0] = poseKeypoints[{person, 5, 0}];  // X5
					pointB[1] = poseKeypoints[{person, 5, 1}];  // Y5
					temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 5-1 |
					limbCount++;
				}
				if (poseKeypoints[{person, 8, 0}] > 0.00001 && poseKeypoints[{person, 8, 1}] > 0.00001)  // | 8-1 |
				{
					pointB[0] = poseKeypoints[{person, 8, 0}];  // X8
					pointB[1] = poseKeypoints[{person, 8, 1}];  // Y8
					temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 8-1 |
					limbCount++;
				}
				if (poseKeypoints[{person, 11, 0}] > 0.00001 && poseKeypoints[{person, 11, 1}] > 0.00001)  // | 11-1 |
				{
					pointB[0] = poseKeypoints[{person, 11, 0}];  // X11
					pointB[1] = poseKeypoints[{person, 11, 1}];  // Y11
					temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 11-1 |
					limbCount++;
				}

			}  // if joint 1 is not zero

			// if joints | 3-2 | are not zero
			if (poseKeypoints[{person, 3, 0}] > 0.00001 && poseKeypoints[{person, 3, 1}] > 0.00001 && poseKeypoints[{person, 2, 0}] > 0.00001 && poseKeypoints[{person, 2, 1}]) 
			{
				pointA[0] = poseKeypoints[{person, 2, 0}];  // X2
				pointA[1] = poseKeypoints[{person, 2, 1}];  // Y2
				pointB[0] = poseKeypoints[{person, 3, 0}];  // X3
				pointB[1] = poseKeypoints[{person, 3, 1}];  // Y3
				temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 3-2 |
				limbCount++;
			}  // if joints | 3-2 | are not zero

			// if joints | 6-5 | are not zero
			if (poseKeypoints[{person, 6, 0}] > 0.00001 && poseKeypoints[{person, 6, 1}] > 0.00001 && poseKeypoints[{person, 5, 0}] > 0.00001 && poseKeypoints[{person, 5, 1}])
			{
				pointA[0] = poseKeypoints[{person, 5, 0}];  // X5
				pointA[1] = poseKeypoints[{person, 5, 1}];  // Y5
				pointB[0] = poseKeypoints[{person, 6, 0}];  // X6
				pointB[1] = poseKeypoints[{person, 6, 1}];  // Y6
				temp_averageLimbLength = temp_averageLimbLength + norm(pointB - pointA);  // | 6-5 |
				limbCount++;
			}  // if joints | 6-5 | are not zero

			if (limbCount != 0) {
				PersonMap[person].averageLimbLength = temp_averageLimbLength / limbCount;
			}
			
		}  // for person

	}
	
    bool display(const std::shared_ptr<std::vector<op::Datum>>& datumsPtr)
    {
        // User's displaying/saving/other processing here
            // datum.cvOutputData: rendered frame with pose or heatmaps
            // datum.poseKeypoints: Array<float> with the estimated pose
        char key = ' ';
		//op::log();
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
            cv::imshow("User worker GUI", datumsPtr->at(0).cvOutputData);  // (cv::Rect(1, 1, 200, 200))
            // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
            key = (char)cv::waitKey(1);
        }
        else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
        return (key == 27);
    }


    bool printKeypoints(const std::shared_ptr<std::vector<op::Datum>>& datumsPtr)
    {	
		char key = ' ';
        // Example: How to use the pose keypoints
        if (datumsPtr != nullptr && !datumsPtr->empty())
        {
			// cwc // for LH RH image cropping
			unsigned int hand_img_width = 64, hand_img_height = 64;
			unsigned int head_img_width = 64, head_img_height = 64;
			unsigned int res_x, res_y;
			int left_hand_img_x_start = 0, left_hand_img_y_start = 0, left_hand_img_x_end = 0, left_hand_img_y_end = 0;  // LH: upper left x and y; lower right x and y
			int right_hand_img_x_start = 0, right_hand_img_y_start = 0, right_hand_img_x_end = 0, right_hand_img_y_end = 0;  // RH: upper left x and y; lower right x and y
			int head_img_x_start = 0, head_img_y_start = 0, head_img_x_end = 0, head_img_y_end = 0;  // Head: upper left x and y; lower right x and y
			
			// for palm keypoint calculations
			float lhForearmLength = 0, rhForearmLength = 0;
			cv::Vec2f lhForearmVect, rhForearmVect;
			float palmRatio = 0.50;  // ideally 0.167

			// { 0,  "Nose" },
			cv::Vec2f noseXY;
			// { 4,  "RWrist" }, { 7,  "LWrist" }, 
			cv::Vec2f lhWrist, rhWrist;
			// { 3,  "RElbow" }, { 6,  "LElbow" },
			cv::Vec2f lhElbow, rhElbow;
			cv::Vec2f lhPalm, rhPalm;

			// Accessing the image pixels (RGB)
			// const auto& rgbImage = datumsPtr->at(0).cvOutputData;
			const auto& rgbImage = datumsPtr->at(0).cvInputData;
			const auto& rgbImageOut = datumsPtr->at(0).cvOutputData;
			//op::log("rgbImage dimesions:" + std::to_string(rgbImage.size[0]) + " " + std::to_string(rgbImage.size[1]) + " " + std::to_string(rgbImage.size[2]));  // 240 x 320
			//op::log("rgbImageOut dimesions:" + std::to_string(rgbImageOut.size[0]) + " " + std::to_string(rgbImageOut.size[1]) + " " + std::to_string(rgbImageOut.size[2]));  // 240 x 320
			res_x = rgbImage.size[1]; 
			res_y = rgbImage.size[0];

						
            // op::log("\nKeypoints:");
            // Accesing each element of the keypoints 
            const auto& poseKeypoints = datumsPtr->at(0).poseKeypoints;
            op::log("Person new frame:"); 
			
			// currently sending only one person Person 0 (Person 0 is (most probably) on the left of a image).
			int bestPersonIndex = 0;  // cwc // change this later after finding the best person to send information about
			float calibrationLimbLength = 51;
			int engagedBit = 0;

			// Find the best (closest) person index in the frame 
			populatePersonMap(poseKeypoints, res_x);

			// ietrate over PersonMap
			for (auto mp = 0; mp < PersonMap.size(); mp++){
				if (PersonMap[mp].isWithinCentralFrame && PersonMap[mp].averageLimbLength > calibrationLimbLength) {
					bestPersonIndex = mp;
					engagedBit = 1;
				}
				else { engagedBit = 0; }

				//op::log("person map " + std::to_string(mp) + " =====> " + std::to_string(PersonMap[mp].averageLimbLength) + " " + std::to_string(PersonMap[mp].meanKeypointX) + " " + std::to_string(PersonMap[mp].isWithinCentralFrame));
			}
			// clear the PersonMap
			PersonMap.clear();
			
			for (auto person = bestPersonIndex ; person < bestPersonIndex+1; person++)
            //for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
            {
                op::log("Person " + std::to_string(person) + " (x, y, score):");
				std::string valueToPrint;
				valueToPrint += std::to_string(engagedBit) + " ";  // first value is the engaged bit and then 18*3 keypoints

                for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                {
                    //std::string valueToPrint; // to print 3 at a time on a line
                    for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                    {
                        valueToPrint += std::to_string(poseKeypoints[{person, bodyPart, xyscore}]) + " ";
						
						// find LH and RH x, y locations
						if (bodyPart == 4) {
							rhWrist[0] = poseKeypoints[{person, bodyPart, 0}];
							rhWrist[1] = poseKeypoints[{person, bodyPart, 1}];
						}
						if (bodyPart == 7) {
							lhWrist[0] = poseKeypoints[{person, bodyPart, 0}];
							lhWrist[1] = poseKeypoints[{person, bodyPart, 1}];
						}

						if (bodyPart == 3) {
							rhElbow[0] = poseKeypoints[{person, bodyPart, 0}];
							rhElbow[1] = poseKeypoints[{person, bodyPart, 1}];
						}
						if (bodyPart == 6) {
							lhElbow[0] = poseKeypoints[{person, bodyPart, 0}];
							lhElbow[1] = poseKeypoints[{person, bodyPart, 1}];
						}

						if (bodyPart == 0) {
							noseXY[0] = poseKeypoints[{person, bodyPart, 0}];
							noseXY[1] = poseKeypoints[{person, bodyPart, 1}];
						}
                    }  // for syscore
                }  // for bodyPart

				op::log(valueToPrint);
				valueToPrint = "";

				// palm keypoints calculations
				int forearmThreshold = 10;
				lhForearmVect = lhWrist - lhElbow;
				rhForearmVect = rhWrist - rhElbow;
				lhForearmLength = norm(lhForearmVect);
				rhForearmLength = norm(rhForearmVect);
				if (lhForearmLength > forearmThreshold) {
					lhPalm[0] = lhWrist[0] + lhForearmVect[0] * (0.75*palmRatio);  // no need for lhForearmLength since it is to be divided for norm and multiplied for palm 
					lhPalm[1] = lhWrist[1] + lhForearmVect[1] * (1.5*palmRatio);  // 
				}
				else {
					lhPalm[0] = lhWrist[0] + lhForearmVect[0] * (palmRatio);
					lhPalm[1] = lhWrist[1] + lhForearmVect[1] * (palmRatio);
				}
				
				if (rhForearmLength > forearmThreshold) {
					rhPalm[0] = rhWrist[0] + rhForearmVect[0] * (0.75*palmRatio);
					rhPalm[1] = rhWrist[1] + rhForearmVect[1] * (1.5*palmRatio);
				}
				else {
					rhPalm[0] = rhWrist[0] + rhForearmVect[0] * (palmRatio);
					rhPalm[1] = rhWrist[1] + rhForearmVect[1] * (palmRatio);
				}
				

				//op::log("Wrist points: " + std::to_string(lhWrist[0]) + " " + std::to_string(lhWrist[1]) + " " + std::to_string(rhWrist[0]) + " " + std::to_string(rhWrist[1]));
				//op::log("Elbow points: " + std::to_string(lhElbow[0]) + " " + std::to_string(lhElbow[1]) + " " + std::to_string(rhElbow[0]) + " " + std::to_string(rhElbow[1]));
				//op::log("Forearm vectors: " + std::to_string(lhForearmVect[0]) + " " + std::to_string(lhForearmVect[1]) + " " + std::to_string(rhForearmVect[0]) + " " + std::to_string(rhForearmVect[1]));
				//op::log("Forearm length: " + std::to_string(lhForearmLength) + " " + std::to_string(rhForearmLength));
				//op::log("Palm points: " + std::to_string(lhPalm[0]) + " " + std::to_string(lhPalm[1]) + " " + std::to_string(rhPalm[0]) + " " + std::to_string(rhPalm[1]));


				// print image pixels; image res = "656 x 368" == "width x height" 
				// print image pixels; image res = "320 x 176" == "width x height"
				// print image pixels; image res = "320 x 240" == "width x height"
				// LH
				left_hand_img_x_start = (int)((int)(lhPalm[0]) - (hand_img_width / 2));
				left_hand_img_y_start = (int)((int)(lhPalm[1]) - (hand_img_height / 2));
				left_hand_img_x_end = (int)((int)(lhPalm[0]) + (hand_img_width / 2));
				left_hand_img_y_end = (int)((int)(lhPalm[1]) + (hand_img_height / 2));

				assert((left_hand_img_x_end - left_hand_img_x_start) == hand_img_width && "Assertion on left hand image width failed.");
				assert((left_hand_img_y_end - left_hand_img_y_start) == hand_img_height && "Assertion on left hand image height failed.");

				op::log("ImageLeftHand: hand_img_x_start, hand_img_y_start, hand_img_x_end, hand_img_y_end: " + std::to_string(left_hand_img_x_start) + " " + std::to_string(left_hand_img_y_start) + " " + std::to_string(left_hand_img_x_end) + " " + std::to_string(left_hand_img_y_end) + " ");
				
				if (left_hand_img_y_start >= (0 - 0.45*hand_img_height) && left_hand_img_x_start >= (0 - 0.45*hand_img_width) && left_hand_img_y_end < (res_y + 0.45*hand_img_height) && left_hand_img_x_end < (res_x + 0.45*hand_img_width))  // if the entire hand image is within the screen  /// *!* cwc access the image res somehow
				{
					if (left_hand_img_y_start >= 0 && left_hand_img_x_start >= 0 && left_hand_img_y_end < res_y && left_hand_img_x_end < res_x) // if the entire hand image is within the screen
					{
						int doNothing = 1;
					}
					if (left_hand_img_y_start < 0)
					{
						left_hand_img_y_start = 0;
						left_hand_img_y_end = left_hand_img_y_start + hand_img_height;
					}
					if (left_hand_img_x_start < 0)
					{
						left_hand_img_x_start = 0;
						left_hand_img_x_end = left_hand_img_x_start + hand_img_width;
					}
					if (left_hand_img_y_end > res_y)
					{
						left_hand_img_y_end = res_y;
						left_hand_img_y_start = left_hand_img_y_end - hand_img_height;
					}
					if (left_hand_img_x_end > res_x)
					{
						left_hand_img_x_end = res_x;
						left_hand_img_x_start = left_hand_img_x_end - hand_img_width;
					}

					cv::imshow("Left Hand", datumsPtr->at(0).cvInputData(cv::Rect(left_hand_img_x_start, left_hand_img_y_start, hand_img_width, hand_img_height)));  // (cv::Rect(1, 1, 200, 200))
					std::string valueToPrintLH;
					for (int row = left_hand_img_y_start; row < left_hand_img_y_end; row++)       
					{
						for (int column = left_hand_img_x_start; column < left_hand_img_x_end; column++)
						{
							//cv::Vec3b pixelBGR; //cvPoint pixelPoint = cvPoint(row, column);
							valueToPrintLH += std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[0]) + " " +
								std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[1]) + " " +
								std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[2]) + " ";  // (cv::Rect(1, 1, 2, 2))
						}  // for column
					}  // for row

					// hand_img_width*hand_img_height*3*4 (3=> 3 channels; 4=> 4 charecters including space e.g., "235 ")
					assert(valueToPrintLH.length() <= hand_img_width*hand_img_height*3*4 && "Left hand string valueToPrintLH has more length that expected");  // op::log("length of string " + std::to_string(valueToPrintLH.length()));
					op::log(valueToPrintLH);
					valueToPrintLH = "";

				}  // if left hand is visible in the frame
				else 
				{
					op::log("[left hand unknown]");
				}


				// RH
				right_hand_img_x_start = (int)((int)(rhPalm[0])-(hand_img_width / 2));
				right_hand_img_y_start = (int)((int)(rhPalm[1])-(hand_img_height / 2));
				right_hand_img_x_end = (int)((int)(rhPalm[0])+(hand_img_width / 2));
				right_hand_img_y_end = (int)((int)(rhPalm[1])+(hand_img_height / 2));

				assert((right_hand_img_x_end - right_hand_img_x_start) == hand_img_width && "Assertion on right hand image width failed.");
				assert((right_hand_img_y_end - right_hand_img_y_start) == hand_img_height && "Assertion on right hand image height failed.");

				op::log("ImageRightHand: hand_img_x_start, hand_img_y_start, hand_img_x_end, hand_img_y_end: " + std::to_string(right_hand_img_x_start) + " " + std::to_string(right_hand_img_y_start) + " " + std::to_string(right_hand_img_x_end) + " " + std::to_string(right_hand_img_y_end) + " ");

				if (right_hand_img_y_start >= (0 - 0.45*hand_img_height) && right_hand_img_x_start >= (0 - 0.45*hand_img_width) && right_hand_img_y_end < (res_y + 0.45*hand_img_height) && right_hand_img_x_end < (res_x + 0.45*hand_img_width))  // if the entire hand image is within the screen and 30 pixels buffer around the screen /// *!* cwc access the image res somehow
				{
					if (right_hand_img_y_start >= 0 && right_hand_img_x_start >= 0 && right_hand_img_y_end < res_y && right_hand_img_x_end < res_x) // if the entire hand image is within the screen
					{
						int doNothing = 1;
					}
					if (right_hand_img_y_start < 0)
					{
						right_hand_img_y_start = 0;
						right_hand_img_y_end = right_hand_img_y_start + hand_img_height;
					}
					if (right_hand_img_x_start < 0)
					{
						right_hand_img_x_start = 0;
						right_hand_img_x_end = right_hand_img_x_start + hand_img_width;
					}
					if (right_hand_img_y_end > res_y) 
					{
						right_hand_img_y_end = res_y;
						right_hand_img_y_start = right_hand_img_y_end - hand_img_height;
					}
					if (right_hand_img_x_end > res_x)
					{
						right_hand_img_x_end = res_x;
						right_hand_img_x_start = right_hand_img_x_end - hand_img_width;
					}

					cv::imshow("Right Hand", datumsPtr->at(0).cvInputData(cv::Rect(right_hand_img_x_start, right_hand_img_y_start, hand_img_width, hand_img_height)));  // (cv::Rect(1, 1, 200, 200))
					std::string valueToPrintRH;
					for (int row = right_hand_img_y_start; row < right_hand_img_y_end; row++)
					{
						for (int column = right_hand_img_x_start; column < right_hand_img_x_end; column++)
						{
							valueToPrintRH += std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[0]) + " " +
									std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[1]) + " " +
									std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[2]) + " ";  // (cv::Rect(1, 1, 2, 2))
						}  // for column
					}  // for row

					// hand_img_width*hand_img_height*3*4 (3=> 3 channels; 4=> 4 charecters including space e.g., "235 ")
					assert(valueToPrintRH.length() <= hand_img_width*hand_img_height * 3 * 4 && "Right hand string valueToPrinRH has more length that expected");  // op::log("length of string " + std::to_string(valueToPrintRH.length()));
					op::log(valueToPrintRH);
					valueToPrintRH = "";

				}  // if right hand is visible in the frame
				else
				{
					op::log("[right hand unknown]");
				}

				// Head
				head_img_x_start = (int)((int)(noseXY[0]) - (head_img_width / 2));
				head_img_y_start = (int)((int)(noseXY[1]) - (head_img_height / 2));
				head_img_x_end = (int)((int)(noseXY[0]) + (head_img_width / 2));
				head_img_y_end = (int)((int)(noseXY[1]) + (head_img_height / 2));

				assert((head_img_x_end - head_img_x_start) == head_img_width && "Assertion on head image width failed.");
				assert((head_img_y_end - head_img_y_start) == head_img_height && "Assertion on head image height failed.");

				op::log("ImageHead: head_img_x_start, head_img_y_start, head_img_x_end, head_img_y_end: " + std::to_string(head_img_x_start) + " " + std::to_string(head_img_y_start) + " " + std::to_string(head_img_x_end) + " " + std::to_string(head_img_y_end) + " ");

				if (head_img_y_start >= (0 - 0.45*head_img_height) && head_img_x_start >= (0 - 0.45*head_img_width) && head_img_y_end < (res_y + 0.45*head_img_height) && head_img_x_end < (res_x + 0.45*head_img_width))  // if the entire head image is within the screen and 30 pixels buffer around the screen
				{
					if (head_img_y_start >= 0 && head_img_x_start >= 0 && head_img_y_end < res_y && head_img_x_end < res_x) // if the entire head image is within the screen
					{
						int doNothing = 1;
					}
					if (head_img_y_start < 0)
					{
						head_img_y_start = 0;
						head_img_y_end = head_img_y_start + head_img_height;
					}
					if (head_img_x_start < 0)
					{
						head_img_x_start = 0;
						head_img_x_end = head_img_x_start + head_img_width;
					}
					if (head_img_y_end > res_y)
					{
						head_img_y_end = res_y;
						head_img_y_start = head_img_y_end - head_img_height;
					}
					if (head_img_x_end > res_x)
					{
						head_img_x_end = res_x;
						head_img_x_start = head_img_x_end - head_img_width;
					}

					cv::imshow("Head", datumsPtr->at(0).cvInputData(cv::Rect(head_img_x_start, head_img_y_start, head_img_width, head_img_height)));  // (cv::Rect(1, 1, 200, 200))
					std::string valueToPrintHead;
					for (int row = head_img_y_start; row < head_img_y_end; row++)
					{
						for (int column = head_img_x_start; column < head_img_x_end; column++)
						{
							valueToPrintHead += std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[0]) + " " +
								std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[1]) + " " +
								std::to_string(datumsPtr->at(0).cvInputData.at<cv::Vec3b>(row, column)[2]) + " ";
						}  // for column
					}  // for row

					   // head_img_width*head_img_height*3*4 (3=> 3 channels; 4=> 4 charecters including space e.g., "235 ")
					assert(valueToPrintHead.length() <= head_img_width*head_img_height * 3 * 4 && "Head string valueToPrintHead has more length that expected");
					op::log(valueToPrintHead);
					valueToPrintHead = "";

				}  // if head is visible in the frame
				else
				{
					op::log("[head unknown]");
				}


				// display
				//cv::imshow("User worker GUI", datumsPtr->at(0).cvOutputData);  // (cv::Rect(1, 1, 200, 200))
				// Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
				key = (char)cv::waitKey(1);
            } // for person

            op::log("[End]");

        }  // if (datumsPtr != nullptr && !datumsPtr->empty())

        else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
		return (key == 27);
    }


};

int openPoseTutorialWrapper3()
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
    //const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");  // original
	const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x-1");
    // faceNetInputSize
    const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
    // handNetInputSize
    const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
    // producerType
    const auto producerSharedPtr = op::flagsToProducer(FLAGS_image_dir, FLAGS_video, FLAGS_ip_camera, FLAGS_camera,
                                                       FLAGS_camera_resolution, FLAGS_camera_fps);
    // poseModel
    const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
    // keypointScale
    const auto keypointScale = op::flagsToScaleMode(FLAGS_keypoint_scale);
    // heatmaps to add
    const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg, FLAGS_heatmaps_add_PAFs);
    op::check(FLAGS_heatmaps_scale >= 0 && FLAGS_heatmaps_scale <= 2, "Non valid `heatmaps_scale`.", __LINE__, __FUNCTION__, __FILE__);
    const auto heatMapScale = (FLAGS_heatmaps_scale == 0 ? op::ScaleMode::PlusMinusOne
                               : (FLAGS_heatmaps_scale == 1 ? op::ScaleMode::ZeroToOne : op::ScaleMode::UnsignedChar ));


	// Variables for LH and RH cropping
	//const auto outputHandSize = op::flagsToPoint(FLAGS_output_hand_resolution, "-1x-1");


	// 

    // Enabling Google Logging
    const bool enableGoogleLogging = true;
    // Logging
    op::log("", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);

    // Configure OpenPose
    // op::log("Configuring OpenPose wrapper.", op::Priority::Low, __LINE__, __FUNCTION__, __FILE__);
    op::Wrapper<std::vector<op::Datum>> opWrapper{op::ThreadManagerMode::AsynchronousOut};
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
    // Producer (use default to disable any input)
    const op::WrapperStructInput wrapperStructInput{producerSharedPtr, FLAGS_frame_first, FLAGS_frame_last, FLAGS_process_real_time,
                                                    FLAGS_frame_flip, FLAGS_frame_rotate, FLAGS_frames_repeat};
    // Consumer (comment or use default argument to disable any output)
    const bool displayGui = true;  // cwc // diplays the same gui as the one displayed by openpose main?
    const bool guiVerbose = true;
    const bool fullScreen = false;
    const op::WrapperStructOutput wrapperStructOutput{displayGui, guiVerbose, fullScreen, FLAGS_write_keypoint,
                                                      op::stringToDataFormat(FLAGS_write_keypoint_format), FLAGS_write_keypoint_json,
                                                      FLAGS_write_coco_json, FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video,
                                                      FLAGS_write_heatmaps, FLAGS_write_heatmaps_format};
    // Configure wrapper
    opWrapper.configure(wrapperStructPose, wrapperStructFace, wrapperStructHand, wrapperStructInput, wrapperStructOutput);
    // Set to single-thread running (e.g. for debugging purposes)
    // opWrapper.disableMultiThreading();   // cwc-imp!

    // op::log("Starting thread(s)", op::Priority::High);
    opWrapper.start();

    // User processing
    UserOutputClass userOutputClass;
    bool userWantsToExit = false;
    while (!userWantsToExit)
    {
        // Pop frame
        std::shared_ptr<std::vector<op::Datum>> datumProcessed;
        if (opWrapper.waitAndPop(datumProcessed))
        {
            //userWantsToExit = userOutputClass.display(datumProcessed);
            userWantsToExit = userOutputClass.printKeypoints(datumProcessed);
        }
        else
            op::log("Processed datum could not be emplaced.", op::Priority::High, __LINE__, __FUNCTION__, __FILE__);
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

    // Running openPoseTutorialWrapper3
    return openPoseTutorialWrapper3();
}



//POSE_COCO_BODY_PARTS{
//	{ 0,  "Nose" },
//	{ 1,  "Neck" },
//	{ 2,  "RShoulder" },
//	{ 3,  "RElbow" },
//	{ 4,  "RWrist" },
//	{ 5,  "LShoulder" },
//	{ 6,  "LElbow" },
//	{ 7,  "LWrist" },
//	{ 8,  "RHip" },
//	{ 9,  "RKnee" },
//	{ 10, "RAnkle" },
//	{ 11, "LHip" },
//	{ 12, "LKnee" },
//	{ 13, "LAnkle" },
//	{ 14, "REye" },
//	{ 15, "LEye" },
//	{ 16, "REar" },
//	{ 17, "LEar" },
//	{ 18, "Bkg" },
//}  