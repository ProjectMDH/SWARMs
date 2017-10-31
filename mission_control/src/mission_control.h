#include "ros/ros.h"
#include "/root/catkin_ws/devel/include/mission_control/motion.h"
#include "nav_msgs/Odometry.h"

#define PI 3.14159265
//max and minimum boundaries for travelling distance
////max and minimum boundaries for travelling distance..
#define MAX_DIST 1000 //TODO to be changed!
mission_control::motion message;
float desiered_height = 2.0;
float tolerance = desiered_height * 0.5;
float angleTolerance = 3.0;
float positionTolerance = 0.15;
char checkpoint = 'B';

struct position{
  float x;
  float y;
  float z;
  float yaw;
  float roll;
  float pitch;
};

position pos; //Position of the Naiad.
position goal;
