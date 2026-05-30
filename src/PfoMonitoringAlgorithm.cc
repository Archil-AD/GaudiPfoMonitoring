
/**
 *  @file   GaudiPfoMonitoring/src/PfoMonitoringAlgorithm.cc
 * 
 *  @brief  Implementation of the dump pfos monitoring algorithm class
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "GaudiKernel/Bootstrap.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/ISvcLocator.h"

#include "LCHelpers/ClusterHelper.h"
#include "LCHelpers/ReclusterHelper.h"
#include "LCHelpers/SortingHelper.h"
#include "PfoMonitoringAlgorithm.h"

#include "EVENT/MCParticle.h"

#include "PfoMonDataCollection.h"
#include "ClusterMonDataCollection.h"

#include <cmath>
#include <algorithm>
#include <iomanip>
#include <limits>

using namespace pandora;


class PfoMonitoringAlgorithmFactory : public pandora::AlgorithmFactory
{
public:
    pandora::Algorithm *CreateAlgorithm() const { return new lc_content::PfoMonitoringAlgorithm; }
};


namespace lc_content
{
PfoMonitoringAlgorithm::PfoMonitoringAlgorithm() :
    m_nCorrectNeutralHadronPfo(0),
    m_nWrongNeutralHadronPfo(0),
    m_nCorrectChargedHadronPfo(0),
    m_nWrongChargedHadronPfo(0),
    m_neutralHadronEnergyFractionCut(0.7)
{
    std::cout << " ------------------------------------------------------------ " << std::endl;
    std::cout << " ------------ PfoMonitoringAlgorithm initialized ------------ " << std::endl;
    std::cout << " ------------------------------------------------------------ " << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

PfoMonitoringAlgorithm::~PfoMonitoringAlgorithm()
{
    std::cout << " ------------------------------------------------------------ " << std::endl;
    std::cout << " ---------- PfoMonitoringAlgorithm : Summary ----------- " << std::endl;
    std::cout << " ------------------------------------------------------------ " << std::endl;
    std::cout << std::endl;
    std::cout << " Correct Charged Hadron Pfos: " << m_nCorrectChargedHadronPfo << std::endl;
    std::cout << " Wrong Charged Hadron Pfos: " << m_nWrongChargedHadronPfo << std::endl;
    std::cout << " Correct Neutral Hadron Pfos: " << m_nCorrectNeutralHadronPfo << std::endl;
    std::cout << " Wrong Neutral Hadron Pfos: " << m_nWrongNeutralHadronPfo << std::endl;
}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode PfoMonitoringAlgorithm::Run()
{
    // 1. Get the Gaudi Event Data Service
    SmartIF<IDataProviderSvc> eventSvc = Gaudi::svcLocator()->service<IDataProviderSvc>("EventDataSvc");
    if (!eventSvc)
    {
        std::cout << "PfoMonitoringAlgorithm: Could not locate EventDataSvc" << std::endl;
        return STATUS_CODE_FAILURE;
    }

    // 2. Retrieve or Create the PFO monitoring collection
    GaudiPfoMonitoring::PfoMonDataCollection* pfoColl = nullptr;
    if (eventSvc->retrieveObject("/Event/PfoMonitoringData", (DataObject*&)pfoColl).isFailure())
    {
        pfoColl = new GaudiPfoMonitoring::PfoMonDataCollection();
        if (eventSvc->registerObject("/Event/PfoMonitoringData", pfoColl).isFailure()) {
            std::cout << "PfoMonitoringAlgorithm: Could not register PfoMonitoringData" << std::endl;
            delete pfoColl;
            return STATUS_CODE_FAILURE;
        }
    }

    // 3. Retrieve or Create the cluster monitoring collection
    GaudiPfoMonitoring::ClusterMonDataCollection* clusterColl = nullptr;
    if (eventSvc->retrieveObject("/Event/ClusterMonitoringData", (DataObject*&)clusterColl).isFailure())
    {
        clusterColl = new GaudiPfoMonitoring::ClusterMonDataCollection();
        if (eventSvc->registerObject("/Event/ClusterMonitoringData", clusterColl).isFailure()) {
            std::cout << "PfoMonitoringAlgorithm: Could not register ClusterMonitoringData" << std::endl;
            delete clusterColl;
            return STATUS_CODE_FAILURE;
        }
    }

    m_trackMcPfoTargets.clear();

    //--------------------------------------------------------------------------------------------------------------
    // get MC particles
    const MCParticleList *pMCParticleList(nullptr);
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pMCParticleList));
    //--------------------------------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------------------------------
    // get PFOs
    const PfoList *pPfoList = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pPfoList));

    if (pPfoList->empty()) {
        // If this prints, your Pandora XML is calling this algorithm too early 
        // or the "Current PFO List" is not set correctly.
        std::cout << "PfoMonitoringAlgorithm: Current PFO list is EMPTY." << std::endl;
    }

    //--------------------------------------------------------------------------------------------------------------
    // get all clusters
    const ClusterList *pAllClusters = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pAllClusters));

    if (pAllClusters->empty()) {
        // If this prints, your Pandora XML is calling this algorithm too early 
        // or the "Current cluster List" is not set correctly.
        std::cout << "PfoMonitoringAlgorithm: Current Cluster list is EMPTY." << std::endl;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Build a set of all clusters that are part of any PFO
    ClusterList pfoClusters;

    //--------------------------------------------------------------------------------------------------------------
    // Fill PFO monitoring data from pPfoList
    for (PfoList::const_iterator pfoIter = pPfoList->begin(); pfoIter != pPfoList->end(); ++pfoIter)
    {
        const ParticleFlowObject *const pPfo = *pfoIter;
       
        const TrackList &trackList(pPfo->GetTrackList());
        const ClusterList &clusterList(pPfo->GetClusterList());
        pfoClusters.insert(pfoClusters.end(), clusterList.begin(), clusterList.end());
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

        float minClusterDistance = -9999.f; // Default minimum distance with other cluster
        float maxCosOpeningAngle = -1.f;   // Default minimum cosine

        for (const Cluster *const pPfoCluster : clusterList)
        {
            if (this->GetPandora().GetPlugins()->GetParticleId()->IsEmShower(pPfoCluster))
                nEmClusters++;

            nHits += pPfoCluster->GetNCaloHits();
            nMipLikeHits += pPfoCluster->GetNPossibleMipHits();
            this->GetMipLikeHits(pPfoCluster, nEcalHits, nHcalHits, nMipEcalHits, nMipHcalHits);

            if (pPfoCluster->GetInnerPseudoLayer() < minInnerLayer)
                minInnerLayer = pPfoCluster->GetInnerPseudoLayer();

            if (pPfoCluster->GetOuterPseudoLayer() > maxOuterLayer)
                maxOuterLayer = pPfoCluster->GetOuterPseudoLayer();

//            const CartesianVector clusterDir(pPfoCluster->GetCentroid().GetUnitVector());

            for (const Cluster *const pOtherCluster : *pAllClusters)
            {
                // Skip clusters that are already part of this PFO
                if (clusterList.end() != std::find(clusterList.begin(), clusterList.end(), pOtherCluster))
                    continue;

                const float distance(ClusterHelper::GetDistanceToClosestHit(pPfoCluster, pOtherCluster));
                if (distance < std::abs(minClusterDistance))
                    minClusterDistance = distance;
/*
                const CartesianVector displacement(pOtherCluster->GetCentroid() - pPfoCluster->GetCentroid());
                if (displacement.GetMagnitudeSquared() > std::numeric_limits<float>::epsilon())
                {
                    const float cosOpeningAngle(clusterDir.GetCosOpeningAngle(displacement));
                    if (cosOpeningAngle > maxCosOpeningAngle)
                        maxCosOpeningAngle = cosOpeningAngle;
                }
*/
            }
        }

        const unsigned int pfoStartLayer = (clusterList.empty() ? 0 : minInnerLayer);
        const unsigned int pfoNLayers = (clusterList.empty() ? 0 : (maxOuterLayer - minInnerLayer + 1));

        // Create the monitoring object and fill reconstructed information first
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

        if (trackList.size() == 1)  // charged PFO
        {
           pfoData.setFNeutral(0.f);
           pfoData.setFPhoton(0.f);
           pfoData.setFCharged(1.f);

           try{
              const Track *const pTrack = trackList.front();
              const MCParticle *const pMCParticle(MCParticleHelper::GetMainMCParticle(pTrack));
              
               if (pMCParticle)
               {
                   m_trackMcPfoTargets.push_back(pMCParticle);

                   // if MC particle is pi+/-, K+/- or proton then this is a correct assignment
                   if( std::abs(pMCParticle->GetParticleId()) == 211 ||
                       std::abs(pMCParticle->GetParticleId()) == 321 ||
                       std::abs(pMCParticle->GetParticleId()) == 2212) m_nCorrectChargedHadronPfo++;
                   else m_nWrongChargedHadronPfo++; 

                   pfoData.setMcPdg(pMCParticle->GetParticleId());
                   pfoData.setMcEnergy(pMCParticle->GetEnergy());

                   // Opening angle between PFO and MC particle
                   const CartesianVector &mcMomentum(pMCParticle->GetMomentum());
                   if (momentum.GetMagnitudeSquared() > std::numeric_limits<float>::epsilon() &&
                       mcMomentum.GetMagnitudeSquared() > std::numeric_limits<float>::epsilon())
                   {
                       pfoData.setAlpha(std::acos(std::max(-1.f, std::min(1.f, momentum.GetCosOpeningAngle(mcMomentum)))));
                   }
               }
           }
           catch (StatusCodeException &e)
           {
               // This usually means Track-to-MC matching is not initialized in the Pandora instance
               static int errorCount = 0;
               if (errorCount++ < 10) std::cout << "PfoMonitoring: MC match not found for track: " << e.ToString() << std::endl;
           }
        }
        else if( trackList.empty() ) // neutral PFO
        {
           float fCharged(0.f), fPhoton(0.f), fNeutral(0.f);

           try{
             // store each MC particle (reco) energy contribution in this PFO
             MCParticleToFloatMap mcParticleContributions;

             // hadronic energy scale
             float totEnergy(0.f);
             float neutralEnergy(0.f);
             float photonEnergy(0.f);
             float chargedEnergy(0.f);

             // find energy fractions and the MC particle with largest energy contribution
             for (ClusterList::const_iterator clusterIter = clusterList.begin(); clusterIter != clusterList.end(); ++clusterIter)
             {
               const Cluster *const pCluster = *clusterIter;
               const float clusterEnergy(pCluster->GetHadronicEnergy());

               this->PfoMonitoringAlgorithm::ClusterEnergyFractions(pCluster, fCharged, fPhoton, fNeutral, mcParticleContributions);
               totEnergy     += clusterEnergy;
               neutralEnergy += clusterEnergy*fNeutral;
               photonEnergy  += clusterEnergy*fPhoton;
               chargedEnergy += clusterEnergy*fCharged;
             }

             // calculate energy fractions for the given PFO
             if (totEnergy > 0.f)
             {
               fCharged = chargedEnergy/totEnergy;
               fPhoton  = photonEnergy/totEnergy;
               fNeutral = neutralEnergy/totEnergy;
             }

             pfoData.setFNeutral(fNeutral);
             pfoData.setFPhoton(fPhoton);
             pfoData.setFCharged(fCharged);

             // Find mc particle with largest associated energy in the given PFO
             const MCParticle *pBestMCMatch(NULL);
             float maximumEnergy(0.f);
             MCParticleList mcParticleList;
             for (const auto &mapEntry : mcParticleContributions) mcParticleList.push_back(mapEntry.first);
             mcParticleList.sort(PointerLessThan<MCParticle>());

             for (const MCParticle *const pMCParticle : mcParticleList)
             {
                 const float energy(mcParticleContributions.at(pMCParticle));
                 if (energy > maximumEnergy)
                 {
                   maximumEnergy = energy;
                   pBestMCMatch = pMCParticle;
                 }
             }

             if (pBestMCMatch)
             {
                 if ( fNeutral > m_neutralHadronEnergyFractionCut )
                    m_nCorrectNeutralHadronPfo++;
                 else
                    m_nWrongNeutralHadronPfo++;

                 pfoData.setMcPdg(pBestMCMatch->GetParticleId());
                 pfoData.setMcEnergy(pBestMCMatch->GetEnergy());

                 // Opening angle (cosine) between PFO and MC particle
                 const CartesianVector &mcMomentum(pBestMCMatch->GetMomentum());
                 if (momentum.GetMagnitudeSquared() > std::numeric_limits<float>::epsilon() &&
                     mcMomentum.GetMagnitudeSquared() > std::numeric_limits<float>::epsilon())
                 {
                     pfoData.setAlpha(std::acos(std::max(-1.f, std::min(1.f, momentum.GetCosOpeningAngle(mcMomentum)))));
                 }
             }
           }
           catch (StatusCodeException &e)
           {
               pfoData.setFNeutral(fNeutral);
               pfoData.setFPhoton(fPhoton);
               pfoData.setFCharged(fCharged);
               static int errorCountNeutral = 0;
               if (errorCountNeutral++ < 10) std::cout << "PfoMonitoring: MC match error for neutral: " << e.ToString() << std::endl;
           }
        } // neutral PFO
    }
    //--------------------------------------------------------------------------------------------------------------


    //--------------------------------------------------------------------------------------------------------------
    // Fill cluster monitoring data from pAllClusters
    for (const Cluster *const pCluster : *pAllClusters)
    {
        unsigned int clusEcalHits   = 0;
        unsigned int clusHcalHits   = 0;
        unsigned int clusMipEcalHits = 0;
        unsigned int clusMipHcalHits = 0;
        this->GetMipLikeHits(pCluster, clusEcalHits, clusHcalHits, clusMipEcalHits, clusMipHcalHits);

        const unsigned int clusStartLayer = pCluster->GetInnerPseudoLayer();
        const unsigned int clusNLayers    = pCluster->GetOuterPseudoLayer() - clusStartLayer + 1;
        const unsigned int clusIsEm       = this->GetPandora().GetPlugins()->GetParticleId()->IsEmShower(pCluster) ? 1 : 0;

        float clusMinDist = -9999.f;
        for (const Cluster *const pOtherCluster : *pAllClusters)
        {
            if (pCluster == pOtherCluster)
                continue;
            const float dist(ClusterHelper::GetDistanceToClosestHit(pCluster, pOtherCluster));
            if (dist < std::abs(clusMinDist))
                clusMinDist = dist;
        }

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
        clusData.setIsInPfo(pfoClusters.end() != std::find(pfoClusters.begin(), pfoClusters.end(), pCluster) ? 1 : 0);
    }
    //--------------------------------------------------------------------------------------------------------------

    return STATUS_CODE_SUCCESS;
}


//------------------------------------------------------------------------------------------------------------------------------------------

void PfoMonitoringAlgorithm::ClusterEnergyFractions(const Cluster *const pCluster, float &fCharged, float &fPhoton, float &fNeutral,
    MCParticleToFloatMap &mcParticleContributions) const
{
    float totEnergy(0.f);
    float neutralEnergy(0.f);
    float photonEnergy(0.f);
    float chargedEnergy(0.f);

    const OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (OrderedCaloHitList::const_iterator iter = orderedCaloHitList.begin(), iterEnd = orderedCaloHitList.end(); iter != iterEnd; ++iter)
    {
        for (CaloHitList::const_iterator hitIter = iter->second->begin(), hitIterEnd = iter->second->end(); hitIter != hitIterEnd; ++hitIter)
        {
            try
            {
                const CaloHit *const pCaloHit = *hitIter;
                const MCParticle *const pMCParticle(MCParticleHelper::GetMainMCParticle(pCaloHit));
                const MCParticle *const pMCPfoTarget(pMCParticle->GetPfoTarget());

                totEnergy += pCaloHit->GetHadronicEnergy();
                MCParticleToFloatMap::iterator it = mcParticleContributions.find(pMCPfoTarget);

                if (it != mcParticleContributions.end())
                {
                    it->second += pCaloHit->GetHadronicEnergy();
                }
                else
                {
                    mcParticleContributions.insert(MCParticleToFloatMap::value_type(pMCPfoTarget, pCaloHit->GetHadronicEnergy()));
                }

                const int pdgCode(pMCPfoTarget->GetParticleId());
                const int charge(PdgTable::GetParticleCharge(pdgCode));

                if ((charge != 0) || (std::abs(pdgCode) == LAMBDA) || (std::abs(pdgCode) == K_SHORT))
                {
                    if (m_trackMcPfoTargets.end() == std::find(m_trackMcPfoTargets.begin(), m_trackMcPfoTargets.end(), pMCParticle))
                    {
                        neutralEnergy += pCaloHit->GetHadronicEnergy();
                    }
                    else
                    {
                        chargedEnergy += pCaloHit->GetHadronicEnergy();
                    }
                }
                else
                {
                    (pMCParticle->GetParticleId() == PHOTON) ? photonEnergy += pCaloHit->GetHadronicEnergy() : neutralEnergy += pCaloHit->GetHadronicEnergy();
                }
            }
            catch (StatusCodeException &)
            {
            }
        }
    }

    if (totEnergy > 0.f)
    {
        fCharged = chargedEnergy/totEnergy;
        fPhoton  = photonEnergy/totEnergy;
        fNeutral = neutralEnergy/totEnergy;
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------


void PfoMonitoringAlgorithm::GetMipLikeHits(const pandora::Cluster *const pCluster, unsigned int &nEcalHits, unsigned int &nHcalHits,
 unsigned int &nMipEcalHits, unsigned int &nMipHcalHits) const
{
    // The OrderedCaloHitList organizes hits by pseudo-layer
    const pandora::OrderedCaloHitList &orderedCaloHitList(pCluster->GetOrderedCaloHitList());

    for (const auto &layerEntry : orderedCaloHitList)
    {
        for (const pandora::CaloHit *const pCaloHit : *layerEntry.second)
        {
            const pandora::HitType hitType(pCaloHit->GetHitType());
            //const pandora::HitRegion hitRegion(pCaloHit->GetHitRegion());

            // Check if the hit belongs to the ECAL (Barrel or Endcap)
            if (hitType == pandora::ECAL || hitType == pandora::ECAL)
            {
                nEcalHits++;
                // IsPossibleMip() is the internal Pandora flag for MIP-like hits
                if (pCaloHit->IsPossibleMip())
                {
                    nMipEcalHits++;
                }
            }
     	    // Check if the hit belongs to the HCAL (Barrel or Endcap)
            if (hitType == pandora::HCAL || hitType == pandora::HCAL)
            {
                nHcalHits++;
                // IsPossibleMip() is the internal Pandora flag for MIP-like hits
                if (pCaloHit->IsPossibleMip())
                {
                    nMipHcalHits++;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------


pandora::StatusCode PfoMonitoringAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NeutralHadronEnergyFractionCut", m_neutralHadronEnergyFractionCut));

    return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
