// Copyright 2018, Bosch Software Innovations GmbH.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "generic_subscription.hpp"

#include <memory>
#include <string>

#include "rclcpp/any_subscription_callback.hpp"
#include "rclcpp/logger.hpp"
#include "rclcpp/subscription.hpp"

namespace iceoryx_ros2_bridge
{

GenericSubscription::GenericSubscription(
  rclcpp::node_interfaces::NodeBaseInterface * node_base,
  const rosidl_message_type_support_t & ts,
  const std::string & topic_name,
  std::function<void(std::shared_ptr<rmw_serialized_message_t>)> callback)
: SubscriptionBase(
    node_base,
    ts,
    topic_name,
    rcl_subscription_get_default_options(),
    true),
  default_allocator_(rcutils_get_default_allocator()),
  callback_(callback)
{}

GenericSubscription::GenericSubscription(
  rclcpp::node_interfaces::NodeBaseInterface * node_base,
  const rosidl_message_type_support_t & ts,
  const std::string & topic_name)
: SubscriptionBase(
    node_base,
    ts,
    topic_name,
    rcl_subscription_get_default_options(),
    true),
  default_allocator_(rcutils_get_default_allocator())
{}

void GenericSubscription::set_callback(
  std::function<void(std::shared_ptr<rmw_serialized_message_t>)> callback)
{
  callback_ = callback;
}

std::shared_ptr<void> GenericSubscription::create_message()
{
  return create_serialized_message();
}

std::shared_ptr<rmw_serialized_message_t> GenericSubscription::create_serialized_message()
{
  return borrow_serialized_message(0);
}

void GenericSubscription::handle_message(
  std::shared_ptr<void> & message, const rclcpp::MessageInfo & message_info)
{
  (void) message_info;
  auto typed_message = std::static_pointer_cast<rmw_serialized_message_t>(message);
  callback_(typed_message);
}

void GenericSubscription::return_message(std::shared_ptr<void> & message)
{
  auto typed_message = std::static_pointer_cast<rmw_serialized_message_t>(message);
  return_serialized_message(typed_message);
}

void GenericSubscription::return_serialized_message(
  std::shared_ptr<rmw_serialized_message_t> & message)
{
  message.reset();
}

void GenericSubscription::handle_loaned_message(
  void * loaned_message,
  const rclcpp::MessageInfo & message_info)
{
  (void) loaned_message;
  (void) message_info;
}

std::shared_ptr<rmw_serialized_message_t>
GenericSubscription::borrow_serialized_message(size_t capacity)
{
  auto message = new rmw_serialized_message_t;
  *message = rmw_get_zero_initialized_serialized_message();
  auto init_return = rmw_serialized_message_init(message, capacity, &default_allocator_);
  if (init_return != RCL_RET_OK) {
    rclcpp::exceptions::throw_from_rcl_error(init_return);
  }

  auto serialized_msg = std::shared_ptr<rmw_serialized_message_t>(
    message,
    [](rmw_serialized_message_t * msg) {
      auto fini_return = rmw_serialized_message_fini(msg);
      delete msg;
      if (fini_return != RCL_RET_OK) {
        RCLCPP_ERROR_STREAM(
          rclcpp::get_logger("iceoryx_ros2_bridge"),
          "Failed to destroy serialized message: " << rcl_get_error_string().str);
      }
    });

  return serialized_msg;
}

}  // namespace iceoryx_ros2_bridge
