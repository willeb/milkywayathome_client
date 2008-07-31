#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "atSurveyGeometry.h"
#include "stCoords.h"
#include "probability.h"
#include "stVector.h"
#include "numericalIntegration.h"

#define PI 3.1415926535897932384626433832795028841971693993751
double sigmoid_curve_parameters[3] = { 0.9402, 1.6171, 23.5877 };
double stdev = 0.6;

double coordparConvolved[3];
double bparsConvolved[4];
double sparsConvolved[5];
double gPrime;

/* Convert r in parsecs into the apparent magnitude, given an absolute magnitude
   of 4.2. */ 
double r2mag(double r) {
    double absm, result;
	absm = 4.2;
    result = 5 * (log10( r ) - 1) + absm;

	return result;
}

/* Convert apparent magnitude, given absolute an magnitude of 4.2, into r in parsecs */
double mag2r(double g) {
	double absm, result, exponent;
	absm = 4.2;
	exponent = (g - absm)/5 + 1;
	result = pow(10, exponent);
	return result;
} 
          
/* Get the detection efficiency at a distance of kr kiloparsecs. */
double reff(double kr) {
    double gstar, result, exp_result, pre_exp;

	gstar = r2mag( kr * 1000 );

	pre_exp = sigmoid_curve_parameters[1] * (gstar - sigmoid_curve_parameters[2]);
	exp_result = exp(pre_exp);
	result = sigmoid_curve_parameters[0] / (exp_result + 1);
	
	return result;
}

/* Calculate the Jacobian for a given stream coordinate */
double Jacob(const double* a, const double* b, double sint, double xp, int verb) {
    double aa, bb, abn, tmp, j;
    
    aa = dotp(a, a);
    bb = dotp(b, b);
    abn = norm(a) * norm(b);
    
    tmp = aa * sint * sint + bb * ((1 - sint * sint)*(1 - sint * sint));
    j = sqrt(tmp) + (abn * xp) / tmp;
        
    return fabs(j);
}

/* stPbxFunction - determines the probability that a star with the given coordinates
   (coordpar, in lbr format) is in the galaxy defined by the given parameters (bkgdpar)
   verb flags the function to output it's progress as it works
   Return: a double that indicates the probability. Higher values indicate
   higher likelyhood that the star is a part of the galaxy distribution.
   A return < 0 indicates an error:
   -1 - q is 0
   -2 - magnitude of star is NaN
*/
double stPbxFunction(const double* coordpar, const double* bpars) {
    double xg[3];
    double alpha, q, delta, P;
    double r0, rg;
    
    lbr2xyz(coordpar, xg);
    
    /* alpha more negative, background falls off faster
     * q bigger means more squashed galaxy
     * r0, z0 defines the cylinder in the middle of the galaxy that will be empty
     * maxz integrate up to in steps of dz, numerical consideration */
    alpha = bpars[0];
    q     = bpars[1];
    r0    = bpars[2];
    delta = bpars[3];
    
    /* if q is 0, there is no probability */
    if (q == 0) return -1;
    
    /* background probability */
    rg = sqrt(xg[0]*xg[0] + xg[1]*xg[1] + (xg[2]/q)*(xg[2]/q));

    P = 1 / (pow(rg, alpha) * pow(rg + r0, 3 - alpha + delta));

    return P;
}

/* stPsgFunction - determines the probability that a star with given coordinates (coordpar, in lbr format)
   is a part of a stream defined by the given parameters (pars). It is assumed that lbr coordinates
   are solar-centered and xyz coordinates are galactic centered.
   verb flags the function to output it's work as it executes.
   Return: a double value is returned indicating the probability that the star is in the stream.
   A higher value indicates a higher probability. 
   If a value < 0 is returned, an error occured. 
   -1 - a parameters is NaN
   -2 - an error occured in the call to lbr2stream
*/
double stPsgFunction(const double* coordpar, const double* spars, int wedge) {
	//update: allow for new coordinate transforms
	double xyz[3], lbr[3], a[3], c[3];
	double mu, r, theta, phi, sigma;
	double ra, dec, l, b;
	double dotted, xyz_norm, prob;
	
	mu = spars[0];
	r = spars[1];
	theta = spars[2];
	phi = spars[3];
	sigma = spars[4];

	//update: convert from mu, nu, r geometry to a and c geometry
	atGCToEq(mu, 0, &ra, &dec, get_node(), wedge_incl(wedge));
	atEqToGal(ra, dec, &l, &b);
	lbr[0] = l;
	lbr[1] = b;
	lbr[2] = r;
	lbr2xyz(lbr, c);

	a[0] = sin(theta) * cos(phi);
	a[1] = sin(theta) * sin(phi);
	a[2] = cos(theta);

	//Sigma near 0 so star prob is 0.
	if (sigma > -0.0001 && sigma < 0.0001) return 0;

	lbr2xyz(coordpar, xyz);
	xyz[0] = xyz[0] - c[0];
	xyz[1] = xyz[1] - c[1];
	xyz[2] = xyz[2] - c[2];
                
	dotted = dotp(a, xyz);
	xyz[0] = xyz[0] - dotted*a[0];
	xyz[1] = xyz[1] - dotted*a[1];
	xyz[2] = xyz[2] - dotted*a[2];

	xyz_norm = norm(xyz);
	
//	fprintf(stderr, "dotted: %lf, xyz_norm: %lf, sigma: %lf\n", dotted, xyz_norm, sigma);
	prob = exp( -(xyz_norm*xyz_norm) / 2 / (sigma*sigma) );

//	fprintf(stderr, "prob before ref: %lf\n", prob);
	return prob;
}

double stPbxConvolved(const double* coordpar, const double* bpars, int numpoints, int wedge) {
	int i;
    	double pbx, reff_value, prob, rPrime, rPrime3, a, b;

	rPrime = coordpar[2];
	gPrime = r2mag(rPrime*1000);
	rPrime3 = rPrime * rPrime * rPrime;

//	fprintf(stderr, "rPrime: %lf\n", rPrime);
//	fprintf(stderr, "gPrime: %lf\n", gPrime);
//	fprintf(stderr, "rPrime3: %lf\n", rPrime3);
	a = gPrime - 3.0*stdev;
	b = gPrime + 3.0*stdev; 
	
	for (i = 0; i < 3; i++) {
		coordparConvolved[i] = coordpar[i];
	}	
	for (i = 0; i < 4; i++) {
		bparsConvolved[i] = bpars[i];
	}

//	fprintf(stderr, "coordpar: %g, %g, %g\n", coordpar[0], coordpar[1], coordpar[2]);
//	fprintf(stderr, "coordparConvolved: %g, %g, %g\n", coordparConvolved[0], coordparConvolved[1], coordparConvolved[2]);
//	fprintf(stderr, "bpars: %g, %g, %g, %g\n", bpars[0], bpars[1], bpars[2], bpars[3]);
//	fprintf(stderr, "bparsConvolved: %g, %g, %g, %g\n", bparsConvolved[0], bparsConvolved[1], bparsConvolved[2], bparsConvolved[3]);
	
	pbx = qgaus(backgroundConvolve, a, b, numpoints, wedge);
	pbx *= 1/rPrime3;
//	fprintf(stderr, "pbx: %lf\n", pbx);

	reff_value = reff(coordpar[2]);
	prob = pbx*reff_value;
	
	return prob;
}

double stPbx(const double* coordpar, const double* bpars) {
//        int i;
        double pbx, reff_value, prob;
//		double rPrime, rPrime3, a, b;

//      fprintf(stderr, "rPrime: %lf\n", rPrime);

//      fprintf(stderr, "coordpar: %g, %g, %g\n", coordpar[0], coordpar[1], coordpar[2]);
//      fprintf(stderr, "bpars: %g, %g, %g, %g\n", bpars[0], bpars[1], bpars[2], bpars[3]);

        pbx = stPbxFunction(coordpar, bpars);
//      fprintf(stderr, "pbx: %lf\n", pbx);

        reff_value = reff(coordpar[2]);
        prob = pbx*reff_value;

        return prob;
}

double stPsgConvolved(const double* coordpar, const double* spars, int wedge, int numpoints) {
	int i;
	double psg, reff_value, prob, rPrime, rPrime3, a, b; 

        rPrime = coordpar[2];
        //fprintf(stderr, "rPrime: %lf\n", rPrime);
        gPrime = r2mag(rPrime*1000);
        //fprintf(stderr, "gPrime: %lf\n", gPrime);
        rPrime3 =  rPrime * rPrime * rPrime;
        //fprintf(stderr, "rPrime3: %lf\n", rPrime3);
	a = gPrime - 3.0*stdev;
	b = gPrime + 3.0*stdev;

        for (i = 0; i < 3; i++) {
                coordparConvolved[i] = coordpar[i];
        }
        for (i = 0; i < 5; i++) {
                sparsConvolved[i] = spars[i];
        }
        
	//fprintf(stderr, "coordpar: %g, %g, %g\n", coordpar[0], coordpar[1], coordpar[2]);
        //fprintf(stderr, "coordparConvolved: %g, %g, %g\n", coordparConvolved[0], coordparConvolved[1], coordparConvolved[2]);
        //fprintf(stderr, "spars: %g, %g, %g, %g\n", spars[0], spars[1], spars[2], spars[3]);
        //fprintf(stderr, "sparsConvolved: %g, %g, %g, %g\n", sparsConvolved[0], sparsConvolved[1], sparsConvolved[2], sparsConvolved[3]);

        psg = qgaus(streamConvolve, a, b, numpoints, wedge);
	psg *= 1/rPrime3;
       
        //fprintf(stderr, "psg: %lf\n", psg);

        reff_value = reff(coordpar[2]);
//      fprintf(stderr, "reff_value: %lf\n", reff_value);
        prob = psg * reff_value;
//      fprintf(stderr, "prob after ref: %lf\n", prob);
	return prob;
}

double stPsg(const double* coordpar, const double* spars, int wedge) {
//        int i;
        double psg, reff_value, prob; 

        //fprintf(stderr, "coordpar: %g, %g, %g\n", coordpar[0], coordpar[1], coordpar[2]);
        //fprintf(stderr, "spars: %g, %g, %g, %g\n", spars[0], spars[1], spars[2], spars[3]);

	psg = stPsgFunction(coordpar, spars, wedge);
        //fprintf(stderr, "psg: %lf\n", psg);

        reff_value = reff(coordpar[2]);
//      fprintf(stderr, "reff_value: %lf\n", reff_value);
        prob = psg * reff_value;
//      fprintf(stderr, "prob after ref: %lf\n", prob);
        return prob;
}


/* Convolution Functions for use with qgaus */
double backgroundConvolve(double g, int wedge) {
        double exponent, coeff, pbx, r3, N, r, prob;

        r = mag2r(g)/1000;      //r in kpc
        r3 =  r * r * r;
        coordparConvolved[2] = r;

        exponent = pow( (g-gPrime), 2 ) / (2*stdev*stdev);
        coeff = 1 / (stdev * sqrt(2*PI));
        N = coeff * exp(-exponent);     //value of gaussian convolution function

        pbx = stPbxFunction(coordparConvolved, bparsConvolved);         //prob of star in background given app mag, g

        prob = pbx * r3 * N;
        return prob;
}

double streamConvolve(double g, int wedge) {
        double exponent, coeff, psg, r3, N, r, prob;

        r = mag2r(g)/1000;      //r in kpc
        r3 =  r * r * r;
        coordparConvolved[2] = r;

        exponent = pow( (g-gPrime), 2 ) / (2*stdev*stdev);
        coeff = 1 / (stdev * sqrt(2*PI));
        N = coeff * exp(-exponent);     //value of gaussian convolution function

        psg = stPsgFunction(coordparConvolved, sparsConvolved, wedge);         //prob of star in stream given app. mag. g
        prob = psg * r3 * N;
        return prob;
}
