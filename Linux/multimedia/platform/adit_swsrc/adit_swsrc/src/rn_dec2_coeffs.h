/**
 * \file: rn_dec2_coeffs.h
 *
 * ADIT SRC core implementation - Filter koefficients.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RN_DEC2_COEFFS_H__
#define __RN_DEC2_COEFFS_H__

static const float dec2_coeffs[FILT_LEN_DEC2] __attribute__ ((aligned (32))) =
{
	(float) -1.144409180e-05,	(float) -7.629394531e-06,
	(float)  2.288818359e-05,	(float)  2.670288086e-05,
	(float) -3.051757812e-05,	(float) -6.103515625e-05,
	(float)  3.051757812e-05,	(float)  1.106262207e-04,
	(float) -7.629394531e-06,	(float) -1.754760742e-04,
	(float) -5.340576172e-05,	(float)  2.403259277e-04,
	(float)  1.602172852e-04,	(float) -2.937316895e-04,
	(float) -3.242492676e-04,	(float)  2.975463867e-04,
	(float)  5.455017090e-04,	(float) -2.250671387e-04,
	(float) -8.125305176e-04,	(float)  3.433227539e-05,
	(float)  1.087188721e-03,	(float)  3.051757812e-04,
	(float) -1.316070557e-03,	(float) -8.163452148e-04,
	(float)  1.426696777e-03,	(float)  1.499176025e-03,
	(float) -1.331329346e-03,	(float) -2.311706543e-03,
	(float)  9.346008301e-04,	(float)  3.181457520e-03,
	(float) -1.602172852e-04,	(float) -3.982543945e-03,
	(float) -1.056671143e-03,	(float)  4.558563232e-03,
	(float)  2.716064453e-03,	(float) -4.707336426e-03,
	(float) -4.772186279e-03,	(float)  4.226684570e-03,
	(float)  7.091522217e-03,	(float) -2.902984619e-03,
	(float) -9.471893311e-03,	(float)  5.607604980e-04,
	(float)  1.160812378e-02,	(float)  2.929687500e-03,
	(float) -1.312637329e-02,	(float) -7.640838623e-03,
	(float)  1.357269287e-02,	(float)  1.357269287e-02,
	(float) -1.239395142e-02,	(float) -2.067565918e-02,
	(float)  8.911132812e-03,	(float)  2.891921997e-02,
	(float) -2.117156982e-03,	(float) -3.844070435e-02,
	(float) -9.838104248e-03,	(float)  4.999542236e-02,
	(float)  3.162384033e-02,	(float) -6.677246094e-02,
	(float) -8.140563965e-02,	(float)  1.084022522e-01,
	(float)  3.723907471e-01,	(float)  4.451675415e-01,
	(float)  2.483329773e-01,	(float) -1.033782959e-02,
	(float) -9.663009644e-02,	(float) -1.533126831e-02,
	(float)  5.528640747e-02,	(float)  2.317047119e-02,
	(float) -3.342819214e-02,	(float) -2.519607544e-02,
	(float)  1.906585693e-02,	(float)  2.426910400e-02,
	(float) -8.899688721e-03,	(float) -2.161026001e-02,
	(float)  1.670837402e-03,	(float)  1.798248291e-02,
	(float)  3.238677979e-03,	(float) -1.393508911e-02,
	(float) -6.248474121e-03,	(float)  9.902954102e-03,
	(float)  7.717132568e-03,	(float) -6.214141846e-03,
	(float) -7.995605469e-03,	(float)  3.089904785e-03,
	(float)  7.408142090e-03,	(float) -6.484985352e-04,
	(float) -6.278991699e-03,	(float) -1.079559326e-03,
	(float)  4.878997803e-03,	(float)  2.132415771e-03,
	(float) -3.440856934e-03,	(float) -2.613067627e-03,
	(float)  2.120971680e-03,	(float)  2.647399902e-03,
	(float) -1.033782959e-03,	(float) -2.380371094e-03,
	(float)  2.212524414e-04,	(float)  1.937866211e-03,
	(float)  3.128051758e-04,	(float) -1.438140869e-03,
	(float) -5.989074707e-04,	(float)  9.574890137e-04,
	(float)  6.980895996e-04,	(float) -5.531311035e-04,
	(float) -6.599426270e-04,	(float)  2.517700195e-04,
	(float)  5.493164062e-04,	(float) -4.959106445e-05,
	(float) -4.081726074e-04,	(float) -6.103515625e-05,
	(float)  2.708435059e-04,	(float)  1.068115234e-04,
	(float) -1.602172852e-04,	(float) -1.068115234e-04,
	(float)  8.010864258e-05,	(float)  8.773803711e-05,
	(float) -3.051757812e-05,	(float) -5.722045898e-05,
	(float)  3.814697266e-06,	(float)  3.433227539e-05,
	(float)  3.814697266e-06,	(float) -1.525878906e-05,
	(float) -3.814697266e-06,	(float)  0.000000000e+00,
	(float)  0.000000000e+00,	(float)  0.000000000e+00,
	(float)  0.000000000e+00,	(float)  0.000000000e+00
};
#endif /* __RN_DEC2_COEFFS_H__*/

