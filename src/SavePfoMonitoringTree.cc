#include "SavePfoMonitoringTree.h"
// Include the PODIO generated collection headers
#include "CaloHitMonDataCollection.h"
#include "ClusterMonDataCollection.h"
#include "EventMonDataCollection.h"
#include "PfoMonDataCollection.h"

DECLARE_COMPONENT(GaudiPfoMonitoring::SavePfoMonitoringTree)

namespace GaudiPfoMonitoring {
SavePfoMonitoringTree::SavePfoMonitoringTree(const std::string &name,
                                             ISvcLocator *svcLoc)
    : Gaudi::Algorithm(name, svcLoc), m_outputFile(nullptr),
      m_outputTree(nullptr), m_pfo_energy(), m_pfo_pdg(), m_pfo_fNeutral(),
      m_pfo_fPhoton(), m_pfo_fCharged(), m_pfo_alpha(), m_pfo_px(), m_pfo_py(),
      m_pfo_pz(), m_pfo_mass(), m_pfo_nClusters(), m_pfo_nEmClusters(),
      m_pfo_nHits(), m_pfo_nMipLikeHits(), m_pfo_nEcalHits(),
      m_pfo_nMipEcalHits(), m_pfo_nHcalHits(), m_pfo_nMipHcalHits(),
      m_pfo_minClusterDistance(), m_pfo_startLayer(), m_pfo_nLayers(),
      m_pfo_mcPdg(), // Keep mcPdg as it exists in PfoMonData
      m_pfo_mcEnergy(), m_evt_eventNumber(0),
      m_evt_nClusteredNonIsolatedHits(0), m_evt_nClusteredIsolatedHits(0),
      m_evt_nUnclusteredIsolatedHits(0),
      m_evt_clusteredIsolatedEnergy(0.f), m_evt_clusteredNonIsolatedEnergy(0.f),
      m_evt_unclusteredIsolatedEnergy(0.f), m_evt_unclusteredNonIsolatedEnergy(0.f),
      m_evt_totalEnergy(0.f), m_evt_chargedEnergy(0.f), m_evt_neutralEnergy(0.f),
      m_evt_fChargedEnergyRecoCharged(0.f), m_evt_fChargedEnergyRecoNeutral(0.f), m_evt_fNeutralEnergyRecoCharged(0.f), m_evt_fNeutralEnergyRecoNeutral(0.f),
      m_evt_nUnclusteredNonIsolatedHits(0), m_evt_nClusters(0), m_evt_nPFOs(0),
      m_clus_energy(), m_clus_nHits(), m_clus_nMipLikeHits(),
      m_clus_nEcalHits(), m_clus_nHcalHits(), m_clus_nMipEcalHits(), m_clus_showerStartLayer(),
      m_clus_nMipHcalHits(), m_clus_startLayer(), m_clus_nLayers(), m_clus_passPhotonId(),
      m_clus_isEm(), m_clus_minClusterDistance(), m_clus_isInPfo(),
      m_clus_distToMostEnergeticClusterCentroid(), m_clus_fractionInCone(),
      m_clus_minInnerLayerSeparation(), m_clus_minGenericDistance(),
      m_clus_minParallelDistance(), m_clus_layerSpan(), m_clus_showerLayerSpan(), m_clus_contactFraction(), m_clus_closeHitFraction(),
      m_clus_canBeMerged(), m_clus_rms(), m_clus_dCosR(), m_clus_hasAssociatedTrack(), m_clus_fNeutral(), m_clus_fPhoton(), m_clus_fCharged(), m_clus_nRadiationLengths(), m_clus_nRadiationLengthsBeforeClusterStart(), m_clus_layer90RadLengths(), m_clus_showerMaxRadLengths(), m_clus_radial90(), m_clus_fractionOfEnergyAboveHighRadLengths(), m_clus_mcPdg(), m_clus_mcEnergy(),
      m_hit_energy(),
      m_hit_pseudoLayer(), m_hit_cellLengthScale(), m_hit_isIsolated(),
      m_hit_positionX(), m_hit_positionY(),
      m_hit_positionZ(), m_hit_type(), m_hit_isolationNearbyHits(), m_hit_isPossibleMip(),
      m_hit_distToMostEnergeticClusterCentroid(), m_hit_shortestIsolationDist(), m_hit_mcPdg(),
      m_eventDataSvc("EventDataSvc", "SavePfoMonitoringTree") {}

SavePfoMonitoringTree::~SavePfoMonitoringTree() {}

StatusCode SavePfoMonitoringTree::initialize() {
  StatusCode sc = Gaudi::Algorithm::initialize();
  if (!sc.isSuccess())
    return sc;

  info() << "Initializing " << name() << "..." << endmsg;

  m_outputFile = TFile::Open("GaudiPfoMonitoring.root", "RECREATE");
  if (!m_outputFile || m_outputFile->IsZombie()) {
    error() << "Failed to open output ROOT file GaudiPfoMonitoring.root"
            << endmsg;
    return StatusCode::FAILURE;
  }

  m_outputFile->cd();
  m_outputTree = new TTree("events", "events");
  // Event-level summary branches (one scalar per event)
  m_outputTree->Branch("evt_eventNumber", &m_evt_eventNumber,
                       "evt_eventNumber/I");
  m_outputTree->Branch("evt_nClusteredNonIsolatedHits",
                       &m_evt_nClusteredNonIsolatedHits,
                       "evt_nClusteredNonIsolatedHits/i");
  m_outputTree->Branch("evt_nClusteredIsolatedHits",
                       &m_evt_nClusteredIsolatedHits,
                       "evt_nClusteredIsolatedHits/i");
  m_outputTree->Branch("evt_nUnclusteredNonIsolatedHits",
                       &m_evt_nUnclusteredNonIsolatedHits,
                       "evt_nUnclusteredNonIsolatedHits/i");
  m_outputTree->Branch("evt_nUnclusteredIsolatedHits", &m_evt_nUnclusteredIsolatedHits,
                       "evt_nUnclusteredIsolatedHits/i");
  m_outputTree->Branch("evt_clusteredIsolatedEnergy", &m_evt_clusteredIsolatedEnergy,
                       "evt_clusteredIsolatedEnergy/F");
  m_outputTree->Branch("evt_clusteredNonIsolatedEnergy", &m_evt_clusteredNonIsolatedEnergy,
                       "evt_clusteredNonIsolatedEnergy/F");
  m_outputTree->Branch("evt_unclusteredIsolatedEnergy", &m_evt_unclusteredIsolatedEnergy,
                       "evt_unclusteredIsolatedEnergy/F");
  m_outputTree->Branch("evt_unclusteredNonIsolatedEnergy", &m_evt_unclusteredNonIsolatedEnergy,
                       "evt_unclusteredNonIsolatedEnergy/F");
  m_outputTree->Branch("evt_totalEnergy", &m_evt_totalEnergy,
                       "evt_totalEnergy/F");
  m_outputTree->Branch("evt_chargedEnergy", &m_evt_chargedEnergy, "evt_chargedEnergy/F");
  m_outputTree->Branch("evt_neutralEnergy", &m_evt_neutralEnergy, "evt_neutralEnergy/F");
  m_outputTree->Branch("evt_fChargedEnergyRecoCharged", &m_evt_fChargedEnergyRecoCharged, "evt_fChargedEnergyRecoCharged/F");
  m_outputTree->Branch("evt_fChargedEnergyRecoNeutral", &m_evt_fChargedEnergyRecoNeutral, "evt_fChargedEnergyRecoNeutral/F");
  m_outputTree->Branch("evt_fNeutralEnergyRecoCharged", &m_evt_fNeutralEnergyRecoCharged, "evt_fNeutralEnergyRecoCharged/F");
  m_outputTree->Branch("evt_fNeutralEnergyRecoNeutral", &m_evt_fNeutralEnergyRecoNeutral, "evt_fNeutralEnergyRecoNeutral/F");
  m_outputTree->Branch("evt_nClusters", &m_evt_nClusters, "evt_nClusters/i");
  m_outputTree->Branch("evt_nPFOs", &m_evt_nPFOs, "evt_nPFOs/i");

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
  m_outputTree->Branch("clus_energy", &m_clus_energy);
  m_outputTree->Branch("clus_nHits", &m_clus_nHits);
  m_outputTree->Branch("clus_nMipLikeHits", &m_clus_nMipLikeHits);
  m_outputTree->Branch("clus_nEcalHits", &m_clus_nEcalHits);
  m_outputTree->Branch("clus_nHcalHits", &m_clus_nHcalHits);
  m_outputTree->Branch("clus_nMipEcalHits", &m_clus_nMipEcalHits);
  m_outputTree->Branch("clus_nMipHcalHits", &m_clus_nMipHcalHits);
  m_outputTree->Branch("clus_startLayer", &m_clus_startLayer);
  m_outputTree->Branch("clus_showerStartLayer", &m_clus_showerStartLayer);
  m_outputTree->Branch("clus_nLayers", &m_clus_nLayers);
  m_outputTree->Branch("clus_isEm", &m_clus_isEm);
  m_outputTree->Branch("clus_passPhotonId", &m_clus_passPhotonId);
  m_outputTree->Branch("clus_minClusterDistance", &m_clus_minClusterDistance);
  m_outputTree->Branch("clus_isInPfo", &m_clus_isInPfo);
  m_outputTree->Branch("clus_distToMostEnergeticClusterCentroid",
                       &m_clus_distToMostEnergeticClusterCentroid);
  m_outputTree->Branch("clus_mcPdg", &m_clus_mcPdg);
  m_outputTree->Branch("clus_mcEnergy", &m_clus_mcEnergy);
  m_outputTree->Branch("clus_minInnerLayerSeparation", &m_clus_minInnerLayerSeparation);
  m_outputTree->Branch("clus_minGenericDistance", &m_clus_minGenericDistance);
  m_outputTree->Branch("clus_minParallelDistance", &m_clus_minParallelDistance);
  m_outputTree->Branch("clus_layerSpan",      &m_clus_layerSpan);
  m_outputTree->Branch("clus_showerLayerSpan", &m_clus_showerLayerSpan);
  m_outputTree->Branch("clus_contactFraction", &m_clus_contactFraction);
  m_outputTree->Branch("clus_closeHitFraction", &m_clus_closeHitFraction);
  m_outputTree->Branch("clus_fractionInCone", &m_clus_fractionInCone);
  m_outputTree->Branch("clus_canBeMerged", &m_clus_canBeMerged);
  m_outputTree->Branch("clus_rms", &m_clus_rms);
  m_outputTree->Branch("clus_dCosR", &m_clus_dCosR);
  m_outputTree->Branch("clus_nRadiationLengths", &m_clus_nRadiationLengths);
  m_outputTree->Branch("clus_nRadiationLengthsBeforeClusterStart", &m_clus_nRadiationLengthsBeforeClusterStart);
  m_outputTree->Branch("clus_layer90RadLengths", &m_clus_layer90RadLengths);
  m_outputTree->Branch("clus_showerMaxRadLengths", &m_clus_showerMaxRadLengths);
  m_outputTree->Branch("clus_fractionOfEnergyAboveHighRadLengths", &m_clus_fractionOfEnergyAboveHighRadLengths);
  m_outputTree->Branch("clus_radial90", &m_clus_radial90);
  m_outputTree->Branch("clus_fNeutral", &m_clus_fNeutral);
  m_outputTree->Branch("clus_fPhoton", &m_clus_fPhoton);
  m_outputTree->Branch("clus_fCharged", &m_clus_fCharged);
  m_outputTree->Branch("clus_hasAssociatedTrack", &m_clus_hasAssociatedTrack);

  // CaloHit branches (one entry per calo hit)
  m_outputTree->Branch("hit_energy", &m_hit_energy);
  m_outputTree->Branch("hit_pseudoLayer", &m_hit_pseudoLayer);
  m_outputTree->Branch("hit_cellLengthScale", &m_hit_cellLengthScale);
  m_outputTree->Branch("hit_isIsolated", &m_hit_isIsolated);
  m_outputTree->Branch("hit_isPossibleMip", &m_hit_isPossibleMip);
  m_outputTree->Branch("hit_positionX", &m_hit_positionX);
  m_outputTree->Branch("hit_positionY", &m_hit_positionY);
  m_outputTree->Branch("hit_positionZ", &m_hit_positionZ);
  m_outputTree->Branch("hit_type", &m_hit_type);
  m_outputTree->Branch("hit_isolationNearbyHits", &m_hit_isolationNearbyHits);
  m_outputTree->Branch("hit_distToMostEnergeticClusterCentroid",
                       &m_hit_distToMostEnergeticClusterCentroid);
  m_outputTree->Branch("hit_shortestIsolationDist",
                       &m_hit_shortestIsolationDist);
  m_outputTree->Branch("hit_mcPdg", &m_hit_mcPdg);

  info() << "Successfully initialized and opened ROOT file: "
            "GaudiPfoMonitoring.root"
         << endmsg;
  return StatusCode::SUCCESS;
}

StatusCode SavePfoMonitoringTree::execute(const EventContext &) const {
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
  m_evt_eventNumber = 0;
  m_evt_nClusteredNonIsolatedHits = 0;
  m_evt_nClusteredIsolatedHits = 0;
  m_evt_nUnclusteredIsolatedHits = 0;
  m_evt_clusteredIsolatedEnergy = 0.f;
  m_evt_clusteredNonIsolatedEnergy = 0.f;
  m_evt_unclusteredIsolatedEnergy = 0.f;
  m_evt_unclusteredNonIsolatedEnergy = 0.f;
  m_evt_totalEnergy = 0.f;
  m_evt_chargedEnergy = 0.f;
  m_evt_neutralEnergy = 0.f;
  m_evt_fChargedEnergyRecoCharged = 0.f;
  m_evt_fChargedEnergyRecoNeutral = 0.f;
  m_evt_fNeutralEnergyRecoCharged = 0.f;
  m_evt_fNeutralEnergyRecoNeutral = 0.f;
  m_evt_nUnclusteredNonIsolatedHits = 0;
  m_evt_nClusters = 0;
  m_evt_nPFOs = 0;

  // Clear cluster vectors
  m_clus_energy.clear();
  m_clus_nHits.clear();
  m_clus_nMipLikeHits.clear();
  m_clus_nEcalHits.clear();
  m_clus_nHcalHits.clear();
  m_clus_nMipEcalHits.clear();
  m_clus_nMipHcalHits.clear();
  m_clus_startLayer.clear();
  m_clus_showerStartLayer.clear();
  m_clus_nLayers.clear();
  m_clus_isEm.clear();
  m_clus_passPhotonId.clear();
  m_clus_minClusterDistance.clear();
  m_clus_distToMostEnergeticClusterCentroid.clear();
  m_clus_isInPfo.clear();
  m_clus_mcPdg.clear();
  m_clus_mcEnergy.clear();
  m_clus_minInnerLayerSeparation.clear();
  m_clus_minGenericDistance.clear();
  m_clus_minParallelDistance.clear();
  m_clus_layerSpan.clear();
  m_clus_showerLayerSpan.clear();
  m_clus_contactFraction.clear();
  m_clus_closeHitFraction.clear();
  m_clus_fractionInCone.clear();
  m_clus_canBeMerged.clear();
  m_clus_rms.clear();
  m_clus_dCosR.clear();
  m_clus_nRadiationLengths.clear();
  m_clus_nRadiationLengthsBeforeClusterStart.clear();
  m_clus_layer90RadLengths.clear();
  m_clus_showerMaxRadLengths.clear();
  m_clus_radial90.clear();
  m_clus_fractionOfEnergyAboveHighRadLengths.clear();
  m_clus_fNeutral.clear();
  m_clus_fPhoton.clear();
  m_clus_fCharged.clear();
  m_clus_hasAssociatedTrack.clear();

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
  m_hit_mcPdg.clear();

  // Retrieve the PFO collection from the Event Store
  const GaudiPfoMonitoring::PfoMonDataCollection *pfoDataBuffer = nullptr;
  if (eventSvc()
          ->retrieveObject("/Event/PfoMonitoringData",
                           (DataObject *&)pfoDataBuffer)
          .isSuccess()) {
    debug() << "Saving " << pfoDataBuffer->size() << " PFOs to Tree." << endmsg;

    for (const auto &pfoData : *pfoDataBuffer) {
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
  } else {
    warning()
        << "PfoMonitoringData not found. Tree will have empty branches for "
           "this event."
        << endmsg;
  }

  // Retrieve the cluster collection from the Event Store
  const GaudiPfoMonitoring::ClusterMonDataCollection *clusterDataBuffer =
      nullptr;
  if (eventSvc()
          ->retrieveObject("/Event/ClusterMonitoringData",
                           (DataObject *&)clusterDataBuffer)
          .isSuccess()) {
    debug() << "Saving " << clusterDataBuffer->size() << " clusters to Tree."
            << endmsg;
    for (const auto &clusData : *clusterDataBuffer) {
      m_clus_energy.push_back(clusData.getEnergy());
      m_clus_nHits.push_back(clusData.getNHits());
      m_clus_nMipLikeHits.push_back(clusData.getNMipLikeHits());
      m_clus_nEcalHits.push_back(clusData.getNEcalHits());
      m_clus_nHcalHits.push_back(clusData.getNHcalHits());
      m_clus_nMipEcalHits.push_back(clusData.getNMipEcalHits());
      m_clus_nMipHcalHits.push_back(clusData.getNMipHcalHits());
      m_clus_startLayer.push_back(clusData.getStartLayer());
      m_clus_showerStartLayer.push_back(clusData.getShowerStartLayer());
      m_clus_nLayers.push_back(clusData.getNLayers());
      m_clus_isEm.push_back(clusData.getIsEm());
      m_clus_passPhotonId.push_back(clusData.getPassPhotonId());
      m_clus_minClusterDistance.push_back(clusData.getMinClusterDistance());
      m_clus_isInPfo.push_back(clusData.getIsInPfo());
      m_clus_distToMostEnergeticClusterCentroid.push_back(
          clusData.getDistToMostEnergeticClusterCentroid());
      m_clus_mcPdg.push_back(clusData.getMcPdg());
      m_clus_mcEnergy.push_back(clusData.getMcEnergy());
      m_clus_minInnerLayerSeparation.push_back(clusData.getMinInnerLayerSeparation());
      m_clus_minGenericDistance.push_back(clusData.getMinGenericDistance());
      m_clus_minParallelDistance.push_back(clusData.getMinParallelDistance());
      m_clus_layerSpan.push_back(clusData.getLayerSpan());
      m_clus_showerLayerSpan.push_back(clusData.getShowerLayerSpan());
      m_clus_contactFraction.push_back(clusData.getContactFraction());
      m_clus_closeHitFraction.push_back(clusData.getCloseHitFraction());
      m_clus_fractionInCone.push_back(clusData.getFractionInCone());
      m_clus_canBeMerged.push_back(clusData.getCanBeMerged());
      m_clus_rms.push_back(clusData.getRms());
      m_clus_dCosR.push_back(clusData.getDCosR());
      m_clus_nRadiationLengths.push_back(clusData.getNRadiationLengths());
      m_clus_nRadiationLengthsBeforeClusterStart.push_back(clusData.getNRadiationLengthsBeforeClusterStart());
      m_clus_showerMaxRadLengths.push_back(clusData.getShowerMaxRadLengths());
      m_clus_radial90.push_back(clusData.getRadial90());
      m_clus_layer90RadLengths.push_back(clusData.getLayer90RadLengths());
      m_clus_fractionOfEnergyAboveHighRadLengths.push_back(clusData.getFractionOfEnergyAboveHighRadLengths());
      m_clus_fNeutral.push_back(clusData.getFNeutral());
      m_clus_fPhoton.push_back(clusData.getFPhoton());
      m_clus_fCharged.push_back(clusData.getFCharged());
      m_clus_hasAssociatedTrack.push_back(clusData.getHasAssociatedTrack());
    }
  } else {
    warning() << "ClusterMonitoringData not found for this event." << endmsg;
  }

  // Retrieve the event-level summary from the Event Store
  const GaudiPfoMonitoring::EventMonDataCollection *eventDataBuffer = nullptr;
  if (eventSvc()
          ->retrieveObject("/Event/EventMonitoringData",
                           (DataObject *&)eventDataBuffer)
          .isSuccess() &&
      !eventDataBuffer->empty()) {
    const auto &evtData = eventDataBuffer->front();
    m_evt_eventNumber = evtData.getEventNumber();
    m_evt_nClusteredNonIsolatedHits = evtData.getNClusteredNonIsolatedHits();
    m_evt_nClusteredIsolatedHits = evtData.getNClusteredIsolatedHits();
    m_evt_nUnclusteredIsolatedHits = evtData.getNUnclusteredIsolatedHits();
    m_evt_clusteredIsolatedEnergy = evtData.getClusteredIsolatedEnergy();
    m_evt_clusteredNonIsolatedEnergy = evtData.getClusteredNonIsolatedEnergy();
    m_evt_unclusteredIsolatedEnergy = evtData.getUnclusteredIsolatedEnergy();
    m_evt_unclusteredNonIsolatedEnergy = evtData.getUnclusteredNonIsolatedEnergy();
    m_evt_totalEnergy = evtData.getTotalEnergy();
    m_evt_chargedEnergy = evtData.getChargedEnergy();
    m_evt_neutralEnergy = evtData.getNeutralEnergy();
    m_evt_fChargedEnergyRecoCharged = evtData.getFChargedEnergyRecoCharged();
    m_evt_fChargedEnergyRecoNeutral = evtData.getFChargedEnergyRecoNeutral();
    m_evt_fNeutralEnergyRecoCharged = evtData.getFNeutralEnergyRecoCharged();
    m_evt_fNeutralEnergyRecoNeutral = evtData.getFNeutralEnergyRecoNeutral();
    m_evt_nUnclusteredNonIsolatedHits =
        evtData.getNUnclusteredNonIsolatedHits();
    m_evt_nClusters = evtData.getNClusters();
    m_evt_nPFOs = evtData.getNPFOs();
    debug() << "Event summary: event=" << m_evt_eventNumber
            << " clusteredNonIsolatedHits=" << m_evt_nClusteredNonIsolatedHits
            << " clusteredIsolatedHits=" << m_evt_nClusteredIsolatedHits
            << " unclusteredIsolatedHits=" << m_evt_nUnclusteredIsolatedHits
            << " unclusteredNonIsolatedHits="
            << m_evt_nUnclusteredNonIsolatedHits
            << " clusteredIsolatedEnergy=" << m_evt_clusteredIsolatedEnergy
            << " clusteredNonIsolatedEnergy=" << m_evt_clusteredNonIsolatedEnergy
            << " unclusteredIsolatedEnergy=" << m_evt_unclusteredIsolatedEnergy
            << " unclusteredNonIsolatedEnergy=" << m_evt_unclusteredNonIsolatedEnergy
            << " totalEnergy=" << m_evt_totalEnergy
            << " clusters=" << m_evt_nClusters << " PFOs=" << m_evt_nPFOs
            << endmsg;
  } else {
    warning() << "EventMonitoringData not found for this event." << endmsg;
  }

  // Retrieve the calo hit collection from the Event Store
  const GaudiPfoMonitoring::CaloHitMonDataCollection *caloHitDataBuffer =
      nullptr;
  if (eventSvc()
          ->retrieveObject("/Event/CaloHitMonitoringData",
                           (DataObject *&)caloHitDataBuffer)
          .isSuccess()) {
    debug() << "Saving " << caloHitDataBuffer->size() << " calo hits to Tree."
            << endmsg;
    for (const auto &hitData : *caloHitDataBuffer) {
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
      m_hit_distToMostEnergeticClusterCentroid.push_back(
          hitData.getDistToMostEnergeticClusterCentroid());
      m_hit_shortestIsolationDist.push_back(hitData.getShortestIsolationDist());
      m_hit_mcPdg.push_back(hitData.getMcPdg());
    }
  } else {
    warning() << "CaloHitMonitoringData not found for this event." << endmsg;
  }

  m_outputTree->Fill();

  // Note: No clear() needed! Gaudi clears the Event Store automatically.

  return StatusCode::SUCCESS;
}

StatusCode SavePfoMonitoringTree::finalize() {
  info() << "Finalizing " << name() << "..." << endmsg;

  if (m_outputFile) {
    m_outputFile->Write();
    m_outputFile->Close();
    delete m_outputFile;
    m_outputFile = nullptr;
  }
  return Gaudi::Algorithm::finalize();
}
} // namespace GaudiPfoMonitoring
