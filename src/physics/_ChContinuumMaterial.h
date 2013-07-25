#ifndef CHCONTINUUMMATERIAL_H
#define CHCONTINUUMMATERIAL_H

//////////////////////////////////////////////////
//  
//   ChContinuumMaterial.h
//
//   Class for material of elastoplatic continuum
//
//
//   HEADER file for CHRONO,
//	 Multibody dynamics engine
//
// ------------------------------------------------
// 	 Copyright:Alessandro Tasora / DeltaKnowledge
//             www.deltaknowledge.com
// ------------------------------------------------
///////////////////////////////////////////////////


#include <stdlib.h>
#include "core/ChApiCE.h"
#include "core/ChMath.h"
#include "core/ChShared.h"

namespace chrono 
{


/// Base class for stress and strain tensors, in compact Voight notation
/// that is with 6 components in a column. This saves some
/// memory when compared to traditional 3D tensors with three
/// rows and three columns, that are symmetric.

template <class Real = double>
class ChApi ChVoightTensor : public ChMatrixNM<Real,6,1>
{
public:
					/// Constructors (default empty)
	ChVoightTensor()  {};

					/// Copy constructor, from a typical 3D rank-two stress or strain tensor (as 3x3 matrix)
	template <class RealB>
	inline ChVoightTensor (const ChMatrix33<RealB>& msource) 
					  {
						 this->ConvertFromMatrix(msource);
					  };

	inline Real& XX()   { return ChMatrix<Real>::ElementN(0); };
	inline const Real& XX () const { return ChMatrix<Real>::ElementN(0); };

	inline Real& YY()   { return ChMatrix<Real>::ElementN(1); };
	inline const Real& YY () const { return ChMatrix<Real>::ElementN(1); };

	inline Real& ZZ()   { return ChMatrix<Real>::ElementN(2); };
	inline const Real& ZZ () const { return ChMatrix<Real>::ElementN(2); };

	inline Real& XY()   { return ChMatrix<Real>::ElementN(3); };
	inline const Real& XY () const { return ChMatrix<Real>::ElementN(3); };

	inline Real& XZ()   { return ChMatrix<Real>::ElementN(4); };
	inline const Real& XZ () const { return ChMatrix<Real>::ElementN(4); };

	inline Real& YZ()   { return ChMatrix<Real>::ElementN(5); };
	inline const Real& YZ () const { return ChMatrix<Real>::ElementN(5); };
		
			/// Convert from a typical 3D rank-two stress or strain tensor (a 3x3 matrix)
	template <class RealB>
	void ConvertFromMatrix(const ChMatrix33<RealB>& msource)
					{
						 XX()= (Real)msource(0,0);
						 YY()= (Real)msource(1,1);
 						 ZZ()= (Real)msource(2,2);
						 XY()= (Real)msource(0,1);
						 XZ()= (Real)msource(0,2);
						 YZ()= (Real)msource(1,2);
					};

			/// Convert to a typical 3D rank-two stress or strain tensor (a 3x3 matrix)
	template <class RealB>
	void ConvertToMatrix(ChMatrix33<RealB>& mdest)
					{
						 mdest(0,0) = (RealB)XX();
						 mdest(1,1) = (RealB)YY();
 						 mdest(2,2) = (RealB)ZZ();
						 mdest(0,1) = (RealB)XY();
						 mdest(0,2) = (RealB)XZ();
						 mdest(1,2) = (RealB)YZ();
						 mdest(1,0) = (RealB)XY();
						 mdest(2,0) = (RealB)XZ();
						 mdest(2,1) = (RealB)YZ();
					};

			/// Compute the volumetric part of the tensor, that is 
			/// the trace V =Txx+Tyy+Tzz.
	Real GetVolumetricPart() const
					{
						return XX()+YY()+ZZ();
					}
			/// Compute the deviatoric part of the tensor, storing
			/// it in mdeviatoric
	void GetDeviatoricPart(ChVoightTensor<Real>& mdeviatoric) const
					{
						Real mM = GetVolumetricPart()/3.0;
						mdeviatoric = *this;
						mdeviatoric.XX() -= mM;
						mdeviatoric.YY() -= mM;
						mdeviatoric.ZZ() -= mM;
					}

			/// Compute the I1 invariant
	Real GetInvariant_I1() const
			{
				return XX()+YY()+ZZ();
			}

			/// Compute the I2 invariant
	Real GetInvariant_I2() const
			{
				return XX()*YY() + YY()*ZZ() + XX()*ZZ() 
					 - XY()*XY() - YZ()*YZ() - XZ()*XZ();
			}

			/// Compute the I3 invariant
	Real GetInvariant_I3() const
			{
				return XX()*YY()*ZZ() + 2*XY()*YZ()*XZ() 
					  -XY()*XY()*ZZ() - YZ()*YZ()*XX() - XZ()*XZ()*YY();
			}

			/// Compute the J1 invariant of the deviatoric part (that is always 0)
	Real GetInvariant_J1() const { return 0;}
			 
			/// Compute the J2 invariant of the deviatoric part
	Real GetInvariant_J2() const
	{
		return ChMax(0.0,  ((pow(this->GetInvariant_I1(),2))/3.0) -this->GetInvariant_I2() );
	}
			/// Compute the J3 invariant of the deviatoric part
	Real GetInvariant_J3() const
	{
		return ( pow(this->GetInvariant_I1(),3)*(2./27.)
			        -this->GetInvariant_I1()*this->GetInvariant_I2()*(1./3.)
					+this->GetInvariant_I3() );
	}

		  /// FORMULAS THAT ARE USEFUL FOR YELD CRITERIONS: 

			/// Compute the Von Mises equivalent
	double GetEquivalentVonMises() const
			{	
				return sqrt( 0.5*(pow(this->XX()-this->YY(),2.) + pow(this->YY()-this->ZZ(),2.) + pow(this->ZZ()-this->XX(),2.)) + 3.0*( this->XY()*this->XY() + this->XZ()*this->XZ() + this->YZ()*this->YZ()) );
			}

};

/// Class for stress tensors, in compact Voight notation
/// that is with 6 components in a column. 

template <class Real = double>
class ChApi ChStressTensor : public ChVoightTensor<Real>
{
public:

};


/// Class for strain tensors, in compact Voight notation
/// that is with 6 components in a column. 

template <class Real = double>
class ChApi ChStrainTensor : public ChVoightTensor<Real>
{
public:

};





/// Base class for properties of materials 
/// in a continuum. 

class ChApi ChContinuumMaterial : public ChShared
{
private:
	double density;

public:

	ChContinuumMaterial(double mdensity=1000) : density(mdensity) {};

	virtual ~ChContinuumMaterial() {};
	
			/// Set the density of the material, in kg/m^2.
	void   Set_density (double m_density) {density = m_density;}
			/// Get the density of the material, in kg/m^2.
	double Get_density () {return density;}

				/// Method to allow deserializing
	void StreamIN(ChStreamInBinary& mstream);

				/// Method to allow serializing 
	void StreamOUT(ChStreamOutBinary& mstream);

};



/// Class for the basic properties of materials 
/// in an elastic continuum. 
/// This is a base material with isothropic hookean 
/// elasticity.

class ChApi ChContinuumElastic : public ChContinuumMaterial
{
private:

	double E;			// Young Modulus
	double v;			// Poisson ratio
	double G;			// shear modulus
	double l;			// Lame's modulus

public:

			/// Create a continuum isothropic hookean material. 
			/// Default value for Young elastic modulus is low (like a 
			/// rubber-type material), and same for density.
	ChContinuumElastic(double myoung = 10000000, double mpoisson=0.4, double mdensity=1000);

	virtual ~ChContinuumElastic();
	

			/// Set the Young E elastic modulus, in Pa (N/m^2), as the ratio of the uniaxial 
			/// stress over the uniaxial strain, for hookean materials. Intuitively, the 
			/// tensile pressure on a side of a parallelepiped in order to double its length.
			/// Note that most metal materials require very high values, ex. steel 
			/// has E=210GPa (E=210e9), aluminium E=69e9, and this can cause numerical 
			/// problems if you do not set up the simulation integrator properly.
	void   Set_E (double m_E);
			/// Get the Young E elastic modulus, in Pa (N/m^2).
	double Get_E () {return E;}


			/// Set the Poisson v ratio, as v=-transverse_strain/axial_strain, so
			/// takes into account the 'squeezing' effect of materials that are pulled (so,
			/// if zero, when you push the two sizes of a cube, it won't inflate). Most
			/// materials have some 0<v<0.5, for example steel has v=0.27..0.30, aluminium v=0.33,
			/// rubber=0.49, etc. Note! v=0.5 means perfectly incompressible material, that 
			/// could give problems with some type of solvers.
			/// Setting v also changes G.
	void   Set_v (double m_v);
			/// Get the Young v ratio, as v=-transverse_strain/axial_strain.
	double Get_v () {return v;}

			/// Set the shear modulus G, in Pa (N/m^2), as the ratio of shear stress to 
			/// the shear strain. Setting G also changes Poisson ratio v.
	void   Set_G (double m_G);
			/// Get the shear modulus G, in Pa (N/m^2)
	double Get_G () {return G;}

			/// Get Lam� first parameter (the second is shear modulus, so Get_G() )
	double Get_l () {return l;}

			/// Get bulk modulus (increase of pressure for decrease of volume), in Pa. 
	double Get_BulkModulus () {return E/(3.*(1.-2.*v));}

			/// Get P-wave modulus (if V=speed of propagation of a P-wave, then (M/density)=V^2 ) 
	double Get_WaveModulus () {return E*((1.-v)/(1.+v)*(1.-2.*v));}


			/// Compute elastic stress from elastic strain
			/// (using column tensors, in Voight notation)
	void ComputeElasticStress(ChStressTensor<>& mstress, const ChStrainTensor<>& mstrain) const
				{
					mstress.XX() = mstrain.XX() * (l+2*G) + mstrain.YY() * l       + mstrain.ZZ() * l;
					mstress.YY() = mstrain.XX() * l		  + mstrain.YY() * (l+2*G) + mstrain.ZZ() * l;
					mstress.ZZ() = mstrain.XX() * l       + mstrain.YY() * l	   + mstrain.ZZ() * (l+2*G);
					mstress.XY() = mstrain.XY() * 2*G;
					mstress.XZ() = mstrain.XZ() * 2*G;
					mstress.YZ() = mstrain.YZ() * 2*G;
				}

			/// Compute elastic strain from elastic stress
			/// (using column tensors, in Voight notation)
	void ComputeElasticStrain(ChStrainTensor<>& mstrain, const ChStressTensor<>& mstress) const
				{
					double invE = 1./E;
					double invhG = 0.5/G;
					mstrain.XX() = invE*( mstress.XX()		- mstress.YY() * v	- mstress.ZZ() * v);
					mstrain.YY() = invE*(-mstress.XX() * v	+ mstress.YY()		- mstress.ZZ() * v);
					mstrain.ZZ() = invE*(-mstress.XX() * v	- mstress.YY() * v	+ mstress.ZZ()    );
					mstrain.XY() = mstress.XY() * invhG;
					mstrain.XZ() = mstress.XZ() * invhG;
					mstrain.YZ() = mstress.YZ() * invhG;
				}


			//
			// STREAMING
			//


				/// Method to allow deserializing a persistent binary archive (ex: a file)
				/// into transient data.
	void StreamIN(ChStreamInBinary& mstream);

				/// Method to allow serializing transient data into a persistent
				/// binary archive (ex: a file).
	void StreamOUT(ChStreamOutBinary& mstream);


};





/// Class for all elastic materials that can undergo plastic flow
/// Defines simply some interface functions.

class ChApi ChContinuumElastoplastic : public ChContinuumElastic
{
public:
	ChContinuumElastoplastic(double myoung = 10000000, double mpoisson=0.4, double mdensity=1000) : ChContinuumElastic(myoung,mpoisson,mdensity) {};

			/// Return a scalar value that is 0 on the yeld surface, <0 inside (elastic), >0 outside (incompatible->plastic flow)
	virtual double ComputeYeldFunction(const ChStressTensor<>& mstress) const = 0;

			/// Compute plastic strain flow (flow derivative dE_plast/dt) from strain,
			/// according to VonMises strain yeld theory. 
	virtual void ComputePlasticStrainFlow(ChStrainTensor<>& mplasticstrainflow, const ChStrainTensor<>& mtotstrain) const =0;

			/// Correct the strain-stress by enforcing that elastic stress must remain on the yeld 
			/// surface, computing a plastic flow to be added to plastic strain while integrating.
	virtual void ComputeReturnMapping(ChStrainTensor<>& mplasticstrainflow, 
									const ChStrainTensor<>&	mincrementstrain, 
									const ChStrainTensor<>& mlastelasticstrain,
									const ChStrainTensor<>& mlastplasticstrain) const = 0;

			/// Set the plastic flow rate, i.e. the 'creep' speed. The lower the value, the slower 
			/// the plastic flow during dynamic simulations, with delayed plasticity. 
	virtual void   Set_flow_rate (double mflow_rate)=0;
			/// Set the plastic flow rate.
	virtual double Get_flow_rate ()=0;
};





/// Class for the basic properties of materials 
/// in an elastoplastic continuum, with strain yeld limit
/// based on Von Mises yeld

class ChApi ChContinuumPlasticVonMises : public ChContinuumElastoplastic
{
private:

	double elastic_yeld;
	double plastic_yeld;
	
	double flow_rate;

public:

			/// Create a continuum isothropic elastoplastic material,
			/// where you can define also plastic and elastic max. stress (yeld limits
			/// for transition elastic->blastic and plastic->fracture).
	ChContinuumPlasticVonMises(double myoung = 10000000, double mpoisson=0.4, double mdensity=1000,
							double melastic_yeld = 0.1, double  mplastic_yeld = 0.2);

	virtual ~ChContinuumPlasticVonMises() {};
	

			/// Set the elastic yeld modulus as the maximum VonMises
			/// equivalent strain that can be withstood by material before
			/// starting plastic flow. It defines the transition elastic->plastic.
	void   Set_elastic_yeld (double melastic_yeld) {elastic_yeld = melastic_yeld;};
			/// Get the elastic yeld modulus.
	double Get_elastic_yeld () {return elastic_yeld;}

			/// Set the plastic yeld modulus as the maximum VonMises
			/// equivalent strain that can be withstood by material before
			/// fracture. It defines the transition plastic->fracture.
	void   Set_plastic_yeld (double mplastic_yeld) {plastic_yeld = mplastic_yeld;};
			/// Get the plastic yeld modulus.
	double Get_plastic_yeld () {return plastic_yeld;}

			/// Set the plastic flow rate. The lower the value, the slower 
			/// the plastic flow during dynamic simulations.
	void   Set_flow_rate (double mflow_rate) {flow_rate = mflow_rate;};
			/// Set the plastic flow rate.
	double Get_flow_rate () {return flow_rate;}


	virtual double ComputeYeldFunction(const ChStressTensor<>& mstress) const;

	virtual void ComputeReturnMapping(ChStrainTensor<>& mplasticstrainflow, 
									const ChStrainTensor<>&	mincrementstrain, 
									const ChStrainTensor<>& mlastelasticstrain,
									const ChStrainTensor<>& mlastplasticstrain) const;


			/// Compute plastic strain flow (flow derivative dE_plast/dt) from strain,
			/// according to VonMises strain yeld theory. 
	void ComputePlasticStrainFlow(ChStrainTensor<>& mplasticstrainflow, const ChStrainTensor<>& mestrain) const;


			//
			// STREAMING
			//

				/// Method to allow deserializing a persistent binary archive (ex: a file)
				/// into transient data.
	void StreamIN(ChStreamInBinary& mstream);

				/// Method to allow serializing transient data into a persistent
				/// binary archive (ex: a file).
	void StreamOUT(ChStreamOutBinary& mstream);

};




/// Class for the basic properties of elastoplastic materials 
/// of Drucker-Prager type, that are useful for simulating
/// soils

class ChApi ChContinuumDruckerPrager : public ChContinuumElastoplastic
{
private:

	double elastic_yeld;
	double alpha;
	double dilatancy;
	double hardening_speed;
	double hardening_limit;
	double flow_rate;

public:

			/// Create a continuum isothropic Drucker-Prager material
	ChContinuumDruckerPrager(double myoung = 10000000, double mpoisson=0.4, double mdensity=1000,
							double melastic_yeld = 0.1, double  malpha = 0.5, double mdilatancy = 0);

	virtual ~ChContinuumDruckerPrager() {};
	

			/// Set the D-P yeld modulus C, for Drucker-Prager
			/// yeld. It defines the transition elastic->plastic.
	void   Set_elastic_yeld (double melastic_yeld) {elastic_yeld = melastic_yeld;};
			/// Get the elastic yeld modulus C
	double Get_elastic_yeld () {return elastic_yeld;}

			/// Set the internal friction coefficient A
	void   Set_alpha (double malpha) {alpha = malpha;};
			/// Get the internal friction coefficient A
	double Get_alpha () {return alpha;}	

			/// Sets the C and A parameters of the Drucker-Prager model
			/// starting from more 'practical' values of inner friction angle phi
			/// and cohesion, as used in the faceted Mohr-Coulomb model. 
			/// Use the optional parameter inner_approx to set if the faceted
			/// Mohr-Coulomg must be approximated with D-P inscribed (default) or circumscribed.
	void   Set_from_MohrCoulomb(double phi, double cohesion, bool inner_approx=true);

			/// Set the plastic flow rate multiplier. The lower the value, the slower 
			/// the plastic flow during dynamic simulations.
	void   Set_flow_rate (double mflow_rate) {flow_rate = mflow_rate;};
			/// Get the flow rate multiplier.
	double Get_flow_rate () {return flow_rate;}

			/// Set the internal dilatancy coefficient (usually 0.. < int.friction)
	void   Set_dilatancy (double mdilatancy) {dilatancy = mdilatancy;};
			/// Get the internal dilatancy coefficient 
	double Get_dilatancy () {return dilatancy;}

			/// Set the hardening limit (usually a bit larger than yeld), or softening
	void   Set_hardening_limit (double mhl) {hardening_limit = mhl;};
			/// Get the hardening limit 
	double Get_hardening_limit () {return hardening_limit;}

			/// Set the hardening inverse speed coeff. for exponential hardening 
			/// (the larger, the slower the hardening or softening process that 
			/// will asymptotycally make yeld = hardening_limit )
	void   Set_hardening_speed (double mhl) {hardening_speed = mhl;};
			/// Get the hardening speed
	double Get_hardening_speed () {return hardening_speed;}


	virtual double ComputeYeldFunction(const ChStressTensor<>& mstress) const;

	virtual void ComputeReturnMapping(ChStrainTensor<>& mplasticstrainflow, 
									const ChStrainTensor<>&	mincrementstrain, 
									const ChStrainTensor<>& mlastelasticstrain,
									const ChStrainTensor<>& mlastplasticstrain) const;

			/// Compute plastic strain flow direction from strain
			/// according to Drucker-Prager. 
	void ComputePlasticStrainFlow(ChStrainTensor<>& mplasticstrainflow, const ChStrainTensor<>& mestrain) const;


			//
			// STREAMING
			//

				/// Method to allow deserializing a persistent binary archive (ex: a file)
				/// into transient data.
	void StreamIN(ChStreamInBinary& mstream);

				/// Method to allow serializing transient data into a persistent
				/// binary archive (ex: a file).
	void StreamOUT(ChStreamOutBinary& mstream);

};



} // END_OF_NAMESPACE____
#endif 