import matplotlib.pyplot as plt
import numpy as np
import ROOT

"""
This file is part of MiniScatter.

MiniScatter is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MiniScatter is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MiniScatter.  If not, see <https://www.gnu.org/licenses/>.
"""

def plotRZgray(objects, nevents_simulated, nparts_actual):
    """
    Scale an RZ histogram (produced by MiniScatter) to Gray (J/kg).

    Input:
    The ROOT objects map returned from the simulation,
        from which the RZ histogram [mm,mm,MeV/bin] and the density [g/cm^3] is loaded;
    the number of events for the simulation, and
    the number of particles to scale to [int].

    Output:
    A histogram where the bins have been scaled to dose [Gy];
    """

    # Copy and normalize the histogram
    rzScaled = ROOT.TH2D(objects['target_edep_rdens']) #[MeV/bin]
    density  = objects['metadata'][2] #[g/cm^3]

    scaleFactor_energyUnit = 1.6021766e-13; #J/MeV
    scaleFactor_nPart = nparts_actual/nevents_simulated

    for rBinIdx in range(1,rzScaled.GetYaxis().GetNbins()+1):
        dA = np.pi*(rzScaled.GetYaxis().GetBinUpEdge(rBinIdx)**2   -\
                    rzScaled.GetYaxis().GetBinLowEdge(rBinIdx)**2   ) #[mm^2]

        for zBinIdx in range(1,rzScaled.GetXaxis().GetNbins()+1):
            dZ = rzScaled.GetXaxis().GetBinUpEdge(zBinIdx) - rzScaled.GetXaxis().GetBinLowEdge(zBinIdx) #[mm]
            dV = dA*dZ # [mm^3]

            #Scale to volume unit
            binIdx = rzScaled.GetBin(zBinIdx,rBinIdx)
            rzScaled.SetBinContent(binIdx, rzScaled.GetBinContent(binIdx)/dV) #[MeV/mm^3]

    #Scale with density and unit conversion
    rzScaled.Scale((scaleFactor_nPart/density)*(scaleFactor_energyUnit*1e6))
    rzScaled.SetTitle("Dose distribution [Gy]")

    return rzScaled

def plotZgray(rzHist_Gy, r0):
    """
    Get the central column of radius r0 [mm] and plot the dose deposition in Gy.
    Use the plotRZgray function to obtain rzHist_Gy.
    """

    #Reset scaling and check that the scaling we're attempting is on a bin edge
    firstX_old = rzHist_Gy.GetXaxis().GetFirst()
    lastX_old  = rzHist_Gy.GetXaxis().GetLast()

    firstY_old = rzHist_Gy.GetYaxis().GetFirst()
    lastY_old  = rzHist_Gy.GetYaxis().GetLast()

    rzHist_Gy.GetXaxis().SetRange(1,rzHist_Gy.GetXaxis().GetNbins())

    yAxis = rzHist_Gy.GetYaxis()
    yAxis_lastbin = yAxis.FindFixBin(r0)
    if yAxis_lastbin == -1:
        raise (ValueError("The chose r0={} is outside the r range of the histogram {} to {}.".\
                format(r0, yAxis.GetBinLowEdge(yAxis.GetFirst()), yAxis.GetBinLowEdge(yAxis.GetLast()))\
                ))
    assert (rzHist_Gy.GetYaxis().GetBinLowEdge(yAxis.GetFirst()) == 0.0)

    r0_actual = yAxis.GetBinUpEdge(yAxis_lastbin)
    if abs(r0_actual - r0)/r0 > 0.05:
        print("Warning, selected r0 is not on a bin edge.")
        print( " Closest edges are at {} [mm] and {} [mm]".\
                format(yAxis.GetBinLowEdge(yAxis_lastbin), yAxis.GetBinUpEdge(yAxis_lastbin)) )

    centerHist = ROOT.TH1D(rzHist_Gy.GetName()+"_centralColumn",\
                           "Dose [Gy] within r < {} [mm]".format(r0_actual),\
                           rzHist_Gy.GetXaxis().GetNbins(),\
                           0, rzHist_Gy.GetXaxis().GetBinUpEdge(rzHist_Gy.GetXaxis().GetLast())
                          )

    for zIdx in range(1,rzHist_Gy.GetNbinsX()):
        avgDoseZ = 0.0 #[Gy*mm^2]
        for rIdx in range(1,yAxis_lastbin):
            dA = np.pi*(rzHist_Gy.GetYaxis().GetBinUpEdge(rIdx)**2 - rzHist_Gy.GetYaxis().GetBinLowEdge(rIdx)**2) #[mm^2]
            avgDoseZ += rzHist_Gy.GetBinContent(zIdx,rIdx)*dA
        volTot = np.pi*r0_actual**2 #[mm^2]
        centerHist.SetBinContent(zIdx,avgDoseZ/volTot) #[Gy]

    #Restore the range settings of the 2D histogram
    rzHist_Gy.GetXaxis().SetRange(firstX_old, lastX_old)
    rzHist_Gy.GetYaxis().SetRange(firstY_old, lastY_old)

    centerHist.SetTitle("Average dose [Gy] within r < {} [mm]".format(r0_actual))
    centerHist.SetXTitle(rzHist_Gy.GetXaxis().GetTitle())
    centerHist.SetYTitle("Dose [Gy]")

    return centerHist
