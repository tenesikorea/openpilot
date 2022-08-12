#include "car.h"

namespace {
#define DIM 9
#define EDIM 9
#define MEDIM 9
typedef void (*Hfun)(double *, double *, double *);

double mass;

void set_mass(double x){ mass = x;}

double rotational_inertia;

void set_rotational_inertia(double x){ rotational_inertia = x;}

double center_to_front;

void set_center_to_front(double x){ center_to_front = x;}

double center_to_rear;

void set_center_to_rear(double x){ center_to_rear = x;}

double stiffness_front;

void set_stiffness_front(double x){ stiffness_front = x;}

double stiffness_rear;

void set_stiffness_rear(double x){ stiffness_rear = x;}
const static double MAHA_THRESH_25 = 3.8414588206941227;
const static double MAHA_THRESH_24 = 5.991464547107981;
const static double MAHA_THRESH_30 = 3.8414588206941227;
const static double MAHA_THRESH_26 = 3.8414588206941227;
const static double MAHA_THRESH_27 = 3.8414588206941227;
const static double MAHA_THRESH_29 = 3.8414588206941227;
const static double MAHA_THRESH_28 = 3.8414588206941227;
const static double MAHA_THRESH_31 = 3.8414588206941227;

/******************************************************************************
 *                       Code generated with sympy 1.9                        *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_3614741465952876957) {
   out_3614741465952876957[0] = delta_x[0] + nom_x[0];
   out_3614741465952876957[1] = delta_x[1] + nom_x[1];
   out_3614741465952876957[2] = delta_x[2] + nom_x[2];
   out_3614741465952876957[3] = delta_x[3] + nom_x[3];
   out_3614741465952876957[4] = delta_x[4] + nom_x[4];
   out_3614741465952876957[5] = delta_x[5] + nom_x[5];
   out_3614741465952876957[6] = delta_x[6] + nom_x[6];
   out_3614741465952876957[7] = delta_x[7] + nom_x[7];
   out_3614741465952876957[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_7473851160931657756) {
   out_7473851160931657756[0] = -nom_x[0] + true_x[0];
   out_7473851160931657756[1] = -nom_x[1] + true_x[1];
   out_7473851160931657756[2] = -nom_x[2] + true_x[2];
   out_7473851160931657756[3] = -nom_x[3] + true_x[3];
   out_7473851160931657756[4] = -nom_x[4] + true_x[4];
   out_7473851160931657756[5] = -nom_x[5] + true_x[5];
   out_7473851160931657756[6] = -nom_x[6] + true_x[6];
   out_7473851160931657756[7] = -nom_x[7] + true_x[7];
   out_7473851160931657756[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_2741669290193596834) {
   out_2741669290193596834[0] = 1.0;
   out_2741669290193596834[1] = 0;
   out_2741669290193596834[2] = 0;
   out_2741669290193596834[3] = 0;
   out_2741669290193596834[4] = 0;
   out_2741669290193596834[5] = 0;
   out_2741669290193596834[6] = 0;
   out_2741669290193596834[7] = 0;
   out_2741669290193596834[8] = 0;
   out_2741669290193596834[9] = 0;
   out_2741669290193596834[10] = 1.0;
   out_2741669290193596834[11] = 0;
   out_2741669290193596834[12] = 0;
   out_2741669290193596834[13] = 0;
   out_2741669290193596834[14] = 0;
   out_2741669290193596834[15] = 0;
   out_2741669290193596834[16] = 0;
   out_2741669290193596834[17] = 0;
   out_2741669290193596834[18] = 0;
   out_2741669290193596834[19] = 0;
   out_2741669290193596834[20] = 1.0;
   out_2741669290193596834[21] = 0;
   out_2741669290193596834[22] = 0;
   out_2741669290193596834[23] = 0;
   out_2741669290193596834[24] = 0;
   out_2741669290193596834[25] = 0;
   out_2741669290193596834[26] = 0;
   out_2741669290193596834[27] = 0;
   out_2741669290193596834[28] = 0;
   out_2741669290193596834[29] = 0;
   out_2741669290193596834[30] = 1.0;
   out_2741669290193596834[31] = 0;
   out_2741669290193596834[32] = 0;
   out_2741669290193596834[33] = 0;
   out_2741669290193596834[34] = 0;
   out_2741669290193596834[35] = 0;
   out_2741669290193596834[36] = 0;
   out_2741669290193596834[37] = 0;
   out_2741669290193596834[38] = 0;
   out_2741669290193596834[39] = 0;
   out_2741669290193596834[40] = 1.0;
   out_2741669290193596834[41] = 0;
   out_2741669290193596834[42] = 0;
   out_2741669290193596834[43] = 0;
   out_2741669290193596834[44] = 0;
   out_2741669290193596834[45] = 0;
   out_2741669290193596834[46] = 0;
   out_2741669290193596834[47] = 0;
   out_2741669290193596834[48] = 0;
   out_2741669290193596834[49] = 0;
   out_2741669290193596834[50] = 1.0;
   out_2741669290193596834[51] = 0;
   out_2741669290193596834[52] = 0;
   out_2741669290193596834[53] = 0;
   out_2741669290193596834[54] = 0;
   out_2741669290193596834[55] = 0;
   out_2741669290193596834[56] = 0;
   out_2741669290193596834[57] = 0;
   out_2741669290193596834[58] = 0;
   out_2741669290193596834[59] = 0;
   out_2741669290193596834[60] = 1.0;
   out_2741669290193596834[61] = 0;
   out_2741669290193596834[62] = 0;
   out_2741669290193596834[63] = 0;
   out_2741669290193596834[64] = 0;
   out_2741669290193596834[65] = 0;
   out_2741669290193596834[66] = 0;
   out_2741669290193596834[67] = 0;
   out_2741669290193596834[68] = 0;
   out_2741669290193596834[69] = 0;
   out_2741669290193596834[70] = 1.0;
   out_2741669290193596834[71] = 0;
   out_2741669290193596834[72] = 0;
   out_2741669290193596834[73] = 0;
   out_2741669290193596834[74] = 0;
   out_2741669290193596834[75] = 0;
   out_2741669290193596834[76] = 0;
   out_2741669290193596834[77] = 0;
   out_2741669290193596834[78] = 0;
   out_2741669290193596834[79] = 0;
   out_2741669290193596834[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_8576855921907495355) {
   out_8576855921907495355[0] = state[0];
   out_8576855921907495355[1] = state[1];
   out_8576855921907495355[2] = state[2];
   out_8576855921907495355[3] = state[3];
   out_8576855921907495355[4] = state[4];
   out_8576855921907495355[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8000000000000007*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_8576855921907495355[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_8576855921907495355[7] = state[7];
   out_8576855921907495355[8] = state[8];
}
void F_fun(double *state, double dt, double *out_1643883066210526332) {
   out_1643883066210526332[0] = 1;
   out_1643883066210526332[1] = 0;
   out_1643883066210526332[2] = 0;
   out_1643883066210526332[3] = 0;
   out_1643883066210526332[4] = 0;
   out_1643883066210526332[5] = 0;
   out_1643883066210526332[6] = 0;
   out_1643883066210526332[7] = 0;
   out_1643883066210526332[8] = 0;
   out_1643883066210526332[9] = 0;
   out_1643883066210526332[10] = 1;
   out_1643883066210526332[11] = 0;
   out_1643883066210526332[12] = 0;
   out_1643883066210526332[13] = 0;
   out_1643883066210526332[14] = 0;
   out_1643883066210526332[15] = 0;
   out_1643883066210526332[16] = 0;
   out_1643883066210526332[17] = 0;
   out_1643883066210526332[18] = 0;
   out_1643883066210526332[19] = 0;
   out_1643883066210526332[20] = 1;
   out_1643883066210526332[21] = 0;
   out_1643883066210526332[22] = 0;
   out_1643883066210526332[23] = 0;
   out_1643883066210526332[24] = 0;
   out_1643883066210526332[25] = 0;
   out_1643883066210526332[26] = 0;
   out_1643883066210526332[27] = 0;
   out_1643883066210526332[28] = 0;
   out_1643883066210526332[29] = 0;
   out_1643883066210526332[30] = 1;
   out_1643883066210526332[31] = 0;
   out_1643883066210526332[32] = 0;
   out_1643883066210526332[33] = 0;
   out_1643883066210526332[34] = 0;
   out_1643883066210526332[35] = 0;
   out_1643883066210526332[36] = 0;
   out_1643883066210526332[37] = 0;
   out_1643883066210526332[38] = 0;
   out_1643883066210526332[39] = 0;
   out_1643883066210526332[40] = 1;
   out_1643883066210526332[41] = 0;
   out_1643883066210526332[42] = 0;
   out_1643883066210526332[43] = 0;
   out_1643883066210526332[44] = 0;
   out_1643883066210526332[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_1643883066210526332[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_1643883066210526332[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_1643883066210526332[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_1643883066210526332[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_1643883066210526332[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_1643883066210526332[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_1643883066210526332[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_1643883066210526332[53] = -9.8000000000000007*dt;
   out_1643883066210526332[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_1643883066210526332[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_1643883066210526332[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_1643883066210526332[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_1643883066210526332[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_1643883066210526332[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_1643883066210526332[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_1643883066210526332[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_1643883066210526332[62] = 0;
   out_1643883066210526332[63] = 0;
   out_1643883066210526332[64] = 0;
   out_1643883066210526332[65] = 0;
   out_1643883066210526332[66] = 0;
   out_1643883066210526332[67] = 0;
   out_1643883066210526332[68] = 0;
   out_1643883066210526332[69] = 0;
   out_1643883066210526332[70] = 1;
   out_1643883066210526332[71] = 0;
   out_1643883066210526332[72] = 0;
   out_1643883066210526332[73] = 0;
   out_1643883066210526332[74] = 0;
   out_1643883066210526332[75] = 0;
   out_1643883066210526332[76] = 0;
   out_1643883066210526332[77] = 0;
   out_1643883066210526332[78] = 0;
   out_1643883066210526332[79] = 0;
   out_1643883066210526332[80] = 1;
}
void h_25(double *state, double *unused, double *out_6621228261081700792) {
   out_6621228261081700792[0] = state[6];
}
void H_25(double *state, double *unused, double *out_127652376429038503) {
   out_127652376429038503[0] = 0;
   out_127652376429038503[1] = 0;
   out_127652376429038503[2] = 0;
   out_127652376429038503[3] = 0;
   out_127652376429038503[4] = 0;
   out_127652376429038503[5] = 0;
   out_127652376429038503[6] = 1;
   out_127652376429038503[7] = 0;
   out_127652376429038503[8] = 0;
}
void h_24(double *state, double *unused, double *out_5957372959990195350) {
   out_5957372959990195350[0] = state[4];
   out_5957372959990195350[1] = state[5];
}
void H_24(double *state, double *unused, double *out_9091026511211317888) {
   out_9091026511211317888[0] = 0;
   out_9091026511211317888[1] = 0;
   out_9091026511211317888[2] = 0;
   out_9091026511211317888[3] = 0;
   out_9091026511211317888[4] = 1;
   out_9091026511211317888[5] = 0;
   out_9091026511211317888[6] = 0;
   out_9091026511211317888[7] = 0;
   out_9091026511211317888[8] = 0;
   out_9091026511211317888[9] = 0;
   out_9091026511211317888[10] = 0;
   out_9091026511211317888[11] = 0;
   out_9091026511211317888[12] = 0;
   out_9091026511211317888[13] = 0;
   out_9091026511211317888[14] = 1;
   out_9091026511211317888[15] = 0;
   out_9091026511211317888[16] = 0;
   out_9091026511211317888[17] = 0;
}
void h_30(double *state, double *unused, double *out_1142036995738971649) {
   out_1142036995738971649[0] = state[4];
}
void H_30(double *state, double *unused, double *out_1686570714201567) {
   out_1686570714201567[0] = 0;
   out_1686570714201567[1] = 0;
   out_1686570714201567[2] = 0;
   out_1686570714201567[3] = 0;
   out_1686570714201567[4] = 1;
   out_1686570714201567[5] = 0;
   out_1686570714201567[6] = 0;
   out_1686570714201567[7] = 0;
   out_1686570714201567[8] = 0;
}
void h_26(double *state, double *unused, double *out_1462368325069726002) {
   out_1462368325069726002[0] = state[7];
}
void H_26(double *state, double *unused, double *out_3613850942445017721) {
   out_3613850942445017721[0] = 0;
   out_3613850942445017721[1] = 0;
   out_3613850942445017721[2] = 0;
   out_3613850942445017721[3] = 0;
   out_3613850942445017721[4] = 0;
   out_3613850942445017721[5] = 0;
   out_3613850942445017721[6] = 0;
   out_3613850942445017721[7] = 1;
   out_3613850942445017721[8] = 0;
}
void h_27(double *state, double *unused, double *out_3584442209637368107) {
   out_3584442209637368107[0] = state[3];
}
void H_27(double *state, double *unused, double *out_2176449882514626478) {
   out_2176449882514626478[0] = 0;
   out_2176449882514626478[1] = 0;
   out_2176449882514626478[2] = 0;
   out_2176449882514626478[3] = 1;
   out_2176449882514626478[4] = 0;
   out_2176449882514626478[5] = 0;
   out_2176449882514626478[6] = 0;
   out_2176449882514626478[7] = 0;
   out_2176449882514626478[8] = 0;
}
void h_29(double *state, double *unused, double *out_3859636271921873996) {
   out_3859636271921873996[0] = state[1];
}
void H_29(double *state, double *unused, double *out_508544773600190617) {
   out_508544773600190617[0] = 0;
   out_508544773600190617[1] = 1;
   out_508544773600190617[2] = 0;
   out_508544773600190617[3] = 0;
   out_508544773600190617[4] = 0;
   out_508544773600190617[5] = 0;
   out_508544773600190617[6] = 0;
   out_508544773600190617[7] = 0;
   out_508544773600190617[8] = 0;
}
void h_28(double *state, double *unused, double *out_2640932356735705672) {
   out_2640932356735705672[0] = state[0];
}
void H_28(double *state, double *unused, double *out_8972211626453708085) {
   out_8972211626453708085[0] = 1;
   out_8972211626453708085[1] = 0;
   out_8972211626453708085[2] = 0;
   out_8972211626453708085[3] = 0;
   out_8972211626453708085[4] = 0;
   out_8972211626453708085[5] = 0;
   out_8972211626453708085[6] = 0;
   out_8972211626453708085[7] = 0;
   out_8972211626453708085[8] = 0;
}
void h_31(double *state, double *unused, double *out_7079429623240262454) {
   out_7079429623240262454[0] = state[8];
}
void H_31(double *state, double *unused, double *out_158298338305998931) {
   out_158298338305998931[0] = 0;
   out_158298338305998931[1] = 0;
   out_158298338305998931[2] = 0;
   out_158298338305998931[3] = 0;
   out_158298338305998931[4] = 0;
   out_158298338305998931[5] = 0;
   out_158298338305998931[6] = 0;
   out_158298338305998931[7] = 0;
   out_158298338305998931[8] = 1;
}
#include <eigen3/Eigen/Dense>
#include <iostream>

typedef Eigen::Matrix<double, DIM, DIM, Eigen::RowMajor> DDM;
typedef Eigen::Matrix<double, EDIM, EDIM, Eigen::RowMajor> EEM;
typedef Eigen::Matrix<double, DIM, EDIM, Eigen::RowMajor> DEM;

void predict(double *in_x, double *in_P, double *in_Q, double dt) {
  typedef Eigen::Matrix<double, MEDIM, MEDIM, Eigen::RowMajor> RRM;

  double nx[DIM] = {0};
  double in_F[EDIM*EDIM] = {0};

  // functions from sympy
  f_fun(in_x, dt, nx);
  F_fun(in_x, dt, in_F);


  EEM F(in_F);
  EEM P(in_P);
  EEM Q(in_Q);

  RRM F_main = F.topLeftCorner(MEDIM, MEDIM);
  P.topLeftCorner(MEDIM, MEDIM) = (F_main * P.topLeftCorner(MEDIM, MEDIM)) * F_main.transpose();
  P.topRightCorner(MEDIM, EDIM - MEDIM) = F_main * P.topRightCorner(MEDIM, EDIM - MEDIM);
  P.bottomLeftCorner(EDIM - MEDIM, MEDIM) = P.bottomLeftCorner(EDIM - MEDIM, MEDIM) * F_main.transpose();

  P = P + dt*Q;

  // copy out state
  memcpy(in_x, nx, DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
}

// note: extra_args dim only correct when null space projecting
// otherwise 1
template <int ZDIM, int EADIM, bool MAHA_TEST>
void update(double *in_x, double *in_P, Hfun h_fun, Hfun H_fun, Hfun Hea_fun, double *in_z, double *in_R, double *in_ea, double MAHA_THRESHOLD) {
  typedef Eigen::Matrix<double, ZDIM, ZDIM, Eigen::RowMajor> ZZM;
  typedef Eigen::Matrix<double, ZDIM, DIM, Eigen::RowMajor> ZDM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, EDIM, Eigen::RowMajor> XEM;
  //typedef Eigen::Matrix<double, EDIM, ZDIM, Eigen::RowMajor> EZM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, 1> X1M;
  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> XXM;

  double in_hx[ZDIM] = {0};
  double in_H[ZDIM * DIM] = {0};
  double in_H_mod[EDIM * DIM] = {0};
  double delta_x[EDIM] = {0};
  double x_new[DIM] = {0};


  // state x, P
  Eigen::Matrix<double, ZDIM, 1> z(in_z);
  EEM P(in_P);
  ZZM pre_R(in_R);

  // functions from sympy
  h_fun(in_x, in_ea, in_hx);
  H_fun(in_x, in_ea, in_H);
  ZDM pre_H(in_H);

  // get y (y = z - hx)
  Eigen::Matrix<double, ZDIM, 1> pre_y(in_hx); pre_y = z - pre_y;
  X1M y; XXM H; XXM R;
  if (Hea_fun){
    typedef Eigen::Matrix<double, ZDIM, EADIM, Eigen::RowMajor> ZAM;
    double in_Hea[ZDIM * EADIM] = {0};
    Hea_fun(in_x, in_ea, in_Hea);
    ZAM Hea(in_Hea);
    XXM A = Hea.transpose().fullPivLu().kernel();


    y = A.transpose() * pre_y;
    H = A.transpose() * pre_H;
    R = A.transpose() * pre_R * A;
  } else {
    y = pre_y;
    H = pre_H;
    R = pre_R;
  }
  // get modified H
  H_mod_fun(in_x, in_H_mod);
  DEM H_mod(in_H_mod);
  XEM H_err = H * H_mod;

  // Do mahalobis distance test
  if (MAHA_TEST){
    XXM a = (H_err * P * H_err.transpose() + R).inverse();
    double maha_dist = y.transpose() * a * y;
    if (maha_dist > MAHA_THRESHOLD){
      R = 1.0e16 * R;
    }
  }

  // Outlier resilient weighting
  double weight = 1;//(1.5)/(1 + y.squaredNorm()/R.sum());

  // kalman gains and I_KH
  XXM S = ((H_err * P) * H_err.transpose()) + R/weight;
  XEM KT = S.fullPivLu().solve(H_err * P.transpose());
  //EZM K = KT.transpose(); TODO: WHY DOES THIS NOT COMPILE?
  //EZM K = S.fullPivLu().solve(H_err * P.transpose()).transpose();
  //std::cout << "Here is the matrix rot:\n" << K << std::endl;
  EEM I_KH = Eigen::Matrix<double, EDIM, EDIM>::Identity() - (KT.transpose() * H_err);

  // update state by injecting dx
  Eigen::Matrix<double, EDIM, 1> dx(delta_x);
  dx  = (KT.transpose() * y);
  memcpy(delta_x, dx.data(), EDIM * sizeof(double));
  err_fun(in_x, delta_x, x_new);
  Eigen::Matrix<double, DIM, 1> x(x_new);

  // update cov
  P = ((I_KH * P) * I_KH.transpose()) + ((KT.transpose() * R) * KT);

  // copy out state
  memcpy(in_x, x.data(), DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
  memcpy(in_z, y.data(), y.rows() * sizeof(double));
}




}
extern "C" {

void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_25, H_25, NULL, in_z, in_R, in_ea, MAHA_THRESH_25);
}
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<2, 3, 0>(in_x, in_P, h_24, H_24, NULL, in_z, in_R, in_ea, MAHA_THRESH_24);
}
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_30, H_30, NULL, in_z, in_R, in_ea, MAHA_THRESH_30);
}
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_26, H_26, NULL, in_z, in_R, in_ea, MAHA_THRESH_26);
}
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_27, H_27, NULL, in_z, in_R, in_ea, MAHA_THRESH_27);
}
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_29, H_29, NULL, in_z, in_R, in_ea, MAHA_THRESH_29);
}
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_28, H_28, NULL, in_z, in_R, in_ea, MAHA_THRESH_28);
}
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_31, H_31, NULL, in_z, in_R, in_ea, MAHA_THRESH_31);
}
void car_err_fun(double *nom_x, double *delta_x, double *out_3614741465952876957) {
  err_fun(nom_x, delta_x, out_3614741465952876957);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_7473851160931657756) {
  inv_err_fun(nom_x, true_x, out_7473851160931657756);
}
void car_H_mod_fun(double *state, double *out_2741669290193596834) {
  H_mod_fun(state, out_2741669290193596834);
}
void car_f_fun(double *state, double dt, double *out_8576855921907495355) {
  f_fun(state,  dt, out_8576855921907495355);
}
void car_F_fun(double *state, double dt, double *out_1643883066210526332) {
  F_fun(state,  dt, out_1643883066210526332);
}
void car_h_25(double *state, double *unused, double *out_6621228261081700792) {
  h_25(state, unused, out_6621228261081700792);
}
void car_H_25(double *state, double *unused, double *out_127652376429038503) {
  H_25(state, unused, out_127652376429038503);
}
void car_h_24(double *state, double *unused, double *out_5957372959990195350) {
  h_24(state, unused, out_5957372959990195350);
}
void car_H_24(double *state, double *unused, double *out_9091026511211317888) {
  H_24(state, unused, out_9091026511211317888);
}
void car_h_30(double *state, double *unused, double *out_1142036995738971649) {
  h_30(state, unused, out_1142036995738971649);
}
void car_H_30(double *state, double *unused, double *out_1686570714201567) {
  H_30(state, unused, out_1686570714201567);
}
void car_h_26(double *state, double *unused, double *out_1462368325069726002) {
  h_26(state, unused, out_1462368325069726002);
}
void car_H_26(double *state, double *unused, double *out_3613850942445017721) {
  H_26(state, unused, out_3613850942445017721);
}
void car_h_27(double *state, double *unused, double *out_3584442209637368107) {
  h_27(state, unused, out_3584442209637368107);
}
void car_H_27(double *state, double *unused, double *out_2176449882514626478) {
  H_27(state, unused, out_2176449882514626478);
}
void car_h_29(double *state, double *unused, double *out_3859636271921873996) {
  h_29(state, unused, out_3859636271921873996);
}
void car_H_29(double *state, double *unused, double *out_508544773600190617) {
  H_29(state, unused, out_508544773600190617);
}
void car_h_28(double *state, double *unused, double *out_2640932356735705672) {
  h_28(state, unused, out_2640932356735705672);
}
void car_H_28(double *state, double *unused, double *out_8972211626453708085) {
  H_28(state, unused, out_8972211626453708085);
}
void car_h_31(double *state, double *unused, double *out_7079429623240262454) {
  h_31(state, unused, out_7079429623240262454);
}
void car_H_31(double *state, double *unused, double *out_158298338305998931) {
  H_31(state, unused, out_158298338305998931);
}
void car_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
void car_set_mass(double x) {
  set_mass(x);
}
void car_set_rotational_inertia(double x) {
  set_rotational_inertia(x);
}
void car_set_center_to_front(double x) {
  set_center_to_front(x);
}
void car_set_center_to_rear(double x) {
  set_center_to_rear(x);
}
void car_set_stiffness_front(double x) {
  set_stiffness_front(x);
}
void car_set_stiffness_rear(double x) {
  set_stiffness_rear(x);
}
}

const EKF car = {
  .name = "car",
  .kinds = { 25, 24, 30, 26, 27, 29, 28, 31 },
  .feature_kinds = {  },
  .f_fun = car_f_fun,
  .F_fun = car_F_fun,
  .err_fun = car_err_fun,
  .inv_err_fun = car_inv_err_fun,
  .H_mod_fun = car_H_mod_fun,
  .predict = car_predict,
  .hs = {
    { 25, car_h_25 },
    { 24, car_h_24 },
    { 30, car_h_30 },
    { 26, car_h_26 },
    { 27, car_h_27 },
    { 29, car_h_29 },
    { 28, car_h_28 },
    { 31, car_h_31 },
  },
  .Hs = {
    { 25, car_H_25 },
    { 24, car_H_24 },
    { 30, car_H_30 },
    { 26, car_H_26 },
    { 27, car_H_27 },
    { 29, car_H_29 },
    { 28, car_H_28 },
    { 31, car_H_31 },
  },
  .updates = {
    { 25, car_update_25 },
    { 24, car_update_24 },
    { 30, car_update_30 },
    { 26, car_update_26 },
    { 27, car_update_27 },
    { 29, car_update_29 },
    { 28, car_update_28 },
    { 31, car_update_31 },
  },
  .Hes = {
  },
  .sets = {
    { "mass", car_set_mass },
    { "rotational_inertia", car_set_rotational_inertia },
    { "center_to_front", car_set_center_to_front },
    { "center_to_rear", car_set_center_to_rear },
    { "stiffness_front", car_set_stiffness_front },
    { "stiffness_rear", car_set_stiffness_rear },
  },
  .extra_routines = {
  },
};

ekf_init(car);
