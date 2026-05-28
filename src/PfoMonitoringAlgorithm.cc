
/**
 *  @file   LCContent/src/LCMonitoring/PfoMonitoringAlgorithm.cc
 * 
 *  @brief  Implementation of the dump pfos monitoring algorithm class
 * 
 *  $Log: $
 */

#include "Pandora/AlgorithmHeaders.h"

#include "LCHelpers/ClusterHelper.h"
#include "LCHelpers/ReclusterHelper.h"
#include "LCHelpers/SortingHelper.h"
#include "PfoMonitoringData.h"
#include "PfoMonitoringAlgorithm.h"

#include "EVENT/MCParticle.h"

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

StatusCode PfoMonitoringAlgorithm::Run()
{
    // clear the global PfoData buffer
    lc_content::g_pfoMonitoringBuffer.clear();

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

    const ClusterList *pAllClusters = NULL;
    PANDORA_RETURN_RESULT_IF(STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pAllClusters));

    for (PfoList::const_iterator pfoIter = pPfoList->begin(); pfoIter != pPfoList->end(); ++pfoIter)
    {
        lc_content::PfoData currentPfoData;
        const ParticleFlowObject *const pPfo = *pfoIter;

        const TrackList &trackList(pPfo->GetTrackList());
        const ClusterList &clusterList(pPfo->GetClusterList());
        const float pfoEnergy(pPfo->GetEnergy());
        const int pfoPid(pPfo->GetParticleId());

        const CartesianVector &momentum(pPfo->GetMomentum());
        const float px = momentum.GetX();
        const float py = momentum.GetY();
        const float pz = momentum.GetZ();
        const float mass = pPfo->GetMass();

        unsigned int minInnerLayer(std::numeric_limits<unsigned int>::max());
        unsigned int maxOuterLayer(0);

        unsigned int nHits = 0;
        unsigned int nMipLikeHits = 0;
        unsigned int nEcalHits = 0;
        unsigned int nMipEcalHits = 0;
        unsigned int nHcalHits = 0;
        unsigned int nMipHcalHits = 0;
        for (const Cluster *const pCluster : clusterList)
        {
            nHits += pCluster->GetNCaloHits();
            nMipLikeHits += pCluster->GetNPossibleMipHits();
            // get ECAL/HCAL hits
            this->GetMipLikeHits(pCluster, nEcalHits, nHcalHits, nMipEcalHits, nMipHcalHits);

            if (pCluster->GetInnerPseudoLayer() < minInnerLayer)
                minInnerLayer = pCluster->GetInnerPseudoLayer();

            if (pCluster->GetOuterPseudoLayer() > maxOuterLayer)
                maxOuterLayer = pCluster->GetOuterPseudoLayer();
        }

        const unsigned int pfoStartLayer = (clusterList.empty() ? 0 : minInnerLayer);
        const unsigned int pfoNLayers = (clusterList.empty() ? 0 : (maxOuterLayer - minInnerLayer + 1));

        float minClusterDistance = -9999.f;
        float maxCosOpeningAngle = -1.f;   // Default minimum cosine

        for (const Cluster *const pPfoCluster : clusterList)
        {
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

        if (trackList.size() == 1)  // charged PFO
        {
           try{
              const Track *const pTrack = trackList.front();
              const MCParticle *const pMCParticle(MCParticleHelper::GetMainMCParticle(pTrack));
              m_trackMcPfoTargets.push_back(pMCParticle);

              // if MC particle is pi+/-, K+/- or proton then this is a correct assignment
              if( std::abs(pMCParticle->GetParticleId()) == 211 ||
                  std::abs(pMCParticle->GetParticleId()) == 321 ||
                  std::abs(pMCParticle->GetParticleId()) == 2212) m_nCorrectChargedHadronPfo++;
              else m_nWrongChargedHadronPfo++; // those can be electrons and muons

              currentPfoData.pfo_energy = pfoEnergy;
              currentPfoData.pfo_pdg = pfoPid;
              currentPfoData.pfo_px = px;
              currentPfoData.pfo_py = py;
              currentPfoData.pfo_pz = pz;
              currentPfoData.pfo_mass = mass;
              currentPfoData.pfo_nHits = nHits;
              currentPfoData.pfo_nMipLikeHits = nMipLikeHits;
              currentPfoData.pfo_nEcalHits = nEcalHits;
              currentPfoData.pfo_nMipEcalHits = nMipEcalHits;
              currentPfoData.pfo_nHcalHits = nHcalHits;
              currentPfoData.pfo_nMipHcalHits = nMipHcalHits;
              currentPfoData.pfo_minClusterDistance = minClusterDistance;
              currentPfoData.pfo_maxCosOpeningAngle = maxCosOpeningAngle;
              currentPfoData.pfo_startLayer = pfoStartLayer;
              currentPfoData.pfo_nLayers = pfoNLayers;
              currentPfoData.pfo_fNeutral = 0.f;
              currentPfoData.pfo_fPhoton = 0.f;
              currentPfoData.pfo_fCharged = 0.f;
              currentPfoData.pfo_alpha = 0.f;
              currentPfoData.pfo_mcPdg = pMCParticle->GetParticleId();
//              currentPfoData.pfo_mcGenStatus = reinterpret_cast<const edm4hep::MCParticle*>(pMCParticle->GetUid())->getGeneratorStatus();
              currentPfoData.pfo_mcEnergy = pMCParticle->GetEnergy();
              lc_content::g_pfoMonitoringBuffer.push_back(currentPfoData);
           }
           catch (StatusCodeException &)
           {
           }
        }
        else if( trackList.empty() ) // neutral PFO
        {
           try{
             // store each MC particle (reco) energy contribution in this PFO
             MCParticleToFloatMap mcParticleContributions;

             // hadronic energy scale
             float totEnergy(0.f);
             float neutralEnergy(0.f);
             float photonEnergy(0.f);
             float chargedEnergy(0.f);

             float fCharged(0.f);
             float fPhoton(0.f);
             float fNeutral(0.f);

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

             // Fake natural PFO
             if(pBestMCMatch == NULL) {
                continue;
             }
             //const MCParticle *const pMainClustersMCParticle(MCParticleHelper::GetMainMCParticle(&clusterList));


             if ( fNeutral > m_neutralHadronEnergyFractionCut )
             {
                m_nCorrectNeutralHadronPfo++;
             }
             else
             {
                m_nWrongNeutralHadronPfo++;
             }

             currentPfoData.pfo_energy = pfoEnergy;
             currentPfoData.pfo_pdg = pfoPid;
             currentPfoData.pfo_px = px;
             currentPfoData.pfo_py = py;
             currentPfoData.pfo_pz = pz;
             currentPfoData.pfo_mass = mass;
             currentPfoData.pfo_nHits = nHits;
             currentPfoData.pfo_nMipLikeHits = nMipLikeHits;
             currentPfoData.pfo_nEcalHits = nEcalHits;
             currentPfoData.pfo_nMipEcalHits = nMipEcalHits;
             currentPfoData.pfo_nHcalHits = nHcalHits;
             currentPfoData.pfo_nMipHcalHits = nMipHcalHits;
             currentPfoData.pfo_minClusterDistance = minClusterDistance;
             currentPfoData.pfo_maxCosOpeningAngle = maxCosOpeningAngle;
             currentPfoData.pfo_startLayer = pfoStartLayer;
             currentPfoData.pfo_nLayers = pfoNLayers;
             currentPfoData.pfo_fNeutral = fNeutral;
             currentPfoData.pfo_fPhoton = fPhoton;
             currentPfoData.pfo_fCharged = fCharged;
             currentPfoData.pfo_alpha = 0.f;
             currentPfoData.pfo_mcPdg = pBestMCMatch->GetParticleId();
//             currentPfoData.pfo_mcGenStatus = reinterpret_cast<const edm4hep::MCParticle*>(pBestMCMatch->GetUid())->getGeneratorStatus();
             currentPfoData.pfo_mcEnergy = pBestMCMatch->GetEnergy();
             lc_content::g_pfoMonitoringBuffer.push_back(currentPfoData);
           }
           catch (StatusCodeException &)
           {
           }
        } // neutral PFO
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

            // Check if the hit belongs to the ECAL (Barrel or Endcap)
            if (hitType == pandora::ECAL_BARREL || hitType == pandora::ECAL_ENDCAP)
            {
                nEcalHits++;
                // IsPossibleMip() is the internal Pandora flag for MIP-like hits
                if (pCaloHit->IsPossibleMip())
                {
                    nMipEcalHits++;
                }
            }
     	    // Check if the hit belongs to the HCAL (Barrel or Endcap)
            if (hitType == pandora::HCAL_BARREL || hitType == pandora::HCAL_ENDCAP)
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


StatusCode PfoMonitoringAlgorithm::ReadSettings(const TiXmlHandle xmlHandle)
{

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=, XmlHelper::ReadValue(xmlHandle,
        "NeutralHadronEnergyFractionCut", m_neutralHadronEnergyFractionCut));

    return STATUS_CODE_SUCCESS;
}

} // namespace lc_content
