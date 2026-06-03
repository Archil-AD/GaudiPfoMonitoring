/**
 *  @file   LCContent/include/LCMonitoring/PfoMonitoringAlgorithm.h
 * 
 *  @brief  Header file for the dump pfos monitoring algorithm class
 * 
 *  $Log: $
 */
#ifndef LC_PFO_MONITORING_ALGORITHM_H
#define LC_PFO_MONITORING_ALGORITHM_H 1

#include "Pandora/Algorithm.h"
#include "Api/PandoraApi.h"

#include <TFile.h>
#include <string>
#include <TTree.h>
#include <vector>

namespace lc_content
{

//------------------------------------------------------------------------------------------------------------------------------------------


/**
 *  @brief PfoMonitoringAlgorithm class
 */
class PfoMonitoringAlgorithm : public pandora::Algorithm
{
public:
    /**
     *  @brief Default constructor
     */
    PfoMonitoringAlgorithm();

    /**
     *  @brief  Destructor
     */
    ~PfoMonitoringAlgorithm();

private:
    typedef std::map<const pandora::MCParticle*, float> MCParticleToFloatMap;

    pandora::StatusCode Run();
    pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

    /**
     *  @brief  ClusterEnergyFractions
     * 
     *  @param  pCluster
     *  @param  fCharged
     *  @param  fPhoton
     *  @param  fneutral
     *  @param  pBestMatchedMcPfo
     */
    void ClusterEnergyFractions(const pandora::Cluster *const pCluster, float &fCharged, float &fPhoton, float &fneutral,
        MCParticleToFloatMap &mcParticleContributions) const;

    /**
     *  @brief  ClusterEnergyFractions
     * 
     *  @param  pCluster
     *  @param  nEcalHits
     *  @param  nHcalHits
     *  @param  nMipEcalHits
     *  @param  nMipHcalHits
     */
     void GetMipLikeHits(const pandora::Cluster *const pCluster, unsigned int &nEcalHits, unsigned int &nHcalHits,
                          unsigned int &nMipEcalHits, unsigned int &nMipHcalHits) const;

    /**
     *  @brief  GetClusterMCParticleInfo
     *
     *  @param  pCluster
     *
     *  @return the best matched MC particle
     */
    const pandora::MCParticle* GetClusterMCParticleInfo(const pandora::Cluster *const pCluster) const;

    /**
     *  @brief  GetBestMCParticleMatch
     *
     *  @param  mcParticleContributions
     *
     *  @return the best matched MC particle
     */
    const pandora::MCParticle* GetBestMCParticleMatch(const MCParticleToFloatMap &mcParticleContributions) const;

    pandora::MCParticleList m_trackMcPfoTargets;

    unsigned int m_nCorrectNeutralHadronPfo;
    unsigned int m_nWrongNeutralHadronPfo;
    unsigned int m_nCorrectChargedHadronPfo;
    unsigned int m_nWrongChargedHadronPfo;
    float m_neutralHadronEnergyFractionCut;

    bool m_createPfoMonData;
    bool m_createClusterMonData;
    bool m_createEventMonData;
    bool m_createCaloHitMonData;
    std::string m_isolatedCaloHitListName; ///< The name of the dedicated isolated hit list

    float m_isolationCutDistanceFine2;      ///< Squared isolation cut distance for fine-granularity hits
    float m_isolationCutDistanceCoarse2;    ///< Squared isolation cut distance for coarse-granularity hits
    float m_isolationSearchSafetyFactor;    ///< Safety factor applied to the KD-tree search radius for isolation
    unsigned int m_isolationNLayers;        ///< Number of adjacent pseudo-layers to examine for isolation

    int m_eventNumber; ///< running event counter, incremented every Run()
};

//------------------------------------------------------------------------------------------------------------------------------------------


class PfoMonitoringAlgorithmFactory : public pandora::AlgorithmFactory
{
public:
    pandora::Algorithm *CreateAlgorithm() const { return new lc_content::PfoMonitoringAlgorithm; }
};

pandora::StatusCode RegisterPfoMonitoringAlgorithm(const pandora::Pandora &pandora)
{
    const pandora::StatusCode statusCode(PandoraApi::RegisterAlgorithmFactory(pandora, "PfoMonitoring", new PfoMonitoringAlgorithmFactory()));
    if (pandora::STATUS_CODE_SUCCESS != statusCode)
        return statusCode;
    return pandora::STATUS_CODE_SUCCESS;
}

} // namespace lc_content

#endif // #ifndef LC_PFO_MONITORING_ALGORITHM_H
