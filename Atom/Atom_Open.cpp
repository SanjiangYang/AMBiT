#include "Include.h"
#include "Atom.h"
#include "Universal/Constant.h"
#include "Configuration/NonRelInfo.h"
#include "Configuration/ConfigGenerator.h"
#include "Configuration/HamiltonianMatrix.h"

void Atom::RunOpen()
{
    DebugOptions.LogFirstBuild(false);
    DebugOptions.LogHFIterations(false);
    DebugOptions.OutputHFExcited(true);
    DebugOptions.HartreeEnergyUnits(true);

    //CreateHFBasis();
    //CreateCustomBasis();
    CreateRBasis();
    //CreateBSplineBasis();

    DebugOptions.OutputHFExcited(false);

    SD_CI = true;
    Configuration config;
    config.SetOccupancy(NonRelInfo(3, 2), 6);
    config.SetOccupancy(NonRelInfo(4, 0), 1);

    unsigned int two_j;
    NumSolutions = 4;

//    OpenShellEnergy(9, config);
    SMS_V2(9, config);
    return;

    *outstream << "\nGS Parity:\n" << std::endl;
    for(two_j = 9; two_j < 9; two_j += 2)
    {
        *outstream << "V2" << std::endl;
        SMS_V2(two_j, config);
        *outstream << "V0" << std::endl;
        SMS_V0(two_j, config);
        *outstream << "V1" << std::endl;
        SMS_V1(two_j, config);

        //DoOpenShellSMS(two_j, config);
        //DoOpenShellVolumeShift(two_j, config);
    }

    config.RemoveSingleParticle(NonRelInfo(4, 0));
    config.AddSingleParticle(NonRelInfo(4, 1));

    NumSolutions = 6;

    *outstream << "\nOpposite Parity:\n" << std::endl;

    for(two_j = 7; two_j <= 11; two_j+=2)
    {
        *outstream << "V2" << std::endl;
        SMS_V2(two_j, config);
        *outstream << "V0" << std::endl;
        SMS_V0(two_j, config);
        *outstream << "V1" << std::endl;
        SMS_V1(two_j, config);
    }
}

void Atom::OpenShellEnergy(int twoJ, Configuration& config, bool size_only)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    *outstream << " Number of non-rel configurations = " << nrlist.size() << std::endl;
    *outstream << " Number of rel configurations = " << rlist.size() << std::endl;
    generator.GenerateProjections(rlist, twoJ);

    if(size_only)
    {   unsigned int N = 0;
        RelativisticConfigList::const_iterator it = rlist.begin();
        while(it != rlist.end())
        {   N += it->GetJCoefficients().size();
            it++;
        }
        *outstream << " Number of J-configurations = " << N << std::endl;
    }
    else
    {   core->ToggleOpenShellCore();
        core->Update();
        excited->Update();
        core->ToggleClosedShellCore();
        HamiltonianMatrix H(*excited, rlist);
        H.GenerateMatrix();
        //H.PollMatrix();
        H.SolveMatrix(NumSolutions, twoJ, true);
    }
}

void Atom::DoOpenShellSMS(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    for(double ais = -0.002; ais <= 0.002; ais += 0.002)
    {
        *outstream << "\nNuclearInverseMass = " << ais << std::endl;

        core->ToggleOpenShellCore();
        core->SetNuclearInverseMass(ais);
        core->Update();
        excited->Update();

        core->ToggleClosedShellCore();
        HamiltonianMatrix H(*excited, rlist);
        H.IncludeSMS_V2(true);
        H.GenerateMatrix();
        H.SolveMatrix(NumSolutions, twoJ);

        //if(ais == 0.0)  // Get g-factors
        //    H.SolveMatrix(NumSolutions, twoJ, true);
        //else
        //    H.SolveMatrix(NumSolutions, twoJ);
    }
}

void Atom::SMS_V0(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    for(double ais = -0.002; ais <= 0.002; ais += 0.004)
    {
        *outstream << "\nNuclearInverseMass = " << ais << std::endl;

        core->ToggleOpenShellCore();
        core->SetNuclearInverseMass(ais);
        core->Update();
        excited->Update();
        core->SetNuclearInverseMass(0.);

        core->ToggleClosedShellCore();
        HamiltonianMatrix H(*excited, rlist);
        H.GenerateMatrix();
        H.SolveMatrix(NumSolutions, twoJ);
    }
}

void Atom::SMS_V1(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    core->ToggleOpenShellCore();
    core->SetNuclearInverseMass(0.);
    core->Update();
    excited->Update();
    core->ToggleClosedShellCore();

    for(double ais = -0.002; ais <= 0.002; ais += 0.004)
    {
        *outstream << "\nNuclearInverseMass = " << ais << std::endl;

        core->SetNuclearInverseMass(ais);

        HamiltonianMatrix H(*excited, rlist);
        H.IncludeSMS_V2(false);
        H.GenerateMatrix();
        H.SolveMatrix(NumSolutions, twoJ);
    }
}

void Atom::SMS_V2(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    core->ToggleOpenShellCore();
    core->SetNuclearInverseMass(0.);
    core->Update();
    excited->Update();

    core->ToggleClosedShellCore();
    HamiltonianMatrix H(*excited, rlist);
    H.GenerateMatrix();
    H.SolveMatrix(NumSolutions, twoJ, true);

    for(double ais = -0.002; ais <= 0.002; ais += 0.004)
    {
        *outstream << "\nNuclearInverseMass = " << ais << std::endl;

        core->SetNuclearInverseMass(ais);
        H.GetEigenvalues();
    }
}

void Atom::DoOpenShellVolumeShift(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    static bool calculated_potential = false;
    if(!calculated_potential)
    {   core->CalculateVolumeShiftPotential(0.05/Constant::AtomicToFermi);
        calculated_potential = true;
    }

    Write();
    for(double ais = -100.; ais <= 100.; ais += 50.)
    {
        *outstream << "\nVolumeShiftParameter = " << ais << std::endl;

        core->ToggleOpenShellCore();
        core->SetVolumeShiftParameter(ais);
        core->Update();
        excited->Update();

        core->ToggleClosedShellCore();
        HamiltonianMatrix H(*excited, rlist);
        H.GenerateMatrix();
        H.SolveMatrix(NumSolutions, twoJ);
    }

    core->ToggleOpenShellCore();
    core->SetVolumeShiftParameter(0.);
    Read();
    excited->Update();
}

void Atom::DoOpenShellAlphaVar(int twoJ, Configuration& config)
{
    unsigned int electron_excitations;
    if(SD_CI)
        electron_excitations = 2;
    else
        electron_excitations = config.NumParticles();

    ConfigList nrlist;
    nrlist.push_back(config);

    RelativisticConfigList rlist;
    ConfigGenerator generator(excited);
    generator.GenerateMultipleExcitations(nrlist, electron_excitations);
    generator.GenerateRelativisticConfigs(nrlist, rlist);
    generator.GenerateProjections(rlist, twoJ);

    double alpha0 = Constant::Alpha;

    for(double x = -0.25; x <= 0.25; x += 0.125)
    {
        Constant::Alpha = alpha0 * sqrt(x+1.);
        Constant::AlphaSquared = alpha0 * alpha0 * (x+1.);

        *outstream << "\nx = " << x << std::endl;

        core->ToggleOpenShellCore();
        core->Update();
        excited->Update();

        core->ToggleClosedShellCore();
        HamiltonianMatrix H(*excited, rlist);
        H.GenerateMatrix();

        if(x == 0.)
            H.SolveMatrix(NumSolutions, twoJ, true);
        else
            H.SolveMatrix(NumSolutions, twoJ);
    }

    Constant::Alpha = alpha0;
    Constant::AlphaSquared = alpha0 * alpha0;
}
