/*
	Suave.c
		Subregion-adaptive Vegas Monte-Carlo integration
		by Thomas Hahn
		last modified 22 Jul 14 th
*/


#define SUAVE
#define ROUTINE "Suave"

#include "decl.h"
#include "CSample.c"

/*********************************************************************/

Extern void EXPORT(Suave)(ccount ndim, ccount ncomp,
  Integrand integrand, void *userdata, cnumber nvec,
  creal epsrel, creal epsabs,
  cint flags, cint seed,
  cnumber mineval, cnumber maxeval,
  cnumber nnew, creal flatness,
  cchar *statefile, Spin **pspin,
  count *pnregions, number *pneval, int *pfail,
  real *integral, real *error, real *prob)
{
  This t;

  VerboseInit();

  t.ndim = ndim;
  t.ncomp = ncomp;
  t.integrand = integrand;
  t.userdata = userdata;
  t.nvec = nvec;
  t.epsrel = epsrel;
  t.epsabs = epsabs;
  t.flags = MaxVerbose(flags);
  t.seed = seed;
  t.mineval = mineval;
  t.maxeval = maxeval;
  t.nnew = nnew;
  t.flatness = flatness;
  t.statefile = statefile;
  FORK_ONLY(t.spin = Invalid(pspin) ? NULL : *pspin;)

  *pfail = Integrate(&t, integral, error, prob);
  *pnregions = t.nregions;
  *pneval = t.neval;

  WaitCores(&t, pspin);
}

/*********************************************************************/

Extern void EXPORT(suave)(ccount *pndim, ccount *pncomp,
  Integrand integrand, void *userdata, cnumber *pnvec,
  creal *pepsrel, creal *pepsabs,
  cint *pflags, cint *pseed,
  cnumber *pmineval, cnumber *pmaxeval,
  cnumber *pnnew, creal *pflatness,
  cchar *statefile, Spin **pspin,
  count *pnregions, number *pneval, int *pfail,
  real *integral, real *error, real *prob, cint statefilelen)
{
  This t;

  VerboseInit();

  t.ndim = *pndim;
  t.ncomp = *pncomp;
  t.integrand = integrand;
  t.userdata = userdata;
  t.nvec = *pnvec;
  t.epsrel = *pepsrel;
  t.epsabs = *pepsabs;
  t.flags = MaxVerbose(*pflags);
  t.seed = *pseed;
  t.mineval = *pmineval;
  t.maxeval = *pmaxeval;
  t.nnew = *pnnew;
  t.flatness = *pflatness;
  CString(t.statefile, statefile, statefilelen);
  FORK_ONLY(t.spin = Invalid(pspin) ? NULL : *pspin;)

  *pfail = Integrate(&t, integral, error, prob);
  *pnregions = t.nregions;
  *pneval = t.neval;

  WaitCores(&t, pspin);
}

