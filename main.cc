// #include "rosmodule.h"
#include "user_func.hpp"

int main(int argc, char **argv)
{
  // init ros
  // std::shared_ptr<RosStatusBridge> ros_bridge =
  //     std::make_shared<RosStatusBridge>();

  MakeBitbotEverywhere everyone(
      "/home/norco/xky/work/rob/mc/config/efc.xml",
      "/home/norco/xky/work/rob/mc/config/efc.yaml");
  everyone.WillMake();
  everyone.BeMaking();
  everyone.HaveMade();
  return 0;
}
