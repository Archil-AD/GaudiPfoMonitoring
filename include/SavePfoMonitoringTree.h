#ifndef GAUDI_PFO_MONITORING_SAVE_PFO_MONITORING_TREE_H
#define GAUDI_PFO_MONITORING_SAVE_PFO_MONITORING_TREE_H

#include "Gaudi/Algorithm.h"
#include "k4FWCore/DataHandle.h"
#include "k4FWCore/DataWrapper.h"

#include "TFile.h"
#include "TTree.h"

namespace GaudiPfoMonitoring
{
    /**
     * @brief A Gaudi algorithm to read PFO monitoring data from a global buffer
     *        and write it to a ROOT TTree.
     */

   class SavePfoMonitoringTree : public Gaudi::Algorithm {
     public:
        SavePfoMonitoringTree(const std::string& name, ISvcLocator* svcLoc);

        virtual ~SavePfoMonitoringTree();


        virtual StatusCode initialize();
        virtual StatusCode execute(const EventContext&) const;
        virtual StatusCode finalize();

    private:
        TFile* m_outputFile = nullptr;
        TTree* m_outputTree = nullptr;

        // Data members to be filled into the TTree (mirroring PfoData struct)
        mutable std::vector<float> m_pfo_energy;
        mutable std::vector<int> m_pfo_pdg;
        mutable std::vector<float> m_pfo_fNeutral;
        mutable std::vector<float> m_pfo_fPhoton;
        mutable std::vector<float> m_pfo_fCharged;
        mutable std::vector<float> m_pfo_alpha;
        mutable std::vector<float> m_pfo_px;
        mutable std::vector<float> m_pfo_py;
        mutable std::vector<float> m_pfo_pz;
        mutable std::vector<float> m_pfo_mass;
        mutable std::vector<unsigned int> m_pfo_nClusters;
        mutable std::vector<unsigned int> m_pfo_nEmClusters;
        mutable std::vector<unsigned int> m_pfo_nHits;
        mutable std::vector<unsigned int> m_pfo_nMipLikeHits;
        mutable std::vector<unsigned int> m_pfo_nEcalHits;
        mutable std::vector<unsigned int> m_pfo_nMipEcalHits;
        mutable std::vector<unsigned int> m_pfo_nHcalHits;
        mutable std::vector<unsigned int> m_pfo_nMipHcalHits;
        mutable std::vector<float> m_pfo_minClusterDistance;
        mutable std::vector<unsigned int> m_pfo_startLayer;
        mutable std::vector<unsigned int> m_pfo_nLayers;
        mutable std::vector<int> m_pfo_mcPdg;
        mutable std::vector<int> m_pfo_mcGenStatus;
        mutable std::vector<float> m_pfo_mcEnergy;

        // Event-level summary (one scalar per event)
        mutable int          m_evt_eventNumber;
        mutable unsigned int m_evt_nClusteredNonIsolatedHits;
        mutable unsigned int m_evt_nClusteredIsolatedHits;
        mutable unsigned int m_evt_nUnclusteredIsolatedHits;
        mutable unsigned int m_evt_nUnclusteredNonIsolatedHits;
        mutable float        m_evt_clusteredIsolatedEnergy;
        mutable float        m_evt_clusteredNonIsolatedEnergy;
        mutable float        m_evt_unclusteredIsolatedEnergy;
        mutable float        m_evt_unclusteredNonIsolatedEnergy;
        mutable float        m_evt_totalEnergy;
        mutable unsigned int m_evt_nClusters;
        mutable unsigned int m_evt_nPFOs;

        // Cluster branches (one entry per cluster in pAllClusters)
        mutable std::vector<float>        m_clus_energy;
        mutable std::vector<unsigned int> m_clus_nHits;
        mutable std::vector<unsigned int> m_clus_nMipLikeHits;
        mutable std::vector<unsigned int> m_clus_nEcalHits;
        mutable std::vector<unsigned int> m_clus_nHcalHits;
        mutable std::vector<unsigned int> m_clus_nMipEcalHits;
        mutable std::vector<unsigned int> m_clus_nMipHcalHits;
        mutable std::vector<unsigned int> m_clus_startLayer;
        mutable std::vector<unsigned int> m_clus_nLayers;
        mutable std::vector<unsigned int> m_clus_isEm;
        mutable std::vector<float>        m_clus_minClusterDistance;
        mutable std::vector<unsigned int> m_clus_isInPfo;
        mutable std::vector<float>        m_clus_distToMostEnergeticClusterCentroid;
        mutable std::vector<int>          m_clus_mcPdg;
        mutable std::vector<float>        m_clus_mcEnergy;
        mutable std::vector<float>        m_clus_minInnerLayerSeparation;
        mutable std::vector<float>        m_clus_minGenericDistance;
        mutable std::vector<float>        m_clus_minParallelDistance;
        mutable std::vector<int>          m_clus_layerSpan;
        mutable std::vector<int>          m_clus_showerLayerSpan;
        mutable std::vector<float>        m_clus_contactFraction;
        mutable std::vector<float>        m_clus_closeHitFraction;

        // CaloHit branches (one entry per calo hit)
        mutable std::vector<float>        m_hit_energy;
        mutable std::vector<unsigned int> m_hit_pseudoLayer;
        mutable std::vector<float>        m_hit_cellLengthScale;
        mutable std::vector<unsigned int> m_hit_isIsolated;
        mutable std::vector<unsigned int> m_hit_isPossibleMip;
        mutable std::vector<float>        m_hit_positionX;
        mutable std::vector<float>        m_hit_positionY;
        mutable std::vector<float>        m_hit_positionZ;
        mutable std::vector<unsigned int> m_hit_type;
        mutable std::vector<unsigned int> m_hit_isolationNearbyHits;
        mutable std::vector<float>        m_hit_distToMostEnergeticClusterCentroid;
        mutable std::vector<float>        m_hit_shortestIsolationDist;

        ServiceHandle<IDataProviderSvc> m_eventDataSvc;
    };
}

#endif // GAUDI_PFO_MONITORING_SAVE_PFO_MONITORING_TREE_H
