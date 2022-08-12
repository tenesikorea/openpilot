#pragma once
#include "rednose/helpers/common_ekf.h"
extern "C" {
void live_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_9(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_12(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_32(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_33(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_H(double *in_vec, double *out_1948723160519481493);
void live_err_fun(double *nom_x, double *delta_x, double *out_1326617294892026553);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_4209877855577787445);
void live_H_mod_fun(double *state, double *out_7504121032138680857);
void live_f_fun(double *state, double dt, double *out_4646576867899475088);
void live_F_fun(double *state, double dt, double *out_5174316526860583237);
void live_h_4(double *state, double *unused, double *out_9110616446286677265);
void live_H_4(double *state, double *unused, double *out_2178751594556403216);
void live_h_9(double *state, double *unused, double *out_8551277240316342594);
void live_H_9(double *state, double *unused, double *out_2419941241185993861);
void live_h_10(double *state, double *unused, double *out_4620945272970696394);
void live_H_10(double *state, double *unused, double *out_2076156875284812434);
void live_h_12(double *state, double *unused, double *out_3174412120140823682);
void live_H_12(double *state, double *unused, double *out_7198208002588365011);
void live_h_31(double *state, double *unused, double *out_2255803214784134484);
void live_H_31(double *state, double *unused, double *out_8502973038796172896);
void live_h_32(double *state, double *unused, double *out_4753541036473000575);
void live_H_32(double *state, double *unused, double *out_5719314874377755303);
void live_h_13(double *state, double *unused, double *out_5314515647160870884);
void live_H_13(double *state, double *unused, double *out_2793196039033508332);
void live_h_14(double *state, double *unused, double *out_8551277240316342594);
void live_H_14(double *state, double *unused, double *out_2419941241185993861);
void live_h_33(double *state, double *unused, double *out_3196238603845286244);
void live_H_33(double *state, double *unused, double *out_5352416034157315292);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}