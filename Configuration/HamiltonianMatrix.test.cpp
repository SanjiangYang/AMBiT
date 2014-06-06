#include "HamiltonianMatrix.h"
#include "gtest/gtest.h"
#include "Include.h"
#include "HartreeFock/Core.h"
#include "HartreeFock/ConfigurationParser.h"
#include "Basis/BasisGenerator.h"
#include "ConfigGenerator.h"
#include "GFactor.h"
#include "Atom/MultirunOptions.h"

TEST(HamiltonianMatrixTester, MgIILevels)
{
    DebugOptions.LogHFIterations(true);
    DebugOptions.OutputHFExcited(true);

    pLattice lattice(new Lattice(1000, 1.e-6, 50.));

    // MgII
    std::string user_input_string = std::string() +
        "NuclearRadius = 3.7188\n" +
        "NuclearThickness = 2.3\n" +
        "Z = 12\n" +
        "[HF]\n" +
        "N = 10\n" +
        "Configuration = '1s2 2s2 2p6'\n" +
        "[Basis]\n" +
        "--bspline-basis\n" +
        "ValenceBasis = 6spdf\n" +
        "BSpline/Rmax = 45.0\n" +
        "[CI]\n" +
        "LeadingConfigurations = '3s2, 3s1 3p1'\n" +
        "ElectronExcitations = 1\n" +
        "EvenParityTwoJ = '0'\n" +
        "OddParityTwoJ = '0, 2'\n" +
        "NumSolutions = 3\n";

    std::stringstream user_input_stream(user_input_string);
    MultirunOptions userInput(user_input_stream, "//", "\n", ",");

    // Get core and excited basis
    BasisGenerator basis_generator(lattice, userInput);
    pCore core = basis_generator.GenerateHFCore();
    pOrbitalManagerConst orbitals = basis_generator.GenerateBasis();

    // Generate integrals
    pHFOperatorConst hf = basis_generator.GetHFOperator();
    pCoulombOperator coulomb(new CoulombOperator(lattice));
    pHartreeY hartreeY(new HartreeY(hf->GetOPIntegrator(), coulomb));
    pSlaterIntegrals integrals(new SlaterIntegralsMap(orbitals, hartreeY));
    integrals->CalculateTwoElectronIntegrals(orbitals->valence, orbitals->valence, orbitals->valence, orbitals->valence);

    EXPECT_EQ(31747, integrals->size());

    ConfigGenerator config_generator(orbitals, userInput);
    pRelativisticConfigList relconfigs;
    Symmetry sym(0, Parity::even);
    pLevelMap levels = pLevelMap(new LevelMap());
    double ground_state_energy = 0., excited_state_energy = 0.;

    // Generate matrix and configurations
    relconfigs = config_generator.GenerateRelativisticConfigurations(sym);
    pHFElectronOperator hf_electron(new HFElectronOperator(hf, orbitals));
    pTwoElectronCoulombOperator twobody_electron(new TwoElectronCoulombOperator<pSlaterIntegrals>(integrals));
    HamiltonianMatrix H_even(hf_electron, twobody_electron, relconfigs);
    H_even.GenerateMatrix();

    // Solve matrix
    H_even.SolveMatrix(sym, 3, levels);
    levels->Print(sym);
    ground_state_energy = levels->begin(sym)->second->GetEnergy();

    // Rinse and repeat for excited state
    sym = Symmetry(2, Parity::odd);
    relconfigs = config_generator.GenerateRelativisticConfigurations(sym);

    HamiltonianMatrix H_odd(hf_electron, twobody_electron, relconfigs);
    H_odd.GenerateMatrix();
    H_odd.SolveMatrix(sym, 3, levels);
    GFactorCalculator g_factors(hf->GetOPIntegrator(), orbitals);
    g_factors.CalculateGFactors(*levels, sym);
    levels->Print(sym);
    excited_state_energy = levels->begin(sym)->second->GetEnergy();

    EXPECT_NEAR(1.5, levels->begin(sym)->second->GetgFactor(), 0.001);

    // Check energy 3s2 -> 3s3p J = 1 (should be within 20%)
    EXPECT_NEAR(21870, (excited_state_energy - ground_state_energy) * MathConstant::Instance()->HartreeEnergyInInvCm(), 4000);
}