/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/*
 * warm_start_ipopt_interface.cc
 */

#include "modules/planning/planner/open_space/warm_start_ipopt_interface.h"

#include <math.h>
#include <utility>

#include "modules/common/log.h"
#include "modules/common/math/math_utils.h"
#include "modules/common/util/util.h"
#include "modules/planning/common/planning_gflags.h"

namespace apollo {
namespace planning {

constexpr std::size_t N = 80;

WarmStartIPOPTInterface::WarmStartIPOPTInterface(
    int num_of_variables, int num_of_constraints, std::size_t horizon, float ts,
    float wheelbase_length, Eigen::MatrixXd x0, Eigen::MatrixXd xf,
    Eigen::MatrixXd XYbounds)
    : num_of_variables_(num_of_variables),
      num_of_constraints_(num_of_constraints),
      horizon_(horizon),
      ts_(ts),
      wheelbase_length_(wheelbase_length),
      x0_(x0),
      xf_(xf),
      XYbounds_(XYbounds) {}

void WarmStartIPOPTInterface::get_optimization_results() const {}

bool WarmStartIPOPTInterface::get_nlp_info(int& n, int& m, int& nnz_jac_g,
                                           int& nnz_h_lag,
                                           IndexStyleEnum& index_style) {
  // number of variables
  n = num_of_variables_;

  // number of constraints

  m = num_of_constraints_;

  // number of nonzero hessian and lagrangian.

  // TODO(QiL) : Update nnz_jac_g;
  nnz_jac_g = 0;

  // TOdo(QiL) : Update nnz_h_lag;
  nnz_h_lag = 0;

  index_style = IndexStyleEnum::C_STYLE;
  return true;
}

bool WarmStartIPOPTInterface::get_bounds_info(int n, double* x_l, double* x_u,
                                              int m, double* g_l, double* g_u) {
  // here, the n and m we gave IPOPT in get_nlp_info are passed back to us.
  // If desired, we could assert to make sure they are what we think they are.
  CHECK(n == num_of_variables_) << "num_of_variables_ mismatch, n: " << n
                                << ", num_of_variables_: " << num_of_variables_;
  CHECK(m == num_of_constraints_)
      << "num_of_constraints_ mismatch, n: " << n
      << ", num_of_constraints_: " << num_of_constraints_;

  // Variables: includes u and sample time

  // 1. state variables
  // start point pose
  std::size_t variable_index = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    x_l[i] = x0_(i, 0);
    x_u[i] = x0_(i, 0);
  }
  variable_index += 4;

  // During horizons
  for (std::size_t i = 1; i < horizon_ - 1; ++i) {
    // x
    x_l[variable_index] = XYbounds_(0, 0);
    x_u[variable_index] = XYbounds_(1, 0);

    // y
    x_l[variable_index + 1] = XYbounds_(2, 0);
    x_u[variable_index + 1] = XYbounds_(3, 0);

    // phi
    // TODO(QiL): Change this to configs
    x_l[variable_index + 2] = -7;
    x_u[variable_index + 2] = 7;

    // v
    // TODO(QiL) : Change this to configs
    x_l[variable_index + 3] = -1;
    x_u[variable_index + 3] = 2;

    variable_index += 4;
  }

  // end point pose
  for (std::size_t i = 0; i < 4; ++i) {
    x_l[variable_index + i] = xf_(i, 0);
    x_u[variable_index + i] = xf_(i, 0);
  }
  variable_index += 4;
  ADEBUG << "variable_index after adding state constraints : "
         << variable_index;

  // 2. input constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    // u1
    x_l[variable_index] = -0.6;
    x_u[variable_index] = 0.6;

    // u2
    x_l[variable_index + 1] = -1;
    x_u[variable_index + 1] = 1;

    variable_index += 2;
  }
  ADEBUG << "variable_index after adding input constraints : "
         << variable_index;

  // 3. sampling time constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    x_l[variable_index] = -0.6;
    x_u[variable_index] = 0.6;

    ++variable_index;
  }

  ADEBUG << "variable_index : " << variable_index;

  // Constraints

  // 1. state constraints
  // start point pose
  std::size_t constraint_index = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    g_l[i] = x0_(i, 0);
    g_u[i] = x0_(i, 0);
  }
  constraint_index += 4;

  // During horizons
  for (std::size_t i = 1; i < horizon_ - 1; ++i) {
    // x
    g_l[constraint_index] = XYbounds_(0, 0);
    g_u[constraint_index] = XYbounds_(1, 0);

    // y
    g_l[constraint_index + 1] = XYbounds_(2, 0);
    g_u[constraint_index + 1] = XYbounds_(3, 0);

    // phi
    // TODO(QiL): Change this to configs
    g_l[constraint_index + 2] = -7;
    g_u[constraint_index + 2] = 7;

    // v
    // TODO(QiL) : Change this to configs
    g_l[constraint_index + 3] = -1;
    g_u[constraint_index + 3] = 2;

    constraint_index += 4;
  }

  // end point pose
  for (std::size_t i = 0; i < 4; ++i) {
    g_l[constraint_index + i] = xf_(i, 0);
    g_u[constraint_index + i] = xf_(i, 0);
  }
  constraint_index += 4;
  ADEBUG << "constraint_index after adding state constraints : "
         << constraint_index;

  // 2. input constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    // u1
    g_l[constraint_index] = -0.6;
    g_u[constraint_index] = 0.6;

    // u2
    g_l[constraint_index + 1] = -1;
    g_u[constraint_index + 1] = 1;

    constraint_index += 2;
  }
  ADEBUG << "constraint_index after adding input constraints : "
         << constraint_index;

  // 3. sampling time constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    g_l[constraint_index] = -0.6;
    g_u[constraint_index] = 0.6;

    ++constraint_index;
  }
  ADEBUG << "constraint_index after adding sampling time constraints : "
         << constraint_index;

  return true;
}

bool WarmStartIPOPTInterface::eval_g(int n, const double* x, bool new_x, int m,
                                     double* g) {
  return true;
}

bool WarmStartIPOPTInterface::eval_jac_g(int n, const double* x, bool new_x,
                                         int m, int nele_jac, int* iRow,
                                         int* jCol, double* values) {
  return true;
}

bool WarmStartIPOPTInterface::eval_h(int n, const double* x, bool new_x,
                                     double obj_factor, int m,
                                     const double* lambda, bool new_lambda,
                                     int nele_hess, int* iRow, int* jCol,
                                     double* values) {
  return true;
}

bool WarmStartIPOPTInterface::get_starting_point(int n, bool init_x, double* x,
                                                 bool init_z, double* z_L,
                                                 double* z_U, int m,
                                                 bool init_lambda,
                                                 double* lambda) {
  CHECK(n == num_of_variables_) << "No. of variables wrong. n : " << n;
  CHECK(init_x == true) << "Warm start init_x setting failed";
  CHECK(init_z == false) << "Warm start init_z setting failed";
  CHECK(init_lambda == false) << "Warm start init_lambda setting failed";

  // 1. state variables linspace initialization

  std::vector<std::vector<double>> x_guess(4, std::vector<double>(horizon_));

  for (std::size_t i = 0; i < 4; ++i) {
    ::apollo::common::util::uniform_slice(x0_(i, 0), xf_(i, 0), horizon_,
                                          &x_guess[i]);
  }

  for (std::size_t i = 0; i <= horizon_; ++i) {
    for (std::size_t j = 0; j < 4; ++j) {
      x[i * 4 + j] = x_guess[j][i];
    }
  }

  // 2. input initialization
  for (std::size_t i = 0; i <= 2 * horizon_; ++i) {
    x[i] = 0.6;
  }

  // 3. sampling time constraints
  for (std::size_t i = 0; i <= horizon_; ++i) {
    x[i] = 0.5;
  }

  return true;
}

bool WarmStartIPOPTInterface::eval_f(int n, const double* x, bool new_x,
                                     double& obj_value) {
  // first (horizon_ + 1) * 4 is state, then next horizon_ * 2 is control input,
  // then last horizon_ + 1 is sampling time
  std::size_t start_index = (horizon_ + 1) * 4;
  std::size_t time_start_index = start_index + horizon_ * 2;
  for (std::size_t i = 0; i < horizon_; ++i) {
    obj_value += 0.1 * x[start_index + i] * x[start_index + i] +
                 x[start_index + i] * x[start_index + i + 1] +
                 0.5 * x[time_start_index + i] +
                 x[time_start_index + i] * x[time_start_index + i];
  }
  obj_value += 0.5 * x[time_start_index + horizon_] +
               x[time_start_index + horizon_] * x[time_start_index + horizon_];
  return true;
}

}  // namespace planning
}  // namespace apollo