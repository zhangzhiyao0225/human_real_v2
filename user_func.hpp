#ifndef USER_FUNC_HPP
#define USER_FUNC_HPP

#include <float.h>
#include <math.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <memory>
#include <unordered_map>

// #include "ovinf/controller/init_pos.hpp"
// #include "ovinf/controller/policy_controller_factory.hpp"
// #include "ovinf/robot/efc_common.h"
// #include "ovinf/robot/robot_efc.hpp"

#include "controller/init_pos.hpp"
#include "controller/policy_controller_factory.hpp"
#include "robot/common.h"
#include "robot/robot.h"

using RobotT = ovinf::RobotEfc;

enum Events
{
  InitPose = 1001,
  RunPolicy,
  EnableStandingPolicy,
  EnableWarkingPolicy,
  EnableRobustPolicy,
  // EnableLocomotion21Policy,
  PolicySwitch,

  VeloxIncrease = 2001,
  VeloxDecrease = 2002,
  VeloyIncrease = 2003,
  VeloyDecrease = 2004,
  VelowIncrease = 2005,
  VelowDecrease = 2006,

  SetVelX = 2014,
  SetVelY = 2015,
  SetVelW = 2016,
  SetVelAll = BACKEND_EVENT_SET_ALL_VELO,
};

enum class States : bitbot::StateId
{
  Waiting = 1001,

  InitPose,
  PolicyRunning,
  PolicySwitching,
};

static std::unordered_map<bitbot::EventValue, bitbot::EventId> s_key_2_evts =
    {
        // start -> waing
        {0, static_cast<bitbot::EventId>(bitbot::KernelEvent::STOP)},
        // waiting -> init
        {1, static_cast<bitbot::EventId>(Events::InitPose)},
        // init -> policy running
        {2, static_cast<bitbot::EventId>(Events::RunPolicy)},
        {3, static_cast<bitbot::EventId>(Events::EnableStandingPolicy)},
        {4, static_cast<bitbot::EventId>(Events::EnableWarkingPolicy)},
        {5, static_cast<bitbot::EventId>(Events::EnableRobustPolicy)},
        // {6, static_cast<bitbot::EventId>(Events::EnableLocomotion21Policy)},
};

static bitbot::bitbot_init_param s_initparam = {
    &s_key_2_evts,
    {
        bitbot::EventId(States::PolicyRunning),
    },
    [](bitbot::EventId)
    { return false; }};

class MakeBitbotEverywhere
{
protected:
public:
  MakeBitbotEverywhere(std::string const &kernel_config,
                       std::string const &controller_config)
      : kernel_(kernel_config, s_initparam)
  {

    logger_ = bitbot::Logger().ConsoleLogger();
    YAML::Node config = YAML::LoadFile(controller_config);

    switching_time_ = config["RobotConfig"]["switching_time"].as<double>();

    // robot
    robot_ = std::make_shared<RobotT>(config["RobotConfig"]);
    robot_->SetRobNotiPublisher(kernel_.GetRobNotiPublisher()->GetRobotNotificationPublisher());

    // init controller
    init_pos_controller_ = std::make_shared<ovinf::InitPosController>(
        robot_, config["RobotConfig"]["init_pos"]);

    // Policy net
    standing_controller_ =
        ovinf::PolicyControllerFactory::CreatePolicyController(
            robot_, config["RobotConfig"]["policy_standing"]);
    walking_controller_ =
        ovinf::PolicyControllerFactory::CreatePolicyController(
            robot_, config["RobotConfig"]["policy_walking"]);
    robust_controller_ = ovinf::PolicyControllerFactory::CreatePolicyController(
        robot_, config["RobotConfig"]["policy_robust"]);
    // locomotion21_controller_ = ovinf::PolicyControllerFactory::CreatePolicyController(
    //     robot_, config["RobotConfig"]["policy_robust"]);
    current_policy_controller_ = standing_controller_;
    target_policy_controller_ = standing_controller_;

    command_.setZero();
  }

  void WillMake()
  {
    // Config
    kernel_.RegisterConfigFunc(
        [this](const KernelBus &bus, UserData &)
        { robot_->GetDevice(bus); });

    // Event
    kernel_.RegisterEvent(
        "init_pose", static_cast<bitbot::EventId>(Events::InitPose),
        [this](bitbot::EventValue, UserData &)
        {
          std::cout << "proc InitPose 1001 event" << std::endl;
          init_pos_controller_->Init();
          return static_cast<bitbot::StateId>(States::InitPose);
        });

    kernel_.RegisterEvent(
        "run_policy", static_cast<bitbot::EventId>(Events::RunPolicy),
        [this](bitbot::EventValue, UserData &)
        {
          // std::cout << "proc RunPolicy 1002 event" << std::endl;
          current_policy_controller_ = standing_controller_;
          current_policy_controller_->Init();
          return static_cast<bitbot::StateId>(States::PolicyRunning);
        });

    kernel_.RegisterEvent(
        "enable_standing_policy",
        static_cast<bitbot::EventId>(Events::EnableStandingPolicy),
        [this](bitbot::EventValue, UserData &)
        {
          if (current_policy_controller_ == standing_controller_)
          {
            logger_->warn("Standing policy is already enabled");
            return static_cast<bitbot::StateId>(States::PolicyRunning);
          }
          else
          {
            logger_->info("Enabling standing policy");
            target_policy_controller_ = standing_controller_;
            target_policy_controller_->Init();
            return static_cast<bitbot::StateId>(States::PolicySwitching);
          }
        });

    kernel_.RegisterEvent(
        "enable_warking_policy",
        static_cast<bitbot::EventId>(Events::EnableWarkingPolicy),
        [this](bitbot::EventValue, UserData &)
        {
          if (current_policy_controller_ == walking_controller_)
          {
            logger_->warn("Walking policy is already enabled");
            return static_cast<bitbot::StateId>(States::PolicyRunning);
          }
          else
          {
            logger_->info("Enabling walking policy");
            target_policy_controller_ = walking_controller_;
            target_policy_controller_->Init();
            return static_cast<bitbot::StateId>(States::PolicySwitching);
          }
        });

    kernel_.RegisterEvent(
        "enable_robust_policy",
        static_cast<bitbot::EventId>(Events::EnableRobustPolicy),
        [this](bitbot::EventValue, UserData &)
        {
          if (current_policy_controller_ == robust_controller_)
          {
            logger_->warn("Robust policy is already enabled");
            return static_cast<bitbot::StateId>(States::PolicyRunning);
          }
          else
          {
            logger_->info("Enabling robust policy");
            target_policy_controller_ = robust_controller_;
            target_policy_controller_->Init();
            return static_cast<bitbot::StateId>(States::PolicySwitching);
          }
        });

    kernel_.RegisterEvent(
        "policy_switch", static_cast<bitbot::EventId>(Events::PolicySwitch),
        [this](bitbot::EventValue, UserData &)
        {
          // std::cout << "proc PolicySwitch 1006 event" << std::endl;
          current_policy_controller_->Stop();
          current_policy_controller_ = target_policy_controller_;
          return static_cast<bitbot::StateId>(States::PolicyRunning);
        });

    kernel_.RegisterEvent(
        "velo_x_increase", static_cast<bitbot::EventId>(Events::VeloxIncrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[0] += 0.05;
            logger_->info("current velocity: x={}", command_[0]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "velo_x_decrease", static_cast<bitbot::EventId>(Events::VeloxDecrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[0] -= 0.05;
            logger_->info("current velocity: x={}", command_[0]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "velo_y_increase", static_cast<bitbot::EventId>(Events::VeloyIncrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[1] += 0.05;
            logger_->info("current velocity: y={}", command_[1]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "velo_y_decrease", static_cast<bitbot::EventId>(Events::VeloyDecrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[1] -= 0.05;
            logger_->info("current velocity: y={}", command_[1]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "velo_w_increase", static_cast<bitbot::EventId>(Events::VelowIncrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[2] += 0.05;
            logger_->info("current velocity: w={}", command_[2]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "velo_w_decrease", static_cast<bitbot::EventId>(Events::VelowDecrease),
        [this](bitbot::EventValue key_state, UserData &)
        {
          if (1) // key_state ==
          //              static_cast<bitbot::EventValue>(bitbot::KeyboardEvent::Up))
          {
            command_[2] -= 0.05;
            logger_->info("current velocity: w={}", command_[2]);
          }
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "set_vel_x", static_cast<bitbot::EventId>(Events::SetVelX),
        [this](bitbot::EventValue key_state, UserData &)
        {
          double value = *reinterpret_cast<double *>(&key_state);
          command_[0] = value;
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "set_vel_y", static_cast<bitbot::EventId>(Events::SetVelY),
        [this](bitbot::EventValue key_state, UserData &)
        {
          double value = *reinterpret_cast<double *>(&key_state);
          command_[1] = value;
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "set_vel_w", static_cast<bitbot::EventId>(Events::SetVelW),
        [this](bitbot::EventValue key_state, UserData &)
        {
          double value = *reinterpret_cast<double *>(&key_state);
          command_[2] = value;
          // logger_->info("current velocity: x={} y={} w={}", command_[0],
          //               command_[1], command_[2]);
          return std::nullopt;
        });

    kernel_.RegisterEvent(
        "set_vel_all", static_cast<bitbot::EventId>(Events::SetVelAll),
        [this](bitbot::EventValue key_state, UserData &)
        {
          Twist *value = reinterpret_cast<Twist *>(key_state);
          command_[0] = value->linear().x();
          command_[1] = value->linear().y();
          command_[2] = value->angular().z();
          std::cout << "set_vel_all: " << command_[0] << " " << command_[1] << " " << command_[2] << std::endl;
          // logger_->info("current velocity: x={} y={} w={}", command_[0],
          //               command_[1], command_[2]);
          return std::nullopt;
        });

    // kernel_.RegisterEvent(
    //     "action_cmd", static_cast<bitbot::EventId>(Events::ActionCommand),
    //     [this](bitbot::EventValue key_state, UserData &user_data)
    //     {
    //       // do nothing
    //       assert(0);
    //       bitbot::EventId action = (bitbot::EventId)key_state;
    //       std::cout << "action_cmd: " << action << s_key_2_evts[action] << std::endl;
    //       kernel_.HandleRobotEvent(std::make_pair(s_key_2_evts[action], 0));
    //       return std::nullopt;
    //     });

    // State
    kernel_.RegisterState(
        "waiting", static_cast<bitbot::StateId>(States::Waiting),
        [this](const bitbot::KernelInterface &kernel,
               Kernel::ExtraData &extra_data, UserData &user_data)
        {
          // std::cout << "waiting for init" << std::endl;
          static bool first = true;
          if (first)
          {
            first = false;
            robot_->SetExtraData(extra_data);
          }

          robot_->Observer()->Update();
        },
        {Events::InitPose});

    kernel_.RegisterState(
        "init_pose", static_cast<bitbot::StateId>(States::InitPose),
        [this](const bitbot::KernelInterface &kernel,
               Kernel::ExtraData &extra_data, UserData &user_data)
        { 
	// static int i = 0;
	// if (i ++ < 10)
	// {
	// std::cout << "process init" << std::endl;
	// }
          robot_->Observer()->Update();
          init_pos_controller_->Step();
          target_policy_controller_->WarmUp();
          robot_->Executor()->ExecuteJointPosition();
        },
        {Events::RunPolicy});

    kernel_.RegisterState(
        "policy_running", static_cast<bitbot::StateId>(States::PolicyRunning),
        [this](const bitbot::KernelInterface &kernel,
               Kernel::ExtraData &extra_data, UserData &user_data)
        {
          std::cout << "policy running state" << std::endl;
          robot_->Observer()->Update();
          current_policy_controller_->GetCommand() = command_;
          current_policy_controller_->Step();
          robot_->Executor()->ExecuteJointPosition();
        },
        {
            Events::VeloxDecrease,
            Events::VeloxIncrease,
            Events::VeloyDecrease,
            Events::VeloyIncrease,
            Events::VelowIncrease,
            Events::VelowDecrease,
            Events::SetVelX,
            Events::SetVelY,
            Events::SetVelW,
            Events::SetVelAll,
            Events::EnableStandingPolicy,
            Events::EnableWarkingPolicy,
            Events::EnableRobustPolicy,
            // Events::EnableLocomotion21Policy,
            static_cast<bitbot::EventId>(bitbot::KernelEvent::STOP),
        });

    kernel_.RegisterState(
        "policy_switching",
        static_cast<bitbot::StateId>(States::PolicySwitching),
        [this](const bitbot::KernelInterface &kernel,
               Kernel::ExtraData &extra_data, UserData &user_data)
        {
          if (!switching_flag_)
          {
            switching_flag_ = true;
            switching_start_time_ = std::chrono::steady_clock::now();
          }
          double current_switching_time =
              std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                            switching_start_time_)
                  .count();
          if (current_switching_time < switching_time_)
          {
            robot_->Observer()->Update();
            current_policy_controller_->GetCommand() = command_;
            current_policy_controller_->Step();
            target_policy_controller_->WarmUp();
            robot_->Executor()->ExecuteJointPosition();
          }
          else
          {
            switching_flag_ = false;
            kernel.EmitEvent(Events::PolicySwitch, 0);
          }
        },
        {Events::PolicySwitch});

    // First state
    kernel_.SetFirstState(static_cast<bitbot::StateId>(States::Waiting));
    // kernel_.SetFirstState(static_cast<bitbot::StateId>(States::InitPose));
  }

  void
  BeMaking()
  {
    kernel_.Run();
  }

  void HaveMade()
  {
    logger_->info("Make BITBOT great forever!!!!!!!!!!!!!!!!");
  }

private:
  Kernel kernel_;
  bitbot::SpdLoggerSharedPtr logger_;
  RobotT::Ptr robot_;
  ovinf::InitPosController::Ptr init_pos_controller_;

  ovinf::PolicyControllerBase::Ptr standing_controller_ = nullptr;
  ovinf::PolicyControllerBase::Ptr walking_controller_ = nullptr;
  ovinf::PolicyControllerBase::Ptr robust_controller_ = nullptr;
  // ovinf::PolicyControllerBase::Ptr locomotion21_controller_ = nullptr;
  ovinf::PolicyControllerBase::Ptr current_policy_controller_ = nullptr;
  ovinf::PolicyControllerBase::Ptr target_policy_controller_ = nullptr;

  bool switching_flag_ = false;
  double switching_time_;
  std::chrono::steady_clock::time_point switching_start_time_;

  Eigen::Vector3f command_;
};

#endif // !USER_FUNC_H
