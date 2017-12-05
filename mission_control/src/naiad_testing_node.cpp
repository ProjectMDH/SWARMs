#include "mission_control.h"


nav_msgs::Odometry msg;
//message to quit:
//mission_control::motion qMsg;
//std_msgs::Int16 commandMsg;

//ros::NodeHandle n;

// used for resetting coordinates.
//ros::Publisher quit = n.advertise<mission_control::motion>("command", 1);
/*
virtual class Node
{
  private:
    ros::NodeHandle n;
};*/

//-----------------------------------------
/* PREVIOUS IMPLEMENTATION!!!!!!!!!!!!!
class QuitController : private Node
{
  public:
    QuitController();
    mission_control::motion qMsg;
    void QuitPublish();

  private:
    ros::NodeHandle n;
    ros::Publisher quit;
};
*/
//-----------------------------------------

class QuitController
{
  public:
    QuitController();
    void QuitPublish();
    void SetQuitMsg(int setData);
    std_msgs::Int16 GetQuitMsg();

  private:
    ros::NodeHandle n;
    ros::Publisher quit;
    std_msgs::Int16 qMsg;
};

void QuitController::QuitPublish()
{
  quit.publish(qMsg);
}

void QuitController::SetQuitMsg(int setData)
{
  qMsg.data = setData;
}

std_msgs::Int16 QuitController::GetQuitMsg()
{
  return qMsg;
}

QuitController::QuitController()
{
  quit = n.advertise<std_msgs::Int16>("qMsg", 1);
  qMsg.data = 0;
}

int EnterCoordinate()
{
  float inputX;
  float inputY;
  msg.pose.pose.position.z = 4;
  std::cout << "Enter x coordinate: ";
  std::cin >> inputX;
  if (std::cin.fail())
  {
		std::cin.clear();
    std::cin.ignore();
    std::cout << "Not a number..." << std::endl;
    return -1;
  }
  std::cout << std::endl;
  std::cout << "Enter y coordinate: ";
  std::cin >> inputY;
  if (std::cin.fail())
  {
  	std::cin.clear();
    std::cin.ignore();
    std::cout << "Not a number..." << std::endl;
    return -1;
  }
  msg.pose.pose.position.x = inputX;
  msg.pose.pose.position.y = inputY;
}


int CheckInput(QuitController quitController)
{ 
  float inputAngle;
  char firstArgument;
  //flag to be sent to quit

  //print command list:
  std::cout << "Enter '0' for help" << std::endl;
  std::cin >> firstArgument;
  //int check = 0.0;
  //check = std::atoi(inputX);
  
  //switch for checking the frist input
  std::cout << firstArgument << std::endl;
  switch(firstArgument)
  {
    // Used to disply the menu. It is a sort of help.
    case '0':
      std::cout << "Enter '1' to increase the thrusters threshold" << std::endl;
      std::cout << "Enter '2' to decrease the thrusters threshold" << std::endl;
      std::cout << "Enter '3' to activate the square pattern" << std::endl;
      std::cout << "Enter '4' to insert custom coordiantes" << std::endl;
      std::cout << "Enter '5' go up" << std::endl;
      std::cout << "Enter '6' go down" << std::endl;
      std::cout << "Enter '7' to turn to a angle" << std::endl;
      std::cout << "Enter '8' to stop" << std::endl;
      std::cout << "Enter '9' to quit" << std::endl;
      break;
    // Used to let the user increase the thrusters threshhold.
    case '1': 
      msg.pose.pose.position.z = 1;
      break;
    // Used to let the user decrease the thrusters threshhold.
    case '2':
      msg.pose.pose.position.z = 2;
      break;
    // Let the Naiad move and perform a square.
    case '3':
      msg.pose.pose.position.z = 3;
      break;
    // Let the user inser custom coordinates.
    // Check for inputs and allows only integers.
    case '4':
			EnterCoordinate();
      std::cout << std::endl;
      break;
    // Move the Naiad up.
    case '5':
      msg.pose.pose.position.z = 5;
      break;
    // Move the Naiad down.
    case '6':
      msg.pose.pose.position.z = 6;
      break;
    // Let the user set an angle to face the Naiad.
    // Check for inputs and allows only integers.
    case '7' :
      msg.pose.pose.position.z = 7;
      std::cout << "Enter a angle from -180 - 180: ";
      std::cin >> inputAngle;
      if (std::cin.fail())
      {
        std::cin.clear();
        std::cin.ignore();
        std::cout << "Not a number..." << std::endl;
        break;
      }
      if (inputAngle > 180 || inputAngle < -180)
      {
        std::cout << "Value to small or to big..." << std::endl;
        break;
      }
      msg.pose.pose.orientation.z = inputAngle;
      break;
    // Stop the Naiad.
    case '8':
      msg.pose.pose.position.z = 8;
      quitController.SetQuitMsg(1);
      quitController.QuitPublish();
      break;
    // Quit.
    case '9':
      msg.pose.pose.position.z = 9;
      //Flag to quit
      quitController.SetQuitMsg(2);
      quitController.QuitPublish();
      std::cout << "Exitng...\n" << std::endl;
      break;
others:
      break;
  }
  
  
  /*
  switch(input[0])
  case 'q':
  case 'm':
  case 'r':
    */

  /*
  if (inputX[0] == 'q'){
    std::cout << "Exiting...\n" << std::endl;
    return -1;
  }
  else if (check == 100)
  {
    std::cout << "Enter angle: ";
    std::cin >> msg.pose.pose.orientation.z;
    msg.pose.pose.position.x = 100;
    return 1;
  }
  else if (abs(std::atoi(inputX.c_str())) < MAX_DIST){
    msg.pose.pose.position.x = std::atoi(inputX.c_str());
    std::cout << "Enter goal for Y: " << std::endl;
    std::cin >> inputY;
    if (inputY[0] == 'q'){
      std::cout << "Exit ..\n" << std::endl;
      return -1;
    }
    else if(abs(std::atoi(inputY.c_str())) < MAX_DIST){
      msg.pose.pose.position.y = std::atoi(inputY.c_str());
      return 1;
    }
    else {
      std::cout << "Invalid input y!" << std::endl;
      return 0;
    }
  }
  else {   
    std::cout << "Invalid input x!" << std::endl;
    return 0;
  }
  */
}



int main(int argc, char **argv)
{
  int inputChecked;
  ros::init(argc, argv,"Naiad_test_node");
  //ros::NodeHandle n;
  ros::NodeHandle nh;
  ros::Publisher pub = nh.advertise<nav_msgs::Odometry>("naiad_testing", 1);

  QuitController quitController;

  std::cout << "Enter '1' to increase the thrusters threshold" << std::endl;
  std::cout << "Enter '2' to decrease the thrusters threshold" << std::endl;
  std::cout << "Enter '3' to activate the square pattern" << std::endl;
  std::cout << "Enter '4' to insert custom coordiantes" << std::endl;
  std::cout << "Enter '5' go up" << std::endl;
  std::cout << "Enter '6' go down" << std::endl;
  std::cout << "Enter '7' to turn to a angle" << std::endl;
  std::cout << "Enter '8' to stop" << std::endl;
  std::cout << "Enter '9' to quit" << std::endl;
  
  while(ros::ok())
  {
    inputChecked = CheckInput(quitController);
    pub.publish(msg);
  }
}
