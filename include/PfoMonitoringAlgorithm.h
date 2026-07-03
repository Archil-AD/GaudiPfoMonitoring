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
     *  @brief  GetClusterShowerQuantities
     *
     *  @param  pCluster the cluster to examine
     *  @param  nRadiationLengths output: total radiation lengths spanned by the cluster
     *  @param  nRadiationLengthsBeforeClusterStart output: radiation lengths before cluster start
     *  @param  layer90RadLengths output: radiation lengths before layer90
     *  @param  showerMaxRadLengths output: radiation lengths before shower maximum
     *  @param  energyAboveHighRadLengths output: energy above high radiation length threshold
     *  @param  radial90 output: radius containing 90% of electromagnetic energy
     */
     void GetClusterShowerQuantities(const pandora::Cluster *const pCluster, float &nRadiationLengths, float &nRadiationLengthsBeforeClusterStart, float &layer90RadLengths,
         float &showerMaxRadLengths, float &energyAboveHighRadLengths, float &radial90) const;

    /**
     *  @brief  GetClusterMCParticleInfo
     *
     *  @param  pCluster
     *
     *  @return the best matched MC particle
     */
    const pandora::MCParticle* GetClusterMCParticleInfo(const pandora::Cluster *const pCluster, float &fCharged, float &fPhoton,
    float &fNeutral) const;

    /**
     *  @brief  GetBestMCParticleMatch
     *
     *  @param  mcParticleContributions
     *
     *  @return the best matched MC particle
     */
    const pandora::MCParticle* GetBestMCParticleMatch(const MCParticleToFloatMap &mcParticleContributions) const;

    /**
     *  @brief  Get the fraction of hits in a daughter cluster that lie within a cone defined by a parent cluster's fit
     *
     *  @param  pParentCluster the parent cluster, defining the cone
     *  @param  pDaughterCluster the daughter cluster, whose hits are to be checked
     *  @param  parentMipFitResult the result of a fit to the parent cluster's MIP-like segment
     *
     *  @return the fraction of hits in the cone
     */
    float GetFractionInCone(const pandora::Cluster *const pParentCluster, const pandora::Cluster *const pDaughterCluster, const pandora::ClusterFitResult &parentMipFitResult) const;

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
    float m_clusterContactThreshold;        ///< Cluster contact threshold from ProximityBasedMergingAlgorithm
    float m_closeHitThreshold;              ///< Close hit threshold from ProximityBasedMergingAlgorithm
    unsigned int m_nGenericDistanceLayers;  ///< Number of layers to examine for generic distance calculation
    unsigned int m_nAdjacentLayers;         ///< Number of adjacent layers to examine for generic distance calculation
    float m_canMergeMinMipFraction;         ///< Min MIP fraction for a cluster to be mergeable, from ProximityBasedMergingAlgorithm
    float m_canMergeMaxRms;                 ///< Max hit RMS for a cluster to be mergeable, from ProximityBasedMergingAlgorithm
    float m_highRadLengths;                 ///< High radiation length threshold for energyAboveHighRadLengths computation, from LCParticleIdPlugins::LCEmShowerId

    // ConeBasedMergingAlgorithm parameters
    unsigned int m_minLayersToShowerStart;  ///< Minimum number of layers from inner layer to shower start for MIP fit
    float m_coneCosineHalfAngle;            ///< Cosine of the cone half-angle for merging
    float m_minCosConeAngleWrtRadial;       ///< Minimum cosine of the angle between cone axis and radial direction
    float m_cosConeAngleWrtRadialCut1;      ///< First cut on cosine of angle between cone axis and radial direction
    float m_minHitSeparationCut1;           ///< First cut on minimum hit separation for low-angle cones
    float m_cosConeAngleWrtRadialCut2;      ///< Second cut on cosine of angle between cone axis and radial direction
    float m_minHitSeparationCut2;           ///< Second cut on minimum hit separation for low-angle cones
       
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
