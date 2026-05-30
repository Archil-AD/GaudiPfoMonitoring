#include "SavePfoMonitoringTree.h"
// Include the PODIO generated collection header
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
    m_pfo_maxCosOpeningAngle(),
    m_pfo_startLayer(),
    m_pfo_nLayers(),
    m_pfo_mcPdg(), // Keep mcPdg as it exists in PfoMonData
    m_pfo_mcEnergy(),
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
        // Set up branches for the TTree
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
        m_outputTree->Branch("pfo_maxCosOpeningAngle", &m_pfo_maxCosOpeningAngle);
        m_outputTree->Branch("pfo_startLayer", &m_pfo_startLayer);
        m_outputTree->Branch("pfo_nLayers", &m_pfo_nLayers);
        m_outputTree->Branch("pfo_mcPdg", &m_pfo_mcPdg); // Keep mcPdg branch
        m_outputTree->Branch("pfo_mcEnergy", &m_pfo_mcEnergy);

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
        m_pfo_maxCosOpeningAngle.clear();
        m_pfo_startLayer.clear();
        m_pfo_nLayers.clear();
        m_pfo_mcPdg.clear(); // Clear mcPdg
        m_pfo_mcEnergy.clear();

        // Retrieve the PODIO collection from the Event Store
        const GaudiPfoMonitoring::PfoMonDataCollection* pfoDataBuffer = nullptr;
        if (eventSvc()->retrieveObject("/Event/PfoMonitoringData", (DataObject*&)pfoDataBuffer).isFailure()) {
            warning() << "PfoMonitoringData not found. Tree will have empty branches for this event." << endmsg;
            m_outputTree->Fill();
            return StatusCode::SUCCESS;
        }

        debug() << "Saving " << pfoDataBuffer->size() << " PFOs to Tree." << endmsg;

        for (const auto& pfoData : *pfoDataBuffer) // Dereference the pointer here
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
            m_pfo_maxCosOpeningAngle.push_back(pfoData.getMaxCosOpeningAngle());
            m_pfo_startLayer.push_back(pfoData.getStartLayer());
            m_pfo_nLayers.push_back(pfoData.getNLayers());
            m_pfo_mcPdg.push_back(pfoData.getMcPdg());
            m_pfo_mcEnergy.push_back(pfoData.getMcEnergy());
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
