#include "potential.h"
#include "utils.h"
#include "jamestools/numrec/press.h"
#include "get_closed_Rz.h"

static double ds2dv0(double v, void*p){
    closed_approach_st *P = (closed_approach_st*)p;
    // Equation (31)
    return 2*(P->z*P->DS->sqDR*sin(v)
              -cos(v)*(P->R*P->DS->R0+P->DS->D2*sin(v)));
}

static double ds2totaldD2(double D2,void*p){
    // sums ds2dD2 for closest v points to each R,z
    delta_st *DD = (delta_st *)p;
    double sum=0; DD->D2 = D2;DD->sqDR=sqrt(DD->D2+DD->R0*DD->R0);
    for(int i=0; i<DD->N; i++){
        DD->get_v0(DD->Rp[i],DD->zp[i]);
        sum+=DD->ds2dD2(DD->Rp[i],DD->zp[i]);
    }
    return sum;
}


delta_st::delta_st(double R0,double *Ri, double *zi,int N):N(N),R0(R0){
    Rp = new double[N]; zp = new double[N];
    for(int i=0;i<N; i++){
        Rp[i]=Ri[i]; zp[i]=zi[i];
    }
}

double delta_st::s2(double v, double R, double z){
    // Equation (30)
    return pow(R0*sin(v)-R,2)+pow(sqDR*cos(v)-z,2);
}

void delta_st::get_v0(double Ri,double zi){
    //returns v of closest approach to (R,z)
    //solves equation (31) for v via Brent's method
    closed_approach_st P(this,Ri,zi);
    if(ds2dv0(0,&P)*ds2dv0(PIH,&P)>0){
        printf("In get_v0 %f %f %f %f %f",Ri,zi,sqDR,Ri,R0);
    }
    root_find RF(1e-10,25);
    v0=RF.findroot(&ds2dv0,0,PIH,&P);
    sv=sin(v0); cv=cos(v0);
}

double delta_st::dv0dD2(double R, double z){
    // equation (32)
    return .5*sv*(z/sqDR-2*cv)/(cv*z*sqDR+sv*R*R0-D2*(cv*cv-sv*sv));
}
double delta_st::ds2dD2(double R, double z){
    // equation (33)
    closed_approach_st P(this,R,z);
    return (cv-z/sqDR)*cv+ds2dv0(v0,&P)*dv0dD2(R,z);
}

double delta_st::get_delta2(void){
    double D2min=-.001,D2max=5;
    double dsdDmin=ds2totaldD2(D2min,this),dsdDmax=ds2totaldD2(D2max,this);
    while(dsdDmin*dsdDmax>0){
        if(dsdDmax<0){
            D2max*=5; dsdDmax=ds2totaldD2(D2max,this);
        }else{
            D2min-=.1; dsdDmin=ds2totaldD2(D2min,this);
        }
    }
    if(dsdDmin*dsdDmax<0){
        root_find RF(1e-10,25);
        D2=RF.findroot(&ds2totaldD2,D2min,D2max,this);
    }
    return D2;
}

static void derivs(double t,double *y,double *dydt, void*params){
    find_best_delta *P = (find_best_delta *) params;
    double dP[2];
    VecDoub f = P->Pot->Forces({y[0],0.,y[1]});
    dP[0]=-f[0];dP[1]=-f[2];
    dP[0]-=P->Lzsq/pow(y[0],3);
    for(int i=0;i<2;i++){
        dydt[i]=y[i+2];
        dydt[i+2]=-dP[i];
    }
}

static double goround(double x, void *params){//throws up and determines Rdown
    find_best_delta *par = (find_best_delta *) params;
    double vsq=par->vsqx(x);
    if(vsq<0){
        printf("%f %f\n",x,vsq); exit(0);
    }
    double x2[4]={x,0.,0,sqrt(vsq)};
    double dxdt[4],xscal[4],t,hdid,hnext,htry=2.e-2,eps=1.e-10;
    t=0;
    for(int i=0; i<2; i++){
        xscal[i]=x; xscal[i+2]=fabs(x2[3]);
    }
    double Rlast=0.,zlast=0.;
    while(x2[1]>=0){
        Rlast=x2[0]; zlast=x2[1];
        derivs(t,x2,dxdt,par);
        rkqs(x2,dxdt,4,&t,htry,eps,xscal,&hdid,&hnext,&derivs,par);
        htry=hnext;
    }
    return x-(x2[0]-x2[1]/(zlast-x2[1])*(Rlast-x2[0]));//difference between up and down
}

double find_best_delta::dPhi_eff(double x){
    return -Pot->Forces({x,0.,0.})[0]-Lzsq/pow(x,3);
}

double find_best_delta::vsqx(double x){
    return 2*(E-(Pot->Phi({x,0.,0.})+.5*Lzsq/(x*x)));
}

double find_best_delta::Ez(double *x2){
    return .5*(pow(x2[2],2)+pow(x2[3],2))+.5*Lzsq/pow(x2[0],2)+Pot->Phi({x2[0],0.,x2[1]});
}

int find_best_delta::go_up(double x,double *Ri,double *zi,int nmaxR){//throws up and determines Rdown
    double vsq=vsqx(x);
    if(vsq<0){
        printf("%f %f\n",x,vsq); exit(0);
    }
    double x2[4]={x,0,0,sqrt(vsq)};
    double dxdt[4],xscal[4],t,hdid,hnext,htry=2.e-2,eps=1.e-10;
    t=0;
    for(int i=0; i<2; i++){
        xscal[i]=x; xscal[i+2]=fabs(x2[3]);
    }
    int j=0;
    while(x2[3]>=0){
        derivs(t,x2,dxdt,this);
        rkqs(x2,dxdt,4,&t,htry,eps,xscal,&hdid,&hnext,&derivs,this);
        htry=hnext;
        Ri[j]=x2[0]; zi[j]=x2[1]; j++; if(j==nmaxR) break;
    }
    // std::cout<<zi[j]<<" ";
    //if(Ri[0]<zi[j-1]) setcolour("black"); else setcolour("red");
    return j;
}

double find_best_delta::sorted(double x0){
    int k=0;
    while(vsqx(x0)<0){
        if(dPhi_eff(x0)<0) x0*=1.1;
        else x0*=.9;
        k++; if(k>50){ printf("vsq: %f %f %f\n",x0,E,Lzsq); exit(0);}
    }
    return x0;
}

double find_best_delta::delta(double x0){
    // finds closed shell

    x0=sorted(x0);// ensure vsq>=0
    double x1,dx0,dx1;
    dx0=goround(x0,this);
    double dx=.8; x1=sorted(x0/dx); dx1=goround(x1,this);
    int k=0,bo=0;
    while(dx0*dx1>0){//we haven't bracketed the root
        if(fabs(dx1)<fabs(dx0)){//more of same
            x0=x1; dx0=dx1;
        }else{// back off
            if(bo==0){
                dx=1/dx; bo=1;
            }else dx=pow(dx,-0.9);
        }
        x1=sorted(x0*dx);dx1=goround(x1,this);
        k++;
        if(k==21){
            printf("get_closed: %f %f %g %g %g\n",x0,x1,dx0,dx1,dx);
            if(fabs(dx0)<1.e-7){
                return dx0;
            }else{
                printf("problem in get_closed(): %f %f %f %f\n",x0,x1,dx0,dx1);
                return -1;
            }
        }
    }
    root_find RF(1e-4,100);
    if(x1<x0){double tmp=x1;x1=x0;x0=tmp;}
    double R0=RF.findroot(&goround,x0,x1,this);
    double Ri[nmaxRi],zi[nmaxRi];
    int np=go_up(R0,Ri,zi,nmaxRi);
    // and fits ellipse
    delta_st DD(R0,Ri,zi,np);
    double max=0.;
    for(int i=0;i<np;i++) if(zi[i]>max) max = zi[i];
    return sqrt(DD.get_delta2());
}



