//-------------------------------
// Mission Control
// Last modified: 26.10.2017
// Extra notes:
//-------------------------------


#include "mission_control.h"

// Set the goal for the NAIAD in 3D coordinates. This will consider also yaw, roll and pitch.
void SetGoal(float x, float y, float z, float yaw, float roll, float pitch)
{
  goal.x = x;
  goal.y = y;
  goal.z = z;
  goal.yaw = yaw;
  goal.roll = roll;
  goal.pitch = pitch;
}

// Set the goal for the NAIAD in 2D coordinates. This will only consider x, y and z axis.
void SetGoal(float x, float y, float z)
{
  goal.x = x;
  goal.y = y;
  goal.z = z;
  ROS_INFO("New goal has been set. (%f, %f, %f)", x,y,z);
}

// Set the position of the NAIAD in 3D coordinates.
void SetPos(float x, float y, float z, float yaw, float roll, float pitch)
{
  pos.x = x;
  pos.y = y;
  pos.z = z;
  pos.yaw = yaw;
  pos.roll = roll;
  pos.pitch = pitch;
}

// TODO REFACTOR THIS FUNCTION!!!
// comment in here.
float CalculateYawAngle()
{
  ROS_INFO("Goal: %f, %f, %f", goal.x, goal.y, goal.z);
  ROS_INFO("Position: %f, %f, %f", pos.x, pos.y, pos.z);

  float x = goal.x - pos.x;
  float y = goal.y - pos.y;
  float angle = atan2(y,x) * 180/PI;
  //float angle = atan2(y,x) - atan2(pos.y, pos.x) * 180/PI; //Need the current angle? Dont think so
  ROS_INFO("Angle to go in: %f", angle);
  ROS_INFO("Angle pointing in: %f", pos.yaw);
  return angle;
}

// This function is usded to control the NAIAD in order to keep it at a desired distance from the sea-floor.
// Check the documentation for better information about this function. 
float HeightControl()
{
  float changeInHeight;
  changeInHeight = -2*(pow((desiered_height-pos.z), 3))+((desiered_height-pos.z)/3);
  if (changeInHeight > 20)
  {
   changeInHeight = 20;
  }else if( changeInHeight < -20)
  {
   changeInHeight = -20;
  } 
  ROS_INFO("HeightControl: %f", changeInHeight);
  
  return changeInHeight;
}

// This function is used to plan the movements of the NAIAD.
void GoToCurrentGoal()
{
  //Check if the goal has been reached.
  
  if (pos.x <= (goal.x+positionTolerance) && pos.x > (goal.x-positionTolerance) && pos.y <= (goal.y+positionTolerance) && pos.y > (goal.y-positionTolerance))
  {
    ROS_INFO("GOAL REACHED");
  }
  
  ROS_INFO("GOAL: (%f, %f, %f)", goal.x, goal.y, goal.z);
  ROS_INFO("POS: (%f, %f, %f)", pos.x, pos.y, pos.z);
  
  /*  ROS_INFO("SWITCH");
    //Dive 2 meters and go in a square pattern of length 50 meters.
    switch(checkpoint){
      //starting position
      case 'O' :
        SetGoal(0, 0, 0);
        checkpoint = 'A';
        break;
      //move to the upper left corner
      case 'B' :
        SetGoal(0, 50, 2);
        checkpoint = 'C';
        break;
      //move to the upper right corner
      case 'C' :
        SetGoal(50, 50, 2);
        checkpoint = 'D';
        break;
      //move to the lower right corner
      case 'D' :
        SetGoal(50, 0, 2);
        checkpoint = 'A';
        break;
      //move to the initial position but 2 meters lower
      case 'A' :
        SetGoal(0, 0, 2);
        checkpoint = 'B';
        break;
    }
  }*/


  //Calculate the angle to the goal and make sure the Naiad is pointed in the right direction before moving forward.
  if (CalculateYawAngle() - pos.yaw > 170 || CalculateYawAngle() - pos.yaw < -170)
  {
    //move forward
    message.yaw = 500;
  }else if (CalculateYawAngle() > pos.yaw)
  {
    //turn counterclockwise
    message.yaw = 100;
  } else {
    //turn clockwise
    message.yaw = -100;
  }

  ROS_INFO("YAW: %f", message.yaw);
  float ang = CalculateYawAngle() - pos.yaw;
  if (ang <= angleTolerance && ang > -angleTolerance)
  {
    float distance = pow((goal.x - pos.x),2) + pow((goal.y - pos.y), 2);
    message.x = sqrt(distance);
    ROS_INFO("X-distance: %f", distance);
  }
  
  //Should not move in the lockal y-direction and should not roll but maybe pitch.
  message.y = 0;
  message.roll = goal.roll;
  message.pitch = goal.roll;

}

void callback(const nav_msgs::Odometry::ConstPtr& msg)
{
  if (msg->pose.pose.position.z == desiered_height)
  {
    pos.z = msg->pose.pose.position.z;
    message.z = 0;
  }
  else
  {
    message.z = HeightControl();
  }
  SetPos(msg->pose.pose.position.x, msg->pose.pose.position.y, msg->pose.pose.position.z, msg->pose.pose.orientation.z, msg->pose.pose.orientation.x, msg->pose.pose.orientation.y);
  GoToCurrentGoal();
  std::cout << std::endl << "---------------------------------" << std::endl;
}

void TestingCallback(const nav_msgs::Odometry::ConstPtr& msg)
{
  SetGoal(msg->pose.pose.position.x, msg->pose.pose.position.y, 2);
}


int main(int argc, char **argv)
{
  ros::init(argc, argv, "control_publisher");
  ros::NodeHandle n;
  
  ros::Publisher pub_control = n.advertise<mission_control::motion>("control", 10);
  ros::Subscriber sub_heght_speed = n.subscribe("odom", 10, callback);
  ros::Subscriber sub_testing = n.subscribe("naiad_testing", 1, TestingCallback); 

  SetPos(0,0,0,0,0,0);
  SetGoal(0,0,0,0,0,0);


  while(ros::ok())
  {
    ros::spinOnce();
    pub_control.publish(message);
  }
}
