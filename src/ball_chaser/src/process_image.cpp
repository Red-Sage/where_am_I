#include "ros/ros.h"
#include "ball_chaser/DriveToTarget.h"
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <std_msgs/Float64.h>
#include <image_transport/image_transport.h>


// Define a global client that can request services
//ros::ServiceClient client;

class process_image
{
    public:
      process_image()
        : it(n)
      {

        client = n.serviceClient<ball_chaser::DriveToTarget>("/ball_chaser/command_robot");
        sub1 = n.subscribe("/camera/rgb/image_raw", 10, &process_image::process_image_callback, this);
        image_pub = it.advertise("ball_chaser/marked_image", 1);

      }

    // This callback function continuously executes and reads the image data
    void process_image_callback(const sensor_msgs::Image img)
    {

        int white_pixel = 255;
        ball_chaser::DriveToTarget srv_req;
    
        // TODO: Loop through each pixel in the image and check if there's a bright white one
        // Then, identify if this pixel falls in the left, mid, or right side of the image
        // Depending on the white ball position, call the drive_bot function and pass velocities to it
        // Request a stop when there's no white ball seen by the camera       

        try
        {
          cv_img_ptr = cv_bridge::toCvCopy(img, sensor_msgs::image_encodings::BGR8);
        } 
        catch (cv_bridge::Exception& e)
        {
          ROS_ERROR("cv_bridge exception: %s", e.what());
          return;
        }

        cv::Mat  grayImage, binaryImage, labels, stats, centroids;
        cv::cvtColor(cv_img_ptr->image, grayImage, cv::COLOR_BGR2GRAY);
        cv::threshold(grayImage, binaryImage, 245, 255, cv::THRESH_BINARY);

        int numLabels = cv::connectedComponentsWithStats(binaryImage, labels, stats, centroids);

        if (numLabels >= 2){
          int u = centroids.at<double>(1, 0);
          int v = centroids.at<double>(1, 1);

          cv::Point ball_center(u, v);


          if (u > img.width/3 && u < 2*img.width/3){
            // The ball is in the center third so drive straight.

            srv_req.request.linear_x = forward_velocity;
            srv_req.request.angular_z = 0;
            
            cv::circle(cv_img_ptr->image, ball_center, 3, color, cv::FILLED);
            cv::putText(cv_img_ptr->image, "C", ball_center, fontFace, fontScale, color, thickness);

          }
          else if ( u < img.width/3){
            // The ball is to the left so turn to the left.

            srv_req.request.linear_x = 0;
            srv_req.request.angular_z = angular_velocity;
            
            cv::circle(cv_img_ptr->image, ball_center, 3, color, cv::FILLED);
            cv::putText(cv_img_ptr->image, "L", ball_center, fontFace, fontScale, color, thickness);

          }
          else if (u > 2*img.width/3){
            // The ball is to the right so turn right.

            srv_req.request.linear_x = 0;
            srv_req.request.angular_z = -angular_velocity; 
            
            cv::circle(cv_img_ptr->image, ball_center, 3, color, cv::FILLED);
            cv::putText(cv_img_ptr->image, "R", ball_center, fontFace, fontScale, color, thickness);

          }
          else{
            // This code should neve be reached.

            srv_req.request.linear_x = 0;
            srv_req.request.angular_z = 0;
            ROS_WARN_STREAM("An inturnal error has occured. Contact a developer.");

          }

          if (numLabels > 2){
            ROS_WARN_STREAM("More than on target detected...chasing one");
          } 

        }
        else{
          // If no targets are detected we either stop or left turn to search.

          ROS_INFO_STREAM("No targets detected.");
          
          if (search){
            
            srv_req.request.linear_x = 0;
            srv_req.request.angular_z = angular_velocity;

          }
          else{

            srv_req.request.linear_x = 0;
            srv_req.request.angular_z = 0;

          };

        };

        if (!client.call(srv_req)){
          ROS_ERROR("Failed to call service command_robot.");
        };
        
        ptr_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", cv_img_ptr->image).toImageMsg();
        image_pub.publish(ptr_msg);
      
        
    }

    // This function calls the command_robot service to drive the robot in the specified direction
    void drive_robot(float lin_x, float ang_z)
    {
        // TODO: Request a service and pass the velocities to it to drive the robot
    }

    private:
        ros::NodeHandle n;
        ros::ServiceClient client;
        ros::Subscriber sub1;
        cv_bridge::CvImagePtr cv_img_ptr;
        bool search = true;
        float forward_velocity = 1.0;
        float angular_velocity = 1.0;
        
        image_transport::ImageTransport it;
        image_transport::Publisher image_pub;
        sensor_msgs::ImagePtr ptr_msg;

        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 2.0;
        cv::Scalar color = cv::Scalar(0, 0, 255);
        int thickness = 2;
};




int main(int argc, char** argv)
{
    // Initialize the process_image node and create a handle to it
    ros::init(argc, argv, "process_image");
    //ros::NodeHandle n;

    // Define a client service capable of requesting services from command_robot
    //client = n.serviceClient<ball_chaser::DriveToTarget>("/ball_chaser/command_robot");

    // Subscribe to /camera/rgb/image_raw topic to read the image data inside the process_image_callback function
    //ros::Subscriber sub1 = n.subscribe("/camera/rgb/image_raw", 10, process_image_callback);
    
    process_image ProcessImageObject;
    // Handle ROS communication events
    ros::spin();

    return 0;
}