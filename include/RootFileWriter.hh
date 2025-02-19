/*
 * This file is part of MiniScatter.
 *
 *  MiniScatter is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  MiniScatter is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MiniScatter.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ROOTFILEWRITER_HH_
#define ROOTFILEWRITERANALYSIS_HH_
#include "G4Event.hh"

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include <map>

class TRandom;

// Use a simple struct for writing to ROOT file,
// since this requires no dictionary to read.
struct trackerHitStruct {
    Double_t x; // [mm]
    Double_t y; // [mm]
    Double_t z; // [mm]

    Double_t px; // [MeV/c]
    Double_t py; // [MeV/c]
    Double_t pz; // [MeV/c]

    Double_t E; // [MeV]

    Int_t PDG;
    Int_t charge;

    Int_t eventID;
};

class particleTypesCounter {
public:
    particleTypesCounter(){
        particleTypes.clear();
        particleNames.clear();
        numParticles = 0;
    }
    // In both cases, the index is the PDG id.
    std::map<G4int,G4int> particleTypes;    // The number of particles of each type
    std::map<G4int,G4String> particleNames; // The name of each particle type
    G4int numParticles;
};

class RootFileWriter {
public:
    //! Singleton pattern
    static RootFileWriter* GetInstance() {
        if ( RootFileWriter::singleton == NULL ) RootFileWriter::singleton = new RootFileWriter();
        return RootFileWriter::singleton;
    }

    void initializeRootFile();
    void finalizeRootFile();
    void doEvent(const G4Event* event);

    void setFilename(G4String filename_out_arg) {
        this->filename_out = filename_out_arg;
        has_filename_out = true;
    };
    void setFoldername(G4String foldername_out_arg) {
        this->foldername_out = foldername_out_arg;
    };

    void setQuickmode(G4bool quickmode_arg) {
        this->quickmode = quickmode_arg;
    };
    void setMiniFile(G4bool miniFile_arg) {
        this->miniFile = miniFile_arg;
    };

    void setBeamEnergyCutoff(G4double cutFrac){
        this->beamEnergy_cutoff = cutFrac;
    }
    void setPositionCutoffR(G4double cutR) {
        this->position_cutoffR = cutR;
    }

    void setNumEvents(G4int numEvents_in) {
        this->numEvents = numEvents_in;
    }

    void setEdepDensDZ(G4double edep_dens_dz_in) {
        this->edep_dens_dz = edep_dens_dz_in;
    }
    void setEngNbins(G4int edepNbins_in);

private:
    RootFileWriter(){
        has_filename_out = false;
    };

    //! Singleton static instance
    static RootFileWriter* singleton;

    //The ROOT file
    TFile *histFile;

    // TTrees //
    TTree* targetExit;
    TTree* trackerHits;
    trackerHitStruct targetExitBuffer;
    trackerHitStruct trackerHitsBuffer;

    Double_t* magnetEdepsBuffer = NULL;
    TTree* magnetEdeps;

    // Histograms //

    // Target histograms
    TH1D* targetEdep;
    TH1D* targetEdep_NIEL;
    TH1D* targetEdep_IEL;

    std::map<G4int,TH1D*> target_exit_energy;
    std::map<G4int,TH1D*> target_exit_cutoff_energy;

    TH1D* target_exitangle_hist;
    TH1D* target_exitangle_hist_cutoff;

    TH2D* target_exit_phasespaceX;
    TH2D* target_exit_phasespaceY;
    TH2D* target_exit_phasespaceX_cutoff;
    TH2D* target_exit_phasespaceY_cutoff;

    std::map<G4int,TH1D*> target_exit_Rpos;
    std::map<G4int,TH1D*> target_exit_Rpos_cutoff;

    TH3D* target_edep_dens;
    TH2D* target_edep_rdens;

    // Magnet histograms
    std::vector<TH1D*> magnet_edep;
    std::vector<std::map<G4int,TH1D*>> magnet_exit_Rpos;
    std::vector<std::map<G4int,TH1D*>> magnet_exit_Rpos_cutoff;
    std::vector<TH2D*> magnet_exit_phasespaceX;
    std::vector<TH2D*> magnet_exit_phasespaceY;
    std::vector<TH2D*> magnet_exit_phasespaceX_cutoff;
    std::vector<TH2D*> magnet_exit_phasespaceY_cutoff;
    std::vector<std::map<G4int,TH1D*>> magnet_exit_energy;
    std::vector<std::map<G4int,TH1D*>> magnet_exit_cutoff_energy;

    //Tracker histograms
    TH1D* tracker_numParticles;
    TH1D* tracker_energy;
    std::map<G4int,TH1D*> tracker_type_energy;
    std::map<G4int,TH1D*> tracker_type_cutoff_energy;
    TH2D* tracker_hitPos;
    TH2D* tracker_hitPos_cutoff;
    TH2D* tracker_phasespaceX;
    TH2D* tracker_phasespaceY;
    TH2D* tracker_phasespaceX_cutoff;
    TH2D* tracker_phasespaceY_cutoff;

    std::map<G4int,TH1D*> tracker_Rpos;
    std::map<G4int,TH1D*> tracker_Rpos_cutoff;

    //Initial distribution
    TH2D* init_phasespaceX;
    TH2D* init_phasespaceY;
    TH2D* init_phasespaceXY;
    TH1D* init_E;

    // End-of-run statistics

    // Count the number of each particle type that hits the tracker
    std::map<G4String,particleTypesCounter> typeCounter;

    // Compute means and standard deviations of where the particles hit the tracker
    G4double tracker_particleHit_x;
    G4double tracker_particleHit_xx;
    G4double tracker_particleHit_y;
    G4double tracker_particleHit_yy;

    G4double tracker_particleHit_x_cutoff;
    G4double tracker_particleHit_xx_cutoff;
    G4double tracker_particleHit_y_cutoff;
    G4double tracker_particleHit_yy_cutoff;

    G4int numParticles_cutoff;

    //Target exit angle RMS
    G4double target_exitangle;
    G4double target_exitangle2;
    G4int target_exitangle_numparticles;
    G4double target_exitangle_cutoff;
    G4double target_exitangle2_cutoff;
    G4int target_exitangle_cutoff_numparticles;

    // Internal stuff
    //Output file naming
    G4String filename_out;
    G4bool has_filename_out;

    G4String foldername_out = "plots";

    G4bool quickmode = false;
    G4bool miniFile = false;

    G4double beamEnergy; // [MeV]

    // Compute statistics for charged particles with energy > this cutoff
    G4double beamEnergy_cutoff = 0.95;
    // Compute statistics for charged particles with position inside this radius
    G4double position_cutoffR = 1.0; // [mm]

    static const G4double phasespacehist_posLim;
    static const G4double phasespacehist_angLim;

    //Delta z for the energy deposition density TH3Ds [mm, 0 => Disable]
    G4double edep_dens_dz = 0.0;

    //Energy deposition and energy-remaining number of bins
    G4int engNbins = 1000;

    // RNG for sampling over the step
    TRandom* RNG;

    Int_t eventCounter; // Used for EventID-ing and metadata
    Int_t numEvents;    // Used for comparing to eventCounter with metadata;
                        // only reflects the -n <int> command line flag
                        // so it may be 0 if this was not set.

    void PrintTwissParameters(TH2D* phaseSpaceHist);
    void PrintParticleTypes(particleTypesCounter& pt, G4String name);
    void FillParticleTypes(particleTypesCounter& pt, G4int PDG, G4String type);
};

#endif
