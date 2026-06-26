# GaudiPfoMonitoring
A software framework to identify PandoraPFA algorithms that require tuning.

This framework combines PandoraPFA algorithms with Gaudi (key4hep's event processing framework) to produce a simple ROOT ntuple containing the key CaloHit and Cluster variables on which PandoraPFA applies cuts during Particle Flow reconstruction.

In addition, reconstructed PFO variables are included for performance evaluation.

## Configurable Parameters

The `PfoMonitoringAlgorithm` exposes the following configurable parameters via Pandora XML settings:

### Monitoring toggles
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `CreatePfoMonData` | bool | `true` | Enable/disable PFO-level monitoring |
| `CreateClusterMonData` | bool | `true` | Enable/disable cluster-level monitoring |
| `CreateEventMonData` | bool | `true` | Enable/disable event-level monitoring |
| `CreateCaloHitMonData` | bool | `true` | Enable/disable calorimeter hit-level monitoring |

### Isolation parameters (CaloHit monitoring)
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `IsolatedCaloHitListName` | string | `IsolatedCaloHitList` | Name of the dedicated isolated hit list |
| `IsolationCutDistanceFine` | float | `25.0` | Isolation cut distance for fine-granularity hits (mm) |
| `IsolationCutDistanceCoarse` | float | `200.0` | Isolation cut distance for coarse-granularity hits (mm) |
| `IsolationSearchSafetyFactor` | float | `2.0` | Safety factor applied to the KD-tree search radius for isolation |
| `IsolationNLayers` | uint | `2` | Number of adjacent pseudo-layers to examine for isolation |

### Cluster merging parameters (ProximityBasedMergingAlgorithm)
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `ClusterContactThreshold` | float | `2.0` | Contact threshold for calculating the contact fraction between cluster pairs |
| `CloseHitThreshold` | float | `50.0` | Distance threshold (mm) for determining if a hit is "close" to the most energetic cluster |
| `NGenericDistanceLayers` | uint | `5` | Number of layers to examine when computing the generic (perpendicular) distance between clusters |
| `NAdjacentLayers` | uint | `2` | Number of adjacent layers to examine when computing the generic distance between clusters |
| `CanMergeMinMipFraction` | float | `0.7` | Minimum MIP fraction for a cluster to be considered mergeable |
| `CanMergeMaxRms` | float | `5.0` | Maximum hit RMS (mm) for a cluster to be considered mergeable |

### Shower shape parameters (LCParticleIdPlugins::LCEmShowerId)
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `HighRadLengths` | float | `30.0` | High radiation length threshold used to compute `fractionOfEnergyAboveHighRadLengths` |

## Monitoring Variables

The framework monitors four main categories of data, each provided as a TTree for analysis.

### 1. Calorimeter Hits (`CaloHitMonData`)
Monitors individual hit properties and the results of Pandora's hit-level preprocessing.
* **energy**: Input energy of the hit (GeV).
* **position[X,Y,Z]**: Cartesian coordinates of the hit.
* **pseudoLayer**: Longitudinal layer identifier.
* **hitType**: Detector region (0: ECAL, 1: HCAL, 2: Other).
* **isIsolated / isPossibleMip**: Binary flags indicating if a hit is considered isolated or MIP-like by Pandora.
* **isolationNearbyHits**: Count of hits within the isolation search radius.
* **shortestIsolationDist**: Distance to the absolute nearest hit.
* **distToMostEnergeticClusterCentroid**: Spatial relationship to the event's lead cluster centroid.

### 2. Clusters (`ClusterMonData`)
Describes the topological clusters formed from hits, with variables used for shower identification and cluster merging tuning.
* **energy**: Total reconstructed energy of the cluster (GeV).
* **nHits**: Total number of calorimeter hits in the cluster.
* **nMipLikeHits**: Number of hits tagged as MIP-like within the cluster.
* **nEcalHits / nHcalHits**: Number of hits in the ECAL / HCAL parts of the cluster.
* **nMipEcalHits / nMipHcalHits**: Number of MIP-like hits in the ECAL / HCAL parts.
* **startLayer / nLayers**: Innermost pseudo-layer occupied by the cluster and number of layers spanned.
* **isEm**: Flag (0 or 1) indicating if the cluster is identified as electromagnetic by Pandora's `LCEmShowerId`.
* **passPhotonId**: Flag (0 or 1) indicating if the cluster passed the Pandora photon identification criteria.
* **minClusterDistance**: Minimum distance between this cluster and the most energetic cluster in the event.
* **isInPfo**: Flag (0 or 1) indicating if the cluster is associated with a Particle Flow Object.
* **distToMostEnergeticClusterCentroid**: 3D distance from the cluster's energy-weighted centroid to that of the most energetic cluster.
* **mcPdg / mcEnergy**: PDG code and energy of the best-matched MC particle for the cluster.
* **minInnerLayerSeparation**: Minimum separation distance between inner-layer centroids of this cluster and the most energetic cluster.
* **minGenericDistance**: Minimum generic (perpendicular) distance between this cluster and the most energetic cluster, as used in `ProximityBasedMergingAlgorithm`.
* **minParallelDistance**: Minimum parallel distance between this cluster and the most energetic cluster, as used in `ProximityBasedMergingAlgorithm`.
* **layerSpan**: Layer span as defined in `ProximityBasedMerging`: `min(mostEnergetic.outerLayer - cluster.innerLayer, cluster.outerLayer - mostEnergetic.innerLayer)`. Positive values indicate overlap.
* **showerLayerSpan**: Shower layer span as defined in `ProximityBasedMerging`: `cluster.innerLayer - mostEnergetic.showerStartLayer`. Positive values indicate the cluster starts after the shower start of the most energetic cluster.
* **contactFraction**: Fraction of overlapping layers in contact with the most energetic cluster, as defined in `ProximityBasedMergingAlgorithm`.
* **closeHitFraction**: Fraction of hits in this cluster that are close to the most energetic cluster, as defined in `ProximityBasedMergingAlgorithm`.
* **canBeMerged**: Flag (0 or 1) indicating if the cluster is a candidate for merging based on `ProximityBasedMergingAlgorithm` criteria (MIP fraction and RMS if passPhotonId).
* **rms**: Transverse root mean square (RMS) of the cluster's hits, as used in `ClusterHelper::CanMergeCluster` and `LCParticleIdPlugins::LCEmShowerId`.
* **dCosR**: Radial Direction Cosine (`dCosR`) of the cluster, as used in `LCParticleIdPlugins::LCEmShowerId`.
* **hasAssociatedTrack**: Flag (0 or 1) indicating if the cluster has at least one associated track.
* **nRadiationLengthsBeforeShowerStart**: Total number of radiation lengths before the cluster start, computed layer-by-layer as in `LCParticleIdPlugins::LCEmShowerId`.
* **layer90RadLengths**: Total number of radiation lengths before the longitudinal layer containing 90% of the electromagnetic energy, computed as in `LCParticleIdPlugins::LCEmShowerId`.
* **showerMaxRadLengths**: Number of radiation lengths before the shower maximum layer (the layer with the highest energy deposit), computed as in `LCParticleIdPlugins::LCEmShowerId`.
* **fractionOfEnergyAboveHighRadLengths**: Fraction of electromagnetic energy deposited above a configurable high radiation length threshold, computed as `energyAboveHighRadLengths / totalElectromagneticEnergy` as in `LCParticleIdPlugins::LCEmShowerId`.
* **radial90**: Radius (in mm) containing 90% of the cluster's electromagnetic energy, computed from hit energy and radial distance as in `LCParticleIdPlugins::LCEmShowerId`.

### 3. Particle Flow Objects (`PfoMonData`)
The final reconstructed particles (PFOs).
* **energy, px, py, pz, mass, pdg**: Standard 4-momentum and particle identity.
* **alpha**: Angular separation between the PFO and its matched MC particle.
* **fNeutral / fPhoton / fCharged**: Fractions of hadronic energy originating from specific MC particle types, used to study "confusion".
* **nHits**: Total number of hits, with breakdowns for `nEcalHits`, `nHcalHits`, `nMipLikeHits`, `nMipEcalHits`, and `nMipHcalHits`.
* **startLayer / nLayers**: Longitudinal extent of the constituent clusters.
* **nClusters / nEmClusters**: Number of constituent clusters.
* **mcPdg / mcEnergy**: MC truth identity and energy.
* **minClusterDistance**: Proximity of the PFO's clusters to the lead cluster.

### 4. Event Summary (`EventMonData`)
Global metrics for energy flow and reconstruction efficiency per event.
* **eventNumber**: Event identifier.
* **Hit Matrix**: Counts and energy sums for hits classified by the reconstruction state:
    * `nClusteredNonIsolatedHits` / `clusteredNonIsolatedEnergy`
    * `nClusteredIsolatedHits` / `clusteredIsolatedEnergy`
    * `nUnclusteredIsolatedHits` / `unclusteredIsolatedEnergy`
    * `nUnclusteredNonIsolatedHits` / `unclusteredNonIsolatedEnergy`
* **totalEnergy**: Sum of all hit energy in the event.
* **nClusters / nPFOs**: Total number of reconstructed objects.