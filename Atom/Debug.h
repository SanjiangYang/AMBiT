#ifndef DEBUG_H
#define DEBUG_H

class Debug
{
    /** Set of options for debugging and logging.
        We also hijack this class for directing output from outside of class Atom.
     */
    friend class Atom;
public:
    Debug();
    ~Debug() {}

public:
    inline bool LogFirstBuild() const { return bFirstBuild; }
    inline bool LogHFIterations() const { return bHFIterations; }
    inline bool OutputHFExcited() const { return bHFExcited; }
    inline bool LogHFContinuum() const { return bHFContinuum; }

    /** Generic Options */
    inline bool HartreeEnergyUnits() const { return bHartreeUnits; }
    inline bool InvCmEnergyUnits() const { return bInvCmUnits; }
    
protected:
    /** Only class Atom can change these. */
    inline void LogFirstBuild(bool debugon) { bFirstBuild = debugon; }
    inline void LogHFIterations(bool debugon) { bHFIterations = debugon; }
    inline void OutputHFExcited(bool debugon) { bHFExcited = debugon; }
    inline void LogHFContinuum(bool debugon) { bHFContinuum = debugon; }

    /** Generic Options */
    inline void HartreeEnergyUnits(bool turnon) { bHartreeUnits = turnon; }
    inline void InvCmEnergyUnits(bool turnon) { bInvCmUnits = turnon; }

private:
    bool bFirstBuild;
    bool bHFIterations;
    bool bHFExcited;
    bool bHFContinuum;
    bool bVShift;
    
    bool bHartreeUnits;
    bool bInvCmUnits;
};

inline Debug::Debug()
{
    bFirstBuild = false;
    bHFIterations = false;
    bHFExcited = false;
    bHFContinuum = false;
    bVShift = false;

    bHartreeUnits = false;
    bInvCmUnits = false;
}

#endif
