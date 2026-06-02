
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
#include "LCHelpers/ReclusterHelper.h"
#include "LCHelpers/SortingHelper.h"
#include "LCUtility/KDTreeLinkerAlgoT.h"
#include "LCUtility/KDTreeLinkerToolsT.h"
#include "PfoMonitoringAlgorithm.h"

#include "EVENT/MCParticle.h"

#include "CaloHitMonDataCollection.h"
#include "ClusterMonDataCollection.h"
#include "EventMonDataCollection.h"
#include "PfoMonDataCollection.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>

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
      m_isolationSearchSafetyFactor(2.f),
      m_isolationNLayers(2),
      m_eventNumber(0) {
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
  // Build a set of all clusters that are part of any PFO
  ClusterList pfoClusters;

  //--------------------------------------------------------------------------------------------------------------
  // Fill PFO monitoring data from pPfoList
  for (PfoList::const_iterator pfoIter = pPfoList->begin();
       pfoIter != pPfoList->end(); ++pfoIter) {
    const ParticleFlowObject *const pPfo = *pfoIter;

    const TrackList &trackList(pPfo->GetTrackList());
    const ClusterList &clusterList(pPfo->GetClusterList());
    pfoClusters.insert(pfoClusters.end(), clusterList.begin(),
                       clusterList.end());
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
        -9999.f; // Default minimum distance with other cluster
    float maxCosOpeningAngle = -1.f; // Default minimum cosine

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

      //            const CartesianVector
      //            clusterDir(pPfoCluster->GetCentroid().GetUnitVector());

      for (const Cluster *const pOtherCluster : *pAllClusters) {
        // Skip clusters that are already part of this PFO
        if (clusterList.end() !=
            std::find(clusterList.begin(), clusterList.end(), pOtherCluster))
          continue;

        const float distance(
            ClusterHelper::GetDistanceToClosestHit(pPfoCluster, pOtherCluster));
        if (distance < std::abs(minClusterDistance))
          minClusterDistance = distance;
        /*
                        const CartesianVector
           displacement(pOtherCluster->GetCentroid() -
           pPfoCluster->GetCentroid()); if (displacement.GetMagnitudeSquared() >
           std::numeric_limits<float>::epsilon())
                        {
                            const float
           cosOpeningAngle(clusterDir.GetCosOpeningAngle(displacement)); if
           (cosOpeningAngle > maxCosOpeningAngle) maxCosOpeningAngle =
           cosOpeningAngle;
                        }
        */
      }
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
    pfoData.setMaxCosOpeningAngle(maxCosOpeningAngle);
    pfoData.setStartLayer(pfoStartLayer);
    pfoData.setNLayers(pfoNLayers);
    pfoData.setAlpha(0.f);

    // Default MC values in case matching fails
    pfoData.setMcPdg(0);
    pfoData.setMcEnergy(0.f);

    if (trackList.size() == 1) // charged PFO
    {
      pfoData.setFNeutral(0.f);
      pfoData.setFPhoton(0.f);
      pfoData.setFCharged(1.f);

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
    } else if (trackList.empty()) // neutral PFO
    {
      float fCharged(0.f), fPhoton(0.f), fNeutral(0.f);

      try {
        // store each MC particle (reco) energy contribution in this PFO
        MCParticleToFloatMap mcParticleContributions;

        // hadronic energy scale
        float totEnergy(0.f);
        float neutralEnergy(0.f);
        float photonEnergy(0.f);
        float chargedEnergy(0.f);

        // find energy fractions and the MC particle with largest energy
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

        // Find mc particle with largest associated energy in the given PFO
        const MCParticle *pBestMCMatch(NULL);
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
        pfoData.setFNeutral(fNeutral);
        pfoData.setFPhoton(fPhoton);
        pfoData.setFCharged(fCharged);
        static int errorCountNeutral = 0;
        if (errorCountNeutral++ < 10)
          std::cout << "PfoMonitoring: MC match error for neutral: "
                    << e.ToString() << std::endl;
      }
    } // neutral PFO
  }
  //--------------------------------------------------------------------------------------------------------------

  if (m_createClusterMonData) {
    //--------------------------------------------------------------------------------------------------------------
    // Fill cluster monitoring data from pAllClusters
    for (const Cluster *const pCluster : *pAllClusters) {
      unsigned int clusEcalHits = 0;
      unsigned int clusHcalHits = 0;
      unsigned int clusMipEcalHits = 0;
      unsigned int clusMipHcalHits = 0;
      this->GetMipLikeHits(pCluster, clusEcalHits, clusHcalHits,
                           clusMipEcalHits, clusMipHcalHits);

      const unsigned int clusStartLayer = pCluster->GetInnerPseudoLayer();
      const unsigned int clusNLayers =
          pCluster->GetOuterPseudoLayer() - clusStartLayer + 1;
      const unsigned int clusIsEm =
          this->GetPandora().GetPlugins()->GetParticleId()->IsEmShower(pCluster)
              ? 1
              : 0;

      float clusMinDist = -9999.f;
      for (const Cluster *const pOtherCluster : *pAllClusters) {
        if (pCluster == pOtherCluster)
          continue;
        const float dist(
            ClusterHelper::GetDistanceToClosestHit(pCluster, pOtherCluster));
        if (dist < std::abs(clusMinDist))
          clusMinDist = dist;
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
      clusData.setNLayers(clusNLayers);
      clusData.setIsEm(clusIsEm);
      clusData.setMinClusterDistance(clusMinDist);
      clusData.setIsInPfo(pfoClusters.end() != std::find(pfoClusters.begin(),
                                                         pfoClusters.end(),
                                                         pCluster)
                              ? 1
                              : 0);
    }
  }
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // Fill calo hit monitoring data from all clusters (regular + isolated hits)
  // and from the current calo hit list (isolated orphan hits)
  if (m_createCaloHitMonData && caloHitColl) {

    // Build a comprehensive set of ALL calo hits from every possible source
    // so the KD-tree used for isolation calculations is complete.
    // Sources:
    //   1. Regular (non-isolated) hits already inside clusters
    //   2. Isolated hits attached to clusters
    //   3. Remaining hits in the current available list (unclustered, any flag)
    //   4. Hits in the dedicated isolated calo hit list (orphan isolated hits)
    CaloHitList allCaloHits;

    // 1 & 2: Hits from clusters
    for (const Cluster *const pCluster : *pAllClusters) {
      const OrderedCaloHitList &orderedList(pCluster->GetOrderedCaloHitList());
      for (const auto &layerEntry : orderedList)
        for (const CaloHit *const pCaloHit : *layerEntry.second)
          allCaloHits.push_back(pCaloHit);

      const CaloHitList &isoHits(pCluster->GetIsolatedCaloHitList());
      allCaloHits.insert(allCaloHits.end(), isoHits.begin(), isoHits.end());
    }

    // 3: Remaining hits in the current available list (not yet clustered)
    const CaloHitList *pCurrentListForKD = nullptr;
    if (PandoraContentApi::GetCurrentList(*this, pCurrentListForKD) == STATUS_CODE_SUCCESS && pCurrentListForKD)
      allCaloHits.insert(allCaloHits.end(), pCurrentListForKD->begin(), pCurrentListForKD->end());

    // 4: Dedicated isolated calo hit list (orphan isolated hits not in any cluster)
    const CaloHitList *pIsolatedListForKD = nullptr;
    if (!m_isolatedCaloHitListName.empty() &&
        PandoraContentApi::GetList(*this, m_isolatedCaloHitListName, pIsolatedListForKD) == STATUS_CODE_SUCCESS
        && pIsolatedListForKD)
      allCaloHits.insert(allCaloHits.end(), pIsolatedListForKD->begin(), pIsolatedListForKD->end());

    // Initialize KDTree for efficient hit isolation calculations
    typedef KDTreeNodeInfoT<const pandora::CaloHit *, 4> HitKDNode4D;
    typedef KDTreeLinkerAlgo<const pandora::CaloHit *, 4> HitKDTree4D;

    std::vector<HitKDNode4D> hitNodes4D;
    HitKDTree4D hitsKdTree4D;
    if (!allCaloHits.empty()) {
      KDTreeTesseract hitsBoundingRegion4D = fill_and_bound_4d_kd_tree(this, allCaloHits, hitNodes4D, true);
      hitsKdTree4D.build(hitNodes4D, hitsBoundingRegion4D);
    }

    // Helper function to replicate CaloHitPreparationAlgorithm::IsolationCountNearbyHits
    auto isolationCountNearbyHits = [&](unsigned int searchLayer, const CaloHit *const pCaloHit, float &shortestIsolationDist) -> unsigned int {
      const CartesianVector &pos(pCaloHit->GetPositionVector());
      const float mag2(pos.GetMagnitudeSquared());
      const float isolationCutDistanceSquared((this->GetPandora().GetGeometry()->GetHitTypeGranularity(pCaloHit->GetHitType()) <= FINE) ? m_isolationCutDistanceFine2 : m_isolationCutDistanceCoarse2);
      const float maxSeparation2(1000.f * 1000.f);

      unsigned int nearbyHitsFound = 0;
      const float searchDistance(m_isolationSearchSafetyFactor * std::sqrt(isolationCutDistanceSquared));
      KDTreeTesseract searchRegionHits = build_4d_kd_search_region(pCaloHit, searchDistance, searchDistance, searchDistance, static_cast<float>(searchLayer));

      std::vector<HitKDNode4D> found;
      hitsKdTree4D.search(searchRegionHits, found);

      for (const auto &hitNode : found) {
        const CaloHit *const pOtherHit = hitNode.data;
        if (pCaloHit == pOtherHit)
          continue;

        const CartesianVector diff(pos - pOtherHit->GetPositionVector());
        if (diff.GetMagnitudeSquared() > maxSeparation2)
          continue;

        float isolationDistance = std::sqrt(pos.GetCrossProduct(diff).GetMagnitudeSquared() / mag2);
        if (isolationDistance < shortestIsolationDist)
          shortestIsolationDist = isolationDistance;
        if (isolationDistance < std::sqrt(isolationCutDistanceSquared))
          ++nearbyHitsFound;
      }
      return nearbyHitsFound;
    };

    // Find the most energetic cluster and compute its energy-weighted centroid
    CartesianVector mostEnergeticClusterCentroid(0.f, 0.f, 0.f);
    bool hasMostEnergeticCluster = false;
    if (!pAllClusters->empty()) {
      const Cluster *pMostEnergeticCluster = nullptr;
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
        const OrderedCaloHitList &orderedList(pMostEnergeticCluster->GetOrderedCaloHitList());
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
          mostEnergeticClusterCentroid = CartesianVector(sumX / sumWeight, sumY / sumWeight, sumZ / sumWeight);
          hasMostEnergeticCluster = true;
        }
      }
    }

    // Helper lambda to fill one CaloHit entry
    auto fillHit = [&](const CaloHit *const pCaloHit, bool isIsolated) {
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
      hitData.setHitType((hitType == pandora::ECAL) ? 0u : (hitType == pandora::HCAL ? 1u : 2u));

      // Calculate isolationNearbyHits (replicating logic from CaloHitPreparationAlgorithm)
      unsigned int isolationNearbyHits = 0;
      float shortestIsolationDist = std::numeric_limits<float>::max();
      if (!allCaloHits.empty() && !hitNodes4D.empty()) {
        const unsigned int layerI(pCaloHit->GetPseudoLayer());
        const unsigned int minLayer((layerI < m_isolationNLayers) ? 0 : layerI - m_isolationNLayers);
        const unsigned int maxLayer(layerI + m_isolationNLayers);

        for (unsigned int iLayer = minLayer; iLayer <= maxLayer; ++iLayer) {
          isolationNearbyHits += isolationCountNearbyHits(iLayer, pCaloHit, shortestIsolationDist);
        }
      }
      hitData.setIsolationNearbyHits(isolationNearbyHits);
      hitData.setShortestIsolationDist(shortestIsolationDist > 500000.f ? -1.f : shortestIsolationDist);

      // 3D distance from hit to energy-weighted centroid of the most energetic cluster
      const float distToMECC = hasMostEnergeticCluster
          ? (pos - mostEnergeticClusterCentroid).GetMagnitude()
          : -1.f;
      hitData.setDistToMostEnergeticClusterCentroid(distToMECC);
    };

    // Iterate over all hits and fill the hitData
    for (const CaloHit *const pCaloHit : allCaloHits) {
        fillHit(pCaloHit, pCaloHit->IsIsolated());
    }
  }
  //--------------------------------------------------------------------------------------------------------------

  //--------------------------------------------------------------------------------------------------------------
  // Fill event-level monitoring data
  if (m_createEventMonData && eventColl) {
    // Count hits already inside clusters (regular and isolated)
    unsigned int nClusteredHits = 0;
    unsigned int nClusteredIsolatedHits = 0;
    for (const Cluster *const pCluster : *pAllClusters) {
      nClusteredHits += pCluster->GetNCaloHits();
      nClusteredIsolatedHits += pCluster->GetNIsolatedCaloHits();
    }

    // Count "orphan" isolated hits (those not in clusters)
    unsigned int nOrphanIsolatedHits = 0;
    float orphanIsolatedEnergy = 0.f;

    // 1. Hits from the dedicated isolated calo hit list
    const CaloHitList *pIsolatedCaloHitList = nullptr;
    if (!m_isolatedCaloHitListName.empty() &&
        PandoraContentApi::GetList(*this, m_isolatedCaloHitListName, pIsolatedCaloHitList) == STATUS_CODE_SUCCESS) {
      nOrphanIsolatedHits += static_cast<unsigned int>(pIsolatedCaloHitList->size());
      for (const CaloHit *const pCaloHit : *pIsolatedCaloHitList)
        orphanIsolatedEnergy += pCaloHit->GetHadronicEnergy();
    }

    // 2. Hits still in the current list that are marked as isolated
    const CaloHitList *pCurrentCaloHitList = nullptr;
    unsigned int nUnclusteredNonIsolatedHits = 0;
    if (PandoraContentApi::GetCurrentList(*this, pCurrentCaloHitList) == STATUS_CODE_SUCCESS && pCurrentCaloHitList) {
      for (const CaloHit *const pCaloHit : *pCurrentCaloHitList) {
        if (pCaloHit->IsIsolated()) {
          nOrphanIsolatedHits++;
          orphanIsolatedEnergy += pCaloHit->GetHadronicEnergy();
        } else {
          nUnclusteredNonIsolatedHits++;
        }
      }
    }

    unsigned int nTotalIsolatedHits = nClusteredIsolatedHits + nOrphanIsolatedHits;

    auto &evtData = eventColl->create();
    evtData.setEventNumber(m_eventNumber++);
    evtData.setNTotalHits(nClusteredHits + nUnclusteredNonIsolatedHits + nTotalIsolatedHits);
    evtData.setNClusteredIsolatedHits(nClusteredIsolatedHits);
    evtData.setNOrphanIsolatedHits(nOrphanIsolatedHits);
    evtData.setOrphanIsolatedEnergy(orphanIsolatedEnergy);
    evtData.setNUnclusteredNonIsolatedHits(nUnclusteredNonIsolatedHits);
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

  const OrderedCaloHitList &orderedCaloHitList(
      pCluster->GetOrderedCaloHitList());

  for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(),
                                          iterEnd = orderedCaloHitList.end();
       iter != iterEnd; ++iter) {
    for (CaloHitList::const_iterator hitIter = iter->second->begin(),
                                     hitIterEnd = iter->second->end();
         hitIter != hitIterEnd; ++hitIter) {
      try {
        const CaloHit *const pCaloHit = *hitIter;
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
              std::find(m_trackMcPfoTargets.begin(), m_trackMcPfoTargets.end(),
                        pMCParticle)) {
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
      XmlHelper::ReadValue(xmlHandle, "IsolatedCaloHitListName", m_isolatedCaloHitListName));

  float isolationCutDistanceFine(std::sqrt(m_isolationCutDistanceFine2));
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationCutDistanceFine", isolationCutDistanceFine));
  m_isolationCutDistanceFine2 = isolationCutDistanceFine * isolationCutDistanceFine;

  float isolationCutDistanceCoarse(std::sqrt(m_isolationCutDistanceCoarse2));
  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationCutDistanceCoarse", isolationCutDistanceCoarse));
  m_isolationCutDistanceCoarse2 = isolationCutDistanceCoarse * isolationCutDistanceCoarse;

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationSearchSafetyFactor", m_isolationSearchSafetyFactor));

  PANDORA_RETURN_RESULT_IF_AND_IF(
      STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
      XmlHelper::ReadValue(xmlHandle, "IsolationNLayers", m_isolationNLayers));

  return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
