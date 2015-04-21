#include "LocalPotentialDecorator.h"
#include "Include.h"
#include "Universal/PhysicalConstant.h"
#include "CoulombOperator.h"
#include "Universal/Interpolator.h"

LocalPotentialDecorator::LocalPotentialDecorator(pHFOperator wrapped_hf, pOPIntegrator integration_strategy):
    HFOperatorDecorator(wrapped_hf)
{
    // If integration_strategy is supplied, use it.
    // Otherwise the integration_strategy from wrapped_hf will be used.
    if(integration_strategy != NULL)
        integrator = integration_strategy;
}

RadialFunction LocalPotentialDecorator::GetDirectPotential() const
{
    RadialFunction ret = wrapped->GetDirectPotential();
    ret += directPotential;

    return ret;
}

void LocalPotentialDecorator::GetODEFunction(unsigned int latticepoint, const SpinorFunction& fg, double* w) const
{
    wrapped->GetODEFunction(latticepoint, fg, w);

    if(latticepoint < directPotential.size())
    {   const double alpha = physicalConstant->GetAlpha();
        w[0] += alpha * directPotential.f[latticepoint] * fg.g[latticepoint];
        w[1] -= alpha * directPotential.f[latticepoint] * fg.f[latticepoint];
    }
}

void LocalPotentialDecorator::GetODECoefficients(unsigned int latticepoint, const SpinorFunction& fg, double* w_f, double* w_g, double* w_const) const
{
    wrapped->GetODECoefficients(latticepoint, fg, w_f, w_g, w_const);

    if(latticepoint < directPotential.size())
    {   const double alpha = physicalConstant->GetAlpha();
        w_g[0] += alpha * directPotential.f[latticepoint];
        w_f[1] -= alpha * directPotential.f[latticepoint];
    }
}

void LocalPotentialDecorator::GetODEJacobian(unsigned int latticepoint, const SpinorFunction& fg, double** jacobian, double* dwdr) const
{
    wrapped->GetODEJacobian(latticepoint, fg, jacobian, dwdr);

    if(latticepoint < directPotential.size())
    {   const double alpha = physicalConstant->GetAlpha();
        jacobian[0][1] += alpha * directPotential.f[latticepoint];
        jacobian[1][0] -= alpha * directPotential.f[latticepoint];

        dwdr[0] += alpha * directPotential.dfdr[latticepoint] * fg.g[latticepoint];
        dwdr[1] -= alpha * directPotential.dfdr[latticepoint] * fg.f[latticepoint];
    }
}

SpinorFunction LocalPotentialDecorator::ApplyTo(const SpinorFunction& a) const
{
    SpinorFunction ta = wrapped->ApplyTo(a);
    ta -= a * directPotential;

    return ta;
}

LocalExchangeApproximation::LocalExchangeApproximation(pHFOperator wrapped_hf, double x_alpha, pOPIntegrator integration_strategy):
    LocalPotentialDecorator(wrapped_hf, integration_strategy), Xalpha(x_alpha)
{}

void LocalExchangeApproximation::SetCore(pCoreConst hf_core)
{
    HFOperatorDecorator::SetCore(hf_core);
    double min_charge = !charge;
    const double* R = lattice->R();

    // Get electron density function
    RadialFunction density;

    auto cs = core->begin();
    while(cs != core->end())
    {
        const Orbital& s = *cs->second;
        double number_electrons = core->GetOccupancy(OrbitalInfo(&s));
        
        density += s.GetDensity() * number_electrons;
        
        cs++;
    }

    RadialFunction y(density.size());

    if(density.size())
        coulombSolver->GetPotential(density, y, Z-charge);

    unsigned int i = 0;

    // Get local exchange approximation
    directPotential.resize(lattice->size());
    double C = Xalpha * 0.635348143228;  // (81/32\pi^2)^(1/3)

    for(i = 0; i < density.size(); i++)
    {
        directPotential.f[i] = C * pow((density.f[i]/(R[i]*R[i])), 1./3.);
        directPotential.dfdr[i] = C/3. * pow((density.f[i]/(R[i]*R[i])), -2./3.) * (density.dfdr[i] - 2.*density.f[i]/R[i])/(R[i]*R[i]);

        if(directPotential.f[i] + Z/R[i] - y.f[i] < min_charge/R[i])
            break;
    }

    while(i < density.size())
    {   directPotential.f[i] = (-Z + charge + min_charge)/R[i] + y.f[i];
        directPotential.dfdr[i] = -(-Z + charge + min_charge)/(R[i]*R[i]) + y.dfdr[i];
        i++;
    }

    while(i < lattice->size())
    {   directPotential.f[i] = min_charge/R[i];
        directPotential.dfdr[i] = - min_charge/(R[i]*R[i]);
        i++;
    }
}

void LocalExchangeApproximation::Alert()
{
    if(directPotential.size() > lattice->size())
        directPotential.resize(lattice->size());
}
