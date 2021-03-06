#ifndef MULTIPOLE_H
#define MULTIPOLE_H

// ============================================================================

#include "coordtransforms.h"
#include "potential.h"
#include "GSLInterface/GSLInterface.h"

// ============================================================================
// General density interface
// ============================================================================

class Density{
private:
    const double TargetMass;
public:
    Density(double m=0.):TargetMass(m){}

    virtual double density(const VecDoub& x){
        return 0.;
    }
    double density_sph(const VecDoub& r){
        return density(conv::SphericalPolarToCartesian(r));
    }
    double mass(void){return TargetMass;}
};

// ============================================================================
// Example Miyamoto-Nagai density interface -- to test axisymmetric
// ============================================================================

class Miyamoto_NagaiDensity: public Density{
private:
    double M,a,b;
public:

    Miyamoto_NagaiDensity(double M, double a, double b)
        :M(M),a(a),b(b){};

    double density(const VecDoub& x){
        double R2 = x[0]*x[0]+x[1]*x[1];
        double z2b2 = x[2]*x[2]+b*b;
        double azb = a+sqrt(z2b2);
        return (b*b*M*0.25/PI)*(a*R2+(a+3.*sqrt(z2b2))*azb*azb)/(pow(R2+azb*azb,2.5)*pow(z2b2,1.5));
    }
    double Phi(const VecDoub& x){
        double R2 = x[0]*x[0]+x[1]*x[1];
        double z2b2 = x[2]*x[2]+b*b;
        double azb = a+sqrt(z2b2);
        return -conv::G*M/sqrt(R2+azb*azb);
    }
};

// ============================================================================
// General Triaxial density interface
// ============================================================================

class TriaxialDensity: public Density{
private:
    VecDoub axes;
public:

    TriaxialDensity(VecDoub axes):axes(axes){};

    double get_a(int i){ return axes[i];}

    virtual double density_m2(double m){
        /* To be overridden */
        return 0.;
    }

    double density(const VecDoub& x){
        double m2 = 0.;
        for(int i=0;i<3;i++){ m2 += x[i]*x[i]/axes[i]/axes[i];}
        m2*=axes[0]*axes[0];
        return density_m2(m2);
    }
};


// ============================================================================
// Some test triaxial densities
// ============================================================================

class TestDensity_Stackel: public TriaxialDensity{
private:
    double rho0;
    StackelTriaxial *ST;
public:
    TestDensity_Stackel(double rho0, double a, double b)
        :TriaxialDensity({1.,sqrt(-b)/sqrt(-a),1./sqrt(-a)}),rho0(rho0){
            ST = new StackelTriaxial(rho0, a, b);
        };
    double density_m2(double m){
        m/=-ST->alpha();
        return rho0/(1+m)/(1+m);
    }
    double Phi(const VecDoub& x){
        return ST->Phi(x);
    }
    VecDoub Forces(const VecDoub& x){
        return ST->Forces(x);
    }
};

class Density_NFW: public TriaxialDensity{
private:
    double GM, rs;
    VecDoub a;
    double rc;
public:
    Density_NFW(double GM, double rs, VecDoub a, double rc)
        :TriaxialDensity(a),GM(GM),rs(rs),a(a),rc(rc){};
    double density_m2(double m){
        double sqm = sqrt(m);
        return GM/(sqm+rc)/(sqm+rs)/(sqm+rs)/conv::FPG;
    }
    double Phi(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+
                        x[1]*x[1]/a[1]/a[1]+
                        x[2]*x[2]/a[2]/a[2]);
        return -GM*log(1.+r/rs)/r;
    }
};

class TestDensity_NFW: public TriaxialDensity{
private:
    double GM, rs;
    VecDoub a;
public:
    TestDensity_NFW(double GM, double rs)
        :TriaxialDensity({1.,0.7,0.3}),GM(GM),rs(rs),a(a){};
    double density_m2(double m){
        double sqm = sqrt(m);
        return GM/sqm/(sqm+rs)/(sqm+rs)/conv::FPG;
    }
    double Phi(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+
                        x[1]*x[1]/a[1]/a[1]+
                        x[2]*x[2]/a[2]/a[2]);
        return -GM*log(1.+r/rs)/r;
    }
};

class TestDensity_Hernquist: public TriaxialDensity{
private:
    double GM, rs;
    VecDoub a;
public:
    TestDensity_Hernquist(double GM, double rs, VecDoub a)
        :TriaxialDensity(a),GM(GM/(a[0]*a[1]*a[2])),rs(rs),a(a){};
    double density_m2(double m){
        double sqm = sqrt(m);
        return 2.*GM*rs/sqm/(sqm+rs)/(sqm+rs)/(sqm+rs)/conv::FPG;
    }
    double Phi(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+x[1]*x[1]/a[1]/a[1]+x[2]*x[2]/a[2]/a[2]);
        return -GM/(r+rs);
    }
    VecDoub Forces(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+x[1]*x[1]/a[1]/a[1]+x[2]*x[2]/a[2]/a[2]);
        double P = -GM/(r+rs)/(r+rs)/r;
        return {x[0]*P,x[1]*P,x[2]*P};
    }
};

class TestDensity_Isochrone: public TriaxialDensity{
private:
    double GM, rs;
    VecDoub a;
public:
    TestDensity_Isochrone(double GM, double rs, VecDoub a)
        :TriaxialDensity(a),GM(GM/(a[0]*a[1]*a[2])),rs(rs),a(a){};
    double density_m2(double m){
        double ra = sqrt(m+rs*rs),r=sqrt(m);
        return GM*(3*(rs+ra)*ra*ra-r*r*(rs+3*ra))/pow(ra*(rs+ra),3)/conv::FPG;
    }
    double Phi(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+x[1]*x[1]/a[1]/a[1]+x[2]*x[2]/a[2]/a[2]+rs*rs);
        return -GM/(rs+r);
    }
    VecDoub Forces(const VecDoub& x){
        double r = sqrt(x[0]*x[0]/a[0]/a[0]+x[1]*x[1]/a[1]/a[1]+x[2]*x[2]/a[2]/a[2]+rs*rs);
        r = 1./(r+rs);
        double P = -GM*r*r;
        return {x[0]*P,x[1]*P,x[2]*P};
    }
};


// ============================================================================



// ============================================================================
// General Triaxial Potential interface
// -- takes a Triaxial Density object
// ============================================================================

class TriaxialPotential: public Potential_JS{
private:
    TriaxialDensity *TD;
    double psi_inf;
    integrator GL;
public:
    TriaxialPotential(TriaxialDensity *TD, double rmax = 1e3):TD(TD),GL(100000){
        // set maximum radius with rmax
        psi_inf=psi_m(rmax);
    }
    inline double density_m2(double x){ return TD->density_m2(x);}
    inline double get_psi_inf(void){ return psi_inf;}
    inline double get_a(int i){ return TD->get_a(i);}
    double psi_m(double m);
    double Phi(const VecDoub& x);
    VecDoub Forces(const VecDoub& x);
};

struct phi_integrand_st{
    TriaxialPotential *TP;
    VecDoub x;
    int index;
    phi_integrand_st(TriaxialPotential *TP, VecDoub x, int index):TP(TP), x(x), index(index){};
};


// ============================================================================
// Multipole expansion
// ============================================================================


class MultipoleExpansion: public Potential_JS{
private:
    Density *rho;
    int NR, NA, NA_theta, NA_phi, LMAX, MMAX;
    double a0, rmin, rmax;
    bool axisymmetric, triaxial, flip;
    VecDoub radial_grid, delta_grid;
    std::vector<std::vector<VecDoub>> Phi_grid, rho_grid, dPhi_grid;
    std::unique_ptr<GaussLegendreIntegrator> GLtheta;
    std::unique_ptr<GaussLegendreIntegrator> GLphi;
    // Can use a logarithmically-spaced grid or a sinh-spaced
    const bool loggrid = true;
    const double err;

    void fill_radial_grid(double aa0, double rrmin, double rrmax);
    void add_radial_gridpoint(void);
    double rholm_s(int i, int l, int m);
    void fill_density_grid(void);
    void extend_density_grid(void);

public:
    void fillPhigrid();
    MultipoleExpansion(void):err(0.){};
    virtual ~MultipoleExpansion(void){};
    MultipoleExpansion(Density *rho, int NR = 50, int NA = 8, int LMAX = -1, int MMAX = -1, double a0=1.,double rmin=0.01,double rmax=100., bool axisymmetric = false, bool triaxial = false, bool flip = true, double err=0.);
    MultipoleExpansion(const std::string&);
    void output_to_file(const std::string&);
    void visualize(const std::string&);
    double Phi(const VecDoub& x);
    VecDoub Forces(const VecDoub& x);
    inline VecDoub const &get_radial_grid() const {return radial_grid; }
    inline double innerradius(void) const {return rmin;}
    inline double outerradius(void) const {return rmax;}
    inline double scaleradius(void) const {return a0;}
    inline int nradial(void) const {return NR;}
    inline int nangular(void) const {return NA;}
    inline int lmax(void) const {return LMAX;}
    inline int mmax(void) const {return MMAX;}
};

class MultipoleExpansion_UpDownSymmetric: public MultipoleExpansion{
public:
    MultipoleExpansion_UpDownSymmetric(Density *rho, int NR = 50, int NA = 8, int LMAX = -1, int MMAX = -1, double a0=1.,double rmin=0.01,double rmax=100.)
     :MultipoleExpansion(rho,NR,NA,LMAX,MMAX,a0,rmin,rmax,false,false,true){};
    MultipoleExpansion_UpDownSymmetric(const std::string& s)
     :MultipoleExpansion(s){}
};

class MultipoleExpansion_Triaxial: public MultipoleExpansion{
public:
    MultipoleExpansion_Triaxial(Density *rho, int NR = 50, int NA = 8, int LMAX = -1, int MMAX = -1, double a0=1.,double rmin=0.01,double rmax=100.,double err=0.01)
     :MultipoleExpansion(rho,NR,NA,LMAX,MMAX,a0,rmin,rmax,false,true,true,err){};
    MultipoleExpansion_Triaxial(const std::string& s)
     :MultipoleExpansion(s){}
};

class MultipoleExpansion_Axisymmetric: public MultipoleExpansion{
public:
    MultipoleExpansion_Axisymmetric(Density *rho, int NR = 50, int NA = 8, int LMAX = -1, double a0=1.,double rmin=0.01,double rmax=100.,double err=0.01)
     :MultipoleExpansion(rho,NR,NA,LMAX,-1,a0,rmin,rmax,true,true,true,err){};
    MultipoleExpansion_Axisymmetric(const std::string& s)
     :MultipoleExpansion(s){}
};

class MultipoleExpansion_Spherical: public MultipoleExpansion{
public:
    MultipoleExpansion_Spherical(Density *rho, int NR = 50, double a0=1.,double rmin=0.01,double rmax=100.,double err=0.01)
     :MultipoleExpansion(rho,NR,1,1,0,a0,rmin,rmax,true,true,true,err){};
    MultipoleExpansion_Spherical(const std::string& s)
     :MultipoleExpansion(s){}
};

class MultipoleExpansion_SphericalPotential: public SphericalPotential{
private:
    MultipoleExpansion_Spherical *MES;
public:
    MultipoleExpansion_SphericalPotential(Density *rho, int NR = 50, double a0=1.,double rmin=0.01,double rmax=100.)
     :MES(new MultipoleExpansion_Spherical(rho,NR,a0,rmin,rmax)){};
    MultipoleExpansion_SphericalPotential(const std::string& s)
     :MES(new MultipoleExpansion_Spherical(s)){}
    ~MultipoleExpansion_SphericalPotential(){delete MES;}
    double Phi_r(double r){return MES->Phi({r,0.,0.});}
    double dPhi_r(double r){return -MES->Forces({r,0.,0.})[0];}
    inline void fillPhigrid(){return MES->fillPhigrid();};
    inline void output_to_file(const std::string& s){return MES->output_to_file(s);}
    inline void visualize(const std::string& s){return MES->visualize(s);}
    inline VecDoub const &get_radial_grid() const {return MES->get_radial_grid(); }
    inline double innerradius(void) const {return MES->innerradius();}
    inline double outerradius(void) const {return MES->outerradius();}
    inline double scaleradius(void) const {return MES->scaleradius();}
    inline int nradial(void) const {return MES->nradial();}
    inline int nangular(void) const {return MES->nangular();}
    inline int lmax(void) const {return MES->lmax();}
    inline int mmax(void) const {return MES->mmax();}
};


#endif
// ============================================================================
