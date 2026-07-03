/**
 *  @file   GaudiPfoMonitoring/src/PfoMonitoringAlgorithm.cc
 *
 *  @brief  Implementation of the dump pfos monitoring algorithm class
 *
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "GaudiKernel/Bootstrap.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/SmartIF.h"

#include "LCHelpers/ClusterHelper.h"
#include "LCHelpers/FragmentRemovalHelper.h"
#include "LCHelpers/ReclusterHelper.h"
#include "LCHelpers/SortingHelper.h"
#include "LCUtility/KDTreeLinkerAlgoT.h"
#include "LCUtility/KDTreeLinkerToolsT.h"
#include "PfoMonitoringAlgorithm.h"

#include "DD4hep/Detector.h"
#include "DDRec/MaterialManager.h"
#include "DDRec/Vector3D.h"

#include "EVENT/MCParticle.h"

#include "CaloHitMonDataCollection.h"
#include "ClusterMonDataCollection.h"
#include "EventMonDataCollection.h"
#include "PfoMonDataCollection.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <unordered_set>

using namespace pandora;

class PfoMonitoringAlgorithmFactory : public pandora::AlgorithmFactory {
public:
  pandora::Algorithm *CreateAlgorithm() const {
    return new lc_content::PfoMonitoringAlgorithm;
  }
};

namespace lc_content {
PfoMonitoringAlgorithm::PfoMonitoringAlgorithm()
    : m_nCorrectNeutralHadronPfo(0), m_nWrongNeutralHadronPfo(0),
      m_nCorrectChargedHadronPfo(0), m_nWrongChargedHadronPfo(0),
      m_neutralHadronEnergyFractionCut(0.7), m_createPfoMonData(true),
      m_createClusterMonData(true), m_createEventMonData(true),
      m_createCaloHitMonData(true),
      m_isolatedCaloHitListName("IsolatedCaloHitList"),
      m_isolationCutDistanceFine2(25.f * 25.f),
      m_isolationCutDistanceCoarse2(200.f * 200.f),
      m_isolationSearchSafetyFactor(2.f), m_isolationNLayers(2),
      m_eventNumber(0),
      // Defaults from ProximityBasedMergingAlgorithm
      m_clusterContactThreshold(2.f), m_closeHitThreshold(50.f),
      m_nGenericDistanceLayers(5), m_nAdjacentLayers(2),
      m_canMergeMinMipFraction(0.7f), m_canMergeMaxRms(5.f),
      // Default from LCParticleIdPlugins::LCEmShowerId
      m_highRadLengths(30.f),
      // Defaults from ConeBasedMergingAlgorithm
      m_minLayersToShowerStart(4), m_coneCosineHalfAngle(0.9f),
      m_minCosConeAngleWrtRadial(0.25f), m_cosConeAngleWrtRadialCut1(0.5f),
      m_minHitSeparationCut1(std::sqrt(1000.f)),
      m_cosConeAngleWrtRadialCut2(0.75f),
      m_minHitSeparationCut2(std::sqrt(1500.f)) {
  std::cout << " ------------------------------------------------------------ "
            << std::endl;
  std::cout << " ------------ PfoMonitoringAlgorithm initialized ------------ "
            << std::endl;
  std::cout << " ------------------------------------------------------------ "
            << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

PfoMonitoringAlgorithm::~PfoMonitoringAlgorithm() {
  std::cout << " ------------------------------------------------------------ "
            << std::endl;
  std::cout << " ---------- PfoMonitoringAlgorithm : Summary ----------- "
            << std::endl;
  std::cout << " ------------------------------------------------------------ "
            << std::endl;
  std::cout << std::endl;
  std::cout << " Correct Charged Hadron Pfos: " << m_nCorrectChargedHadronPfo
            << std::endl;
  std::cout << " Wrong Charged Hadron Pfos: " << m_nWrongChargedHadronPfo
            << std::endl;
  std::cout << " Correct Neutral Hadron Pfos: " << m_nCorrectNeutralHadronPfo
            << std::endl;
  std::cout << " Wrong Neutral Hadron Pfos: " << m_nWrongNeutralHadronPfo
            << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode PfoMonitoringAlgorithm::Run() {
  // 1. Get the Gaudi Event Data Service
  SmartIF<IDataProviderSvc> eventSvc =
      Gaudi::svcLocator()->service<IDataProviderSvc>("EventDataSvc");
  if (!eventSvc) {
    std::cout << "PfoMonitoringAlgorithm: Could not locate EventDataSvc"
              << std::endl;
    return STATUS_CODE_FAILURE;
  }

  // 2. Retrieve or Create the PFO monitoring collection
  GaudiPfoMonitoring::PfoMonDataCollection *pfoColl = nullptr;
  if (m_createPfoMonData) {
    if (eventSvc
            ->retrieveObject("/Event/PfoMonitoringData", (DataObject *&)pfoColl)
            .isFailure()) {
      pfoColl = new GaudiPfoMonitoring::PfoMonDataCollection();
      if (eventSvc->registerObject("/Event/PfoMonitoringData", pfoColl)
              .isFailure()) {
        std::cout
            << "PfoMonitoringAlgorithm: Could not register PfoMonitoringData"
            << std::endl;
        delete pfoColl;
        return STATUS_CODE_FAILURE;
      }
    }
  }

  // 3. Retrieve or Create the cluster monitoring collection
  GaudiPfoMonitoring::ClusterMonDataCollection *clusterColl = nullptr;
  if (m_createClusterMonData) {
    if (eventSvc
            ->retrieveObject("/Event/ClusterMonitoringData",
                             (DataObject *&)clusterColl)
            .isFailure()) {
      clusterColl = new GaudiPfoMonitoring::ClusterMonDataCollection();
      if (eventSvc->registerObject("/Event/ClusterMonitoringData", clusterColl)
              .isFailure()) {
        std::cout << "PfoMonitoringAlgorithm: Could not register "
                     "ClusterMonitoringData"
                  << std::endl;
        delete clusterColl;
        return STATUS_CODE_FAILURE;
      }
    }
  }

  // 4. Retrieve or Create the CaloHit monitoring collection
  GaudiPfoMonitoring::CaloHitMonDataCollection *caloHitColl = nullptr;
  if (m_createCaloHitMonData) {
    if (eventSvc
            ->retrieveObject("/Event/CaloHitMonitoringData",
                             (DataObject *&)caloHitColl)
            .isFailure()) {
      caloHitColl = new GaudiPfoMonitoring::CaloHitMonDataCollection();
      if (eventSvc->registerObject("/Event/CaloHitMonitoringData", caloHitColl)
              .isFailure()) {
        std::cout << "PfoMonitoringAlgorithm: Could not register "
                     "CaloHitMonitoringData"
                  << std::endl;
        delete caloHitColl;
        return STATUS_CODE_FAILURE;
      }
    }
  }

  // 5. Retrieve or Create the event-level monitoring collection
  GaudiPfoMonitoring::EventMonDataCollection *eventColl = nullptr;
  if (m_createEventMonData) {
    if (eventSvc
            ->retrieveObject("/Event/EventMonitoringData",
                             (DataObject *&)eventColl)
            .isFailure()) {
      eventColl = new GaudiPfoMonitoring::EventMonDataCollection();
      if (eventSvc->registerObject("/Event/EventMonitoringData", eventColl)
              .isFailure()) {
        std::cout << "PfoMonitoringAlgorithm: Could not register "
                     "EventMonitoringData"
                  << std::endl;
        delete eventColl;
        return STATUS_CODE_FAILURE;
      }
    }
  }

  m_trackMcPfoTargets.clear();

  //--------------------------------------------------------------------------------------------------------------
  // get MC particles
  const MCParticleList *pMCParticleList(nullptr);
  PANDORA_RETURN_RESULT_IF(
      STATUS_CODE_SUCCESS, !=,
      PandoraContentApi::GetCurrentList(*this, pMCParticleList));
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // get PFOs
  const PfoList *pPfoList = NULL;
  PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=,
                           PandoraContentApi::GetCurrentList(*this, pPfoList));

  if (pPfoList->empty()) {
    // If this prints, your Pandora XML is calling this algorithm too early
    // or the "Current PFO List" is not set correctly.
    std::cout << "PfoMonitoringAlgorithm: Current PFO list is EMPTY."
              << std::endl;
  }

  //--------------------------------------------------------------------------------------------------------------
  // get all clusters
  const ClusterList *pAllClusters = NULL;
  PANDORA_RETURN_RESULT_IF(
      STATUS_CODE_SUCCESS, !=,
      PandoraContentApi::GetCurrentList(*this, pAllClusters));

  if (pAllClusters->empty()) {
    // If this prints, your Pandora XML is calling this algorithm too early
    // or the "Current cluster List" is not set correctly.
    std::cout << "PfoMonitoringAlgorithm: Current Cluster list is EMPTY."
              << std::endl;
  }

  //--------------------------------------------------------------------------------------------------------------
  // Find the most energetic cluster and compute its energy-weighted centroid
  CartesianVector mostEnergeticClusterCentroid(0.f, 0.f, 0.f);
  bool hasMostEnergeticCluster = false;
  const Cluster *pMostEnergeticCluster = nullptr;
  if (!pAllClusters->empty()) {
    float maxClusterEnergy = -std::numeric_limits<float>::max();
    for (const Cluster *const pCluster : *pAllClusters) {
      const float clusterEnergy(pCluster->GetHadronicEnergy());
      if (clusterEnergy > maxClusterEnergy) {
        maxClusterEnergy = clusterEnergy;
        pMostEnergeticCluster = pCluster;
      }
    }

    if (pMostEnergeticCluster) {
      float sumWeight = 0.f;
      float sumX = 0.f, sumY = 0.f, sumZ = 0.f;
      const OrderedCaloHitList &orderedList(
          pMostEnergeticCluster->GetOrderedCaloHitList());
      for (const auto &layerEntry : orderedList) {
        for (const CaloHit *const pHit : *layerEntry.second) {
          const float w(pHit->GetHadronicEnergy());
          const CartesianVector &p(pHit->GetPositionVector());
          sumWeight += w;
          sumX += w * p.GetX();
          sumY += w * p.GetY();
          sumZ += w * p.GetZ();
        }
      }
      if (sumWeight > std::numeric_limits<float>::epsilon()) {
        mostEnergeticClusterCentroid = CartesianVector(
            sumX / sumWeight, sumY / sumWeight, sumZ / sumWeight);
        hasMostEnergeticCluster = true;
      }
    }
  }

  // --------------------------------------------------------------------------------------------------------------
  // Get a MIP fit for the most energetic cluster, if it's a suitable parent for
  // cone merging
  ClusterFitResult mostEnergeticParentFitResult;
  bool hasParentFit = false;
  if (hasMostEnergeticCluster) {
    const unsigned int innerLayer(pMostEnergeticCluster->GetInnerPseudoLayer());
    const unsigned int showerStartLayer(
        pMostEnergeticCluster->GetShowerStartLayer(this->GetPandora()));

    // This logic mirrors ConeBasedMergingAlgorithm::PrepareClusters
    if ((innerLayer <= showerStartLayer) &&
        ((showerStartLayer - innerLayer) >= m_minLayersToShowerStart)) {
      const unsigned int fitEndLayer(
          (showerStartLayer > 1) ? showerStartLayer - 1 : 0);
      if (STATUS_CODE_SUCCESS ==
              ClusterFitHelper::FitLayers(pMostEnergeticCluster, innerLayer,
                                          fitEndLayer,
                                          mostEnergeticParentFitResult) &&
          mostEnergeticParentFitResult.IsFitSuccessful()) {
        hasParentFit = true;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------------------
  // Build a set of all clusters that are part of any PFO
  std::unordered_set<const Cluster *> pfoClusterSet;

  // Build a set of all hits that are part of any cluster to determine
  // availability
  std::unordered_set<const CaloHit *> clusteredHitSet;
  for (const Cluster *const pCluster : *pAllClusters) {
    const OrderedCaloHitList &orderedList(pCluster->GetOrderedCaloHitList());
    for (const auto &layerEntry : orderedList) {
      for (const CaloHit *const pHit : *layerEntry.second) {
        clusteredHitSet.insert(pHit);
      }
    }
  }

  //--------------------------------------------------------------------------------------------------------------
  // Fill PFO monitoring data from pPfoList
  for (PfoList::const_iterator pfoIter = pPfoList->begin();
       pfoIter != pPfoList->end(); ++pfoIter) {
    const ParticleFlowObject *const pPfo = *pfoIter;
    const ClusterList &clusterList(pPfo->GetClusterList());
    for (const Cluster *const pPfoCluster : clusterList) {
      pfoClusterSet.insert(pPfoCluster);
    }

    const TrackList &trackList(pPfo->GetTrackList());
    const float pfoEnergy(pPfo->GetEnergy());
    const int pfoPid(pPfo->GetParticleId());

    const CartesianVector &momentum(pPfo->GetMomentum());
    const float px = momentum.GetX();
    const float py = momentum.GetY();
    const float pz = momentum.GetZ();
    const float mass = pPfo->GetMass();

    unsigned int minInnerLayer(std::numeric_limits<unsigned int>::max());
    unsigned int maxOuterLayer(0);

    unsigned int nEmClusters = 0;
    unsigned int nHits = 0;
    unsigned int nMipLikeHits = 0;
    unsigned int nEcalHits = 0;
    unsigned int nMipEcalHits = 0;
    unsigned int nHcalHits = 0;
    unsigned int nMipHcalHits = 0;

    float minClusterDistance =
        -1.f; // Default minimum distance with most energetic cluster

    for (const Cluster *const pPfoCluster : clusterList) {
      if (this->GetPandora().GetPlugins()->GetParticleId()->IsEmShower(
              pPfoCluster))
        nEmClusters++;

      nHits += pPfoCluster->GetNCaloHits();
      nMipLikeHits += pPfoCluster->GetNPossibleMipHits();
      this->GetMipLikeHits(pPfoCluster, nEcalHits, nHcalHits, nMipEcalHits,
                           nMipHcalHits);

      if (pPfoCluster->GetInnerPseudoLayer() < minInnerLayer)
        minInnerLayer = pPfoCluster->GetInnerPseudoLayer();

      if (pPfoCluster->GetOuterPseudoLayer() > maxOuterLayer)
        maxOuterLayer = pPfoCluster->GetOuterPseudoLayer();

      if (hasMostEnergeticCluster && pMostEnergeticCluster != pPfoCluster)
        minClusterDistance = ClusterHelper::GetDistanceToClosestHit(
            pPfoCluster, pMostEnergeticCluster);
    }

    const unsigned int pfoStartLayer =
        (clusterList.empty() ? 0 : minInnerLayer);
    const unsigned int pfoNLayers =
        (clusterList.empty() ? 0 : (maxOuterLayer - minInnerLayer + 1));

    // Create the monitoring object and fill reconstructed information first
    if (!m_createPfoMonData || !pfoColl)
      continue;
    auto &pfoData = pfoColl->create();
    pfoData.setEnergy(pfoEnergy);
    pfoData.setPdg(pfoPid);
    pfoData.setPx(px);
    pfoData.setPy(py);
    pfoData.setPz(pz);
    pfoData.setMass(mass);
    pfoData.setNClusters(clusterList.size());
    pfoData.setNEmClusters(nEmClusters);
    pfoData.setNHits(nHits);
    pfoData.setNMipLikeHits(nMipLikeHits);
    pfoData.setNEcalHits(nEcalHits);
    pfoData.setNMipEcalHits(nMipEcalHits);
    pfoData.setNHcalHits(nHcalHits);
    pfoData.setNMipHcalHits(nMipHcalHits);
    pfoData.setMinClusterDistance(minClusterDistance);
    pfoData.setStartLayer(pfoStartLayer);
    pfoData.setNLayers(pfoNLayers);
    pfoData.setAlpha(0.f);

    // Default MC values in case matching fails
    pfoData.setMcPdg(0);
    pfoData.setMcEnergy(0.f);

    if (trackList.size() == 1) // charged PFO
    {
      try {
        const Track *const pTrack = trackList.front();
        const MCParticle *const pMCParticle(
            MCParticleHelper::GetMainMCParticle(pTrack));

        if (pMCParticle) {
          m_trackMcPfoTargets.push_back(pMCParticle);

          // if MC particle is pi+/-, K+/- or proton then this is a correct
          // assignment
          if (std::abs(pMCParticle->GetParticleId()) == 211 ||
              std::abs(pMCParticle->GetParticleId()) == 321 ||
              std::abs(pMCParticle->GetParticleId()) == 2212)
            m_nCorrectChargedHadronPfo++;
          else
            m_nWrongChargedHadronPfo++;

          pfoData.setMcPdg(pMCParticle->GetParticleId());
          pfoData.setMcEnergy(pMCParticle->GetEnergy());

          // Opening angle between PFO and MC particle
          const CartesianVector &mcMomentum(pMCParticle->GetMomentum());
          if (momentum.GetMagnitudeSquared() >
                  std::numeric_limits<float>::epsilon() &&
              mcMomentum.GetMagnitudeSquared() >
                  std::numeric_limits<float>::epsilon()) {
            pfoData.setAlpha(std::acos(std::max(
                -1.f, std::min(1.f, momentum.GetCosOpeningAngle(mcMomentum)))));
          }
        }
      } catch (StatusCodeException &e) {
        // This usually means Track-to-MC matching is not initialized in the
        // Pandora instance
        static int errorCount = 0;
        if (errorCount++ < 10)
          std::cout << "PfoMonitoring: MC match not found for track: "
                    << e.ToString() << std::endl;
      }
    }

    // store each MC particle (reco) energy contribution in this PFO
    MCParticleToFloatMap mcParticleContributions;
    float fCharged(0.f), fPhoton(0.f), fNeutral(0.f);
    // hadronic energy scale
    float totEnergy(0.f);
    float neutralEnergy(0.f);
    float photonEnergy(0.f);
    float chargedEnergy(0.f);

    // find energy fractions and the MC particle with largest (reco) energy
    // contribution
    for (ClusterList::const_iterator clusterIter = clusterList.begin();
         clusterIter != clusterList.end(); ++clusterIter) {
      const Cluster *const pCluster = *clusterIter;
      const float clusterEnergy(pCluster->GetHadronicEnergy());

      this->PfoMonitoringAlgorithm::ClusterEnergyFractions(
          pCluster, fCharged, fPhoton, fNeutral, mcParticleContributions);
      totEnergy += clusterEnergy;
      neutralEnergy += clusterEnergy * fNeutral;
      photonEnergy += clusterEnergy * fPhoton;
      chargedEnergy += clusterEnergy * fCharged;
    }

    // calculate energy fractions for the given PFO
    if (totEnergy > 0.f) {
      fCharged = chargedEnergy / totEnergy;
      fPhoton = photonEnergy / totEnergy;
      fNeutral = neutralEnergy / totEnergy;
    }
    pfoData.setFNeutral(fNeutral);
    pfoData.setFPhoton(fPhoton);
    pfoData.setFCharged(fCharged);

    if (trackList.empty()) // neutral PFO
    {
      try {
        const MCParticle *const pBestMCMatch =
            this->GetBestMCParticleMatch(mcParticleContributions);

        if (pBestMCMatch) {
          if (fNeutral > m_neutralHadronEnergyFractionCut)
            m_nCorrectNeutralHadronPfo++;
          else
            m_nWrongNeutralHadronPfo++;

          pfoData.setMcPdg(pBestMCMatch->GetParticleId());
          pfoData.setMcEnergy(pBestMCMatch->GetEnergy());

          // Opening angle (cosine) between PFO and MC particle
          const CartesianVector &mcMomentum(pBestMCMatch->GetMomentum());
          if (momentum.GetMagnitudeSquared() >
                  std::numeric_limits<float>::epsilon() &&
              mcMomentum.GetMagnitudeSquared() >
                  std::numeric_limits<float>::epsilon()) {
            pfoData.setAlpha(std::acos(std::max(
                -1.f, std::min(1.f, momentum.GetCosOpeningAngle(mcMomentum)))));
          }
        }
      } catch (StatusCodeException &e) {
        static int errorCountNeutral = 0;
        if (errorCountNeutral++ < 10)
          std::cout << "PfoMonitoring: MC match error for neutral: "
                    << e.ToString() << std::endl;
      }
    } // neutral PFO
  }
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // Fill cluster monitoring data from pAllClusters
  if (m_createClusterMonData) {
    // sort the clusters with hadronic energy
    ClusterVector sortedClusters(pAllClusters->begin(), pAllClusters->end());
    std::sort(sortedClusters.begin(), sortedClusters.end(),
              [](const Cluster *const pLhs, const Cluster *const pRhs) {
                return (pLhs->GetHadronicEnergy() > pRhs->GetHadronicEnergy());
              });

    for (const Cluster *const pCluster : sortedClusters) {
      // cluster energy fractions and best matched MC particle
      float clusFCharged(0.f), clusFPhoton(0.f), clusFNeutral(0.f);
      const MCParticle *const pBestMCMatch = this->GetClusterMCParticleInfo(
          pCluster, clusFCharged, clusFPhoton, clusFNeutral);

      // flag showing if the cluster can be merged
      const bool canBeMerged = ClusterHelper::CanMergeCluster(
          this->GetPandora(), pCluster, m_canMergeMinMipFraction,
          m_canMergeMaxRms);

      // ECal and HCal hits in the cluster
      unsigned int clusEcalHits = 0;
      unsigned int clusHcalHits = 0;
      unsigned int clusMipEcalHits = 0;
      unsigned int clusMipHcalHits = 0;
      this->GetMipLikeHits(pCluster, clusEcalHits, clusHcalHits,
                           clusMipEcalHits, clusMipHcalHits);

      // cluster start layer and shower start layer
      const unsigned int clusStartLayer = pCluster->GetInnerPseudoLayer();
      const unsigned int clusShowerStartLayer =
          pCluster->GetShowerStartLayer(this->GetPandora());

      // number of layers occupied by the cluster
      const unsigned int clusNLayers =
          pCluster->GetOuterPseudoLayer() - clusStartLayer + 1;

      // if the cluster is classified as EM-shower
      const unsigned int clusIsEm =
          this->GetPandora().GetPlugins()->GetParticleId()->IsEmShower(pCluster)
              ? 1
              : 0;

      // if the cluster passes the photonId
      const unsigned int clusPassPhotonId =
          pCluster->PassPhotonId(this->GetPandora()) ? 1 : 0;

      // minimum distance with most energetic cluster
      float clusMinDist = -1.f;
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster)
        clusMinDist = ClusterHelper::GetDistanceToClosestHit(
            pCluster, pMostEnergeticCluster);

      // calculate energy-weighted centroid of the cluster
      float sumWeight = 0.f;
      float sumX = 0.f, sumY = 0.f, sumZ = 0.f;
      CartesianVector clusterCentroid(0.f, 0.f, 0.f);
      const OrderedCaloHitList &orderedList(pCluster->GetOrderedCaloHitList());
      for (const auto &layerEntry : orderedList) {
        for (const CaloHit *const pHit : *layerEntry.second) {
          const float w(pHit->GetHadronicEnergy());
          const CartesianVector &p(pHit->GetPositionVector());
          sumWeight += w;
          sumX += w * p.GetX();
          sumY += w * p.GetY();
          sumZ += w * p.GetZ();
        }
      }
      if (sumWeight > std::numeric_limits<float>::epsilon()) {
        clusterCentroid = CartesianVector(sumX / sumWeight, sumY / sumWeight,
                                          sumZ / sumWeight);
      }

      // 3D distance from cluster centroid to energy-weighted centroid of the
      // most energetic cluster
      const float distToMECC =
          hasMostEnergeticCluster
              ? (clusterCentroid - mostEnergeticClusterCentroid).GetMagnitude()
              : -1.f;

      // ---------------------------------------------------------------
      // Quantities mirroring ProximityBasedMergingAlgorithm cuts:
      //   minInnerLayerSeparation  -- m_maxInnerLayerSeparation
      //   minGenericDistance       -- m_maxGenericDistance  (perpendicular)
      //   minParallelDistance      -- m_maxParallelDistance (parallel)
      // Calculated specifically with respect to the most energetic cluster.
      // ---------------------------------------------------------------
      float clusMinInnerLayerSep = -1.f;
      float clusMinGenericDist = -1.f;
      float clusMinParallelDist = -1.f;
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster) {

        // --- inner-layer separation ---
        const CartesianVector thisInnerCentroid(
            pCluster->GetCentroid(pCluster->GetInnerPseudoLayer()));
        const CartesianVector otherInnerCentroid(
            pMostEnergeticCluster->GetCentroid(
                pMostEnergeticCluster->GetInnerPseudoLayer()));
        clusMinInnerLayerSep =
            (thisInnerCentroid - otherInnerCentroid).GetMagnitude();

        // --- generic (perpendicular) distance and associated parallel distance
        // --- In the context of GetGenericDistanceBetweenClusters,
        // pMostEnergeticCluster is the parent and pCluster is the daughter.
        // number of layers to examine in the parent cluster starting from the
        // inner layer of daughter cluster number of adjacent layers to examine
        // in the daughter cluster
        const unsigned int startLayer(clusStartLayer);
        const unsigned int endLayer(clusStartLayer + m_nGenericDistanceLayers);

        const OrderedCaloHitList &orderedCaloHitListParent(
            pMostEnergeticCluster->GetOrderedCaloHitList());
        const OrderedCaloHitList &orderedCaloHitListDaughter(
            pCluster->GetOrderedCaloHitList());

        for (unsigned int iLayer = startLayer; iLayer <= endLayer; ++iLayer) {
          OrderedCaloHitList::const_iterator iterP =
              orderedCaloHitListParent.find(iLayer);

          if (orderedCaloHitListParent.end() == iterP)
            continue;

          // Loop over hits in parent cluster that fall between specified layers
          // of dauther cluster
          for (CaloHitList::const_iterator hitIterP = iterP->second->begin(),
                                           hitIterPEnd = iterP->second->end();
               hitIterP != hitIterPEnd; ++hitIterP) {
            const CartesianVector &positionP((*hitIterP)->GetPositionVector());
            const CartesianVector &directionP(
                (*hitIterP)->GetExpectedDirection());

            // For each hit, consider distance to all hits in daughter cluster
            // that lie within +/-nAdjacentLayers
            const unsigned int firstExaminationLayer(
                (iLayer > m_nAdjacentLayers) ? iLayer - m_nAdjacentLayers : 0);
            const unsigned int lastExaminationLayer(iLayer + m_nAdjacentLayers);

            for (unsigned int iExaminationLayer = firstExaminationLayer;
                 iExaminationLayer <= lastExaminationLayer;
                 ++iExaminationLayer) {
              OrderedCaloHitList::const_iterator iterD =
                  orderedCaloHitListDaughter.find(iExaminationLayer);

              if (orderedCaloHitListDaughter.end() == iterD)
                continue;

              for (CaloHitList::const_iterator
                       hitIterD = iterD->second->begin(),
                       hitIterDEnd = iterD->second->end();
                   hitIterD != hitIterDEnd; ++hitIterD) {
                const CartesianVector positionDifference(
                    positionP - (*hitIterD)->GetPositionVector());

                const float perpendicularDistance(
                    (directionP.GetCrossProduct(positionDifference))
                        .GetMagnitude());
                const float parallelDistance(
                    std::fabs(directionP.GetDotProduct(positionDifference)));

                if (clusMinGenericDist < 0.f ||
                    perpendicularDistance < clusMinGenericDist) {
                  clusMinGenericDist = perpendicularDistance;
                  clusMinParallelDist = parallelDistance;
                }
              }
            }
          }
        }
      }

      if (!clusterColl)
        continue;
      auto &clusData = clusterColl->create();
      clusData.setEnergy(pCluster->GetHadronicEnergy());
      clusData.setNHits(pCluster->GetNCaloHits());
      clusData.setNMipLikeHits(pCluster->GetNPossibleMipHits());
      clusData.setNEcalHits(clusEcalHits);
      clusData.setNHcalHits(clusHcalHits);
      clusData.setNMipEcalHits(clusMipEcalHits);
      clusData.setNMipHcalHits(clusMipHcalHits);
      clusData.setStartLayer(clusStartLayer);
      clusData.setShowerStartLayer(clusShowerStartLayer);
      clusData.setNLayers(clusNLayers);
      clusData.setIsEm(clusIsEm);
      clusData.setPassPhotonId(clusPassPhotonId);
      clusData.setMinClusterDistance(clusMinDist);
      clusData.setDistToMostEnergeticClusterCentroid(distToMECC);
      clusData.setIsInPfo(pfoClusterSet.count(pCluster) ? 1 : 0);
      clusData.setHasAssociatedTrack(
          pCluster->GetAssociatedTrackList().empty() ? 0 : 1);
      clusData.setCanBeMerged(canBeMerged ? 1 : 0);
      clusData.setMcPdg(pBestMCMatch ? pBestMCMatch->GetParticleId() : 0);
      clusData.setMcEnergy(pBestMCMatch ? pBestMCMatch->GetEnergy() : 0.f);
      clusData.setFNeutral(clusFNeutral);
      clusData.setFPhoton(clusFPhoton);
      clusData.setFCharged(clusFCharged);
      clusData.setMinInnerLayerSeparation(clusMinInnerLayerSep);
      const pandora::ClusterFitResult &fitResult =
          pCluster->GetFitToAllHitsResult();
      clusData.setRms(fitResult.IsFitSuccessful() ? fitResult.GetRms() : -1.f);
      clusData.setDCosR(fitResult.IsFitSuccessful()
                            ? fitResult.GetRadialDirectionCosine()
                            : -2.f);
      clusData.setMinGenericDistance(clusMinGenericDist);
      clusData.setMinParallelDistance(clusMinParallelDist);

      // --- layer span and shower layer span (mirroring ProximityBasedMerging)
      // --- Parent = most energetic cluster, daughter = current cluster
      int clusLayerSpan = std::numeric_limits<int>::min();
      int clusShowerLayerSpan = std::numeric_limits<int>::min();
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster) {
        const int layerSpan1 =
            static_cast<int>(pMostEnergeticCluster->GetOuterPseudoLayer()) -
            static_cast<int>(clusStartLayer);
        const int layerSpan2 =
            static_cast<int>(pCluster->GetOuterPseudoLayer()) -
            static_cast<int>(pMostEnergeticCluster->GetInnerPseudoLayer());
        clusLayerSpan = std::min(layerSpan1, layerSpan2);
        clusShowerLayerSpan =
            static_cast<int>(clusStartLayer) -
            static_cast<int>(
                pMostEnergeticCluster->GetShowerStartLayer(this->GetPandora()));
      }

      // --- contact fraction (mirroring ProximityBasedMerging) ---
      float clusContactFraction = -1.f;
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster) {
        unsigned int nContactLayers = 0;
        if (STATUS_CODE_SUCCESS ==
            lc_content::FragmentRemovalHelper::GetClusterContactDetails(
                pCluster, pMostEnergeticCluster, m_clusterContactThreshold,
                nContactLayers, clusContactFraction)) {
          // success
        }
      }

      // --- close hit fraction (mirroring ProximityBasedMerging) ---
      float clusCloseHitFraction = -1.f;
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster) {
        clusCloseHitFraction =
            lc_content::FragmentRemovalHelper::GetFractionOfCloseHits(
                pCluster, pMostEnergeticCluster, m_closeHitThreshold);
      }

      // --- fraction in cone (mirroring ConeBasedMerging) ---
      float clusFractionInCone = -1.f;
      if (hasMostEnergeticCluster && pMostEnergeticCluster != pCluster &&
          hasParentFit) {
        clusFractionInCone = this->GetFractionInCone(
            pMostEnergeticCluster, pCluster, mostEnergeticParentFitResult);
      }

      // --- shower quantities (mirroring LCParticleIdPlugins::LCEmShowerId) ---
      float clusNRadLengths(-1.f), clusNRadLengthsBeforeClusterStart(-1.f),
          clusLayer90RadLengths(-1.f), clusShowerMaxRadLengths(-1.f),
          clusEnergyAboveHighRadLengths(-1.f), clusRadial90(-1.f);
      this->GetClusterShowerQuantities(
          pCluster, clusNRadLengths, clusNRadLengthsBeforeClusterStart,
          clusLayer90RadLengths, clusShowerMaxRadLengths,
          clusEnergyAboveHighRadLengths, clusRadial90);

      const float clusTotalEMEnergy =
          pCluster->GetElectromagneticEnergy() -
          pCluster->GetIsolatedElectromagneticEnergy();
      const float clusFractionAboveHighRadLengths =
          (clusTotalEMEnergy > std::numeric_limits<float>::epsilon())
              ? clusEnergyAboveHighRadLengths / clusTotalEMEnergy
              : -1.f;

      clusData.setLayerSpan(clusLayerSpan);
      clusData.setShowerLayerSpan(clusShowerLayerSpan);
      clusData.setContactFraction(clusContactFraction);
      clusData.setCloseHitFraction(clusCloseHitFraction);
      clusData.setFractionInCone(clusFractionInCone);
      clusData.setNRadiationLengths(clusNRadLengths);
      clusData.setNRadiationLengthsBeforeClusterStart(
          clusNRadLengthsBeforeClusterStart);
      clusData.setLayer90RadLengths(clusLayer90RadLengths);
      clusData.setShowerMaxRadLengths(clusShowerMaxRadLengths);
      clusData.setFractionOfEnergyAboveHighRadLengths(
          clusFractionAboveHighRadLengths);
      clusData.setRadial90(clusRadial90);
    }
  }
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // Fill calo hit monitoring data from all hits
  if (m_createCaloHitMonData && caloHitColl) {
    const CaloHitList *pCaloHitList = nullptr;
    if (PandoraContentApi::GetCurrentList(*this, pCaloHitList) !=
            STATUS_CODE_SUCCESS ||
        !pCaloHitList)
      return STATUS_CODE_SUCCESS;

    // Initialize KDTree for efficient hit isolation calculations
    typedef KDTreeNodeInfoT<const pandora::CaloHit *, 4> HitKDNode4D;
    typedef KDTreeLinkerAlgo<const pandora::CaloHit *, 4> HitKDTree4D;

    std::vector<HitKDNode4D> hitNodes4D;
    HitKDTree4D hitsKdTree4D;
    if (!pCaloHitList->empty()) {
      KDTreeTesseract hitsBoundingRegion4D =
          fill_and_bound_4d_kd_tree(this, *pCaloHitList, hitNodes4D, true);
      hitsKdTree4D.build(hitNodes4D, hitsBoundingRegion4D);
    }

    std::vector<HitKDNode4D> found; // Re-use search result buffer

    // Helper function to replicate
    // CaloHitPreparationAlgorithm::IsolationCountNearbyHits
    auto isolationCountNearbyHits =
        [&](unsigned int searchLayer, const CaloHit *const pCaloHit,
            float &shortestIsolationDist,
            std::vector<HitKDNode4D> &foundBuffer) -> unsigned int {
      const CartesianVector &pos(pCaloHit->GetPositionVector());
      const float mag2(pos.GetMagnitudeSquared());
      const float isolationCutDistanceSquared(
          (this->GetPandora().GetGeometry()->GetHitTypeGranularity(
               pCaloHit->GetHitType()) <= FINE)
              ? m_isolationCutDistanceFine2
              : m_isolationCutDistanceCoarse2);
      const float maxSeparation2(1000.f * 1000.f);

      unsigned int nearbyHitsFound = 0;
      const float searchDistance(m_isolationSearchSafetyFactor *
                                 std::sqrt(isolationCutDistanceSquared));
      KDTreeTesseract searchRegionHits = build_4d_kd_search_region(
          pCaloHit, searchDistance, searchDistance, searchDistance,
          static_cast<float>(searchLayer));

      foundBuffer.clear();
      hitsKdTree4D.search(searchRegionHits, foundBuffer);

      for (const auto &hitNode : foundBuffer) {
        const CaloHit *const pOtherHit = hitNode.data;
        if (pCaloHit == pOtherHit)
          continue;

        const CartesianVector diff(pos - pOtherHit->GetPositionVector());
        if (diff.GetMagnitudeSquared() > maxSeparation2)
          continue;

        float isolationDistance =
            std::sqrt(pos.GetCrossProduct(diff).GetMagnitudeSquared() / mag2);
        if (isolationDistance < shortestIsolationDist)
          shortestIsolationDist = isolationDistance;
        if (isolationDistance < std::sqrt(isolationCutDistanceSquared))
          ++nearbyHitsFound;
      }
      return nearbyHitsFound;
    };

    // Helper lambda to fill one CaloHit entry
    auto fillHit = [&](const CaloHit *const pCaloHit, bool isIsolated,
                       std::vector<HitKDNode4D> &foundBuffer) {
      const CartesianVector &pos(pCaloHit->GetPositionVector());
      auto &hitData = caloHitColl->create();
      hitData.setEnergy(pCaloHit->GetInputEnergy());
      hitData.setPseudoLayer(static_cast<uint32_t>(pCaloHit->GetPseudoLayer()));
      hitData.setCellLengthScale(pCaloHit->GetCellLengthScale());
      hitData.setIsIsolated(isIsolated ? 1u : 0u);
      hitData.setIsPossibleMip(pCaloHit->IsPossibleMip() ? 1u : 0u);
      hitData.setPositionX(pos.GetX());
      hitData.setPositionY(pos.GetY());
      hitData.setPositionZ(pos.GetZ());

      const pandora::HitType hitType(pCaloHit->GetHitType());
      // Map: 0 for ECAL, 1 for HCAL, 2 for others
      hitData.setHitType((hitType == pandora::ECAL)
                             ? 0u
                             : (hitType == pandora::HCAL ? 1u : 2u));

      // Calculate isolationNearbyHits (replicating logic from
      // CaloHitPreparationAlgorithm)
      unsigned int isolationNearbyHits = 0;
      float shortestIsolationDist = std::numeric_limits<float>::max();
      if (!hitNodes4D.empty()) {
        const unsigned int layerI(pCaloHit->GetPseudoLayer());
        const unsigned int minLayer(
            (layerI < m_isolationNLayers) ? 0 : layerI - m_isolationNLayers);
        const unsigned int maxLayer(layerI + m_isolationNLayers);

        for (unsigned int iLayer = minLayer; iLayer <= maxLayer; ++iLayer) {
          isolationNearbyHits += isolationCountNearbyHits(
              iLayer, pCaloHit, shortestIsolationDist, foundBuffer);
        }
      }
      hitData.setIsolationNearbyHits(isolationNearbyHits);
      hitData.setShortestIsolationDist(
          shortestIsolationDist > 500000.f ? -1.f : shortestIsolationDist);

      // 3D distance from hit to energy-weighted centroid of the most energetic
      // cluster
      const float distToMECC =
          hasMostEnergeticCluster
              ? (pos - mostEnergeticClusterCentroid).GetMagnitude()
              : -1.f;
      hitData.setDistToMostEnergeticClusterCentroid(distToMECC);
      // determine if the hit is from charged or neutral particle
      const MCParticle *const pMCParticle(
          MCParticleHelper::GetMainMCParticle(pCaloHit));
      const MCParticle *const pMCPfoTarget(pMCParticle->GetPfoTarget());
      hitData.setMcPdg(pMCPfoTarget ? pMCPfoTarget->GetParticleId() : 0);
    };

    // Iterate over all hits and fill the hitData
    for (const CaloHit *const pCaloHit : *pCaloHitList) {
      fillHit(pCaloHit, pCaloHit->IsIsolated(), found);
    }
  }
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // Fill event-level monitoring data
  if (m_createEventMonData && eventColl) {
    // Count hits
    unsigned int nClusteredNonIsolatedHits = 0;
    unsigned int nUnclusteredNonIsolatedHits = 0;
    unsigned int nClusteredIsolatedHits = 0;
    unsigned int nUnclusteredIsolatedHits = 0;

    // unclustered isolated hits energy
    float unclusteredIsolatedEnergy = 0.f;
    // unclustered non-isolated hits energy
    float unclusteredNonIsolatedEnergy = 0.f;
    // clustered isolated hits energy
    float clusteredIsolatedEnergy = 0.f;
    // clustered non-isolated hits energy
    float clusteredNonIsolatedEnergy = 0.f;
    // total hits energy
    float totalEnergy = 0.f;
    // charged energy (hadronic scale)
    float chargedEnergy = 0.f;
    // neutral energy (hadronic scale)
    float neutralEnergy = 0.f;
    // charged energy reconstructed as neutral
    float chargedEnergyRecoNeutral = 0.f;
    // charged energy reconstructed as charged
    float chargedEnergyRecoCharged = 0.f;
    // neutral energy reconstructed as charged
    float neutralEnergyRecoCharged = 0.f;
    // neutral energy reconstructed as neutral
    float neutralEnergyRecoNeutral = 0.f;

    // all CaloHits
    const CaloHitList *pCurrentCaloHitList = nullptr;
    if (PandoraContentApi::GetCurrentList(*this, pCurrentCaloHitList) ==
            STATUS_CODE_SUCCESS &&
        pCurrentCaloHitList) {
      for (const CaloHit *const pCaloHit : *pCurrentCaloHitList) {
        // determine if the hit is from charged or neutral particle
        const MCParticle *const pMCParticle(
            MCParticleHelper::GetMainMCParticle(pCaloHit));
        const MCParticle *const pMCPfoTarget(pMCParticle->GetPfoTarget());
        const int pdgCode(pMCPfoTarget->GetParticleId());
        const int charge(PdgTable::GetParticleCharge(pdgCode));
        (charge == 0) ? neutralEnergy += pCaloHit->GetHadronicEnergy()
                      : chargedEnergy += pCaloHit->GetHadronicEnergy();
        // isolated hits
        if (pCaloHit->IsIsolated() &&
            clusteredHitSet.count(pCaloHit)) // clustered
        {
          nClusteredIsolatedHits++;
          clusteredIsolatedEnergy += pCaloHit->GetInputEnergy();
          totalEnergy += pCaloHit->GetInputEnergy();
        } else if (pCaloHit->IsIsolated()) // unclustered
        {
          nUnclusteredIsolatedHits++;
          unclusteredIsolatedEnergy += pCaloHit->GetInputEnergy();
          totalEnergy += pCaloHit->GetInputEnergy();
        }

        // non-isolated hits
        if (!pCaloHit->IsIsolated() &&
            clusteredHitSet.count(pCaloHit)) // clusterd
        {
          nClusteredNonIsolatedHits++;
          clusteredNonIsolatedEnergy += pCaloHit->GetInputEnergy();
          totalEnergy += pCaloHit->GetInputEnergy();
        } else if (!pCaloHit->IsIsolated()) // unclustered
        {
          nUnclusteredNonIsolatedHits++;
          unclusteredNonIsolatedEnergy += pCaloHit->GetInputEnergy();
          totalEnergy += pCaloHit->GetInputEnergy();
        }
      }
    }

    // Compute charged/neutral energies from clusters 
    if (m_createClusterMonData) {
      for (const auto &clusData : *clusterColl) {
        // consider only clusters that can from a PFO
        if (!m_createPfoMonData && clusData.getEnergy() < 0.25) continue;
        if(m_createClusterMonData && clusData.getIsInPfo() == 0) continue;
        const float clusEnergy(clusData.getEnergy());
        const float fCh(clusData.getFCharged());
        const float fNe(clusData.getFNeutral());
        const float fPh(clusData.getFPhoton());
        
        // charged cluster
        if (clusData.getHasAssociatedTrack()) {
          chargedEnergyRecoCharged  += clusEnergy * fCh;
          neutralEnergyRecoCharged  += clusEnergy * (fNe + fPh);
        }
        // neutral cluster
        else {
          chargedEnergyRecoNeutral  += clusEnergy * fCh;
          neutralEnergyRecoNeutral  += clusEnergy * (fNe + fPh);
        }
      }
    }
    auto &evtData = eventColl->create();
    evtData.setEventNumber(m_eventNumber++);
    evtData.setNClusteredNonIsolatedHits(nClusteredNonIsolatedHits);
    evtData.setNClusteredIsolatedHits(nClusteredIsolatedHits);
    evtData.setNUnclusteredIsolatedHits(nUnclusteredIsolatedHits);
    evtData.setNUnclusteredNonIsolatedHits(nUnclusteredNonIsolatedHits);
    evtData.setClusteredIsolatedEnergy(clusteredIsolatedEnergy);
    evtData.setClusteredNonIsolatedEnergy(clusteredNonIsolatedEnergy);
    evtData.setUnclusteredIsolatedEnergy(unclusteredIsolatedEnergy);
    evtData.setUnclusteredNonIsolatedEnergy(unclusteredNonIsolatedEnergy);
    evtData.setTotalEnergy(totalEnergy);
    evtData.setChargedEnergy(chargedEnergy);
    evtData.setNeutralEnergy(neutralEnergy);
    evtData.setFChargedEnergyRecoCharged(
        chargedEnergy > std::numeric_limits<float>::epsilon()
            ? chargedEnergyRecoCharged / chargedEnergy
            : -1.f);
    evtData.setFChargedEnergyRecoNeutral(
        chargedEnergy > std::numeric_limits<float>::epsilon()
            ? chargedEnergyRecoNeutral / chargedEnergy
            : -1.f);
    evtData.setFNeutralEnergyRecoCharged(
        neutralEnergy > std::numeric_limits<float>::epsilon()
            ? neutralEnergyRecoCharged / neutralEnergy
            : -1.f);
    evtData.setFNeutralEnergyRecoNeutral(
        neutralEnergy > std::numeric_limits<float>::epsilon()
            ? neutralEnergyRecoNeutral / neutralEnergy
            : -1.f);
    evtData.setNClusters(static_cast<unsigned int>(pAllClusters->size()));
    evtData.setNPFOs(static_cast<unsigned int>(pPfoList->size()));
  }
  //--------------------------------------------------------------------------------------------------------------

  return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void PfoMonitoringAlgorithm::ClusterEnergyFractions(
    const Cluster *const pCluster, float &fCharged, float &fPhoton,
    float &fNeutral, MCParticleToFloatMap &mcParticleContributions) const {
  float totEnergy(0.f);
  float neutralEnergy(0.f);
  float photonEnergy(0.f);
  float chargedEnergy(0.f);

  // Create a single list of all hits in the cluster
  CaloHitList allHits;
  pCluster->GetOrderedCaloHitList().FillCaloHitList(allHits);
  // Add isolated hits
  const CaloHitList &isolatedHits = pCluster->GetIsolatedCaloHitList();
  allHits.insert(allHits.end(), isolatedHits.begin(), isolatedHits.end());

  for (const CaloHit *const pCaloHit : allHits) {
    try {
      const MCParticle *const pMCParticle(
          MCParticleHelper::GetMainMCParticle(pCaloHit));
      const MCParticle *const pMCPfoTarget(pMCParticle->GetPfoTarget());

      totEnergy += pCaloHit->GetHadronicEnergy();
      MCParticleToFloatMap::iterator it =
          mcParticleContributions.find(pMCPfoTarget);

      if (it != mcParticleContributions.end()) {
        it->second += pCaloHit->GetHadronicEnergy();
      } else {
        mcParticleContributions.insert(MCParticleToFloatMap::value_type(
            pMCPfoTarget, pCaloHit->GetHadronicEnergy()));
      }

      const int pdgCode(pMCPfoTarget->GetParticleId());
      const int charge(PdgTable::GetParticleCharge(pdgCode));

      if ((charge != 0) || (std::abs(pdgCode) == LAMBDA) ||
          (std::abs(pdgCode) == K_SHORT)) {
        if (m_trackMcPfoTargets.end() ==
                std::find(m_trackMcPfoTargets.begin(),
                          m_trackMcPfoTargets.end(), pMCParticle) &&
            m_createPfoMonData) {
          neutralEnergy += pCaloHit->GetHadronicEnergy();
        } else {
          chargedEnergy += pCaloHit->GetHadronicEnergy();
        }
      } else {
        (pMCParticle->GetParticleId() == PHOTON)
            ? photonEnergy += pCaloHit->GetHadronicEnergy()
            : neutralEnergy += pCaloHit->GetHadronicEnergy();
      }
    } catch (StatusCodeException &) {
    }
  }
  if (totEnergy > 0.f) {
    fCharged = chargedEnergy / totEnergy;
    fPhoton = photonEnergy / totEnergy;
    fNeutral = neutralEnergy / totEnergy;
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void PfoMonitoringAlgorithm::GetMipLikeHits(
    const pandora::Cluster *const pCluster, unsigned int &nEcalHits,
    unsigned int &nHcalHits, unsigned int &nMipEcalHits,
    unsigned int &nMipHcalHits) const {
  // The OrderedCaloHitList organizes hits by pseudo-layer
  const pandora::OrderedCaloHitList &orderedCaloHitList(
      pCluster->GetOrderedCaloHitList());

  for (const auto &layerEntry : orderedCaloHitList) {
    for (const pandora::CaloHit *const pCaloHit : *layerEntry.second) {
      const pandora::HitType hitType(pCaloHit->GetHitType());
      // const pandora::HitRegion hitRegion(pCaloHit->GetHitRegion());

      // Check if the hit belongs to the ECAL (Barrel or Endcap)
      if (hitType == pandora::ECAL || hitType == pandora::ECAL) {
        nEcalHits++;
        // IsPossibleMip() is the internal Pandora flag for MIP-like hits
        if (pCaloHit->IsPossibleMip()) {
          nMipEcalHits++;
        }
      }
      // Check if the hit belongs to the HCAL (Barrel or Endcap)
      if (hitType == pandora::HCAL || hitType == pandora::HCAL) {
        nHcalHits++;
        // IsPossibleMip() is the internal Pandora flag for MIP-like hits
        if (pCaloHit->IsPossibleMip()) {
          nMipHcalHits++;
        }
      }
    }
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void PfoMonitoringAlgorithm::GetClusterShowerQuantities(
    const pandora::Cluster *const pCluster, float &nRadiationLengths,
    float &nRadiationLengthsBeforeClusterStart, float &layer90RadLengths,
    float &showerMaxRadLengths, float &energyAboveHighRadLengths,
    float &radial90) const {
  nRadiationLengths = -1.f;
  nRadiationLengthsBeforeClusterStart = -1.f;
  layer90RadLengths = -1.f;
  showerMaxRadLengths = -1.f;
  energyAboveHighRadLengths = -1.f;
  radial90 = -1.f;

  const float totalElectromagneticEnergy(
      pCluster->GetElectromagneticEnergy() -
      pCluster->GetIsolatedElectromagneticEnergy());
  if (totalElectromagneticEnergy < std::numeric_limits<float>::epsilon())
    return;

  const CartesianVector &clusterDirection(
      pCluster->GetFitToAllHitsResult().IsFitSuccessful()
          ? pCluster->GetFitToAllHitsResult().GetDirection()
          : pCluster->GetInitialDirection());

  const CartesianVector &clusterIntercept(
      pCluster->GetFitToAllHitsResult().IsFitSuccessful()
          ? pCluster->GetFitToAllHitsResult().GetIntercept()
          : CartesianVector(0.f, 0.f, 0.f));

  const float minCosAngle(0.3f);
  const OrderedCaloHitList &orderedCaloHitList(
      pCluster->GetOrderedCaloHitList());
  const unsigned int innerPseudoLayer(pCluster->GetInnerPseudoLayer());
  const unsigned int firstPseudoLayer(this->GetPandora()
                                          .GetPlugins()
                                          ->GetPseudoLayerPlugin()
                                          ->GetPseudoLayerAtIp());

  // Calculate properties of longitudinal shower profile: layer90 and shower max
  // layer
  bool foundLayer90(false);
  float layer90EnergySum(0.f);
  float maxEnergyInlayer(0.f);
  float nRadLengths(0.f), nRadLengthsInLastLayer(0.f);

  energyAboveHighRadLengths = 0.f;
  nRadiationLengthsBeforeClusterStart = 0.f;
  showerMaxRadLengths = 0.f;

  // For radial90: collect (hitEnergy, radialDistance) pairs
  typedef std::pair<float, float> HitEnergyDistance;
  std::vector<HitEnergyDistance> hitEnergyDistanceVector;

  for (unsigned int iLayer = innerPseudoLayer,
                    outerPseudoLayer = pCluster->GetOuterPseudoLayer();
       iLayer <= outerPseudoLayer; ++iLayer) {
    OrderedCaloHitList::const_iterator iter = orderedCaloHitList.find(iLayer);
    if ((orderedCaloHitList.end() == iter) || (iter->second->empty())) {
      nRadLengths += nRadLengthsInLastLayer;
      continue;
    }

    const unsigned int nHitsInLayer(iter->second->size());
    if (0 == nHitsInLayer)
      continue;

    float energyInLayer(0.f);
    float nRadiationLengthsInLayer(0.f);

    for (CaloHitList::const_iterator hitIter = iter->second->begin(),
                                     hitIterEnd = iter->second->end();
         hitIter != hitIterEnd; ++hitIter) {
      float cosOpeningAngle(
          std::fabs((*hitIter)->GetCellNormalVector().GetCosOpeningAngle(
              clusterDirection)));
      cosOpeningAngle = std::max(cosOpeningAngle, minCosAngle);

      const float hitEnergy((*hitIter)->GetElectromagneticEnergy());
      energyInLayer += hitEnergy;
      nRadiationLengthsInLayer +=
          (*hitIter)->GetNCellRadiationLengths() / cosOpeningAngle;

      const float radialDistance(
          ((*hitIter)->GetPositionVector() - clusterIntercept)
              .GetCrossProduct(clusterDirection)
              .GetMagnitude());
      hitEnergyDistanceVector.push_back(
          HitEnergyDistance(hitEnergy, radialDistance));
    }

    layer90EnergySum += energyInLayer;
    nRadiationLengthsInLayer /= static_cast<float>(nHitsInLayer);
    nRadLengthsInLastLayer = nRadiationLengthsInLayer;
    nRadLengths += nRadiationLengthsInLayer;

    // Identify number of radiation lengths before cluster start
    // NOTE: this follows the logic of LCParticleIdPlugins::LCEmShowerId, which
    // assumes that all layers are identical (this is not true for ALLEGRO).
    if (innerPseudoLayer == iLayer) {
      nRadLengths *=
          static_cast<float>(innerPseudoLayer + 1 - firstPseudoLayer);
      nRadiationLengthsBeforeClusterStart = nRadLengths;
    }

    // Identify number of radiation lengths before longitudinal layer90
    if (!foundLayer90 &&
        (layer90EnergySum > 0.9f * totalElectromagneticEnergy)) {
      foundLayer90 = true;
      layer90RadLengths = nRadLengths;
    }

    // Identify number of radiation lengths before the shower maximum layer
    // (layer with highest energy)
    if (energyInLayer > maxEnergyInlayer) {
      showerMaxRadLengths = nRadLengths;
      maxEnergyInlayer = energyInLayer;
    }
    // Count energy above specified "high" number of radiation lengths
    if (nRadLengths > m_highRadLengths) {
      energyAboveHighRadLengths += energyInLayer;
    }
  }

  // Copy total radiation lengths to output parameter
  nRadiationLengths = nRadLengths;

  // Sort by radial distance and find radial90
  std::sort(hitEnergyDistanceVector.begin(), hitEnergyDistanceVector.end(),
            [](const HitEnergyDistance &a, const HitEnergyDistance &b) {
              return a.second < b.second;
            });

  float radial90EnergySum(0.f);
  radial90 = std::numeric_limits<float>::max();
  for (std::vector<HitEnergyDistance>::const_iterator
           iter = hitEnergyDistanceVector.begin(),
           iterEnd = hitEnergyDistanceVector.end();
       iter != iterEnd; ++iter) {
    radial90EnergySum += iter->first;
    if (radial90EnergySum > 0.9f * totalElectromagneticEnergy) {
      radial90 = iter->second;
      break;
    }
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------

float PfoMonitoringAlgorithm::GetFractionInCone(
    const pandora::Cluster *const pParentCluster,
    const pandora::Cluster *const pDaughterCluster,
    const pandora::ClusterFitResult &parentMipFitResult) const {
  const unsigned int nDaughterCaloHits(pDaughterCluster->GetNCaloHits());

  if (0 == nDaughterCaloHits)
    return 0.f;

  // Identify cone vertex
  const CartesianVector &parentMipFitDirection(
      parentMipFitResult.GetDirection());
  const CartesianVector &parentMipFitIntercept(
      parentMipFitResult.GetIntercept());

  const CartesianVector showerStartDifference(
      pParentCluster->GetCentroid(
          pParentCluster->GetShowerStartLayer(this->GetPandora())) -
      parentMipFitIntercept);
  const float parallelDistanceToShowerStart(
      showerStartDifference.GetDotProduct(parentMipFitDirection));
  const CartesianVector coneApex(
      parentMipFitIntercept +
      (parentMipFitDirection * parallelDistanceToShowerStart));

  // Don't allow large distance associations at low angle
  const float cosConeAngleWrtRadial(
      coneApex.GetUnitVector().GetDotProduct(parentMipFitDirection));

  if (cosConeAngleWrtRadial < m_minCosConeAngleWrtRadial)
    return 0.f;

  // Count daughter cluster hits in cone
  unsigned int nHitsInCone(0);
  float minHitSeparation(std::numeric_limits<float>::max());
  const OrderedCaloHitList &orderedCaloHitList(
      pDaughterCluster->GetOrderedCaloHitList());

  for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(),
                                          iterEnd = orderedCaloHitList.end();
       iter != iterEnd; ++iter) {
    for (CaloHitList::const_iterator hitIter = iter->second->begin(),
                                     hitIterEnd = iter->second->end();
         hitIter != hitIterEnd; ++hitIter) {
      const CartesianVector positionDifference((*hitIter)->GetPositionVector() -
                                               coneApex);
      const float hitSeparation(positionDifference.GetMagnitude());

      if (hitSeparation < std::numeric_limits<float>::epsilon())
        continue; // In analysis code, better to continue than throw

      if (hitSeparation < minHitSeparation)
        minHitSeparation = hitSeparation;

      const float cosTheta(
          parentMipFitDirection.GetDotProduct(positionDifference) /
          hitSeparation);

      if (cosTheta > m_coneCosineHalfAngle)
        nHitsInCone++;
    }
  }

  // Further checks to prevent large distance associations at low angle
  if (((cosConeAngleWrtRadial < m_cosConeAngleWrtRadialCut1) &&
       (minHitSeparation > m_minHitSeparationCut1)) ||
      ((cosConeAngleWrtRadial < m_cosConeAngleWrtRadialCut2) &&
       (minHitSeparation > m_minHitSeparationCut2))) {
    return 0.f;
  }

  return static_cast<float>(nHitsInCone) /
         static_cast<float>(nDaughterCaloHits);
}

//------------------------------------------------------------------------------------------------------------------------------------------

const MCParticle *PfoMonitoringAlgorithm::GetBestMCParticleMatch(
    const MCParticleToFloatMap &mcParticleContributions) const {
  const MCParticle *pBestMCMatch(nullptr);
  float maximumEnergy(0.f);

  MCParticleList mcParticleList;
  for (const auto &mapEntry : mcParticleContributions)
    mcParticleList.push_back(mapEntry.first);

  mcParticleList.sort(PointerLessThan<MCParticle>());

  for (const MCParticle *const pMCParticle : mcParticleList) {
    const float energy(mcParticleContributions.at(pMCParticle));
    if (energy > maximumEnergy) {
      maximumEnergy = energy;
      pBestMCMatch = pMCParticle;
    }
  }

  return pBestMCMatch;
}

//------------------------------------------------------------------------------------------------------------------------------------------

const MCParticle *PfoMonitoringAlgorithm::GetClusterMCParticleInfo(
    const Cluster *const pCluster, float &fCharged, float &fPhoton,
    float &fNeutral) const {
  MCParticleToFloatMap mcParticleContributions;
  this->ClusterEnergyFractions(pCluster, fCharged, fPhoton, fNeutral,
                               mcParticleContributions);

  return this->GetBestMCParticleMatch(mcParticleContributions);
}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode
PfoMonitoringAlgorithm::ReadSettings(const TiXmlHandle xmlHandle) {

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "NeutralHadronEnergyFractionCut",
                           m_neutralHadronEnergyFractionCut));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CreatePfoMonData", m_createPfoMonData));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CreateClusterMonData",
                           m_createClusterMonData));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CreateEventMonData",
                           m_createEventMonData));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CreateCaloHitMonData",
                           m_createCaloHitMonData));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolatedCaloHitListName",
                           m_isolatedCaloHitListName));

  float isolationCutDistanceFine(std::sqrt(m_isolationCutDistanceFine2));
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationCutDistanceFine",
                           isolationCutDistanceFine));
  m_isolationCutDistanceFine2 =
      isolationCutDistanceFine * isolationCutDistanceFine;

  float isolationCutDistanceCoarse(std::sqrt(m_isolationCutDistanceCoarse2));
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationCutDistanceCoarse",
                           isolationCutDistanceCoarse));
  m_isolationCutDistanceCoarse2 =
      isolationCutDistanceCoarse * isolationCutDistanceCoarse;

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationSearchSafetyFactor",
                           m_isolationSearchSafetyFactor));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationNLayers", m_isolationNLayers));

  // Value from ProximityBasedMergingAlgorithm for contact fraction calculation
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "ClusterContactThreshold",
                           m_clusterContactThreshold));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "MinLayersToShowerStart",
                           m_minLayersToShowerStart));

  // Value from ProximityBasedMergingAlgorithm for close hit fraction
  // calculation
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CloseHitThreshold",
                           m_closeHitThreshold));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "NGenericDistanceLayers",
                           m_nGenericDistanceLayers));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "NAdjacentLayers", m_nAdjacentLayers));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CanMergeMinMipFraction",
                           m_canMergeMinMipFraction));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CanMergeMaxRms", m_canMergeMaxRms));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "HighRadLengths", m_highRadLengths));

  // ConeBasedMergingAlgorithm parameters
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "ConeCosineHalfAngle",
                           m_coneCosineHalfAngle));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "MinCosConeAngleWrtRadial",
                           m_minCosConeAngleWrtRadial));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CosConeAngleWrtRadialCut1",
                           m_cosConeAngleWrtRadialCut1));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "MinHitSeparationCut1",
                           m_minHitSeparationCut1));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "CosConeAngleWrtRadialCut2",
                           m_cosConeAngleWrtRadialCut2));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "MinHitSeparationCut2",
                           m_minHitSeparationCut2));

  return STATUS_CODE_SUCCESS;
}

} // namespace lc_content