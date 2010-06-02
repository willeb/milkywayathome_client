/****************************************************************************/
/* IO.C: I/O routines for export version of hierarchical N-body code.       */
/* Public routines: inputdata(), initoutput(), stopoutput(), output().      */
/*                                                                          */
/* Copyright (c) 1993 by Joshua E. Barnes, Honolulu, HI.                    */
/* It's free because it's yours.                                            */
/****************************************************************************/

#include "code.h"

local void diagnostics(void);
local void in_int(stream, int *);
local void in_real(stream, real *);
local void in_vector(stream, vector);
local void out_int(stream, int);
local void out_real(stream, real);
local void out_vector(stream, vector);
local void out_2vectors(stream, vector, vector);
local void printvec(string, vector);

/*
 * INPUTDATA: read initial conditions from input file.
 */

void inputdata(void)
{
    stream instr;
    permanent char headbuf[128];
    int ndim;
    bodyptr p;

    instr = fopen(infile, "r");			/* open input stream        */
    if (instr == NULL)
	error("inputdata: cannot find file %s\n", infile);
    sprintf(headbuf, "Hierarchical code: input file %s", infile);
    headline = headbuf;
    in_int(instr, &nbody);
    if (nbody < 1)
	error("inputdata: nbody = %d is absurd\n", nbody);
    in_int(instr, &ndim);
    if (ndim != NDIM)
	error("inputdata: ndim = %d is absurd\n", ndim);
    in_real(instr, &tnow);
    bodytab = (bodyptr) allocate(nbody * sizeof(body));
    for (p = bodytab; p < bodytab+nbody; p++)	/* loop over new bodies     */
	Type(p) = BODY;				/*   init body type         */
    for (p = bodytab; p < bodytab+nbody; p++)
	in_real(instr, &Mass(p));
    for (p = bodytab; p < bodytab+nbody; p++)
	in_vector(instr, Pos(p));
    for (p = bodytab; p < bodytab+nbody; p++)
	in_vector(instr, Vel(p));
    fclose(instr);				/* close input stream       */
}

/*
 * INITOUTPUT: initialize output routines.
 */

local stream outstr;                  /* output stream pointer */
local stream outstr2;                  /* output stream pointer */

void initoutput(void)
{
    if (*outfile != NULL) {                     /* output file specified?   */
        outstr = fopen(outfile, "w");           /*   setup output stream    */
	if (outstr == NULL)
	    error("initoutput: cannot open file %s\n", outfile);
    } else
        outstr = NULL;				/*   turn off data output   */

}

/*
 * STOPOUTPUT: finish up after a run.
 */

void stopoutput(void)
{
    if (outstr != NULL)
        fclose(outstr);
}

/*
 * Counters and accumulators for output routines.
 */

local real mtot;                /* total mass of N-body system */
local real etot[3];             /* binding, kinetic, potential energy */
local matrix keten;		/* kinetic energy tensor */
local matrix peten;		/* potential energy tensor */
local vector cmphase[2];	/* center of mass coordinates */
local vector amvec;		/* angular momentum vector */

/*
 * OUTPUT: compute diagnostics and output data.
 */

void output(void)
{
    bodyptr p;
    vector lbR;
    diagnostics();				/* compute std diagnostics  */
	//printf("tnow = %f\n", tnow);
	if(tstop - tnow < 0.01/freq) {
	printf("tnow = %f\n", tnow);
		for (p = bodytab; p < bodytab+nbody; p++) {
			(lbR)[2] = sqrt(Pos(p)[0]*Pos(p)[0] + Pos(p)[1]*Pos(p)[1] + Pos(p)[2]*Pos(p)[2]);
			(lbR)[1] = (180.0/3.141592654)*atan2(Pos(p)[2], sqrt((Pos(p)[0])*(Pos(p)[0]) + Pos(p)[1]*Pos(p)[1]));
			(lbR)[0] = (180.0/3.141592654)*atan2(Pos(p)[1],Pos(p)[0]);	

			if((lbR)[0] < 0) { (lbR)[0] += 360.0;}

	    		out_2vectors(outstr, lbR, Vel(p));
		}	
		printf("\tParticle data written to file %s\n\n", outfile);
	fflush(outstr);				/*   drain output buffer    */
	tout += 1 / freqout;			/*   schedule next data out */
     }
}

/*
 * DIAGNOSTICS: compute various dynamical diagnostics.
 */

local void diagnostics(void)
{
    register bodyptr p;
    real velsq;
    vector tmpv;
    matrix tmpt;

    mtot = 0.0;					/* zero total mass          */
    etot[1] = etot[2] = 0.0;			/* zero total KE and PE     */
    CLRM(keten);				/* zero ke tensor           */
    CLRM(peten);				/* zero pe tensor           */
    CLRV(cmphase[0]);				/* zero c. of m. position   */
    CLRV(cmphase[1]);				/* zero c. of m. velocity   */
    CLRV(amvec);				/* zero am vector           */
    for (p = bodytab; p < bodytab+nbody; p++) {	/* loop over all particles  */
	mtot += Mass(p);                        /*   sum particle masses    */
	DOTVP(velsq, Vel(p), Vel(p));		/*   square vel vector      */
	etot[1] += 0.5 * Mass(p) * velsq;	/*   sum current KE         */
	etot[2] += 0.5 * Mass(p) * Phi(p);	/*   and current PE         */
	MULVS(tmpv, Vel(p), 0.5 * Mass(p));	/*   sum 0.5 m v_i v_j      */
	OUTVP(tmpt, tmpv, Vel(p));
	ADDM(keten, keten, tmpt);
	MULVS(tmpv, Pos(p), Mass(p));		/*   sum m r_i a_j          */
	OUTVP(tmpt, tmpv, Acc(p));
	ADDM(peten, peten, tmpt);
	MULVS(tmpv, Pos(p), Mass(p));		/*   sum cm position        */
	ADDV(cmphase[0], cmphase[0], tmpv);
	MULVS(tmpv, Vel(p), Mass(p));		/*   sum cm momentum        */
	ADDV(cmphase[1], cmphase[1], tmpv);
	CROSSVP(tmpv, Pos(p), Vel(p));		/*   sum angular momentum   */
	MULVS(tmpv, tmpv, Mass(p));
	ADDV(amvec, amvec, tmpv);
    }
    etot[0] = etot[1] + etot[2];                /* sum KE and PE            */
    DIVVS(cmphase[0], cmphase[0], mtot);        /* normalize cm coords      */
    DIVVS(cmphase[1], cmphase[1], mtot);
}

/*
 * Low-level input and output operations.
 */

local void in_int(stream str, int *iptr)
{
    if (fscanf(str, "%d", iptr) != 1)
	error("in_int: input conversion error\n");
}

local void in_real(stream str, real *rptr)
{
    double tmp;

    if (fscanf(str, "%lf", &tmp) != 1)
	error("in_real: input conversion error\n");
    *rptr = tmp;
}

local void in_vector(stream str, vector vec)
{
    double tmpx, tmpy, tmpz;

    if (fscanf(str, "%lf%lf%lf", &tmpx, &tmpy, &tmpz) != 3)
	error("in_vector: input conversion error\n");
    vec[0] = tmpx;    vec[1] = tmpy;    vec[2] = tmpz;
}

local void out_int(stream str, int ival)
{
    fprintf(str, "  %d\n", ival);
}

local void out_real(stream str, real rval)
{
    fprintf(str, " %21.14E\n", rval);
}

local void out_vector(stream str, vector vec)
{
    fprintf(str, " %21.14E %21.14E %21.14E\n", vec[0], vec[1], vec[2]);
}

local void out_2vectors(stream str, vector vec1, vector vec2)
{
    fprintf(str, " %21.14E %21.14E %21.14E %21.14E %21.14E %21.14E\n", vec1[0], vec1[1], vec1[2], vec2[0], vec2[1], vec2[2]);
}

local void printvec(string name, vector vec)
{
    printf("          %10s%10.4f%10.4f%10.4f\n",
	   name, vec[0], vec[1], vec[2]);
}
