#include "Atom.h"
#include "Include.h"
#include "HartreeFock/HartreeFocker.h"
#include "Configuration/ManyBodyOperator.h"

void Atom::Autoionization(std::pair<LevelID, pLevelConst> target, const Symmetry& sym, double ionization_energy)
{
//    double ionization_energy = -6.8010831;
    std::vector<double> total(levels->size(sym), 0.0);
    auto math = MathConstant::Instance();

    ConfigGenerator gen(orbitals, user_input);
    pOrbitalMap valence(new OrbitalMap(*orbitals->valence));

    // Select orbitals in target wavefunction
    std::set<OrbitalInfo> target_info_set;
    pOrbitalMap target_map(new OrbitalMap(lattice));
    for(const auto& rconfig: *target.second->GetRelativisticConfigList())
    {
        for(const auto& orb_pair: rconfig)
            target_info_set.insert(orb_pair.first);
    }
    for(const auto& info: target_info_set)
    {
        target_map->AddState(valence->GetState(info));
    }

    // Target information
    int target_twoJ = target.first.GetTwoJ();
    const double* target_eigenvector = target.second->GetEigenvector();

    // Get continuum wave (eps)
    Parity eps_parity = sym.GetParity() * target.first.GetParity();
    for(int eps_twoJ = abs(sym.GetTwoJ() - target.first.GetTwoJ()); eps_twoJ <= sym.GetTwoJ() + target.first.GetTwoJ(); eps_twoJ += 2)
    {
        // Kappa = (-1)^(j+1/2 + l) (j + 1/2) = (-1)^(j+1/2).P.(j+1/2)
        int eps_kappa = (eps_twoJ + 1)/2;
        eps_kappa = math->minus_one_to_the_power(eps_kappa) * Sign(eps_parity) * eps_kappa;

        std::vector<double> partial(levels->size(sym), 0.0);
        
        int level_index = 0;
        for(auto level_it = levels->begin(sym); level_it != levels->end(sym); level_it++)
        {
            const auto& compound_configs = level_it->second->GetRelativisticConfigList();
            const auto& compound_eigenvector = level_it->second->GetEigenvector();

            // Build continuum
            double eps_energy = level_it->second->GetEnergy() - ionization_energy;
            if(eps_energy <= 0.)
            {
                level_index++;
                continue;
            }

            pODESolver ode_solver(new AdamsSolver(hf->GetOPIntegrator()));
            HartreeFocker HF_Solver(ode_solver);
            pContinuumWave eps(new ContinuumWave(eps_kappa, 100, eps_energy));
            HF_Solver.CalculateContinuumWave(eps, hf);

            // Create two-body integrals
            pOrbitalMap continuum_map(new OrbitalMap(lattice));
            continuum_map->AddState(eps);

            pOrbitalManager all_orbitals(new OrbitalManager(*orbitals));
            all_orbitals->all->AddState(eps);
            all_orbitals->MakeStateIndexes();
            pOneElectronIntegrals one_body_integrals(new OneElectronIntegrals(all_orbitals, hf));
            pSlaterIntegrals two_body_integrals(new SlaterIntegralsDenseHash(all_orbitals, hartreeY));
            one_body_integrals->clear();
            one_body_integrals->CalculateOneElectronIntegrals(valence, continuum_map);
            two_body_integrals->clear();
            two_body_integrals->CalculateTwoElectronIntegrals(continuum_map, target_map, valence, valence);

            // Create operator for autoionization
            pTwoElectronCoulombOperator two_body_operator(new TwoElectronCoulombOperator<pSlaterIntegrals>(two_body_integrals));
            ManyBodyOperator<pOneElectronIntegrals, pTwoElectronCoulombOperator> H(one_body_integrals, two_body_operator);

            pRelativisticConfigList target_configs(new RelativisticConfigList(*target.second->GetRelativisticConfigList()));

            // Couple target and epsilon; start with maximum target M
            for(int target_twoM = target_twoJ; target_twoM >= -target_twoJ; target_twoM -= 2)
            {
                int eps_twoM = sym.GetTwoJ() - target_twoM;
                if(eps_twoM < -eps_twoJ)
                    continue;
                else if (eps_twoM > eps_twoJ)
                    break;
                ElectronInfo eps_info(100, eps_kappa, eps_twoM);

                gen.GenerateProjections(target_configs, target_twoM, target.first.GetTwoJ());

                // Coupling constant for |(eps_jlm; target_jlm) JJ >
                // Warning: Only works when target is half-integer!
                double coupling = math->Electron3j(eps_twoJ, target_twoJ, sym.GetTwoJ()/2, eps_twoM, target_twoM);
                coupling *= sqrt(double(sym.GetTwoJ() + 1)) * math->minus_one_to_the_power((eps_twoJ - target_twoJ + eps_twoM + target_twoM)/2);

                auto proj_it = target_configs->projection_begin();
                while(proj_it != target_configs->projection_end())
                {
                    auto proj_jt = compound_configs->projection_begin();
                    while(proj_jt != compound_configs->projection_end())
                    {
                        double matrix_element = H.GetMatrixElement(*proj_it, *proj_jt, &eps_info);
                        
                        if(matrix_element)
                        {
                            // Summation over CSFs
                            double coeff = 0.;
                            
                            for(auto coeff_i = proj_it.CSF_begin(); coeff_i != proj_it.CSF_end(); coeff_i++)
                            {
                                for(auto coeff_j = proj_jt.CSF_begin(); coeff_j != proj_jt.CSF_end(); coeff_j++)
                                {
                                    coeff += (*coeff_i) * (*coeff_j)
                                    * target_eigenvector[coeff_i.index()]
                                    * compound_eigenvector[coeff_j.index()];
                                }
                            }
                            
                            partial[level_index] += coupling * coeff * matrix_element;
                        }
                        proj_jt++;
                    }
                    proj_it++;
                }
            }
            level_index++;
        }

        for(int i = 0; i < total.size(); i++)
            total[i] += 2. * math->Pi() * partial[i] * partial[i];
    }

    *outstream << "\nAutoionization for J = " << sym.GetJ() << ", P = " << LowerName(sym.GetParity()) << std::endl;
    
    auto level_it = levels->begin(sym);
    for(int i = 0; i < total.size(); i++)
    {
        double eps_energy = (level_it->second->GetEnergy() - ionization_energy) * math->HartreeEnergyIneV();
        if(eps_energy > 0)
            *outstream << i << " (E = " << std::setprecision(4) << eps_energy << " eV): " <<  total[i] << "\t = " << total[i] * 0.413e8 << " /nsec" << std::endl;
        level_it++;
    }
}