#pragma once
#include "rednose/helpers/common_ekf.h"
extern "C" {
void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_err_fun(double *nom_x, double *delta_x, double *out_3614741465952876957);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_7473851160931657756);
void car_H_mod_fun(double *state, double *out_2741669290193596834);
void car_f_fun(double *state, double dt, double *out_8576855921907495355);
void car_F_fun(double *state, double dt, double *out_1643883066210526332);
void car_h_25(double *state, double *unused, double *out_6621228261081700792);
void car_H_25(double *state, double *unused, double *out_127652376429038503);
void car_h_24(double *state, double *unused, double *out_5957372959990195350);
void car_H_24(double *state, double *unused, double *out_9091026511211317888);
void car_h_30(double *state, double *unused, double *out_1142036995738971649);
void car_H_30(double *state, double *unused, double *out_1686570714201567);
void car_h_26(double *state, double *unused, double *out_1462368325069726002);
void car_H_26(double *state, double *unused, double *out_3613850942445017721);
void car_h_27(double *state, double *unused, double *out_3584442209637368107);
void car_H_27(double *state, double *unused, double *out_2176449882514626478);
void car_h_29(double *state, double *unused, double *out_3859636271921873996);
void car_H_29(double *state, double *unused, double *out_508544773600190617);
void car_h_28(double *state, double *unused, double *out_2640932356735705672);
void car_H_28(double *state, double *unused, double *out_8972211626453708085);
void car_h_31(double *state, double *unused, double *out_7079429623240262454);
void car_H_31(double *state, double *unused, double *out_158298338305998931);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}