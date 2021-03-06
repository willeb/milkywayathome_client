/*
Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#define CHECKPOINT_FILE "astronomy_checkpoint"

#include <math.h>
#include <time.h>

#include "milkyway.h"
#include "milkyway_priv.h"

#include "evaluation_optimized.h"
#include "parameters.h"
#include "probability.h"
#include "atSurveyGeometry.h"
#include "stCoords.h"
#include "star_points.h"
#include "numericalIntegration.h"
#include "evaluation_state.h"



#define stdev 0.6
#define xr 3 * stdev
#define lbr_r 8.5
#define absm 4.2

double sigmoid_curve_params[3] = { 0.9402, 1.6171, 23.5877 };

double alpha, q, r0, delta, coeff, alpha_delta3;
double  bg_a, bg_b, bg_c;
double* qgaus_X, *qgaus_W, **xyz, *dx;
double** stream_a, **stream_c, *stream_sigma, *stream_sigma_sq2;


void init_constants(ASTRONOMY_PARAMETERS* ap)
{
    int i;
    stream_sigma     = (double*)malloc(sizeof(double) * ap->number_streams);
    stream_sigma_sq2 = (double*)malloc(sizeof(double) * ap->number_streams);
    stream_a         = (double**)malloc(sizeof(double*) * ap->number_streams);
    stream_c         = (double**)malloc(sizeof(double*) * ap->number_streams);

    alpha = ap->background_parameters[0];
    q     = ap->background_parameters[1];
    r0    = ap->background_parameters[2];
    delta = ap->background_parameters[3];

    if (ap->aux_bg_profile == 0)
    {
        bg_a = 0;
        bg_b = 0;
        bg_c = 0;
    }
    else if (ap->aux_bg_profile == 1)
    {
        bg_a = ap->background_parameters[4];
        bg_b = ap->background_parameters[5];
        bg_c = ap->background_parameters[6];
    }
    else
    {
        fprintf(stderr,"Error: aux_bg_profile invalid");
    }

    coeff = 1 / (stdev * sqrt(2 * pi));
    alpha_delta3 = 3 - alpha + delta;

    for (i = 0; i < ap->number_streams; i++)
    {
        double ra, dec, lamda, beta, l, b, lbr[3];

        stream_a[i] = (double*)malloc(sizeof(double) * 3);
        stream_c[i] = (double*)malloc(sizeof(double) * 3);
        stream_sigma[i] = ap->stream_parameters[i][4];
        stream_sigma_sq2[i] = 2.0 * stream_sigma[i] * stream_sigma[i];

        if (ap->sgr_coordinates == 0)
        {
            atGCToEq(ap->stream_parameters[i][0], 0, &ra, &dec, get_node(), wedge_incl(ap->wedge));
            atEqToGal(ra, dec, &l, &b);
        }
        else if (ap->sgr_coordinates == 1)
        {
            gcToSgr(ap->stream_parameters[i][0], 0, ap->wedge, &lamda, &beta);
            sgrToGal(lamda, beta, &l, &b);
        }
        else
        {
            fprintf(stderr, "Error: sgr_coordinates not valid");
        }

        lbr[0] = l;
        lbr[1] = b;
        lbr[2] = ap->stream_parameters[i][1];
        lbr2xyz(lbr, stream_c[i]);

        stream_a[i][0] = sin(ap->stream_parameters[i][2]) * cos(ap->stream_parameters[i][3]);
        stream_a[i][1] = sin(ap->stream_parameters[i][2]) * sin(ap->stream_parameters[i][3]);
        stream_a[i][2] = cos(ap->stream_parameters[i][2]);
    }

    xyz     = (double**) malloc(sizeof(double*) * ap->convolve);
    qgaus_X = (double*) malloc(sizeof(double) * ap->convolve);
    qgaus_W = (double*) malloc(sizeof(double) * ap->convolve);
    dx      = (double*) malloc(sizeof(double) * ap->convolve);

    gaussLegendre(-1.0, 1.0, qgaus_X, qgaus_W, ap->convolve);

    for (i = 0; i < ap->convolve; i++)
    {
        xyz[i] = (double*)malloc(sizeof(double) * 3);
        dx[i] = 3 * stdev * qgaus_X[i];
    }
}

void free_constants(ASTRONOMY_PARAMETERS* ap)
{
    int i;

    free(stream_sigma);
    free(stream_sigma_sq2);
    for (i = 0; i < ap->number_streams; i++)
    {
        free(stream_a[i]);
        free(stream_c[i]);
    }
    free(stream_a);
    free(stream_c);

    free(qgaus_X);
    free(qgaus_W);
    free(dx);
    for (i = 0; i < ap->convolve; i++)
    {
        free(xyz[i]);
    }
    free(xyz);
}

void set_probability_constants(int n_convolve, double coords, double* r_point, double* r_in_mag, double* r_in_mag2, double* qw_r3_N, double* reff_xr_rp3)
{
    double gPrime, exp_result, g, exponent, r3, N, reff_value, rPrime3;
    int i;

    //R2MAG
    gPrime = 5.0 * (log10(coords * 1000) - 1.0) + absm;

    //REFF
    exp_result = exp(sigmoid_curve_params[1] * (gPrime - sigmoid_curve_params[2]));
    reff_value = sigmoid_curve_params[0] / (exp_result + 1);
    rPrime3 = coords * coords * coords;

    (*reff_xr_rp3) = reff_value * xr / rPrime3;

    for (i = 0; i < n_convolve; i++)
    {
        g = gPrime + dx[i];

        //MAG2R
        r_in_mag[i] = g;
        r_in_mag2[i] = g * g;
        r_point[i] = pow(10.0, (g - absm) / 5.0 + 1.0) / 1000.0;

        r3 = r_point[i] * r_point[i] * r_point[i];
        exponent = (g - gPrime) * (g - gPrime) / (2 * stdev * stdev);
        N = coeff * exp(-exponent);
        qw_r3_N[i] = qgaus_W[i] * r3 * N;
    }
}

void calculate_probabilities(double* r_point,
                             double* r_in_mag,
                             double* r_in_mag2,
                             double* qw_r3_N,
                             double reff_xr_rp3,
                             double* integral_point,
                             const ASTRONOMY_PARAMETERS* ap,
                             double* bg_prob,
                             double* st_prob)
{
    double bsin, lsin, bcos, lcos, zp;
    double rg, rs, xyzs[3], dotted, xyz_norm;
    double h_prob, aux_prob; //vickej2_bg
    int i, j;

    bsin = sin(integral_point[1] / deg);
    lsin = sin(integral_point[0] / deg);
    bcos = cos(integral_point[1] / deg);
    lcos = cos(integral_point[0] / deg);

    MW_DEBUG("bsin: %.15f lsin: %.15f bcos: %.15f lcos: %.15f\n", bsin, lsin, bcos, lcos);

    /* if q is 0, there is no probability */
    if (q == 0)
    {
        (*bg_prob) = -1;
    }
    else
    {
        (*bg_prob) = 0;
        if (alpha == 1 && delta == 1)
        {
            for (i = 0; i < ap->convolve; i++)
            {
                xyz[i][2] = r_point[i] * bsin;
                zp = r_point[i] * bcos;
                xyz[i][0] = zp * lcos - lbr_r;
                xyz[i][1] = zp * lsin;

                rg = sqrt(xyz[i][0] * xyz[i][0] + xyz[i][1] * xyz[i][1] + (xyz[i][2] * xyz[i][2]) / (q * q));
                rs = rg + r0;

//vickej2_bg changing the hernquist profile to include a quadratic term in g

                if (ap->aux_bg_profile == 1)
                {
                    h_prob = qw_r3_N[i] / (rg * rs * rs * rs);
                    aux_prob = qw_r3_N[i] * ( bg_a * r_in_mag2[i] + bg_b * r_in_mag[i] + bg_c );
                    (*bg_prob) += h_prob + aux_prob;
                }
                else if (ap->aux_bg_profile == 0)
                {
                    (*bg_prob) += qw_r3_N[i] / (rg * rs * rs * rs);
                }
                else
                {
                    fprintf(stderr, "Error: aux_bg_profile invalid");
                }

              MW_DEBUG("reff_xr_rp3: %.15lf r_point: %.15lf rg: %.15lf "
                       "rs: %.15lf qw_r3_N: %.15lf bg_int: %.15lf\n",
                       reff_xr_rp3,
                       r_point[i],
                       rg,
                       rs,
                       qw_r3_N[i],
                       (*bg_prob));
            }
        }
        else
        {
            for (i = 0; i < ap->convolve; i++)
            {
                xyz[i][2] = r_point[i] * bsin;
                zp = r_point[i] * bcos;
                xyz[i][0] = zp * lcos - lbr_r;
                xyz[i][1] = zp * lsin;

                rg = sqrt(xyz[i][0] * xyz[i][0] + xyz[i][1] * xyz[i][1] + (xyz[i][2] * xyz[i][2]) / (q * q));

                (*bg_prob) += qw_r3_N[i] / (pow(rg, alpha) * pow(rg + r0, alpha_delta3));
            }
        }
        (*bg_prob) *= reff_xr_rp3;
    }

    for (i = 0; i < ap->number_streams; i++)
    {
        st_prob[i] = 0;
        if (stream_sigma[i] > -0.0001 && stream_sigma[i] < 0.0001)
            continue;
        for (j = 0; j < ap->convolve; j++)
        {
            xyzs[0] = xyz[j][0] - stream_c[i][0];
            xyzs[1] = xyz[j][1] - stream_c[i][1];
            xyzs[2] = xyz[j][2] - stream_c[i][2];

            dotted = stream_a[i][0] * xyzs[0] + stream_a[i][1] * xyzs[1] + stream_a[i][2] * xyzs[2];

            xyzs[0] = xyzs[0] - dotted * stream_a[i][0];
            xyzs[1] = xyzs[1] - dotted * stream_a[i][1];
            xyzs[2] = xyzs[2] - dotted * stream_a[i][2];

            xyz_norm = xyzs[0] * xyzs[0] + xyzs[1] * xyzs[1] + xyzs[2] * xyzs[2];
            st_prob[i] += qw_r3_N[j] * exp(-xyz_norm / stream_sigma_sq2[i]);
            MW_DEBUG("st_int[%d]: %.15lf sxyz0: %.15lf sxyz1: %.15lf sxyz2: %.15lf\n", i, st_prob[i], xyzs[0], xyzs[1], xyzs[2]);
        }
        st_prob[i] *= reff_xr_rp3;
    }
}

double calculate_progress(EVALUATION_STATE* s)
{
    double total_calc_probs, current_calc_probs, current_probs;
    int i, mu_step_current, nu_step_current, r_step_current;
    INTEGRAL_AREA* ia;

    total_calc_probs = 0;
    current_calc_probs = 0;

    for (i = 0; i < s->number_integrals; i++)
    {
        ia = s->integral[i];

        get_steps(ia, &mu_step_current, &nu_step_current, &r_step_current);

//      printf("mu_step_current: %d, mu_steps: %d, nu_step_current: %d, nu_steps: %d, r_step_current: %d, r_steps: %d\n", mu_step_current, ia->mu_steps, nu_step_current, ia->nu_steps, r_step_current, ia->r_steps);

        current_probs = ia->r_steps * ia->mu_steps * ia->nu_steps;
        total_calc_probs += current_probs;
        if (i < s->current_integral)
        {
            current_calc_probs += current_probs;
        }
        else if (i == s->current_integral)
        {
            current_calc_probs += r_step_current + (nu_step_current * ia->r_steps) + (mu_step_current * ia->nu_steps * ia->r_steps);
        }
//      printf("total_calc_probs: %.2f, current_calc_probs: %.2f\n", total_calc_probs, current_calc_probs);
    }

    total_calc_probs += s->total_stars;
    current_calc_probs += s->current_star_point;
//  printf("total_calc_probs: %.2f, current_calc_probs: %.2f, progress: %.10f\n", total_calc_probs, current_calc_probs, (current_calc_probs/total_calc_probs));

    return (double)current_calc_probs / (double)total_calc_probs;
}

#ifdef MILKYWAY
void do_boinc_checkpoint(EVALUATION_STATE* es)
{
    double progress;

    if (boinc_time_to_checkpoint())
    {
        int retval = write_checkpoint(es);
        if (retval)
        {
            fprintf(stderr, "APP: astronomy checkpoint failed %d\n", retval);
            return;
        }
        boinc_checkpoint_completed();
    }

    progress = calculate_progress(es);
//      printf("progress: %.10f\n", progress);
    boinc_fraction_done(progress);
}
#endif

void cpu__r_constants(int n_convolve,
                      int r_steps,
                      double r_min,
                      double r_step_size,
                      int mu_steps,
                      double mu_min,
                      double mu_step_size,
                      int nu_steps,
                      double nu_min,
                      double nu_step_size,
                      double* irv,
                      double** r_point,
                      double** r_in_mag,
                      double** r_in_mag2,
                      double** qw_r3_N,
                      double* reff_xr_rp3,
                      double* nus,
                      double* ids)
{
    int i;

//vickej2_kpc edits to make volumes even in kpc rather than g
//vickej2_kpc        double log_r, r, next_r, rPrime;

    double r, next_r, rPrime, r_min_kpc, r_max_kpc, r_step_size_kpc, r_max;

    r_max = r_min + r_step_size * r_steps;

    r_min_kpc = pow(10.0, ((r_min - 14.2) / 5.0));
    r_max_kpc = pow(10.0, ((r_max - 14.2) / 5.0));
    r_step_size_kpc = (r_max_kpc - r_min_kpc) / r_steps;


    for (i = 0; i < r_steps; i++)
    {
#ifdef USE_KPC

        r               =       r_min_kpc + (i * r_step_size_kpc);
        next_r          =       r + r_step_size_kpc;

#else
        double log_r    =   r_min + (i * r_step_size);
        r       =   pow(10.0, (log_r - 14.2) / 5.0);
        next_r      =   pow(10.0, (log_r + r_step_size - 14.2) / 5.0);
#endif

        irv[i]      =   (((next_r * next_r * next_r) - (r * r * r)) / 3.0) * mu_step_size / deg;
        rPrime      =   (next_r + r) / 2.0;

        r_point[i] = (double*)malloc(sizeof(double) * n_convolve);
        r_in_mag[i] = (double*)malloc(sizeof(double) * n_convolve);
        r_in_mag2[i] = (double*)malloc(sizeof(double) * n_convolve);
        qw_r3_N[i] = (double*)malloc(sizeof(double) * n_convolve);
        set_probability_constants(n_convolve, rPrime, r_point[i], r_in_mag[i], r_in_mag2[i], qw_r3_N[i], &(reff_xr_rp3[i]));
    }

    for (i = 0; i < nu_steps; i++)
    {
        nus[i] = nu_min + (i * nu_step_size);
        ids[i] = cos((90 - nus[i] - nu_step_size) / deg) - cos((90 - nus[i]) / deg);
        nus[i] += 0.5 * nu_step_size;
    }
}

void calculate_integral(const ASTRONOMY_PARAMETERS* ap, INTEGRAL_AREA* ia, EVALUATION_STATE* es)
{
    int i, mu_step_current, nu_step_current, r_step_current;
    double bg_prob, *st_probs, V;
    double* irv, *reff_xr_rp3, **qw_r3_N, **r_point, **r_in_mag, **r_in_mag2;
    double* ids, *nus;
    double integral_point[3];

    double bg_prob_int, bg_prob_int_c, temp;        // for kahan summation
    double* st_probs_int, *st_probs_int_c;          // for kahan summation

    irv     = (double*)malloc(sizeof(double) * ia->r_steps);
    st_probs    = (double*)malloc(sizeof(double) * ap->number_streams);
    st_probs_int    = (double*)malloc(sizeof(double) * ap->number_streams);
    st_probs_int_c  = (double*)malloc(sizeof(double) * ap->number_streams);
    reff_xr_rp3 = (double*)malloc(sizeof(double) * ia->r_steps);
    qw_r3_N     = (double**)malloc(sizeof(double*) * ia->r_steps);
    r_point     = (double**)malloc(sizeof(double*) * ia->r_steps);
    r_in_mag    = (double**)malloc(sizeof(double*) * ia->r_steps);
    r_in_mag2   = (double**)malloc(sizeof(double*) * ia->r_steps);
    ids     = (double*)malloc(sizeof(double) * ia->nu_steps);
    nus     = (double*)malloc(sizeof(double) * ia->nu_steps);
    cpu__r_constants(ap->convolve, ia->r_steps, ia->r_min, ia->r_step_size,
                     ia->mu_steps, ia->mu_min, ia->mu_step_size,
                     ia->nu_steps, ia->nu_min, ia->nu_step_size,
                     irv, r_point, r_in_mag, r_in_mag2, qw_r3_N, reff_xr_rp3, nus, ids);

    get_steps(ia, &mu_step_current, &nu_step_current, &r_step_current);

    bg_prob_int = ia->background_integral;
    bg_prob_int_c = 0.0;
    for (i = 0; i < ap->number_streams; i++)
    {
        st_probs_int[i] = ia->stream_integrals[i];
        st_probs_int_c[i] = 0.0;
    };

    for (; mu_step_current < ia->mu_steps; mu_step_current++)
    {
        double mu = ia->mu_min + (mu_step_current * ia->mu_step_size);

        for (; nu_step_current < ia->nu_steps; nu_step_current++)
        {
#ifdef MILKYWAY
            ia->background_integral = bg_prob_int + bg_prob_int_c;  // apply correction
            for (i = 0; i < ap->number_streams; i++)
            {
                ia->stream_integrals[i] = st_probs_int[i] + st_probs_int_c[i];  // apply correction
            }

            do_boinc_checkpoint(es);

//              bg_prob_int_c = 0;
//              for (i = 0; i < ap->number_streams; i++) st_probs_int_c[i] = 0;
#endif

            if (ap->sgr_coordinates == 0)
            {
                double ra, dec;
                atGCToEq(mu + 0.5 * ia->mu_step_size, nus[nu_step_current], &ra, &dec, get_node(), wedge_incl(ap->wedge));
                atEqToGal(ra, dec, &integral_point[0], &integral_point[1]);
            }
            else if (ap->sgr_coordinates == 1)
            {
                double lamda, beta;
                gcToSgr(mu + 0.5 * ia->mu_step_size, nus[nu_step_current], ap->wedge, &lamda, &beta);
                sgrToGal(lamda, beta, &integral_point[0], &integral_point[1]);
            }
            else
            {
                printf("Error: ap->sgr_coordinates not valid");
            }
//          printf("nu: %d, glong[%d][%d]: %.15lf, glat[%d][%d]: %.15lf\n", nu_step_current, mu_step_current, nu_step_current, integral_point[0], mu_step_current, nu_step_current, integral_point[1]);

            for (; r_step_current < ia->r_steps; r_step_current++)
            {
                V = irv[r_step_current] * ids[nu_step_current];

                calculate_probabilities(r_point[r_step_current], r_in_mag[r_step_current], r_in_mag2[r_step_current], qw_r3_N[r_step_current], reff_xr_rp3[r_step_current], integral_point, ap, &bg_prob, st_probs);

                bg_prob *= V;

                temp = bg_prob_int;
                bg_prob_int += bg_prob;
                bg_prob_int_c += bg_prob - (bg_prob_int - temp);

//              ia->background_integral += bg_prob;
                for (i = 0; i < ap->number_streams; i++)
                {
                    st_probs[i] *= V;
                    temp = st_probs_int[i];
                    st_probs_int[i] += st_probs[i];
                    st_probs_int_c[i] += st_probs[i] - (st_probs_int[i] - temp);

//                  ia->stream_integrals[i] += st_probs[i] * V;
                }

                ia->current_calculation++;
                if (ia->current_calculation >= ia->max_calculation) break;
            }
            if (ia->current_calculation >= ia->max_calculation) break;
            r_step_current = 0;
        }
        if (ia->current_calculation >= ia->max_calculation) break;
        nu_step_current = 0;
    }
    mu_step_current = 0;

    ia->background_integral = bg_prob_int + bg_prob_int_c;  // apply correction
    for (i = 0; i < ap->number_streams; i++)
    {
        ia->stream_integrals[i] = st_probs_int[i] + st_probs_int_c[i];  // apply correction
    }

//  printf("bg_int: %.15lf ", ia->background_integral);
//  for (i = 0; i < ap->number_streams; i++) printf("st_int[%d]: %.15lf ", i, ia->stream_integrals[i]);
//  printf("\n");


//  if (1) exit(0);
    free(nus);
    free(ids);
    free(irv);
    free(st_probs);
    free(st_probs_int);
    free(st_probs_int_c);
    free(reff_xr_rp3);
    for (i = 0; i < ia->r_steps; i++)
    {
        free(r_point[i]);
        free(r_in_mag[i]);
        free(r_in_mag2[i]);
        free(qw_r3_N[i]);
    }
    free(r_point);
    free(r_in_mag);
    free(r_in_mag2);
    free(qw_r3_N);
}

int calculate_integrals(const ASTRONOMY_PARAMETERS* ap, EVALUATION_STATE* es, const STAR_POINTS* sp)
{
    int i, j;
#ifdef MW_ENABLE_DEBUG
  time_t start_time, finish_time;
  time(&start_time);
#endif

#ifdef MILKYWAY
    read_checkpoint(es);
#endif

    for (; es->current_integral < ap->number_integrals; es->current_integral++)
    {
        calculate_integral(ap, es->integral[es->current_integral], es);
//      fprintf(stderr, "<background_integral>%.25lf</background_integral>\n", es->integral[es->current_integral]->background_integral);
//      for (i = 0; i < ap->number_streams; i++)
//        fprintf(stderr, "<stream_integral>%d %.25lf</stream_integral>\n", i, es->integral[es->current_integral]->stream_integrals[i]);
    }

    es->background_integral = es->integral[0]->background_integral;
    for (i = 0; i < ap->number_streams; i++)
        es->stream_integrals[i] = es->integral[0]->stream_integrals[i];

    for (i = 1; i < ap->number_integrals; i++)
    {
        es->background_integral -= es->integral[i]->background_integral;
        for (j = 0; j < ap->number_streams; j++)
            es->stream_integrals[j] -= es->integral[i]->stream_integrals[j];
    }

#ifdef MILKYWAY
    fprintf(stderr, "<background_integral> %.20lf </background_integral>\n", es->background_integral);
    fprintf(stderr, "<stream_integrals>");
    for (i = 0; i < ap->number_streams; i++)
    {
        fprintf(stderr, " %.20lf", es->stream_integrals[i]);
    }
    fprintf(stderr, " </stream_integrals>\n");
#endif

    #ifdef MW_ENABLE_DEBUG
    time(&finish_time);
    MW_DEBUG("integrals calculated in: %lf\n", (double)finish_time - (double)start_time);
    #endif

    return 0;
}

int calculate_likelihood(const ASTRONOMY_PARAMETERS* ap, EVALUATION_STATE* es, const STAR_POINTS* sp)
{
    int i, current_stream;
    double bg_prob, *st_prob;
    double prob_sum, prob_sum_c, temp;  // for Kahan summation
    double exp_background_weight, sum_exp_weights, *exp_stream_weights;
    double* r_point, *r_in_mag, *r_in_mag2, *qw_r3_N, reff_xr_rp3;

    double bg_only, bg_only_sum, bg_only_sum_c;
    double st_only, *st_only_sum, *st_only_sum_c;

#ifdef MW_ENABLE_DEBUG
    time_t start_time, finish_time;
    time (&start_time);
#endif

    exp_stream_weights = (double*)malloc(sizeof(double) * ap->number_streams);
    st_prob = (double*)malloc(sizeof(double) * ap->number_streams);
    r_point = (double*)malloc(sizeof(double) * ap->convolve);
    r_in_mag = (double*)malloc(sizeof(double) * ap->convolve);
    r_in_mag2 = (double*)malloc(sizeof(double) * ap->convolve);
    qw_r3_N = (double*)malloc(sizeof(double) * ap->convolve);

    st_only_sum = (double*)malloc(sizeof(double) * ap->number_streams);
    st_only_sum_c = (double*)malloc(sizeof(double) * ap->number_streams);

    exp_background_weight = exp(ap->background_weight);
    sum_exp_weights = exp_background_weight;
    for (i = 0; i < ap->number_streams; i++)
    {
        exp_stream_weights[i] = exp(ap->stream_weights[i]);
        sum_exp_weights += exp(ap->stream_weights[i]);
    }
    sum_exp_weights *= 0.001;

#ifdef MILKYWAY
    do_boinc_checkpoint(es);
#endif

    prob_sum = 0;
    prob_sum_c = 0;

    bg_only_sum = 0;
    bg_only_sum_c = 0;

    for (i = 0; i < ap->number_streams; i++)
    {
        st_only_sum[i] = 0;
        st_only_sum_c[i] = 0;
    }


    for (; es->current_star_point < sp->number_stars; es->current_star_point++)
    {
        double star_prob;

        set_probability_constants(ap->convolve, sp->stars[es->current_star_point][2], r_point, r_in_mag, r_in_mag2, qw_r3_N, &reff_xr_rp3);
        calculate_probabilities(r_point, r_in_mag, r_in_mag2, qw_r3_N, reff_xr_rp3, sp->stars[es->current_star_point], ap, &bg_prob, st_prob);

        //      printf("bg_prob: %.15lf, st_prob[0]: %.15lf, st_prob[1]: %.15lf", bg_prob, st_prob[0], st_prob[1]);

        bg_only = (bg_prob / es->background_integral) * exp_background_weight;
        star_prob = bg_only;

        if (bg_only == 0.0)
            bg_only = -238;
        else
            bg_only = log10(bg_only / sum_exp_weights);

        temp = bg_only_sum;
        bg_only_sum += bg_only;
        bg_only_sum_c += bg_only - (bg_only_sum - temp);


        for (current_stream = 0; current_stream < ap->number_streams; current_stream++)
        {
            st_only = st_prob[current_stream] / es->stream_integrals[current_stream] * exp_stream_weights[current_stream];
            star_prob += st_only;

            if (st_only == 0.0)
                st_only = -238;
            else
                st_only = log10(st_only / sum_exp_weights);

            temp = st_only_sum[current_stream];
            st_only_sum[current_stream] += st_only;
            st_only_sum_c[current_stream] += st_only - (st_only_sum[current_stream] - temp);
        }
        star_prob /= sum_exp_weights;

        MW_DEBUG(", prob_sum: %.15lf\n", star_prob);

        if (star_prob != 0.0)
        {
            star_prob = log10(star_prob);
            temp = prob_sum;
            prob_sum += star_prob;
            prob_sum_c += star_prob - (prob_sum - temp);
        }
        else
        {
            es->num_zero++;
            prob_sum -= 238.0;
        }
    }
    es->prob_sum = prob_sum + prob_sum_c;
    bg_only_sum += bg_only_sum_c;
    bg_only_sum /= sp->number_stars;

#ifdef MILKYWAY
    fprintf(stderr, "<background_only_likelihood> %.20lf </background_only_likelihood>\n", bg_only_sum - 3.0);
    fprintf(stderr, "<stream_only_likelihood>");
    for (i = 0; i < ap->number_streams; i++)
    {
        st_only_sum[i] += st_only_sum_c[i];
        st_only_sum[i] /= sp->number_stars;

        fprintf(stderr, " %.20lf", st_only_sum[i] - 3.0);
    }
    fprintf(stderr, " </stream_only_likelihood>\n");
#endif

    MW_DEBUG("prob_sum: %.15lf\n", es->prob_sum);

    free(exp_stream_weights);
    free(st_prob);
    free(r_point);
    free(r_in_mag);
    free(r_in_mag2);
    free(qw_r3_N);

    free(st_only_sum);
    free(st_only_sum_c);

#ifdef MW_ENABLE_DEBUG
    time(&finish_time);
    MW_DEBUG("likelihood calculated in: %lf\n", (double)finish_time - (double)start_time);
#endif

    return 0;
}


double cpu_evaluate(double* parameters,
                    ASTRONOMY_PARAMETERS* ap,
                    EVALUATION_STATE* es,
                    STAR_POINTS* sp)
{
    int retval;

    set_astronomy_parameters(ap, parameters);
    reset_evaluation_state(es);

    retval = calculate_integrals(ap, es, sp);
    if (retval)
    {
        fprintf(stderr, "APP: error calculating integrals: %d\n", retval);
        mw_finish(retval);
    }

    MW_DEBUG("calculated integrals: %lf, %lf\n", es->background_integral, es->stream_integrals[0]);

    retval = calculate_likelihood(ap, es, sp);
    free_constants(ap);

    if (retval)
    {
        fprintf(stderr, "APP: error calculating likelihood: %d\n", retval);
        mw_finish(retval);
    }
    return es->prob_sum / (sp->number_stars - es->bad_jacobians);
}


