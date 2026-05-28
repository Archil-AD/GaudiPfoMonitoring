#ifndef LC_PFO_MONITORING_DATA_H
#define LC_PFO_MONITORING_DATA_H

#include <vector>

namespace lc_content
{
    /**
     * @brief PfoData struct to hold monitoring information for a single PFO.
     */
    struct PfoData
    {
        // Reconstructed PFO properties
        float pfo_energy;
        int pfo_pdg;
        float pfo_fNeutral;
        float pfo_fPhoton;
        float pfo_fCharged;
        float pfo_alpha;
        float pfo_px;
        float pfo_py;
        float pfo_pz;
        float pfo_mass;
        unsigned int pfo_nHits;
        unsigned int pfo_nMipLikeHits;
        unsigned int pfo_nEcalHits;
        unsigned int pfo_nMipEcalHits;
        unsigned int pfo_nHcalHits;
        unsigned int pfo_nMipHcalHits;
        float pfo_minClusterDistance;
        float pfo_maxCosOpeningAngle;
        unsigned int pfo_startLayer;
        unsigned int pfo_nLayers;
        // Truth information
        int pfo_mcPdg;
        int pfo_mcGenStatus;
        float pfo_mcEnergy;
    };

    extern std::vector<PfoData> g_pfoMonitoringBuffer; ///< Global buffer to store PFO monitoring data per event.
}

#endif // LC_PFO_MONITORING_DATA_H
