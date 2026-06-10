// #include "rosmodule.h"
#include "user_func.hpp"

#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string getAppDirectory()
{
  // Get the path of the current executable
  fs::path exePath = fs::canonical("/proc/self/exe");
  // Return the parent directory (where the app is located)
  return exePath.parent_path().parent_path().string();
}

int main(int argc, char **argv)
{
  // init ros
  // std::shared_ptr<RosStatusBridge> ros_bridge =
  //     std::make_shared<RosStatusBridge>();

  std::string cfgPath(std::move(getAppDirectory()));
  cfgPath += "/config/";
  std::cout << "cfg Location: " << cfgPath << std::endl;

  MakeBitbotEverywhere everyone(
      cfgPath + "efc.xml",
      cfgPath + "efc.yaml");
  everyone.WillMake();
  everyone.BeMaking();
  everyone.HaveMade();
  return 0;
}
