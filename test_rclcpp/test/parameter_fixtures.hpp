// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef PARAMETER_FIXTURES_HPP_
#define PARAMETER_FIXTURES_HPP_

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "gtest/gtest.h"

#include "rcl_interfaces/srv/list_parameters.hpp"
#include "rclcpp/rclcpp.hpp"

const double test_epsilon = 1e-6;

void set_test_parameters(
  std::shared_ptr<rclcpp::parameter_client::SyncParametersClient> parameters_client)
{
  // Set several differnet types of parameters.
  auto set_parameters_results = parameters_client->set_parameters({
    rclcpp::parameter::ParameterVariant("foo", 2),
    rclcpp::parameter::ParameterVariant("bar", "hello"),
    rclcpp::parameter::ParameterVariant("barstr", std::string("hello_str")),
    rclcpp::parameter::ParameterVariant("baz", 1.45),
    rclcpp::parameter::ParameterVariant("foo.first", 8),
    rclcpp::parameter::ParameterVariant("foo.second", 42),
    rclcpp::parameter::ParameterVariant("foobar", true),
  });

  // Check to see if they were set.
  for (auto & result : set_parameters_results) {
    ASSERT_TRUE(result.successful);
  }
}

void verify_set_parameters_async(
  std::shared_ptr<rclcpp::Node> node,
  std::shared_ptr<rclcpp::parameter_client::AsyncParametersClient> parameters_client)
{
  // Set several differnet types of parameters.
  auto set_parameters_results = parameters_client->set_parameters({
    rclcpp::parameter::ParameterVariant("foo", 2),
    rclcpp::parameter::ParameterVariant("bar", "hello"),
    rclcpp::parameter::ParameterVariant("barstr", std::string("hello_str")),
    rclcpp::parameter::ParameterVariant("baz", 1.45),
    rclcpp::parameter::ParameterVariant("foo.first", 8),
    rclcpp::parameter::ParameterVariant("foo.second", 42),
    rclcpp::parameter::ParameterVariant("foobar", true),
  });
  rclcpp::spin_until_future_complete(node, set_parameters_results);  // Wait for the results.
  // Check to see if they were set.
  for (auto & result : set_parameters_results.get()) {
    ASSERT_TRUE(result.successful);
  }
}

void verify_test_parameters(
  std::shared_ptr<rclcpp::parameter_client::SyncParametersClient> parameters_client)
{
  // Test recursive depth (=0)
  auto parameters_and_prefixes = parameters_client->list_parameters({"foo", "bar"},
      rcl_interfaces::srv::ListParameters::Request::DEPTH_RECURSIVE);
  for (auto & name : parameters_and_prefixes.names) {
    EXPECT_TRUE(name == "foo" || name == "bar" || name == "foo.first" || name == "foo.second");
  }
  for (auto & prefix : parameters_and_prefixes.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }

  // Test different depth
  auto parameters_and_prefixes4 = parameters_client->list_parameters({"foo"}, 1);
  for (auto & name : parameters_and_prefixes4.names) {
    EXPECT_EQ(name, "foo");
  }
  for (auto & prefix : parameters_and_prefixes4.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }

  // Test different depth
  auto parameters_and_prefixes5 = parameters_client->list_parameters({"foo"}, 2);
  for (auto & name : parameters_and_prefixes5.names) {
    EXPECT_TRUE(name == "foo" || name == "foo.first" || name == "foo.second");
  }
  for (auto & prefix : parameters_and_prefixes5.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }

  // Get a few of the parameters just set.
  for (auto & parameter : parameters_client->get_parameters({"foo", "bar", "baz"})) {
    // std::cout << "Parameter is:" << std::endl << parameter.to_yaml() << std::endl;
    if (parameter.get_name() == "foo") {
      EXPECT_STREQ(
        "{\"name\": \"foo\", \"type\": \"integer\", \"value\": \"2\"}",
        std::to_string(parameter).c_str());
      EXPECT_STREQ("integer", parameter.get_type_name().c_str());
    } else if (parameter.get_name() == "bar") {
      EXPECT_STREQ(
        "{\"name\": \"bar\", \"type\": \"string\", \"value\": \"hello\"}",
        std::to_string(parameter).c_str());
      EXPECT_STREQ("string", parameter.get_type_name().c_str());
    } else if (parameter.get_name() == "baz") {
      EXPECT_STREQ("double", parameter.get_type_name().c_str());
      EXPECT_NEAR(1.45, parameter.as_double(), test_epsilon);
    } else {
      ASSERT_FALSE("you should never hit this");
    }
  }

  // Get a few non existant parameters
  for (auto & parameter : parameters_client->get_parameters({"not_foo", "not_baz"})) {
    EXPECT_STREQ("There should be no matches", parameter.get_name().c_str());
  }

  // List all of the parameters, using an empty prefix list and depth=0
  parameters_and_prefixes = parameters_client->list_parameters({},
      rcl_interfaces::srv::ListParameters::Request::DEPTH_RECURSIVE);
  std::vector<std::string> all_names = {
    "foo", "bar", "barstr", "baz", "foo.first", "foo.second", "foobar"
  };
  EXPECT_EQ(parameters_and_prefixes.names.size(), all_names.size());
  for (auto & name : all_names) {
    EXPECT_NE(std::find(
        parameters_and_prefixes.names.cbegin(),
        parameters_and_prefixes.names.cend(),
        name),
      parameters_and_prefixes.names.cend());
  }
  // List all of the parameters, using an empty prefix list and large depth
  parameters_and_prefixes = parameters_client->list_parameters({}, 100);
  EXPECT_EQ(parameters_and_prefixes.names.size(), all_names.size());
  for (auto & name : all_names) {
    EXPECT_NE(std::find(
        parameters_and_prefixes.names.cbegin(),
        parameters_and_prefixes.names.cend(),
        name),
      parameters_and_prefixes.names.cend());
  }
  // List most of the parameters, using an empty prefix list and depth=1
  parameters_and_prefixes = parameters_client->list_parameters({}, 1);
  std::vector<std::string> depth_one_names = {
    "foo", "bar", "barstr", "baz", "foobar"
  };
  EXPECT_EQ(parameters_and_prefixes.names.size(), depth_one_names.size());
  for (auto & name : depth_one_names) {
    EXPECT_NE(std::find(
        parameters_and_prefixes.names.cbegin(),
        parameters_and_prefixes.names.cend(),
        name),
      parameters_and_prefixes.names.cend());
  }
}

void verify_get_parameters_async(
  std::shared_ptr<rclcpp::Node> node,
  std::shared_ptr<rclcpp::parameter_client::AsyncParametersClient> parameters_client)
{
  // Test recursive depth (=0)
  auto result = parameters_client->list_parameters({"foo", "bar"},
      rcl_interfaces::srv::ListParameters::Request::DEPTH_RECURSIVE);
  rclcpp::spin_until_future_complete(node, result);
  auto parameters_and_prefixes = result.get();
  for (auto & name : parameters_and_prefixes.names) {
    EXPECT_TRUE(name == "foo" || name == "bar" || name == "foo.first" || name == "foo.second");
  }
  for (auto & prefix : parameters_and_prefixes.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }

  // Test different depth
  auto result4 = parameters_client->list_parameters({"foo"}, 1);
  rclcpp::spin_until_future_complete(node, result4);
  auto parameters_and_prefixes4 = result4.get();
  for (auto & name : parameters_and_prefixes4.names) {
    EXPECT_EQ(name, "foo");
  }
  for (auto & prefix : parameters_and_prefixes4.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }

  // Test different depth
  auto result4a = parameters_client->list_parameters({"foo"}, 2);
  rclcpp::spin_until_future_complete(node, result4a);
  auto parameters_and_prefixes4a = result4a.get();
  for (auto & name : parameters_and_prefixes4a.names) {
    EXPECT_TRUE(name == "foo" || name == "foo.first" || name == "foo.second");
  }
  for (auto & prefix : parameters_and_prefixes4a.prefixes) {
    EXPECT_STREQ("foo", prefix.c_str());
  }


  // Get a few of the parameters just set.
  auto result2 = parameters_client->get_parameters({"foo", "bar", "baz"});
  rclcpp::spin_until_future_complete(node, result2);
  for (auto & parameter : result2.get()) {
    if (parameter.get_name() == "foo") {
      EXPECT_STREQ("foo", parameter.get_name().c_str());
      EXPECT_STREQ(
        "{\"name\": \"foo\", \"type\": \"integer\", \"value\": \"2\"}",
        std::to_string(parameter).c_str());
      EXPECT_STREQ("integer", parameter.get_type_name().c_str());
    } else if (parameter.get_name() == "bar") {
      EXPECT_STREQ(
        "{\"name\": \"bar\", \"type\": \"string\", \"value\": \"hello\"}",
        std::to_string(parameter).c_str());
      EXPECT_STREQ("string", parameter.get_type_name().c_str());
    } else if (parameter.get_name() == "baz") {
      EXPECT_STREQ("double", parameter.get_type_name().c_str());
      EXPECT_NEAR(1.45, parameter.as_double(), test_epsilon);
    } else {
      ASSERT_FALSE("you should never hit this");
    }
  }

  // Get a few non existant parameters
  auto result3 = parameters_client->get_parameters({"not_foo", "not_baz"});
  rclcpp::spin_until_future_complete(node, result3);
  for (auto & parameter : result3.get()) {
    EXPECT_STREQ("There should be no matches", parameter.get_name().c_str());
  }

  // List all of the parameters, using an empty prefix list
  auto result5 = parameters_client->list_parameters({},
      rcl_interfaces::srv::ListParameters::Request::DEPTH_RECURSIVE);
  rclcpp::spin_until_future_complete(node, result5);
  parameters_and_prefixes = result5.get();
  std::vector<std::string> all_names = {
    "foo", "bar", "barstr", "baz", "foo.first", "foo.second", "foobar"
  };
  EXPECT_EQ(parameters_and_prefixes.names.size(), all_names.size());
  for (auto & name : all_names) {
    EXPECT_NE(std::find(
        parameters_and_prefixes.names.cbegin(),
        parameters_and_prefixes.names.cend(),
        name),
      parameters_and_prefixes.names.cend());
  }
}

#endif  // PARAMETER_FIXTURES_HPP_
