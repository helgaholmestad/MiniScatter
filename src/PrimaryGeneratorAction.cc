#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "Randomize.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4String.hh"

#include <iostream>
#include <cmath>
#include <string>

//System of units defines variables like "s" in the global scope,
// which are then shadowed inside functions in the header. Let's ignore it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "TMatrixD.h"
#include "TVectorD.h"
#pragma GCC diagnostic pop
#include "TRandom1.h"

//#include "CLHEP/Random/RandGauss.h"
//#include "/Applications/geant4.9.6.p02/source/externals/clhep/include/CLHEP/Random/RandLandau.h" //(include this before method definition)

// -----------------------------------------------------------------------------------------

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* DC,
                                               G4double beam_energy_in,
                                               G4String beam_type_in,
                                               G4double beam_offset_in,
                                               G4double beam_zpos_in,
                                               G4String covarianceString_in) :
    Detector(DC),
    beam_energy(beam_energy_in),
    beam_type(beam_type_in),
    beam_offset(beam_offset_in),
    beam_zpos(beam_zpos_in),
    covarianceString(covarianceString_in) {
    
    G4int n_particle = 1;
    particleGun  = new G4ParticleGun(n_particle);
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
                     particle = particleTable->FindParticle(beam_type);
    if (particle == NULL) {
        G4cerr << "Error - particle named '" << beam_type << "'not found" << G4endl;
        //particleTable->DumpTable();
        exit(1);
    }
    particleGun->SetParticleDefinition(particle);

    if (beam_zpos == 0.0) {
        beam_zpos = - ( Detector->getTargetThickness() / 2.0 +
                        Detector->WorldSizeZ_buffer    / 2.0   );
    }
    else {
        beam_zpos *= mm;

        if (beam_zpos >= - Detector->getTargetThickness() / 2.0) {
            G4cout << "Beam starting position = " << beam_zpos/mm
                   << " [mm] is not behind target back plane = "
                   << - Detector->getTargetThickness() / 2.0 / mm<< " [mm]"
                   << G4endl;
            exit(1);
        }
        if (beam_zpos <= - Detector->getWorldSizeZ()/2.0) {
            G4cout << "Beam starting position = " << beam_zpos/mm
                   << " [mm] is behind world back plane = "
                   << - Detector->getWorldSizeZ() / 2.0 / mm<< " [mm]"
                   << G4endl;
            exit(1);
        }
    }

    if (covarianceString != "") {
        hasCovariance = true;
        setupCovariance();

        RNG = new TRandom1();
    }
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete particleGun;
}


void PrimaryGeneratorAction::setupCovariance() {
    // Convert the covarianceString to a set of Twiss parameters,
    // then setup the covariance matrix and do the Cholesky Decomposition
    // Format: epsN[um]:beta[m]:alpha(::epsN_Y[um]:betaY[m]:alphaY)

    G4cout << G4endl;
    G4cout << "Initializing covariance matrices..." << G4endl;

    // Convert the string to relevant variables
    str_size startPos = 0;
    str_size endPos   = covarianceString.index(":",startPos);
    epsN_x            = convertColons(startPos,endPos, "epsN");

    startPos = endPos+1;
    endPos   = covarianceString.index(":",startPos);
    beta_x   = convertColons(startPos,endPos, "beta");

    if ( covarianceString.contains("::") ) {
        startPos = endPos+1;
        endPos   = covarianceString.index(":",startPos);
        alpha_x  = convertColons(startPos,endPos, "alpha");

        startPos = endPos+2;
        endPos   = covarianceString.index(":",startPos);
        epsN_y   = convertColons(startPos,endPos, "epsN_y");

        startPos = endPos+1;
        endPos   = covarianceString.index(":",startPos);
        beta_y   = convertColons(startPos,endPos, "beta_y");

        startPos = endPos+1;
        endPos   = covarianceString.length();
        alpha_y  = convertColons(startPos,endPos, "alpha_y");
    }
    else {
        startPos = endPos+1;
        endPos   = covarianceString.length();
        alpha_x  = convertColons(startPos,endPos, "alpha");

        epsN_y  = epsN_x;
        beta_y  = beta_x;
        alpha_y = alpha_x;
    }

    //Compute the geometrical emittance
    G4double gamma_rel = beam_energy*MeV/particle->GetPDGMass();
    G4cout << "gamma_rel = " << gamma_rel << G4endl;
    G4double beta_rel = sqrt(gamma_rel*gamma_rel - 1.0) / gamma_rel;
    G4cout << "beta_rel = " << beta_rel << G4endl;

    epsG_x = epsN_x / (beta_rel*gamma_rel);
    epsG_y = epsN_y / (beta_rel*gamma_rel);

    G4cout << "Got Twiss parameters:" << G4endl;
    G4cout << "epsN_x  = " << epsN_x << "[um]" << G4endl;
    G4cout << "epsG_x  = " << epsG_x << "[um]" << G4endl;
    G4cout << "beta_x  = " << beta_x << "[m]"  << G4endl;
    G4cout << "alpha_x = " << alpha_x << G4endl;
    G4cout << "epsN_y  = " << epsN_y << "[um]" << G4endl;
    G4cout << "epsG_y  = " << epsG_y << "[um]" << G4endl;
    G4cout << "beta_y  = " << beta_y << "[m]"  << G4endl;
    G4cout << "alpha_y = " << alpha_y << G4endl;

    //Create covariance matrices
    covarX.ResizeTo(2,2);
    covarX[0][0] = beta_x;
    covarX[0][1] = -alpha_x;
    covarX[1][0] = -alpha_x;
    covarX[1][1] = (1+alpha_x*alpha_x)/beta_x;
    //covarX.Print(); // Raw matrix

    covarX *= (epsG_x*1e-6);

    G4cout << "Covariance matrix (X) [m^2, m * rad, rad^2]:" << G4endl;
    covarX.Print();

    covarY.ResizeTo(2,2);
    covarY[0][0] = beta_y;
    covarY[0][1] = -alpha_y;
    covarY[1][0] = -alpha_y;
    covarY[1][1] = (1+alpha_y*alpha_y)/beta_y;
    //covarY.Print(); // Raw matrix

    covarY *= (epsG_y*1e-6);

    G4cout << "Covariance matrix (Y) [m^2, m * rad, rad^2]:" << G4endl;
    covarY.Print();

    // Get the cholesky decomposition
    TDecompChol covarX_Utmp(covarX,1e-9);
    covarX_Utmp.Decompose();
    G4cout << "Decomposed matrix (X):" << G4endl;
    covarX_Utmp.Print();
    covarX_U.ResizeTo(covarX);
    covarX_U = covarX_Utmp.GetU();

    TDecompChol covarY_Utmp(covarY,1e-9);
    covarY_Utmp.Decompose();
    G4cout << "Decomposed matrix (Y):" << G4endl;
    covarY_Utmp.Print();
    covarY_U.ResizeTo(covarY);
    covarY_U = covarY_Utmp.GetU();

    G4cout << G4endl;
}
G4double PrimaryGeneratorAction::convertColons(str_size startPos, str_size endPos, G4String paramName) {
    if (endPos == std::string::npos) {
        G4cout << "PrimaryGeneratorAction::convertColons():" << G4endl
               << " Error while searching for " << paramName << " in '"
               << covarianceString << "'" << G4endl;
        exit(1);
    }
    G4String floatString = covarianceString(startPos,endPos-startPos);

    G4double floatData = 0.0;
    try {
        floatData = std::stod(std::string(floatString));
    }
    catch (const std::invalid_argument& ia) {
        G4cerr << "Invalid float in data '" << floatData << "'" << G4endl;
        exit(1);
    }

    /*
    G4cout << "got floatString = '" << floatString << "',"
           << " startPos = " << startPos << ", endPos = " << endPos << ","
           << " floatData = " << floatData << G4endl;
    */

    return floatData;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {

    if (anEvent->GetEventID() == 0) {
        G4cout << G4endl;
        G4cout << "Injecting beam at z0 = " << beam_zpos/mm << " [mm]" << G4endl;
        G4cout << "Distance to target   = " << (-beam_zpos - Detector->getTargetThickness()/2.0)/mm << "[mm]" << G4endl;
        G4cout << G4endl;
    }

    if (hasCovariance) {
        G4double x  = RNG->Gaus(0,1)*covarX_U[0][0] + RNG->Gaus(0,1)*covarX_U[0][1];
        G4double xp = RNG->Gaus(0,1)*covarX_U[1][0] + RNG->Gaus(0,1)*covarX_U[1][1] + beam_offset*mm;
        G4double y  = RNG->Gaus(0,1)*covarY_U[0][0] + RNG->Gaus(0,1)*covarY_U[0][1];
        G4double yp = RNG->Gaus(0,1)*covarY_U[1][0] + RNG->Gaus(0,1)*covarY_U[1][1];

        particleGun->SetParticlePosition(G4ThreeVector(x,
                                                       y,
                                                       beam_zpos
                                                       )
                                         );

        //Technically not completely accurate but close enough for now
        particleGun->SetParticleMomentumDirection(G4ThreeVector(xp,yp,1));
    }
    else {
        particleGun->SetParticlePosition(G4ThreeVector(beam_offset*mm,
                                                       0.0,
                                                       beam_zpos
                                                       )
                                         );
        particleGun->SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    }

    particleGun->SetParticleEnergy(beam_energy*MeV);
    particleGun->GeneratePrimaryVertex(anEvent);
}
