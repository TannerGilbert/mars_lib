// Copyright (C) 2021 Christian Brommer and Martin Scheiber, Control of Networked Systems, University of Klagenfurt,
// Austria.
//
// All rights reserved.
//
// This software is licensed under the terms of the BSD-2-Clause-License with
// no commercial use allowed, the full terms of which are made available
// in the LICENSE file. No license in patents is granted.
//
// You can contact the author at <christian.brommer@ieee.org>
// and <martin.scheiber@ieee.org>

#include <gmock/gmock.h>
#include <mars/buffer.h>
#include <mars/sensors/imu/imu_sensor_class.h>
#include <mars/sensors/pose/pose_measurement_type.h>
#include <mars/sensors/pose/pose_sensor_class.h>
#include <mars/sensors/position/position_measurement_type.h>
#include <mars/sensors/position/position_sensor_class.h>
#include <mars/time.h>
#include <mars/type_definitions/buffer_entry_type.h>
#include <Eigen/Dense>

class mars_buffer_test : public testing::Test
{
public:
  mars::BufferDataType data_no_state_;
  mars::BufferDataType data_with_state_;

  mars_buffer_test()
  {
    int core_dummy = 13;
    int sensor_dummy = 15;
    data_with_state_.set_states(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));
  }
};

///
/// \brief Test that the CTOR arguments are handled correct
///
TEST_F(mars_buffer_test, CTOR)
{
  // Test buffer size setting by class init argument
  mars::Buffer buffer_ctor_1(100);
  ASSERT_EQ(buffer_ctor_1.get_max_buffer_size(), 100);
  mars::Buffer buffer_ctor_2(-1 * 100);
  ASSERT_EQ(buffer_ctor_2.get_max_buffer_size(), 100);

  // Test setter of max buffer size
  mars::Buffer buffer_setter_1(100);
  // Change trough setter method
  buffer_setter_1.set_max_buffer_size(200);
  // Check setter result
  ASSERT_EQ(buffer_setter_1.get_max_buffer_size(), 200);
  buffer_setter_1.set_max_buffer_size(-1 * 200);
  // Check negative values for setter
  ASSERT_EQ(buffer_setter_1.get_max_buffer_size(), 200);
}

///
/// \brief Ensure that all entry getter return false on an empty buffer
///
TEST_F(mars_buffer_test, GETTER_EMPTY_BUFFER_RETURN)
{
  const int max_buffer_size = 100;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose", core_states_sptr);

  try
  {
    ASSERT_EQ(buffer.IsEmpty(), 1);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what();
    exit(EXIT_FAILURE);
  }

  // Check empty buffer returns
  mars::BufferEntryType latest_state;
  int latest_state_return = buffer.get_latest_state(&latest_state);
  ASSERT_EQ(latest_state_return, 0);

  mars::BufferEntryType oldest_state;
  int oldest_state_return = buffer.get_oldest_state(&oldest_state);
  ASSERT_EQ(oldest_state_return, 0);

  mars::BufferEntryType latest_entry;
  int latest_entry_return = buffer.get_latest_entry(&latest_entry);
  ASSERT_EQ(latest_entry_return, 0);

  mars::BufferEntryType latest_init_state;
  int latest_init_state_return = buffer.get_latest_init_state(&latest_init_state);
  ASSERT_EQ(latest_init_state_return, 0);

  mars::BufferEntryType latest_sensor_handle_state;
  int latest_sensor_handle_state_return =
      buffer.get_latest_sensor_handle_state(pose_sensor_sptr, &latest_sensor_handle_state);
  ASSERT_EQ(latest_sensor_handle_state_return, 0);

  // With index argument
  int latest_sensor_state_idx;
  buffer.get_latest_sensor_handle_state(pose_sensor_sptr, &latest_sensor_handle_state, &latest_sensor_state_idx);
  ASSERT_EQ(latest_sensor_state_idx, -1);

  mars::BufferEntryType latest_sensor_handle_measurement;
  int latest_sensor_handle_measurement_return =
      buffer.get_latest_sensor_handle_measurement(pose_sensor_sptr, &latest_sensor_handle_measurement);
  ASSERT_EQ(latest_sensor_handle_measurement_return, 0);

  mars::BufferEntryType closest_state;
  mars::Time timestamp(1);
  int closest_state_return = buffer.get_closest_state(timestamp, &closest_state);
  ASSERT_EQ(closest_state_return, 0);

  // With index argument
  int closest_state_index;
  closest_state_return = buffer.get_closest_state(timestamp, &closest_state, &closest_state_index);
  ASSERT_EQ(closest_state_index, -1);

  mars::BufferEntryType entry_at_idx;
  int index = 1;
  int entry_at_idx_return = buffer.get_entry_at_idx(index, &entry_at_idx);
  ASSERT_EQ(entry_at_idx_return, 0);
}

///
/// \brief Test that old entrys are removed if max_buffer_size is reached
///
TEST_F(mars_buffer_test, STORAGE_MAX_ENTRY)
{
  // Handle removal of overflowing buffer entrys

  const int num_test_entrys = 20;
  const int max_buffer_size = 10;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  // Normal case
  int core_dummy = 13;
  int sensor_dummy = 15;
  mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  for (int k = 0; k < num_test_entrys; k++)
  {
    mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr, 1);
    buffer.AddEntrySorted(entry);
    buffer.RemoveOverflowEntrys();
  }

  std::cout << "Buffer Length: " << buffer.get_length() << std::endl;
  buffer.PrintBufferEntries();

  ASSERT_EQ(buffer.get_length(), max_buffer_size);

  // Case where the last entry is also the last sensor handle
  mars::BufferDataType data2(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));
  mars::Buffer buffer2(max_buffer_size);

  for (int k = 0; k < num_test_entrys; k++)
  {
    if (k == 0)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr, 1);
      buffer2.AddEntrySorted(entry);
      buffer2.RemoveOverflowEntrys();
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr, 1);
      buffer2.AddEntrySorted(entry);
      buffer2.RemoveOverflowEntrys();
    }
  }
  buffer2.PrintBufferEntries();
  ASSERT_EQ(buffer.get_length(), max_buffer_size);

  mars::BufferEntryType t2_idx0, t2_idx1;
  buffer2.get_entry_at_idx(0, &t2_idx0);
  buffer2.get_entry_at_idx(1, &t2_idx1);

  ASSERT_EQ(t2_idx0.timestamp_, 0);
  ASSERT_EQ(t2_idx1.timestamp_, 11);

  // Case where the last entry becomes the last sensor handle while adding other entries
  mars::BufferDataType data3(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));
  mars::Buffer buffer3(max_buffer_size);

  for (int k = 0; k < num_test_entrys; k++)
  {
    if (k == 0 || k == 5 || k == 9)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr, 1);
      buffer3.AddEntrySorted(entry);
      buffer3.RemoveOverflowEntrys();
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr, 1);
      buffer3.AddEntrySorted(entry);
      buffer3.RemoveOverflowEntrys();
    }
  }
  buffer3.PrintBufferEntries();
  ASSERT_EQ(buffer.get_length(), max_buffer_size);

  mars::BufferEntryType t3_idx0, t3_idx1;
  buffer3.get_entry_at_idx(0, &t3_idx0);
  buffer3.get_entry_at_idx(1, &t3_idx1);

  ASSERT_EQ(t3_idx0.timestamp_, 9);
  ASSERT_EQ(t3_idx1.timestamp_, 11);
}

TEST_F(mars_buffer_test, LATEST_ENTRY)
{
  const int num_test_entrys = 10;
  const int max_buffer_size = 100;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  mars::Time current_timestamp(0);
  int core_dummy = 13;
  int sensor_dummy = 15;

  for (int k = num_test_entrys; k > 0; k--)
  {
    mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

    current_timestamp = current_timestamp + mars::Time(1);

    if (k % 2 == 0)
    {
      mars::BufferEntryType entry(current_timestamp, data, pose_sensor_1_sptr);
      buffer.AddEntrySorted(entry);
    }
    else
    {
      mars::BufferEntryType entry(current_timestamp, data, pose_sensor_2_sptr);
      buffer.AddEntrySorted(entry);
    }
  }

  std::cout << buffer.get_length() << std::endl;
  buffer.PrintBufferEntries();

  mars::BufferEntryType latest_entry;
  buffer.get_latest_entry(&latest_entry);

  std::cout << "Picked:\n" << latest_entry << std::endl;
  EXPECT_EQ(latest_entry.timestamp_, current_timestamp);
}

TEST_F(mars_buffer_test, OLDEST_LATEST_STATE_RETURN)
{
  /// NEEDS REWRITING

  const int num_test_entrys = 10;
  const int max_buffer_size = 100;
  mars::Buffer buffer(max_buffer_size);

  // Empty buffer return tests
  mars::BufferEntryType latest_state_empty;
  int latest_emtpy_state_return = buffer.get_latest_state(&latest_state_empty);
  ASSERT_EQ(latest_emtpy_state_return, 0);

  mars::BufferEntryType oldest_state_empty;
  int oldest_empty_state_return = buffer.get_oldest_state(&oldest_state_empty);
  ASSERT_EQ(oldest_empty_state_return, 0);

  // Filled buffer return tests
  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  int core_dummy = 13;
  int sensor_dummy = 15;
  int measurement_dummy = 12;

  for (int k = num_test_entrys; k > 0; k--)
  {
    mars::BufferDataType data;
    data.set_measurement(std::make_shared<int>(measurement_dummy));
    data.set_states(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

    if (k % 2 == 0)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr);
      buffer.AddEntrySorted(entry);
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr);
      buffer.AddEntrySorted(entry);
    }
  }

  std::cout << buffer.get_length() << std::endl;
  buffer.PrintBufferEntries();

  mars::BufferEntryType latest_state_full;
  int latest_filled_state_return = buffer.get_latest_state(&latest_state_full);
  ASSERT_EQ(latest_filled_state_return, 1);
  ASSERT_EQ(latest_state_full.timestamp_, 10);
  std::cout << "Pick for latest:\n" << latest_state_full << std::endl;

  mars::BufferEntryType oldest_state_full;
  int oldest_filled_state_return = buffer.get_oldest_state(&oldest_state_full);
  ASSERT_EQ(oldest_filled_state_return, 1);
  ASSERT_EQ(oldest_state_full.timestamp_, 1);
  std::cout << "Picked for oldest:\n" << oldest_state_full << std::endl;

  // Second case where the latest and newest do not have a state, only a measurement
  mars::Buffer buffer2(max_buffer_size);
  for (int k = num_test_entrys; k > 0; k--)
  {
    mars::BufferDataType data;
    data.set_measurement(std::make_shared<int>(measurement_dummy));
    data.set_states(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

    if (k == 1 || k == num_test_entrys)
    {
      data.ClearStates();
    }

    if (k % 2 == 0)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr);
      buffer2.AddEntrySorted(entry);
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr);
      buffer2.AddEntrySorted(entry);
    }
  }

  std::cout << buffer2.get_length() << std::endl;
  buffer2.PrintBufferEntries();

  mars::BufferEntryType latest_state_full2;
  int latest_filled_state_return2 = buffer2.get_latest_state(&latest_state_full);
  ASSERT_EQ(latest_filled_state_return2, 1);
  ASSERT_EQ(latest_state_full.timestamp_, 9);
  std::cout << "Pick for latest:\n" << latest_state_full << std::endl;

  mars::BufferEntryType oldest_state_full2;
  int oldest_filled_state_return2 = buffer2.get_oldest_state(&oldest_state_full);
  ASSERT_EQ(oldest_filled_state_return2, 1);
  ASSERT_EQ(oldest_state_full.timestamp_, 2);
  std::cout << "Picked for oldest:\n" << oldest_state_full << std::endl;
}

///
/// \brief Test that resetting the buffer removes all entrys
///
TEST_F(mars_buffer_test, RESET_BUFFER)
{
  const int num_test_entrys = 100;
  const int max_buffer_size = 110;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  int core_dummy = 13;
  int sensor_dummy = 15;

  for (int k = num_test_entrys; k > 0; k--)
  {
    mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

    if (k % 2 == 0)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr);
      buffer.AddEntrySorted(entry);
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr);
      buffer.AddEntrySorted(entry);
    }
  }

  std::cout << buffer.get_length() << std::endl;
  buffer.PrintBufferEntries();
  ASSERT_EQ(buffer.get_length(), num_test_entrys);
  ASSERT_EQ(buffer.IsEmpty(), 0);

  std::cout << "Reset Buffer" << std::endl;
  buffer.ResetBufferData();
  std::cout << buffer.get_length() << std::endl;

  // Check if buffer is reset
  ASSERT_EQ(buffer.get_length(), 0);
  ASSERT_EQ(buffer.IsEmpty(), 1);
}

TEST_F(mars_buffer_test, GET_ENTRY_METHODS)
{
  mars::Buffer buffer;

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  buffer.AddEntrySorted(mars::BufferEntryType(0, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(1, data_with_state, pose_sensor_1_sptr, mars::BufferMetadataType::init));
  buffer.AddEntrySorted(mars::BufferEntryType(2, data_with_state, pose_sensor_2_sptr, mars::BufferMetadataType::init));
  buffer.AddEntrySorted(mars::BufferEntryType(3, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(4, data_with_state, pose_sensor_2_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(5, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(6, data_no_state, pose_sensor_2_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(7, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(8, data_with_state, pose_sensor_2_sptr));
  buffer.AddEntrySorted(
      mars::BufferEntryType(9, data_no_state, pose_sensor_1_sptr, mars::BufferMetadataType::out_of_order));
  buffer.AddEntrySorted(
      mars::BufferEntryType(10, data_no_state, pose_sensor_2_sptr, mars::BufferMetadataType::out_of_order));

  mars::BufferEntryType latest_entry_return;
  buffer.get_latest_entry(&latest_entry_return);
  ASSERT_EQ(latest_entry_return.timestamp_, 10);

  mars::BufferEntryType oldest_state_return;
  buffer.get_oldest_state(&oldest_state_return);
  ASSERT_EQ(oldest_state_return.timestamp_, 1);

  mars::BufferEntryType oldest_core_state_return;
  buffer.get_oldest_core_state(&oldest_core_state_return);
  ASSERT_EQ(oldest_core_state_return.timestamp_, 1);

  mars::BufferEntryType latest_init_state_return;
  buffer.get_latest_init_state(&latest_init_state_return);
  ASSERT_EQ(latest_init_state_return.timestamp_, 2);

  mars::BufferEntryType latest_state_return;
  buffer.get_latest_state(&latest_state_return);
  ASSERT_EQ(latest_state_return.timestamp_, 8);

  mars::BufferEntryType latest_sensor1_handle_state_return;
  buffer.get_latest_sensor_handle_state(pose_sensor_1_sptr, &latest_sensor1_handle_state_return);
  ASSERT_EQ(latest_sensor1_handle_state_return.timestamp_, 7);

  int latest_sensor1_handle_state_index;
  buffer.get_latest_sensor_handle_state(pose_sensor_1_sptr, &latest_sensor1_handle_state_return,
                                        &latest_sensor1_handle_state_index);
  ASSERT_EQ(latest_sensor1_handle_state_index, 7);

  mars::BufferEntryType latest_sensor2_handle_state_return;
  buffer.get_latest_sensor_handle_state(pose_sensor_2_sptr, &latest_sensor2_handle_state_return);
  ASSERT_EQ(latest_sensor2_handle_state_return.timestamp_, 8);

  int latest_sensor2_handle_state_index;
  buffer.get_latest_sensor_handle_state(pose_sensor_2_sptr, &latest_sensor2_handle_state_return,
                                        &latest_sensor2_handle_state_index);
  ASSERT_EQ(latest_sensor2_handle_state_index, 8);

  mars::BufferEntryType latest_sensor1_handle_measurement_return;
  buffer.get_latest_sensor_handle_measurement(pose_sensor_1_sptr, &latest_sensor1_handle_measurement_return);
  ASSERT_EQ(latest_sensor1_handle_measurement_return.timestamp_, 9);

  mars::BufferEntryType latest_sensor2_handle_measurement_return;
  buffer.get_latest_sensor_handle_measurement(pose_sensor_2_sptr, &latest_sensor2_handle_measurement_return);
  ASSERT_EQ(latest_sensor2_handle_measurement_return.timestamp_, 10);
}

TEST_F(mars_buffer_test, GET_CLOSEST_STATE)
{
  mars::Buffer buffer;

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  // Fill buffer with measurements only
  buffer.AddEntrySorted(mars::BufferEntryType(0, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(1, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(2, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(3, data_no_state, pose_sensor_1_sptr));

  // Buffer is not empty but has no state, needs to return false
  mars::BufferEntryType no_state_entry;
  int no_state_return = buffer.get_closest_state(2, &no_state_entry);
  ASSERT_EQ(no_state_return, 0);

  // With index
  int closest_state_idx_no_state;
  buffer.get_closest_state(2, &no_state_entry, &closest_state_idx_no_state);
  ASSERT_EQ(closest_state_idx_no_state, -1);

  buffer.AddEntrySorted(mars::BufferEntryType(4, data_with_state, pose_sensor_1_sptr, mars::BufferMetadataType::init));
  buffer.AddEntrySorted(mars::BufferEntryType(5, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(6, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(7, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(8, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(9, data_with_state, pose_sensor_1_sptr));

  // Equal timestamp
  mars::Time search_timestamp(6);
  mars::BufferEntryType equal_timestamp_return;
  buffer.get_closest_state(search_timestamp, &equal_timestamp_return);
  ASSERT_EQ(equal_timestamp_return.timestamp_, search_timestamp);

  // Equal time distance - needs to return the newer state
  mars::Time search_timestamp_2(8);
  mars::BufferEntryType equal_timestamp_distance_return;
  buffer.get_closest_state(search_timestamp_2, &equal_timestamp_distance_return);
  ASSERT_EQ(equal_timestamp_distance_return.timestamp_, 9);

  // Timestamp closer to older state
  mars::Time search_timestamp_3(6.1);
  mars::BufferEntryType close_to_older_state_return;
  buffer.get_closest_state(search_timestamp_3, &close_to_older_state_return);
  ASSERT_EQ(close_to_older_state_return.timestamp_, 6);

  // Timestamp closer to newer state
  mars::Time search_timestamp_4(6.9);
  mars::BufferEntryType close_to_newer_state_return;
  buffer.get_closest_state(search_timestamp_4, &close_to_newer_state_return);
  ASSERT_EQ(close_to_newer_state_return.timestamp_, 7);

  // Timestamp newer than latest state
  mars::Time search_timestamp_5(10);
  mars::BufferEntryType newer_than_newer_state_return;
  buffer.get_closest_state(search_timestamp_5, &newer_than_newer_state_return);
  ASSERT_EQ(newer_than_newer_state_return.timestamp_, 9);

  // Test if correct entry index is returned
  mars::Time index_timestamp(6);
  int closest_state_idx;
  mars::BufferEntryType dummy_return;
  buffer.get_closest_state(index_timestamp, &dummy_return, &closest_state_idx);
  ASSERT_EQ(closest_state_idx, 6);
}

TEST_F(mars_buffer_test, GET_CLOSEST_STATE_ONLY_ONE_STATE_IN_BUFFER)
{
  mars::Buffer buffer;
  mars::Time timestamp(0.0);
  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  // setup propagation sensor
  std::shared_ptr<mars::ImuSensorClass> imu_sensor_sptr = std::make_shared<mars::ImuSensorClass>("IMU");

  // Only measurement
  mars::BufferEntryType meas_entry(timestamp, data_no_state, imu_sensor_sptr);
  buffer.AddEntrySorted(meas_entry);

  // Measurement and state
  mars::BufferEntryType state_entry(timestamp, data_with_state, imu_sensor_sptr);
  buffer.AddEntrySorted(state_entry);

  mars::BufferEntryType result;
  bool status = buffer.get_closest_state(timestamp, &result);

  EXPECT_TRUE(status);
}

TEST_F(mars_buffer_test, GET_ENTRY_AT_INDEX)
{
  const int max_buffer_size = 10;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  buffer.AddEntrySorted(mars::BufferEntryType(0, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(1, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(2, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(3, data_no_state, pose_sensor_1_sptr));

  // test return entry value
  mars::BufferEntryType entry_return;

  for (int k = 0; k < 4; k++)
  {
    buffer.get_entry_at_idx(k, &entry_return);
    ASSERT_EQ(entry_return.timestamp_, k);
  }

  // Test return success status
  // In valid range
  ASSERT_EQ(buffer.get_entry_at_idx(0, &entry_return), 1);
  ASSERT_EQ(buffer.get_entry_at_idx(3, &entry_return), 1);
  // Outside valid range
  ASSERT_EQ(buffer.get_entry_at_idx(-1, &entry_return), 0);
  ASSERT_EQ(buffer.get_entry_at_idx(4, &entry_return), 0);
}

TEST_F(mars_buffer_test, ADD_SORTED)
{
  const int max_buffer_size = 50;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  buffer.AddEntrySorted(mars::BufferEntryType(1, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(0, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(3.2, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(4, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(2, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(6, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(5, data_with_state, pose_sensor_1_sptr));

  buffer.PrintBufferEntries();
  ASSERT_EQ(buffer.IsSorted(), 1);
  ASSERT_EQ(buffer.get_length(), 7);
}

TEST_F(mars_buffer_test, REMOVE_STATES_STARTING_AT_IDX)
{
  mars::Buffer buffer(100);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  buffer.AddEntrySorted(mars::BufferEntryType(0, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(1, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(2, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(3, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(4, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(5, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(6, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(7, data_no_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(8, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(9, data_no_state, pose_sensor_1_sptr));

  buffer.PrintBufferEntries();
  buffer.ClearStatesStartingAtIdx(4);
  buffer.PrintBufferEntries();

  // Size needs to remain the same because only states are removed
  ASSERT_EQ(buffer.get_length(), 10);

  for (int i = 4; i < buffer.get_length(); i++)
  {
    mars::BufferEntryType entry_return;
    buffer.get_entry_at_idx(i, &entry_return);

    ASSERT_EQ(entry_return.HasStates(), false);
  }

  // Check that auto entries are removed entirely
  buffer.AddEntrySorted(
      mars::BufferEntryType(10, data_no_state, pose_sensor_1_sptr, mars::BufferMetadataType::auto_add));

  buffer.PrintBufferEntries();
  mars::BufferEntryType entry_return;
  buffer.get_latest_entry(&entry_return);

  // Sanity Check that auto added state exists
  EXPECT_TRUE(entry_return.timestamp_ == 10);
  ASSERT_EQ(buffer.get_length(), 11);

  // Clear buffer states and auto entries again
  buffer.ClearStatesStartingAtIdx(4);

  // Expect that the auto added entry is removed
  buffer.get_latest_entry(&entry_return);
  EXPECT_TRUE(entry_return.timestamp_ == 9);
  ASSERT_EQ(buffer.get_length(), 10);

  buffer.PrintBufferEntries();
}

TEST_F(mars_buffer_test, MULTI_SENSOR_TYPE_SETUP)
{
  //   const int max_buffer_size = 100;
  //   mars::Buffer buffer(max_buffer_size);

  //   std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  //   std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //       std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  //   std::shared_ptr<mars::PositionSensorClass> position_sensor_1_sptr =
  //       std::make_shared<mars::PositionSensorClass>("Position_1", core_states_sptr);

  //   mars::BufferDataType data_with_state(this->data_with_state_);
  //   mars::BufferDataType data_no_state(this->data_no_state_);

  //   buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  //   buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::init_state));
  //   buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  //   buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_1_sptr,
  //   mars::BufferMetadataType::sensor_state));

  //   buffer.AddEntrySorted(mars::BufferEntryType(3, data, position_sensor_1_sptr,
  //   mars::BufferMetadataType::measurement)); buffer.AddEntrySorted(mars::BufferEntryType(3, data,
  //   position_sensor_1_sptr, mars::BufferMetadataType::init_state)); buffer.AddEntrySorted(mars::BufferEntryType(4,
  //   data, position_sensor_1_sptr, mars::BufferMetadataType::measurement));
  //   buffer.AddEntrySorted(mars::BufferEntryType(4, data, position_sensor_1_sptr,
  //   mars::BufferMetadataType::sensor_state));

  //   buffer.AddEntrySorted(mars::BufferEntryType(5, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  //   buffer.AddEntrySorted(mars::BufferEntryType(5, data, pose_sensor_1_sptr,
  //   mars::BufferMetadataType::sensor_state));

  //   buffer.AddEntrySorted(mars::BufferEntryType(6, data, position_sensor_1_sptr,
  //   mars::BufferMetadataType::measurement)); buffer.AddEntrySorted(mars::BufferEntryType(6, data,
  //   position_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  //   buffer.AddEntrySorted(mars::BufferEntryType(7, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  //   buffer.AddEntrySorted(mars::BufferEntryType(7, data, pose_sensor_1_sptr,
  //   mars::BufferMetadataType::sensor_state));

  //   buffer.PrintBufferEntries();
  //   ASSERT_EQ(buffer.IsSorted(), 1);
  //   ASSERT_EQ(buffer.get_length(), 14);

  //   // If the new timestamp matches with an existing one, then the new entry is added after the existing one
  //   mars::BufferEntryType entry_return_0;
  //   buffer.get_entry_at_idx(0, &entry_return_0);
  //   ASSERT_EQ(entry_return_0.metadata_, mars::BufferMetadataType::measurement);

  //   mars::BufferEntryType entry_return_1;
  //   buffer.get_entry_at_idx(1, &entry_return_1);
  //   ASSERT_EQ(entry_return_1.metadata_, mars::BufferMetadataType::init_state);
}

TEST_F(mars_buffer_test, INSERT_DATA_IDX_TEST)
{
  mars::Buffer buffer;

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);

  buffer.AddEntrySorted(mars::BufferEntryType(4, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(5, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(6, data_with_state, pose_sensor_1_sptr));
  buffer.AddEntrySorted(mars::BufferEntryType(7, data_with_state, pose_sensor_1_sptr));

  // New entry is newer than newest existing entry and needs to be inserted at idx 4
  int idx_newer = buffer.InsertDataAtTimestamp(mars::BufferEntryType(8, data_with_state, pose_sensor_1_sptr));
  ASSERT_EQ(4, idx_newer);

  // add entry in the middle of the buffer
  int idx_mid = buffer.InsertDataAtTimestamp(mars::BufferEntryType(5.3, data_with_state, pose_sensor_1_sptr));
  ASSERT_EQ(2, idx_mid);

  idx_mid = buffer.InsertDataAtTimestamp(mars::BufferEntryType(5.6, data_with_state, pose_sensor_1_sptr));
  ASSERT_EQ(3, idx_mid);

  // New entry is older than oldest existing entry and needs to be inserted at idx 0
  int idx_older = buffer.InsertDataAtTimestamp(mars::BufferEntryType(1, data_with_state, pose_sensor_1_sptr));
  ASSERT_EQ(int(0), idx_older);
}

// TBD(CHB): Waiting for new test implementation
// TEST_F(mars_buffer_test, INSERT_DATA_IDX_MAX_BUFFER_TEST)
//{
//  const int buffer_size = 10;
//  mars::Buffer buffer(buffer_size);

//  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
//  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
//      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);

//  int core_dummy = 13;
//  int sensor_dummy = 15;
//  mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

//  // Fill the buffer until max_buffer_size and compare the returned index
//  for (int k = 0; k < buffer_size; k++)
//  {
//    int idx = buffer.AddEntrySorted(
//        mars::BufferEntryType(k, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
//    ASSERT_EQ(k, idx);
//  }

//  // Add further entrys at the beginning of the buffer to trigger the deletion of old states and ensure that the
//  correct
//  // index is returned
//  for (int k = 0; k < 5; k++)
//  {
//    int idx = buffer.AddEntrySorted(
//        mars::BufferEntryType(buffer_size, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
//    ASSERT_EQ(buffer_size - 1, idx);
//  }
//}

TEST_F(mars_buffer_test, CHECK_LAST_SENSOR_HANDLE)
{
  mars::Buffer buffer(20);
  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_3_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_3", core_states_sptr);

  mars::BufferDataType data_with_state(this->data_with_state_);
  mars::BufferDataType data_no_state(this->data_no_state_);
  mars::Time timestamp(1);

  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_no_state, pose_sensor_1_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_no_state, pose_sensor_1_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_no_state, pose_sensor_1_sptr));
  // Only non-state entries
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));

  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_no_state, pose_sensor_2_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_no_state, pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));

  // One Pose 1 State
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_1_sptr));
  EXPECT_TRUE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));

  // Two Pose 1 States and one Pose 2 state
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_1_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_TRUE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));

  // Three Pose 1 States, two Pose 2 state, one Pose 3 state
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_1_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_2_sptr));
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_3_sptr));

  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_TRUE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));

  // Three Pose 1 States, two Pose 2 state, two Pose 3 state
  timestamp = timestamp + mars::Time(1);
  buffer.AddEntrySorted(mars::BufferEntryType(timestamp, data_with_state, pose_sensor_3_sptr));

  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_1_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_2_sptr));
  EXPECT_FALSE(buffer.CheckForLastSensorHandleWithState(pose_sensor_3_sptr));
}

TEST_F(mars_buffer_test, REMOVE_SENSOR_FROM_BUFFER)
{
  // Generate sensor entries
  const int num_test_entrys = 100;
  const int max_buffer_size = 110;
  mars::Buffer buffer(max_buffer_size);

  std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
      std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  for (int k = num_test_entrys; k > 0; k--)
  {
    mars::BufferDataType data(std::make_shared<int>(13), std::make_shared<int>(15));

    if (k % 2 == 0 || k == 1 || k == 2)
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_1_sptr);
      buffer.AddEntrySorted(entry);
    }
    else
    {
      mars::BufferEntryType entry(mars::Time(k), data, pose_sensor_2_sptr);
      buffer.AddEntrySorted(entry);
    }
  }

  mars::BufferEntryType data_sink;

  // Check that both sensor instances are present
  EXPECT_TRUE(buffer.get_latest_sensor_handle_measurement(pose_sensor_1_sptr, &data_sink));
  EXPECT_TRUE(buffer.get_latest_sensor_handle_measurement(pose_sensor_2_sptr, &data_sink));

  // Remove sensor 1 from buffer
  buffer.RemoveSensorFromBuffer(pose_sensor_1_sptr);

  // Check that sensor 1 is removed and sensor 2 still exists
  EXPECT_FALSE(buffer.get_latest_sensor_handle_measurement(pose_sensor_1_sptr, &data_sink));
  EXPECT_TRUE(buffer.get_latest_sensor_handle_measurement(pose_sensor_2_sptr, &data_sink));
}

///
/// \brief Tests if getting all measurements from a single sensor from buffer works.
///
/// \author Martin Scheiber <martin.scheiber@ieee.org>
///
TEST_F(mars_buffer_test, GET_SENSOR_MEASUREMENTS)
{
  // const int max_buffer_size = 20;
  // mars::Buffer buffer(max_buffer_size);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);
  // std::shared_ptr<mars::PositionSensorClass> position_sensor_1_sptr =
  //     std::make_shared<mars::PositionSensorClass>("Position", core_states_sptr);
  // std::shared_ptr<mars::ImuSensorClass> imu_sensor_sptr = std::make_shared<mars::ImuSensorClass>("IMU");

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // mars::PoseMeasurementType meas_pose1(Eigen::Vector3d(1, 1, 1), Eigen::Quaterniond::Identity());
  // std::shared_ptr<mars::PoseMeasurementType> meas_pose1_ptr =
  // std::make_shared<mars::PoseMeasurementType>(meas_pose1); mars::BufferDataType data_pose1;
  // data_pose1.set_sensor_state(meas_pose1_ptr);
  // mars::PoseMeasurementType meas_pose2(Eigen::Vector3d(5, 4, 3), Eigen::Quaterniond::Identity());
  // std::shared_ptr<mars::PoseMeasurementType> meas_pose2_ptr =
  // std::make_shared<mars::PoseMeasurementType>(meas_pose2); mars::BufferDataType data_pose2;
  // data_pose2.set_sensor_state(meas_pose2_ptr);

  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::init_state));
  // buffer.AddEntrySorted(
  //     mars::BufferEntryType(5, data_pose1, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(6, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(7, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(
  //     mars::BufferEntryType(8, data_pose1, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(9, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // buffer.AddEntrySorted(mars::BufferEntryType(11, data, pose_sensor_2_sptr, mars::BufferMetadataType::init_state));
  // buffer.AddEntrySorted(
  //     mars::BufferEntryType(9, data_pose2, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(8, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(7, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(
  //     mars::BufferEntryType(5, data_pose2, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(
  //     mars::BufferEntryType(1, data_pose2, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));

  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, position_sensor_1_sptr,
  // mars::BufferMetadataType::init_state)); buffer.AddEntrySorted(mars::BufferEntryType(4, data,
  // position_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // // test return measurements size1
  // std::vector<const mars::BufferEntryType*> entries_return;
  // buffer.get_sensor_handle_measurements(pose_sensor_1_sptr, &entries_return);

  // ASSERT_EQ(entries_return.size(), 2);

  // // iterate over buffer and test elements1
  // int ts = 5;
  // for (const auto& it : entries_return)
  // {
  //   mars::PoseMeasurementType meas = *static_cast<mars::PoseMeasurementType*>(it->data_.sensor_state_.get());
  //   std::cout << "pose1_meas: " << meas.position_.transpose() << std::endl;
  //   // value
  //   ASSERT_EQ((meas.position_ - Eigen::Vector3d(1, 1, 1)).norm(), 0);
  //   // timestamp
  //   ASSERT_EQ(it->timestamp_, ts);
  //   ts += 3;
  //   // check if pointer is corresponding to correct one
  //   ASSERT_EQ(it->data_.sensor_state_, meas_pose1_ptr);
  // }

  // // test return measurements size2
  // buffer.get_sensor_handle_measurements(pose_sensor_2_sptr, &entries_return);

  // ASSERT_EQ(entries_return.size(), 3);

  // // iterate over buffer and test elements2
  // ts = 1;
  // for (const auto& it : entries_return)
  // {
  //   mars::PoseMeasurementType meas = *static_cast<mars::PoseMeasurementType*>(it->data_.sensor_state_.get());
  //   std::cout << "pose2_meas: " << meas.position_.transpose() << std::endl;
  //   ASSERT_EQ((meas.position_ - Eigen::Vector3d(5, 4, 3)).norm(), 0);
  //   // timestamp
  //   ASSERT_EQ(it->timestamp_, ts);
  //   ts += 4;
  //   // check if pointer is corresponding to correct one
  //   ASSERT_EQ(it->data_.sensor_state_, meas_pose2_ptr);
  // }

  // // change value one item, retreive values again, should still be the same
  // mars::PoseMeasurementType meas =
  //     *static_cast<mars::PoseMeasurementType*>(entries_return.at(1)->data_.sensor_state_.get());
  // meas.position_ = Eigen::Vector3d(1, 1, 1);
  // ASSERT_EQ((meas.position_ - Eigen::Vector3d(1, 1, 1)).norm(), 0);

  // // get all entries again and rerun above test case
  // buffer.get_sensor_handle_measurements(pose_sensor_2_sptr, &entries_return);
  // ASSERT_EQ(entries_return.size(), 3);
  // ts = 1;
  // for (const auto& it : entries_return)
  // {
  //   mars::PoseMeasurementType meas = *static_cast<mars::PoseMeasurementType*>(it->data_.sensor_state_.get());
  //   std::cout << "pose2_meas: " << meas.position_.transpose() << std::endl;
  //   ASSERT_EQ((meas.position_ - Eigen::Vector3d(5, 4, 3)).norm(), 0);
  //   ASSERT_EQ(it->timestamp_, ts);
  //   ts += 4;
  // }
  // // if this succeeds then the entries are unchangable in the buffer (as they should be)!

  // // test return measurements
  // buffer.get_sensor_handle_measurements(position_sensor_1_sptr, &entries_return);

  // ASSERT_EQ(entries_return.size(), 0);
  // ASSERT_TRUE(entries_return.empty());

  // // test return success status
  // ASSERT_EQ(buffer.get_sensor_handle_measurements(pose_sensor_1_sptr, &entries_return), 1);
  // ASSERT_EQ(buffer.get_sensor_handle_measurements(pose_sensor_2_sptr, &entries_return), 1);
  // ASSERT_EQ(buffer.get_sensor_handle_measurements(position_sensor_1_sptr, &entries_return), 0);
  // ASSERT_EQ(buffer.get_sensor_handle_measurements(imu_sensor_sptr, &entries_return), 0);
}

///
/// \brief TEST_F Always delete measurement and state together
///
/// Check that, during the removal of overflow entries, sensor measurement and states for the same
/// sensor_handle and the same time, are deleted together
///
TEST_F(mars_buffer_test, ADD_AUTOREMOVE_ENTRIES)
{
  // // Only delete measurement and state pairs

  // const int max_buffer_size = 5;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // // Measurement only, since we only allow to delete pairs, this results in no entry removal
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));

  // ASSERT_EQ(buffer.get_length(), 7);
  // mars::BufferEntryType oldest_entry_return;
  // buffer.get_oldest_state(&oldest_entry_return);
  // ASSERT_EQ(oldest_entry_return.timestamp_, 1);

  // buffer.PrintBufferEntries();

  // // Add state for the corresponding message
  // // Now the oldest sensor handle is not the last one anymore and is deleted
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // ASSERT_EQ(buffer.get_length(), 6);
  // buffer.get_oldest_state(&oldest_entry_return);
  // ASSERT_EQ(oldest_entry_return.timestamp_, 3);

  // buffer.PrintBufferEntries();
}

///
/// \brief TEST_F Buffer consistency, keep two (measurement, state) pairs of a given sensor during out of order addition
///
/// Possible cause is unintended removal during buffer overflow handling
///
/// This ensures that during the addition of an out of order measurement, all states of another sensor still allow the
/// propagtion after reworking of the buffer.
///
TEST_F(mars_buffer_test, ADD_AUTOREMOVE_ENTRIES_W_OOO)
{
  // const int max_buffer_size = 8;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(0, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(0, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));

  // buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));

  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // // This should trigger an overflow removal and out of order integration
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));

  // std::vector<mars::BufferEntryType> buffer_entry_data;

  // for (int k = 0; k < 9; k++)
  // {
  //   mars::BufferEntryType buffer_entry_data_tmp;
  //   buffer.get_entry_at_idx(k, &buffer_entry_data_tmp);
  //   buffer_entry_data.push_back(buffer_entry_data_tmp);
  // }

  // std::vector<bool> result_vec;
  // result_vec.push_back(buffer_entry_data[0].sensor_handle_ == pose_sensor_1_sptr);
  // result_vec.push_back(buffer_entry_data[0].metadata_ == mars::BufferMetadataType::measurement);
  // result_vec.push_back(buffer_entry_data[1].sensor_handle_ == pose_sensor_1_sptr);
  // result_vec.push_back(buffer_entry_data[1].metadata_ == mars::BufferMetadataType::sensor_state);

  // result_vec.push_back(buffer_entry_data[2].sensor_handle_ == pose_sensor_2_sptr);
  // result_vec.push_back(buffer_entry_data[2].metadata_ == mars::BufferMetadataType::measurement);
  // result_vec.push_back(buffer_entry_data[3].sensor_handle_ == pose_sensor_2_sptr);
  // result_vec.push_back(buffer_entry_data[3].metadata_ == mars::BufferMetadataType::sensor_state);

  // result_vec.push_back(buffer_entry_data[4].sensor_handle_ == pose_sensor_2_sptr);
  // result_vec.push_back(buffer_entry_data[4].metadata_ == mars::BufferMetadataType::measurement);
  // result_vec.push_back(buffer_entry_data[5].sensor_handle_ == pose_sensor_2_sptr);
  // result_vec.push_back(buffer_entry_data[5].metadata_ == mars::BufferMetadataType::sensor_state);

  // result_vec.push_back(buffer_entry_data[6].sensor_handle_ == pose_sensor_2_sptr);
  // result_vec.push_back(buffer_entry_data[6].metadata_ == mars::BufferMetadataType::measurement);

  // result_vec.push_back(buffer_entry_data[7].sensor_handle_ == pose_sensor_1_sptr);
  // result_vec.push_back(buffer_entry_data[7].metadata_ == mars::BufferMetadataType::measurement);
  // result_vec.push_back(buffer_entry_data[8].sensor_handle_ == pose_sensor_1_sptr);
  // result_vec.push_back(buffer_entry_data[8].metadata_ == mars::BufferMetadataType::sensor_state);

  // bool final_result = true;
  // for (auto k : result_vec)
  // {
  //   if (!k)
  //   {
  //     final_result = false;
  //     buffer.PrintBufferEntries();
  //     break;
  //   }
  // }

  // EXPECT_TRUE(final_result);
}

///
/// \brief Tests if buffer returning indices works for adding entries
///
/// \author Martin Scheiber <martin.scheiber@ieee.org>
///
TEST_F(mars_buffer_test, ADD_INDEX_TEST)
{
  // const int max_buffer_size = 5;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_3_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_3", core_states_sptr);

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(0, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(0, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_2_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(4, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));

  // int idx =
  //     buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_3_sptr,
  //     mars::BufferMetadataType::measurement));

  // ASSERT_EQ(buffer.get_length(), 7);

  // mars::BufferEntryType entry_01;
  // buffer.get_entry_at_idx(1, &entry_01);

  // ASSERT_EQ(entry_01.timestamp_, 0);
  // ASSERT_EQ(idx, 2);
}

///
/// \brief Tests if given more sensors than buffer size, the buffer will still keep at least one state per sensor
/// if it is the last.
///
/// This requires the buffer to 'grow' larger than its allowed size, which is a desired functionality
///
/// \author Martin Scheiber <martin.scheiber@ieee.org>
///
TEST_F(mars_buffer_test, SIZE_TEST)
{
  // const int max_buffer_size = 2;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_2_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_2", core_states_sptr);
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_3_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_3", core_states_sptr);

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(0, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_2_sptr, mars::BufferMetadataType::sensor_state));

  // buffer.AddEntrySorted(mars::BufferEntryType(2, data, pose_sensor_3_sptr, mars::BufferMetadataType::sensor_state));

  // ASSERT_EQ(buffer.get_length(), 3);
  // buffer.PrintBufferEntries();

  // // Check that last state entry is still pose sensor 2
  // mars::BufferEntryType last_state;
  // buffer.get_oldest_state(&last_state);

  // ASSERT_EQ(pose_sensor_1_sptr, last_state.sensor_handle_);
}

TEST_F(mars_buffer_test, BUFFER_INSERT_INTERMEDIATE_DATA)
{
  // const int max_buffer_size = 10;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::ImuSensorClass> imu_sensor_1_sptr = std::make_shared<mars::ImuSensorClass>("imu");

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(5, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));

  // mars::BufferEntryType meas(4, data, imu_sensor_1_sptr, mars::BufferMetadataType::measurement);
  // mars::BufferEntryType state(4, data, imu_sensor_1_sptr, mars::BufferMetadataType::core_state);

  // EXPECT_FALSE(buffer.InsertIntermediateData(state, state));
  // // Confirm no change in buffersize
  // EXPECT_TRUE(buffer.get_length() == 5);
  // EXPECT_FALSE(buffer.InsertIntermediateData(meas, meas));
  // // Confirm no change in buffersize
  // EXPECT_TRUE(buffer.get_length() == 5);

  // buffer.PrintBufferEntries();

  // EXPECT_TRUE(buffer.InsertIntermediateData(meas, state));
  // EXPECT_TRUE(buffer.get_length() == 7);

  // buffer.PrintBufferEntries();

  // mars::BufferEntryType entry_04;
  // buffer.get_entry_at_idx(4, &entry_04);
  // mars::BufferEntryType entry_05;
  // buffer.get_entry_at_idx(5, &entry_05);

  // EXPECT_TRUE(entry_04.sensor_handle_ == imu_sensor_1_sptr);
  // EXPECT_EQ(entry_04.timestamp_, 4);
  // EXPECT_TRUE(entry_04.metadata_ == mars::BufferMetadataType::measurement_auto);

  // EXPECT_TRUE(entry_05.sensor_handle_ == imu_sensor_1_sptr);
  // EXPECT_EQ(entry_05.timestamp_, 4);
  // EXPECT_TRUE(entry_05.metadata_ == mars::BufferMetadataType::core_state_auto);
}

TEST_F(mars_buffer_test, BUFFER_GET_INTERMEDIATE_ENTRY_PAIR)
{
  // const int max_buffer_size = 10;
  // mars::Buffer buffer(max_buffer_size);
  // buffer.set_keep_last_sensor_handle(true);

  // std::shared_ptr<mars::CoreState> core_states_sptr = std::make_shared<mars::CoreState>();
  // std::shared_ptr<mars::PoseSensorClass> pose_sensor_1_sptr =
  //     std::make_shared<mars::PoseSensorClass>("Pose_1", core_states_sptr);
  // std::shared_ptr<mars::ImuSensorClass> imu_sensor_1_sptr = std::make_shared<mars::ImuSensorClass>("imu");

  // int core_dummy = 13;
  // int sensor_dummy = 15;
  // mars::BufferDataType data(std::make_shared<int>(core_dummy), std::make_shared<int>(sensor_dummy));

  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(1, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, imu_sensor_1_sptr, mars::BufferMetadataType::measurement));
  // buffer.AddEntrySorted(mars::BufferEntryType(3, data, imu_sensor_1_sptr, mars::BufferMetadataType::sensor_state));
  // buffer.AddEntrySorted(mars::BufferEntryType(5, data, pose_sensor_1_sptr, mars::BufferMetadataType::measurement));

  // mars::BufferEntryType state_tmp;
  // EXPECT_FALSE(buffer.get_intermediate_entry_pair(pose_sensor_1_sptr, &state_tmp, &state_tmp));

  // mars::BufferEntryType meas(5, data, imu_sensor_1_sptr, mars::BufferMetadataType::measurement);
  // mars::BufferEntryType state(5, data, imu_sensor_1_sptr, mars::BufferMetadataType::core_state);

  // buffer.InsertIntermediateData(meas, state);

  // // Pretend the Sensor update was finished and a state was added
  // buffer.AddEntrySorted(mars::BufferEntryType(5, data, pose_sensor_1_sptr, mars::BufferMetadataType::sensor_state));

  // mars::BufferEntryType pose_state_at_5;
  // mars::BufferEntryType imu_state_at_5;

  // EXPECT_TRUE(buffer.get_intermediate_entry_pair(pose_sensor_1_sptr, &imu_state_at_5, &pose_state_at_5));

  // EXPECT_TRUE(pose_state_at_5.sensor_handle_ == pose_sensor_1_sptr);
  // EXPECT_TRUE(imu_state_at_5.sensor_handle_ == imu_sensor_1_sptr);

  // EXPECT_TRUE(pose_state_at_5.IsState());
  // EXPECT_TRUE(imu_state_at_5.IsState());

  // EXPECT_EQ(pose_state_at_5.timestamp_, 5);
  // EXPECT_EQ(imu_state_at_5.timestamp_, 5);
}

TEST_F(mars_buffer_test, INSERT_DATA_AT_IDX)
{
  // TODO
}
