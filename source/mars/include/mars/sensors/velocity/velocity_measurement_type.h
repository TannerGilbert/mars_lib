// Copyright (C) 2024 Christian Brommer, Control of Networked Systems, University of Klagenfurt,
// Austria.
//
// All rights reserved.
//
// This software is licensed under the terms of the BSD-2-Clause-License with
// no commercial use allowed, the full terms of which are made available
// in the LICENSE file. No license in patents is granted.
//
// You can contact the author at <christian.brommer@ieee.org>

#ifndef VELOCITYMEASUREMENTTYPE_H
#define VELOCITYMEASUREMENTTYPE_H

#include <mars/sensors/measurement_base_class.h>
#include <Eigen/Dense>
#include <utility>

namespace mars
{
class VelocityMeasurementType : public BaseMeas
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Eigen::Vector3d velocity_;  ///< Velocity [x y z]

  VelocityMeasurementType() = default;

  VelocityMeasurementType(Eigen::Vector3d velocity) : velocity_(std::move(velocity))
  {
  }

  static std::string get_csv_state_header_string()
  {
    std::stringstream os;
    os << "t, ";
    os << "v_x, v_y, v_z";

    return os.str();
  }

  std::string to_csv_string(const double& timestamp) const
  {
    std::stringstream os;
    os.precision(17);
    os << timestamp;

    os << ", " << velocity_.x() << ", " << velocity_.y() << ", " << velocity_.z();

    return os.str();
  }
};
}  // namespace mars
#endif  // VELOCITYMEASUREMENTTYPE_H
