#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "GSLInterface/GSLInterface.h"
#include "gnuplot/gnuplot_i.h"
#include <gsl/gsl_poly.h>
#include "falPot.h"
#include "utils.h"
#include "coordsys.h"
#include "coordtransforms.h"
#include "potential.h"
#include "orbit.h"
#include "stackel_aa.h"
#include "get_closed_Rz.h"
#include "uv_orb.h"
#include "debug.h"

// ============================================================================

void uv_orb::readDeltagrids(const std::string& file){
	std::ifstream infile; infile.open(file);
	if(!infile.is_open())std::cerr<<"Problem: "<<file<<" doesn't exist."<<std::endl;
	E_delta = VecDoub(NE,0.);
	L_delta = std::vector<VecDoub>(NE,VecDoub(NL,0.));
	Delta = L_delta;
	for(int i=0;i<NE;++i) infile>>E_delta[i];
	for(int i=0;i<NE;++i)for(int j=0;j<NL;++j) infile>>L_delta[i][j];
	for(int i=0;i<NE;++i)for(int j=0;j<NL;++j) infile>>Delta[i][j];
	infile.close();
}

void uv_orb::fillDeltagrids(const std::string& file){

	E_delta.clear();L_delta.clear();Delta.clear();
    E_delta = create_range(E0,Emax,NE);
    L_delta = std::vector<VecDoub>(NE,VecDoub(NL,0.));
    Delta = std::vector<VecDoub>(NE,VecDoub(NL,0.));
    #pragma omp parallel for schedule (dynamic)
    for(int i=0; i<NE;i++){
        double R = Pot->R_E(E_delta[i]);
        for(double j=0;j<NL;j++){
            L_delta[i][j] = Pot->L_circ(R)*(j*0.8/(double)(NL-1)+0.001);
            find_best_delta DD(Pot, E_delta[i], L_delta[i][j]);
            Delta[i][j]=DD.delta(R*.9);
            if(Delta[i][j]<0. or Delta[i][j]!=Delta[i][j])
            	Delta[i][j]=R/5.;
            std::cerr<<"DeltaGrid: "
            	<<OUTPUT(E_delta[i])
            	<<OUTPUT(L_delta[i][j])
            	<<OUTPUT(R)
            	<<OUTPUTE(Delta[i][j]);
        }
    }
    std::cerr<<"DeltaGrid calculated: NE = "<<NE<<", NL = "<<NL<<std::endl;

    std::ofstream outfile; outfile.open(file);
	for(int i=0;i<NE;++i)
		for(int j=0;j<NL;++j)
		outfile<<E_delta[i]<<" "<<L_delta[i][j]<<" "<<Delta[i][j]<<std::endl;
	outfile.close();

}

double uv_orb::findDelta_interp(double E, double L){
    int E_bot=0, E_top=0, L_bot1=0, L_bot2=0, L_top1=0, L_top2=0;
    VecDoub t = L_delta[0];
    double delta1, delta2;
    if(E<E_delta[0]){ E_bot=0; E_top=0;}
    else if(E>E_delta[NE-1]){ E_bot = NE-1; E_top = NE-1;}
    else topbottom<double>(E_delta,E,&E_bot,&E_top,"Find Delta E grid");

    if(L<L_delta[E_bot][0]) delta1 = Delta[E_bot][0];
    else if(L>L_delta[E_bot][NL-1]) delta1 = Delta[E_bot][NL-1];
    else{
        topbottom<double>(L_delta[E_bot],L,&L_bot1,&L_top1);
        delta1 = Delta[E_bot][L_bot1]+(L-L_delta[E_bot][L_bot1])*(Delta[E_bot][L_top1]-Delta[E_bot][L_bot1])/(L_delta[E_bot][L_top1]-L_delta[E_bot][L_bot1]);
    }

    if(L<L_delta[E_top][0]) delta2 = Delta[E_top][0];
    else if(L>L_delta[E_top][NL-1]) delta2 = Delta[E_top][NL-1];
    else{
        topbottom<double>(L_delta[E_top],L,&L_bot2,&L_top2);
        delta2 = Delta[E_top][L_bot2]+(L-L_delta[E_top][L_bot2])*(Delta[E_top][L_top2]-Delta[E_top][L_bot2])/(L_delta[E_top][L_top2]-L_delta[E_top][L_bot2]);
    }

    if(E_bot!=E_top) delta1+=(delta2-delta1)*(E-E_delta[E_bot])/(E_delta[E_top]-E_delta[E_bot]);
    return delta1;
}

VecDoub uv_orb::actions(const VecDoub& x,void*params){

	double En = Pot->H(x), Lz = Pot->Lz(x);
	if(params!=nullptr){
		double *alphabeta = (double*)params;
		Actions_AxisymmetricStackel_Fudge ATSF(Pot,alphabeta[0]);
		return ATSF.actions(x);
	}
	else{
		double dd = findDelta_interp(En,fabs(Lz));
		Actions_AxisymmetricStackel_Fudge ATSF(Pot,-1.-dd*dd);
		return ATSF.actions(x);
	}
}

VecDoub uv_orb::angles(const VecDoub& x,void *params){
	double En = Pot->H(x), Lz = Pot->Lz(x);
	if(params!=nullptr){
		double *alphabeta = (double*)params;
		Actions_AxisymmetricStackel_Fudge ATSF(Pot,alphabeta[0]);
		return ATSF.angles(x);
	}
	else{
		double dd = findDelta_interp(En,fabs(Lz));
		Actions_AxisymmetricStackel_Fudge ATSF(Pot,-1.-dd*dd);
		return ATSF.angles(x);
	}
}

// ============================================================================
