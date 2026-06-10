#ifndef OVINF_POLICY_CONTROLLER_FACTORY_HPP
#define OVINF_POLICY_CONTROLLER_FACTORY_HPP

#include <memory>
#include <stdexcept>
#include <string>

#include "policy_controller_base.hpp"
#include "policy_ctl_leg_only.hpp"
#include "policy_ctr_leg_waist.hpp"
#include "policy_ctr_21dof.hpp"
#include "policy_ctr_auto.hpp"
#include "handshake.hpp"
#include "wave_greeting.hpp"

namespace ovinf {

class PolicyControllerFactory {
 public:
  template <typename T = float>
  static std::shared_ptr<ovinf::PolicyControllerBase> CreatePolicyController(
      RobotBase<T>::RobotPtr robot, YAML::Node const &config) {
    std::string controller_type = config["controller_type"].as<std::string>();
    if (controller_type == "LEG_ONLY") {
      return std::make_shared<ovinf::PolicyCtrLegOnly>(robot, config);
    } else if (controller_type == "LEG_WAIST") {
      return std::make_shared<ovinf::PolicyCtrLegWaist>(robot, config);
    } else if (controller_type == "21DOF") {
      return std::make_shared<ovinf::PolicyCtr21DoF>(robot, config);
    } else if (controller_type == "AUTO") {
      return std::make_shared<ovinf::PolicyCtrAuto>(robot, config);
    } else {
      throw std::invalid_argument("Unknown controller type: " +
                                  controller_type);
    }
  }

  static std::shared_ptr<ovinf::PolicyControllerBase> CreatePolicyController(
      RobotBase<float>::RobotPtr robot, YAML::Node const &policy_config,
      YAML::Node const &config) {
    std::string controller_type = config["controller_type"].as<std::string>();
    if (controller_type == "WAVE_GREETING") {
      return std::make_shared<ovinf::WaveGreetingController>(robot,
                                                             policy_config,
                                                             config);
    } else if (controller_type == "HANDSHAKE") {
      return std::make_shared<ovinf::HandshakeController>(robot, policy_config,
                                                          config);
    } else {
      throw std::invalid_argument("Unknown controller type: " +
                                  controller_type);
    }
  }
};

}  // namespace ovinf

#endif  // !OVINF_POLICY_CONTROLLER_FACTORY_HPP
