#include "SavePfoMonitoringTree.h"
// Include the PODIO generated collection headers
#include "CaloHitMonDataCollection.h"
#include "ClusterMonDataCollection.h"
#include "EventMonDataCollection.h"
#include "PfoMonDataCollection.h"

DECLARE_COMPONENT(GaudiPfoMonitoring::SavePfoMonitoringTree)

namespace GaudiPfoMonitoring
{
    SavePfoMonitoringTree::SavePfoMonitoringTree(const std::string& name, ISvcLocator* svcLoc)
    : Gaudi::Algorithm(name, svcLoc),
    m_outputFile(nullptr),
    m_outputTree(nullptr),
    m_pfo_energy(),
    m_pfo_pdg(),
    m_pfo_fNeutral(),
    m_pfo_fPhoton(),
    m_pfo_fCharged(),
    m_pfo_alpha(),
    m_pfo_px(),
    m_pfo_py(),
    m_pfo_pz(),
    m_pfo_mass(),
    m_pfo_nClusters(),
    m_pfo_nEmClusters(),
    m_pfo_nHits(),
    m_pfo_nMipLikeHits(),
    m_pfo_nEcalHits(),
    m_pfo_nMipEcalHits(),
    m_pfo_nHcalHits(),
    m_pfo_nMipHcalHits(),
    m_pfo_minClusterDistance(),
    m_pfo_startLayer(),
    m_pfo_nLayers(),
    m_pfo_mcPdg(), // Keep mcPdg as it exists in PfoMonData
    m_pfo_mcEnergy(),
    m_evt_eventNumber(0),
    m_evt_nClusteredNonIsolatedHits(0),
    m_evt_nClusteredIsolatedHits(0),
    m_evt_nOrphanIsolatedHits(0),
    m_evt_orphanIsolatedEnergy(0.f),
    m_evt_nUnclusteredNonIsolatedHits(0),
    m_evt_nClusters(0),
    m_evt_nPFOs(0),
    m_clus_energy(),
    m_clus_nHits(),
    m_clus_nMipLikeHits(),
    m_clus_nEcalHits(),
    m_clus_nHcalHits(),
    m_clus_nMipEcalHits(),
    m_clus_nMipHcalHits(),
    m_clus_startLayer(),
    m_clus_nLayers(),
    m_clus_isEm(),
    m_clus_minClusterDistance(),
    m_clus_isInPfo(),
    m_clus_distToMostEnergeticClusterCentroid(),
    m_hit_energy(),
    m_hit_pseudoLayer(),
    m_hit_cellLengthScale(),
    m_hit_isIsolated(),
    m_hit_isPossibleMip(),
    m_hit_positionX(),
    m_hit_positionY(),
    m_hit_positionZ(),
    m_hit_type(),
    m_hit_isolationNearbyHits(),
    m_hit_distToMostEnergeticClusterCentroid(),
    m_hit_shortestIsolationDist(),
    m_eventDataSvc("EventDataSvc", "SavePfoMonitoringTree")
    {
    }

    SavePfoMonitoringTree::~SavePfoMonitoringTree()
    {
    }

    StatusCode SavePfoMonitoringTree::initialize()
    {
      StatusCode sc = Gaudi::Algorithm::initialize();
      if (!sc.isSuccess())  return sc;

      info() << "Initializing " << name() << "..." << endmsg;

        m_outputFile = TFile::Open("GaudiPfoMonitoring.root", "RECREATE");
        if (!m_outputFile || m_outputFile->IsZombie())
        {
            error() << "Failed to open output ROOT file GaudiPfoMonitoring.root" << endmsg;
            return StatusCode::FAILURE;
        }

        m_outputFile->cd();
        m_outputTree = new TTree("events","events");
        // Event-level summary branches (one scalar per event)
        m_outputTree->Branch("evt_eventNumber",   &m_evt_eventNumber,   "evt_eventNumber/I");
        m_outputTree->Branch("evt_nClusteredNonIsolatedHits", &m_evt_nClusteredNonIsolatedHits, "evt_nClusteredNonIsolatedHits/i");
        m_outputTree->Branch("evt_nClusteredIsolatedHits", &m_evt_nClusteredIsolatedHits, "evt_nClusteredIsolatedHits/i");
        m_outputTree->Branch("evt_nOrphanIsolatedHits", &m_evt_nOrphanIsolatedHits, "evt_nOrphanIsolatedHits/i");
        m_outputTree->Branch("evt_orphanIsolatedEnergy", &m_evt_orphanIsolatedEnergy, "evt_orphanIsolatedEnergy/F");
        m_outputTree->Branch("evt_nUnclusteredNonIsolatedHits", &m_evt_nUnclusteredNonIsolatedHits, "evt_nUnclusteredNonIsolatedHits/i");
        m_outputTree->Branch("evt_nClusters",     &m_evt_nClusters,     "evt_nClusters/i");
        m_outputTree->Branch("evt_nPFOs",         &m_evt_nPFOs,         "evt_nPFOs/i");

        // Set up PFO branches
        m_outputTree->Branch("pfo_energy", &m_pfo_energy);
        m_outputTree->Branch("pfo_pdg", &m_pfo_pdg);
        m_outputTree->Branch("pfo_fNeutral", &m_pfo_fNeutral);
        m_outputTree->Branch("pfo_fPhoton", &m_pfo_fPhoton);
        m_outputTree->Branch("pfo_fCharged", &m_pfo_fCharged);
        m_outputTree->Branch("pfo_alpha", &m_pfo_alpha);
        m_outputTree->Branch("pfo_px", &m_pfo_px);
        m_outputTree->Branch("pfo_py", &m_pfo_py);
        m_outputTree->Branch("pfo_pz", &m_pfo_pz);
        m_outputTree->Branch("pfo_mass", &m_pfo_mass);
        m_outputTree->Branch("pfo_nClusters", &m_pfo_nClusters);
        m_outputTree->Branch("pfo_nEmClusters", &m_pfo_nEmClusters);
        m_outputTree->Branch("pfo_nHits", &m_pfo_nHits);
        m_outputTree->Branch("pfo_nMipLikeHits", &m_pfo_nMipLikeHits);
        m_outputTree->Branch("pfo_nEcalHits", &m_pfo_nEcalHits);
        m_outputTree->Branch("pfo_nMipEcalHits", &m_pfo_nMipEcalHits);
        m_outputTree->Branch("pfo_nHcalHits", &m_pfo_nHcalHits);
        m_outputTree->Branch("pfo_nMipHcalHits", &m_pfo_nMipHcalHits);
        m_outputTree->Branch("pfo_minClusterDistance", &m_pfo_minClusterDistance);
        m_outputTree->Branch("pfo_startLayer", &m_pfo_startLayer);
        m_outputTree->Branch("pfo_nLayers", &m_pfo_nLayers);
        m_outputTree->Branch("pfo_mcPdg", &m_pfo_mcPdg); // Keep mcPdg branch
        m_outputTree->Branch("pfo_mcEnergy", &m_pfo_mcEnergy);

        // Cluster branches (one entry per cluster in pAllClusters)
        m_outputTree->Branch("clus_energy",      &m_clus_energy);
        m_outputTree->Branch("clus_nHits",        &m_clus_nHits);
        m_outputTree->Branch("clus_nMipLikeHits", &m_clus_nMipLikeHits);
        m_outputTree->Branch("clus_nEcalHits",    &m_clus_nEcalHits);
        m_outputTree->Branch("clus_nHcalHits",    &m_clus_nHcalHits);
        m_outputTree->Branch("clus_nMipEcalHits", &m_clus_nMipEcalHits);
        m_outputTree->Branch("clus_nMipHcalHits", &m_clus_nMipHcalHits);
        m_outputTree->Branch("clus_startLayer",   &m_clus_startLayer);
        m_outputTree->Branch("clus_nLayers",      &m_clus_nLayers);
        m_outputTree->Branch("clus_isEm",              &m_clus_isEm);
        m_outputTree->Branch("clus_minClusterDistance",  &m_clus_minClusterDistance);
        m_outputTree->Branch("clus_isInPfo",             &m_clus_isInPfo);
        m_outputTree->Branch("clus_distToMostEnergeticClusterCentroid", &m_clus_distToMostEnergeticClusterCentroid);

        // CaloHit branches (one entry per calo hit)
        m_outputTree->Branch("hit_energy",         &m_hit_energy);
        m_outputTree->Branch("hit_pseudoLayer",    &m_hit_pseudoLayer);
        m_outputTree->Branch("hit_cellLengthScale",&m_hit_cellLengthScale);
        m_outputTree->Branch("hit_isIsolated",     &m_hit_isIsolated);
        m_outputTree->Branch("hit_isPossibleMip",  &m_hit_isPossibleMip);
        m_outputTree->Branch("hit_positionX",      &m_hit_positionX);
        m_outputTree->Branch("hit_positionY",      &m_hit_positionY);
        m_outputTree->Branch("hit_positionZ",      &m_hit_positionZ);
        m_outputTree->Branch("hit_type",           &m_hit_type);
        m_outputTree->Branch("hit_isolationNearbyHits", &m_hit_isolationNearbyHits);
        m_outputTree->Branch("hit_distToMostEnergeticClusterCentroid", &m_hit_distToMostEnergeticClusterCentroid);
        m_outputTree->Branch("hit_shortestIsolationDist", &m_hit_shortestIsolationDist);

        info() << "Successfully initialized and opened ROOT file: GaudiPfoMonitoring.root" << endmsg;
        return StatusCode::SUCCESS;
    }

    StatusCode SavePfoMonitoringTree::execute(const EventContext&) const {
        m_pfo_energy.clear();
        m_pfo_pdg.clear();
        m_pfo_fNeutral.clear();
        m_pfo_fPhoton.clear();
        m_pfo_fCharged.clear();
        m_pfo_alpha.clear();
        m_pfo_px.clear();
        m_pfo_py.clear();
        m_pfo_pz.clear();
        m_pfo_mass.clear();
        m_pfo_nClusters.clear();
        m_pfo_nEmClusters.clear();
        m_pfo_nHits.clear();
        m_pfo_nMipLikeHits.clear();
        m_pfo_nEcalHits.clear();
        m_pfo_nMipEcalHits.clear();
        m_pfo_nHcalHits.clear();
        m_pfo_nMipHcalHits.clear();
        m_pfo_minClusterDistance.clear();
        m_pfo_startLayer.clear();
        m_pfo_nLayers.clear();
        m_pfo_mcPdg.clear(); // Clear mcPdg
        m_pfo_mcEnergy.clear();

        // Reset event-level scalars
        m_evt_eventNumber   = 0;
        m_evt_nClusteredNonIsolatedHits = 0;
        m_evt_nClusteredIsolatedHits = 0;
        m_evt_nOrphanIsolatedHits = 0;
        m_evt_orphanIsolatedEnergy = 0.f;
        m_evt_nUnclusteredNonIsolatedHits = 0;
        m_evt_nClusters     = 0;
        m_evt_nPFOs         = 0;

        // Clear cluster vectors
        m_clus_energy.clear();
        m_clus_nHits.clear();
        m_clus_nMipLikeHits.clear();
        m_clus_nEcalHits.clear();
        m_clus_nHcalHits.clear();
        m_clus_nMipEcalHits.clear();
        m_clus_nMipHcalHits.clear();
        m_clus_startLayer.clear();
        m_clus_nLayers.clear();
        m_clus_isEm.clear();
        m_clus_minClusterDistance.clear();
        m_clus_distToMostEnergeticClusterCentroid.clear();
        m_clus_isInPfo.clear();

        // Clear calo hit vectors
        m_hit_energy.clear();
        m_hit_pseudoLayer.clear();
        m_hit_cellLengthScale.clear();
        m_hit_isIsolated.clear();
        m_hit_isPossibleMip.clear();
        m_hit_positionX.clear();
        m_hit_positionY.clear();
        m_hit_positionZ.clear();
        m_hit_type.clear();
        m_hit_isolationNearbyHits.clear();
        m_hit_distToMostEnergeticClusterCentroid.clear();
        m_hit_shortestIsolationDist.clear();

        // Retrieve the PFO collection from the Event Store
        const GaudiPfoMonitoring::PfoMonDataCollection* pfoDataBuffer = nullptr;
        if (eventSvc()->retrieveObject("/Event/PfoMonitoringData", (DataObject*&)pfoDataBuffer).isFailure()) {
            warning() << "PfoMonitoringData not found. Tree will have empty branches for this event." << endmsg;
            m_outputTree->Fill();
            return StatusCode::SUCCESS;
        }

        debug() << "Saving " << pfoDataBuffer->size() << " PFOs to Tree." << endmsg;

        for (const auto& pfoData : *pfoDataBuffer)
        {
            m_pfo_energy.push_back(pfoData.getEnergy());
            m_pfo_pdg.push_back(pfoData.getPdg());
            m_pfo_fNeutral.push_back(pfoData.getFNeutral());
            m_pfo_fPhoton.push_back(pfoData.getFPhoton());
            m_pfo_fCharged.push_back(pfoData.getFCharged());
            m_pfo_alpha.push_back(pfoData.getAlpha());
            m_pfo_px.push_back(pfoData.getPx());
            m_pfo_py.push_back(pfoData.getPy());
            m_pfo_pz.push_back(pfoData.getPz());
            m_pfo_mass.push_back(pfoData.getMass());
            m_pfo_nClusters.push_back(pfoData.getNClusters());
            m_pfo_nEmClusters.push_back(pfoData.getNEmClusters());
            m_pfo_nHits.push_back(pfoData.getNHits());
            m_pfo_nMipLikeHits.push_back(pfoData.getNMipLikeHits());
            m_pfo_nEcalHits.push_back(pfoData.getNEcalHits());
            m_pfo_nMipEcalHits.push_back(pfoData.getNMipEcalHits());
            m_pfo_nHcalHits.push_back(pfoData.getNHcalHits());
            m_pfo_nMipHcalHits.push_back(pfoData.getNMipHcalHits());
            m_pfo_minClusterDistance.push_back(pfoData.getMinClusterDistance());
            m_pfo_startLayer.push_back(pfoData.getStartLayer());
            m_pfo_nLayers.push_back(pfoData.getNLayers());
            m_pfo_mcPdg.push_back(pfoData.getMcPdg());
            m_pfo_mcEnergy.push_back(pfoData.getMcEnergy());
        }

        // Retrieve the cluster collection from the Event Store
        const GaudiPfoMonitoring::ClusterMonDataCollection* clusterDataBuffer = nullptr;
        if (eventSvc()->retrieveObject("/Event/ClusterMonitoringData", (DataObject*&)clusterDataBuffer).isSuccess())
        {
            debug() << "Saving " << clusterDataBuffer->size() << " clusters to Tree." << endmsg;
            for (const auto& clusData : *clusterDataBuffer)
            {
                m_clus_energy.push_back(clusData.getEnergy());
                m_clus_nHits.push_back(clusData.getNHits());
                m_clus_nMipLikeHits.push_back(clusData.getNMipLikeHits());
                m_clus_nEcalHits.push_back(clusData.getNEcalHits());
                m_clus_nHcalHits.push_back(clusData.getNHcalHits());
                m_clus_nMipEcalHits.push_back(clusData.getNMipEcalHits());
                m_clus_nMipHcalHits.push_back(clusData.getNMipHcalHits());
                m_clus_startLayer.push_back(clusData.getStartLayer());
                m_clus_nLayers.push_back(clusData.getNLayers());
                m_clus_isEm.push_back(clusData.getIsEm());
                m_clus_minClusterDistance.push_back(clusData.getMinClusterDistance());
                m_clus_isInPfo.push_back(clusData.getIsInPfo());
                m_clus_distToMostEnergeticClusterCentroid.push_back(clusData.getDistToMostEnergeticClusterCentroid());
            }
        }
        else
        {
            warning() << "ClusterMonitoringData not found for this event." << endmsg;
        }

        // Retrieve the event-level summary from the Event Store
        const GaudiPfoMonitoring::EventMonDataCollection* eventDataBuffer = nullptr;
        if (eventSvc()->retrieveObject("/Event/EventMonitoringData", (DataObject*&)eventDataBuffer).isSuccess()
            && !eventDataBuffer->empty())
        {
            const auto& evtData = eventDataBuffer->front();
            m_evt_eventNumber   = evtData.getEventNumber();
            m_evt_nClusteredNonIsolatedHits = evtData.getNClusteredNonIsolatedHits();
            m_evt_nClusteredIsolatedHits = evtData.getNClusteredIsolatedHits();
            m_evt_nOrphanIsolatedHits = evtData.getNOrphanIsolatedHits();
            m_evt_orphanIsolatedEnergy = evtData.getOrphanIsolatedEnergy();
            m_evt_nUnclusteredNonIsolatedHits = evtData.getNUnclusteredNonIsolatedHits();
            m_evt_nClusters     = evtData.getNClusters();
            m_evt_nPFOs         = evtData.getNPFOs();
            debug() << "Event summary: event=" << m_evt_eventNumber
                    << " clusteredNonIsolatedHits=" << m_evt_nClusteredNonIsolatedHits
                    << " clusteredIsolatedHits=" << m_evt_nClusteredIsolatedHits
                    << " orphanIsolatedHits=" << m_evt_nOrphanIsolatedHits
                    << " orphanIsolatedEnergy=" << m_evt_orphanIsolatedEnergy
                    << " unclusteredNonIsolatedHits=" << m_evt_nUnclusteredNonIsolatedHits
                    << " clusters=" << m_evt_nClusters
                    << " PFOs=" << m_evt_nPFOs << endmsg;
        }
        else
        {
            warning() << "EventMonitoringData not found for this event." << endmsg;
        }

        // Retrieve the calo hit collection from the Event Store
        const GaudiPfoMonitoring::CaloHitMonDataCollection* caloHitDataBuffer = nullptr;
        if (eventSvc()->retrieveObject("/Event/CaloHitMonitoringData", (DataObject*&)caloHitDataBuffer).isSuccess())
        {
            debug() << "Saving " << caloHitDataBuffer->size() << " calo hits to Tree." << endmsg;
            for (const auto& hitData : *caloHitDataBuffer)
            {
                m_hit_energy.push_back(hitData.getEnergy());
                m_hit_pseudoLayer.push_back(hitData.getPseudoLayer());
                m_hit_cellLengthScale.push_back(hitData.getCellLengthScale());
                m_hit_isIsolated.push_back(hitData.getIsIsolated());
                m_hit_isPossibleMip.push_back(hitData.getIsPossibleMip());
                m_hit_positionX.push_back(hitData.getPositionX());
                m_hit_positionY.push_back(hitData.getPositionY());
                m_hit_positionZ.push_back(hitData.getPositionZ());
                m_hit_type.push_back(hitData.getHitType());
                m_hit_isolationNearbyHits.push_back(hitData.getIsolationNearbyHits());
                m_hit_distToMostEnergeticClusterCentroid.push_back(hitData.getDistToMostEnergeticClusterCentroid());
                m_hit_shortestIsolationDist.push_back(hitData.getShortestIsolationDist());
            }
        }
        else
        {
            warning() << "CaloHitMonitoringData not found for this event." << endmsg;
        }

        m_outputTree->Fill();

        // Note: No clear() needed! Gaudi clears the Event Store automatically.

        return StatusCode::SUCCESS;
    }

    StatusCode SavePfoMonitoringTree::finalize()
    {
        info() << "Finalizing " << name() << "..." << endmsg;

        if (m_outputFile)
        {
            m_outputFile->Write();
            m_outputFile->Close();
            delete m_outputFile;
            m_outputFile = nullptr;
        }
        return Gaudi::Algorithm::finalize();
    }
}
