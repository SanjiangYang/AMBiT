#include "Include.h"
#include "NonRelInfo.h"
#include "Universal/Constant.h"

std::string NonRelInfo::Name() const
{
    char buffer[20];
    sprintf(buffer, "%d", pqn);
    std::string ret(buffer);

    ret.append(1, Constant::SpectroscopicNotation[L()]);
    return ret;
}

void NonRelInfoSet::AddConfigs(const char* basis_def) 
{

    unsigned int pqn = 0;
    unsigned int L = 0;
    
    unsigned int p = 0;
    bool ReadSpectroscopicInput = true;
    while(basis_def[p]) {
        if(isdigit(basis_def[p])) 
        {
            if(ReadSpectroscopicInput) 
            {
                pqn = atoi(&basis_def[p]);
            }
            ReadSpectroscopicInput = false;
        } 
        else
        {
            for(int i = 0; i <= pqn; i++) 
            {
                if(i > Constant::GetLFromSpectroscopicNotation((char) basis_def[p]))
                {
                    insert(NonRelInfo(i, Constant::GetLFromSpectroscopicNotation((char) basis_def[p])));
                }
            }
            ReadSpectroscopicInput = true;
        }

        p++;
    }
}

void NonRelInfoSet::EraseConfigs(const char* basis_def) 
{
    unsigned int pqn = 0;
    unsigned int L = 0;

    unsigned int p = 0;
    bool ReadSpectroscopicInput = true;
    while(basis_def[p]) {
        if(isdigit(basis_def[p])) 
        {
            if(ReadSpectroscopicInput)
            {
                pqn = atoi(&basis_def[p]);
            }
            ReadSpectroscopicInput = false;
        } else 
        {
            for(int i = 0; i <= pqn; i++) 
            {
                NonRelInfoSet::iterator it = find(NonRelInfo(i, Constant::GetLFromSpectroscopicNotation((char) basis_def[p])));
                if(it != end()){
                    erase(it);
                }
                it++;
            }
            ReadSpectroscopicInput = true;
        }

        p++;
    }
}
