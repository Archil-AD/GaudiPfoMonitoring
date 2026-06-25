# GaudiPfoMonitoring
A software framework to identify PandoraPFA algorithms that require tuning.

This framework combines PandoraPFA algorithms with Gaudi (key4hep’s event processing framework) to produce a simple ROOT ntuple containing the key CaloHit and Cluster variables on which PandoraPFA applies cuts during Particle Flow reconstruction.

In addition, reconstructed PFO variables are included for performance evaluation.

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
Describes the topological clusters formed from hits.
* **energy**: Total reconstructed energy of the cluster.
* **nHits**: Total number of hits, with breakdowns for `nEcalHits`, `nHcalHits`, `nMipLikeHits`, `nMipEcalHits`, and `nMipHcalHits`.
* **startLayer / nLayers**: Longitudinal extent of the cluster.
* **isEm**: Flag indicating if the cluster profile matches an electromagnetic shower.
* **isInPfo**: Indicates if the cluster was successfully associated with a Particle Flow Object.
* **mcPdg / mcEnergy**: Truth information from the best-matched Monte Carlo particle.
* **minClusterDistance**: Minimum distance to the most energetic cluster in the event.
* **distToMostEnergeticClusterCentroid**: Distance to the energy-weighted centroid of the most energetic cluster in the event.
* **minInnerLayerSeparation**: Minimum separation distance between inner-layer centroids of this cluster and the most energetic cluster.
* **minGenericDistance / minParallelDistance**: Advanced distance metrics (perpendicular and parallel projections to the lead cluster direction) used for cluster merging tuning.
* **layerSpan**: Layer span relative to the most energetic cluster, as defined in the `ProximityBasedMergingAlgorithm`. A positive value indicates overlap.
* **showerLayerSpan**: Shower layer span relative to the most energetic cluster, as defined in the `ProximityBasedMergingAlgorithm`. A positive value indicates the cluster starts after the shower start of the most energetic cluster.

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
