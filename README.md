# GaudiPfoMonitoring
A software framework to identify PandoraPFA algorithms that require tuning.

This framework combines PandoraPFA algorithms with Gaudi (key4hep's event processing framework) to produce a simple ROOT ntuple containing the key CaloHit and Cluster variables on which PandoraPFA applies cuts during Particle Flow reconstruction.

In addition, reconstructed PFO variables are included for performance evaluation.

## Architecture

The framework consists of two Gaudi algorithms:

1. **`PfoMonitoringAlgorithm`** (Pandora algorithm, registered as `"PfoMonitoring"`): Runs within the Pandora event processing chain. It extracts CaloHit, Cluster, PFO, MC particle, and event-level monitoring data from the current Pandora lists and stores them in the Gaudi Event Store as custom collections.

2. **`SavePfoMonitoringTree`** (Gaudi algorithm): Reads the monitoring collections from the Event Store at the end of each event and writes them to a ROOT TTree named `events` in the output file `GaudiPfoMonitoring.root`.

### Data Flow

```
Pandora Algorithms → PfoMonitoringAlgorithm → Gaudi Event Store → SavePfoMonitoringTree → GaudiPfoMonitoring.root (TTree "events")
```

### Dependencies

- **PandoraSDK** (≥ 03.00.00): Pandora Software Development Kit
- **LCContent** (≥ 03.00.00): Pandora LC content algorithms
- **Gaudi**: Event processing framework (key4hep)
- **ROOT** (RIO, Tree): Output file format
- **EDM4HEP**: Event data model
- **k4FWCore**: Key4hep framework core
- **podio**: PODIO data model toolkit
- **Marlin** (≥ 1.0): Modular Analysis & Reconstruction for the LINear collider
- **MarlinUtil** (≥ 01.10): Marlin utilities

### YAML-based Code Generation

The monitoring data classes and their Gaudi-compatible collections are auto-generated from YAML definition files using a Python script (`cmake/generate_pfo_monitoring.py`). Each YAML file defines the data members, types, and a unique collection ID (CLID) for the Gaudi Event Store:

| YAML File | Generated Class | Collection CLID |
|-----------|----------------|-----------------|
| `PfoMonData.yaml` | `PfoMonData` | 1001 |
| `ClusterMonData.yaml` | `ClusterMonData` | 1002 |
| `EventMonData.yaml` | `EventMonData` | 1003 |
| `CaloHitMonData.yaml` | `CaloHitMonData` | 1004 |
| `MCParticleMonData.yaml` | `MCParticleMonData` | 1005 |
| `TrackMonData.yaml` | `TrackMonData` | 1006 |

## Configurable Parameters

The `PfoMonitoringAlgorithm` exposes the following configurable parameters via Pandora XML settings:

### Monitoring toggles
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `CreatePfoMonData` | bool | `true` | Enable/disable PFO-level monitoring |
| `CreateClusterMonData` | bool | `true` | Enable/disable cluster-level monitoring |
| `CreateEventMonData` | bool | `true` | Enable/disable event-level monitoring |
| `CreateCaloHitMonData` | bool | `true` | Enable/disable calorimeter hit-level monitoring |
| `CreateMCParticleMonData` | bool | `true` | Enable/disable MC particle-level monitoring |
| `CreateTrackMonData` | bool | `true` | Enable/disable track-level monitoring |

### MCParticle <-> PFO matching parameter
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `McPfoMatchEnergyFractionCut` | float | `0.5` | Minimum fraction of a PFO's energy that must be deposited by an MC particle for that PFO to be considered matched to the MC particle |

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

### Cone-based merging parameters (ConeBasedMergingAlgorithm)
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `MinLayersToShowerStart` | uint | `4` | Minimum number of layers from inner layer to shower start for a valid MIP fit |
| `ConeCosineHalfAngle` | float | `0.9` | Cosine of the cone half-angle for merging |
| `MinCosConeAngleWrtRadial` | float | `0.25` | Minimum cosine of the angle between the cone axis and the radial direction |
| `CosConeAngleWrtRadialCut1` | float | `0.5` | First cut on cosine of angle between cone axis and radial direction |
| `MinHitSeparationCut1` | float | `31.62` | First cut on minimum hit separation for low-angle cones (mm), default = sqrt(1000) |
| `CosConeAngleWrtRadialCut2` | float | `0.75` | Second cut on cosine of angle between cone axis and radial direction |
| `MinHitSeparationCut2` | float | `38.73` | Second cut on minimum hit separation for low-angle cones (mm), default = sqrt(1500) |

### Track-cluster association parameters (TrackClusterAssociationAlgorithm)
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `TcaMaxSearchLayer` | uint | `9` | Maximum pseudo layer to examine when calculating track-cluster distance |
| `TcaParallelDistanceCut` | float | `1000.0` | Maximum allowed projection of track-hit separation along track direction (mm) |
| `TcaMinTrackClusterCosAngle` | float | `0.0` | Minimum cos(angle) between track direction and cluster initial direction |
| `TcaMaxDistanceCut` | float | `10.0` | Maximum track-cluster perpendicular distance (mm) below which the association variables are recorded, mirroring `MaxTrackClusterDistance` in `TrackClusterAssociationAlgorithm`. If the best distance exceeds this cut, `trackClusterDistance` and `trackClusterParallelDistance` are set to -1 |

## Monitoring Variables

The framework monitors six main categories of data, each provided as a TTree for analysis.

### 1. Calorimeter Hits (`CaloHitMonData`)
Monitors individual hit properties and the results of Pandora's hit-level preprocessing.
* **energy**: Input energy of the hit (GeV).
* **position[X,Y,Z]**: Cartesian coordinates of the hit.
* **pseudoLayer**: Longitudinal layer identifier.
* **cellLengthScale**: Geometric scale of the cell.
* **hitType**: Detector region (0: ECAL, 1: HCAL, 2: Other).
* **isIsolated / isPossibleMip**: Binary flags indicating if a hit is considered isolated or MIP-like by Pandora.
* **isolationNearbyHits**: Count of hits within the isolation search radius.
* **shortestIsolationDist**: Distance to the absolute nearest hit.
* **distToMostEnergeticClusterCentroid**: Spatial relationship to the event's lead cluster centroid.
* **mcPdg**: PDG code of the primary MC particle contributing to the hit.

### 2. Clusters (`ClusterMonData`)
Describes the topological clusters formed from hits, with variables used for shower identification and cluster merging tuning.
* **energy**: Total reconstructed energy of the cluster (GeV).
* **nHits**: Total number of calorimeter hits in the cluster.
* **nMipLikeHits**: Number of hits tagged as MIP-like within the cluster.
* **nEcalHits / nHcalHits**: Number of hits in the ECAL / HCAL parts of the cluster.
* **nMipEcalHits / nMipHcalHits**: Number of MIP-like hits in the ECAL / HCAL parts.
* **startLayer / nLayers**: Innermost pseudo-layer occupied by the cluster and number of layers spanned.
* **showerStartLayer**: The pseudo-layer where the particle shower is estimated to begin, as determined by Pandora's shower start finding algorithm.
* **isEm**: Flag (0 or 1) indicating if the cluster is identified as electromagnetic by Pandora's `LCEmShowerId`.
* **passPhotonId**: Flag (0 or 1) indicating if the cluster passed the Pandora photon identification criteria.
* **minClusterDistance**: Minimum distance between this cluster and the most energetic cluster in the event.
* **isInPfo**: Flag (0 or 1) indicating if the cluster is associated with a Particle Flow Object.
* **distToMostEnergeticClusterCentroid**: 3D distance from the cluster's energy-weighted centroid to that of the most energetic cluster.
* **mcPdg / mcEnergy**: PDG code and energy of the best-matched MC particle for the cluster.
* **fNeutral / fPhoton / fCharged**: Fractions of hadronic energy originating from neutral, photon, and charged MC particles respectively.
* **minInnerLayerSeparation**: Minimum separation distance between inner-layer centroids of this cluster and the most energetic cluster.
* **minGenericDistance**: Minimum generic (perpendicular) distance between this cluster and the most energetic cluster, as used in `ProximityBasedMergingAlgorithm`.
* **minParallelDistance**: Minimum parallel distance between this cluster and the most energetic cluster, as used in `ProximityBasedMergingAlgorithm`.
* **layerSpan**: Layer span as defined in `ProximityBasedMerging`: `min(mostEnergetic.outerLayer - cluster.innerLayer, cluster.outerLayer - mostEnergetic.innerLayer)`. Positive values indicate overlap.
* **showerLayerSpan**: Shower layer span as defined in `ProximityBasedMerging`: `cluster.innerLayer - mostEnergetic.showerStartLayer`. Positive values indicate the cluster starts after the shower start of the most energetic cluster.
* **contactFraction**: Fraction of overlapping layers in contact with the most energetic cluster, as defined in `ProximityBasedMergingAlgorithm`.
* **closeHitFraction**: Fraction of hits in this cluster that are close to the most energetic cluster, as defined in `ProximityBasedMergingAlgorithm`.
* **fractionInCone**: Fraction of hits in this cluster (daughter) that fall within the cone of the most energetic cluster (parent), as calculated in `ConeBasedMergingAlgorithm`.
* **canBeMerged**: Flag (0 or 1) indicating if the cluster is a candidate for merging based on `ProximityBasedMergingAlgorithm` criteria (MIP fraction and RMS if passPhotonId).
* **rms**: Transverse root mean square (RMS) of the cluster's hits, as used in `ClusterHelper::CanMergeCluster` and `LCParticleIdPlugins::LCEmShowerId`.
* **dCosR**: Radial Direction Cosine (`dCosR`) of the cluster, as used in `LCParticleIdPlugins::LCEmShowerId`.
* **hasAssociatedTrack**: Flag (0 or 1) indicating if the cluster has at least one associated track.
* **nRadiationLengths**: Total number of radiation lengths spanned by the cluster, computed layer-by-layer as in `LCParticleIdPlugins::LCEmShowerId`.
* **nRadiationLengthsBeforeClusterStart**: Total number of radiation lengths before the cluster start, computed layer-by-layer as in `LCParticleIdPlugins::LCEmShowerId`.
* **layer90RadLengths**: Total number of radiation lengths before the longitudinal layer containing 90% of the electromagnetic energy, computed as in `LCParticleIdPlugins::LCEmShowerId`.
* **showerMaxRadLengths**: Number of radiation lengths before the shower maximum layer (the layer with the highest energy deposit), computed as in `LCParticleIdPlugins::LCEmShowerId`.
* **fractionOfEnergyAboveHighRadLengths**: Fraction of electromagnetic energy deposited above a configurable high radiation length threshold, computed as `energyAboveHighRadLengths / totalElectromagneticEnergy` as in `LCParticleIdPlugins::LCEmShowerId`.
* **radial90**: Radius (in mm) containing 90% of the cluster's electromagnetic energy, computed from hit energy and radial distance as in `LCParticleIdPlugins::LCEmShowerId`.
* **chi**: Chi compatibility between this cluster and the most energetic cluster (assumed parent) when the most energetic cluster has track associations, calculated as in `ProximityBasedMergingAlgorithm`: `(clusterEnergy + parentEnergy - trackEnergySum) / sigmaE`. A value < `MaxTrackClusterChi` (default 2.5) indicates this cluster is energetically compatible with the parent's tracks, suggesting it should be merged. Set to -1 if the most energetic cluster has no tracks or if this cluster IS the most energetic cluster.
* **chi0**: Chi0 compatibility of the most energetic cluster's energy with its own track energy, calculated as in `ProximityBasedMergingAlgorithm`: `(parentEnergy - trackEnergySum) / sigmaE`. The chi² difference criterion `chi² - chi0² < MaxTrackClusterDChi2` (default 1.0) is used to check if the addition of this cluster's energy significantly worsens the energy-track compatibility. Set to -1 if the most energetic cluster has no tracks.
* **trackClusterDistance**: Perpendicular distance between the closest track direction (at calorimeter) and the closest calo hit in the cluster, computed by scanning all tracks in the current track list and selecting the one with the minimum perpendicular distance, as in `ClusterHelper::GetTrackClusterDistance` used by `TrackClusterAssociationAlgorithm`. The distance is the magnitude of the cross product of the track direction with the position difference vector to each hit, minimized over all hits within the search layer and parallel distance cuts. Set to -1 if no track yields a valid distance.
* **trackClusterParallelDistance**: Parallel distance (projection of the position difference along the track direction) for the closest hit of the best-matching track that determined `trackClusterDistance`, as used in `ClusterHelper::GetTrackClusterDistance` (`TrackClusterAssociationAlgorithm`). Only hits with a parallel distance below `TcaParallelDistanceCut` are considered. Set to -1 if no track yields a valid distance.
* **trackClusterCosAngle**: Cos(angle) between the best-matching track direction and the cluster initial direction. When a valid distance is found, this is the cos angle of that track; when no distance is found for any track, the largest cos angle across all tracks is recorded. Set to -2 if there are no tracks in the event.

### 3. Particle Flow Objects (`PfoMonData`)
The final reconstructed particles (PFOs).
* **energy, px, py, pz, mass, pdg**: Standard 4-momentum and particle identity.
* **fNeutral / fPhoton / fCharged**: Fractions of hadronic energy originating from specific MC particle types, used to study "confusion".
* **nHits**: Total number of hits, with breakdowns for `nEcalHits`, `nHcalHits`, `nMipLikeHits`, `nMipEcalHits`, and `nMipHcalHits`.
* **startLayer / nLayers**: Longitudinal extent of the constituent clusters.
* **nClusters / nEmClusters**: Number of constituent clusters.
* **hasMatchedMcParticle**, **matchedMcPdg**, **matchedMcEnergy**, **matchedMcAlpha**: PFO->MCParticle match using the same energy-fraction logic as the MCParticle monitoring data (flag, PDG, energy and opening angle with the matched MC particle). These fields are only filled when `CreateMCParticleMonData` is true; otherwise they remain at their defaults (0 / 0 / 0 / -1). A PFO is considered matched when an MC particle deposited at least `McPfoMatchEnergyFractionCut` (25% by default) of the PFO's hadronic energy; if an MC particle deposited energy into more than one PFO, the PFO with the largest energy deposition is selected. When several MC particles are matched to the same PFO, only the one with the largest energy contribution to that PFO keeps the match and the others are set to 0.
* **minClusterDistance**: Proximity of the PFO's clusters to the lead cluster.

### 4. Event Summary (`EventMonData`)
Global metrics for energy flow and reconstruction efficiency per event.
* **eventNumber**: Sequential event identifier.
* **Hit Matrix**: Counts and energy sums for hits classified by the reconstruction state:
    * `nClusteredNonIsolatedHits` / `clusteredNonIsolatedEnergy`
    * `nClusteredIsolatedHits` / `clusteredIsolatedEnergy`
    * `nUnclusteredIsolatedHits` / `unclusteredIsolatedEnergy`
    * `nUnclusteredNonIsolatedHits` / `unclusteredNonIsolatedEnergy`
* **totalEnergy**: Sum of all hit energy in the event.
* **chargedEnergy**: Total hadronic energy from charged MC particles in the event.
* **neutralEnergy**: Total hadronic energy from neutral MC particles in the event.
* **Confusion Matrix**: Fractions of energy correctly/incorrectly reconstructed:
    * `fChargedEnergyRecoCharged`: Fraction of charged hadronic energy reconstructed as charged.
    * `fChargedEnergyRecoNeutral`: Fraction of charged hadronic energy reconstructed as neutral.
    * `fNeutralEnergyRecoCharged`: Fraction of neutral hadronic energy reconstructed as charged.
    * `fNeutralEnergyRecoNeutral`: Fraction of neutral hadronic energy reconstructed as neutral.
* **nClusters / nPFOs**: Total number of reconstructed objects.

### 5. MC Particles (`MCParticleMonData`)
Truth-level information for every MC particle in Pandora's current MC particle list, used to study reconstruction efficiency and confusion at the truth level.
* **pdg**: PDG code of the MC particle (from `pandora::MCParticle::GetParticleId()`).
* **energy**: Total energy of the MC particle (GeV).
* **px, py, pz**: Cartesian components of the MC particle momentum (from `pandora::MCParticle::GetMomentum()`).
* **generatorStatus**: Generator status of the MC particle. Not directly available from the `pandora::MCParticle` interface, so it is currently set to `0` (unknown) in this collection; for the proper generator status code, retrieve it from the upstream `edm4hep::MCParticleCollection` via the `MCParticles` handle used by `SavePfoMonitoringTree`.
* **hasCaloHit**: Flag (0 or 1) indicating whether the MC particle is the main MC particle of at least one reconstructed calorimeter hit in Pandora's `CaloHits` list, evaluated via `MCParticleHelper::GetMainMCParticle(pCaloHit)`.
* **hasMatchedPfo**: Flag (0 or 1) indicating whether the MC particle was matched to a reconstructed PFO. A PFO is considered matched when the MC particle deposited at least `McPfoMatchEnergyFractionCut` (25% by default) of the PFO's hadronic energy; if the MC particle deposited energy into more than one PFO, the PFO with the largest energy deposition is selected. When several MC particles are matched to the same PFO, only the one with the largest energy contribution to that PFO keeps the match and the others are set to 0.
* **matchedPfoPdg**: PDG code of the matched reconstructed PFO (from `pandora::ParticleFlowObject::GetParticleId()`), or `0` if no PFO was matched.

### 6. Tracks (`TrackMonData`)
Track-level kinematic variables and track-cluster association quantities for every track in Pandora's current track list. The track-cluster association is computed from the track's perspective, scanning all clusters to find the best-matching one (minimum perpendicular distance), as in `ClusterHelper::GetTrackClusterDistance` used by `TrackClusterAssociationAlgorithm`.
* **pt**: Transverse momentum of the track at the distance of closest approach (GeV), computed as `sqrt(px² + py²)` from `pandora::Track::GetMomentumAtDca()`.
* **theta**: Polar angle of the track momentum at the distance of closest approach (rad), computed as `acos(pz / |p|)`.
* **p**: Total momentum of the track at the distance of closest approach (GeV), from `pandora::Track::GetMomentumAtDca().GetMagnitude()`.
* **trackClusterDistance**: Perpendicular distance between this track's direction (at calorimeter) and the closest calo hit in the best-matching cluster, computed by scanning all clusters. The distance is the magnitude of the cross product of the track direction with the position difference vector to each hit, minimized over all clusters and their hits within the search layer and parallel distance cuts. Set to -1 if no cluster yields a valid distance or if the best distance exceeds `TcaMaxDistanceCut` (default 10 mm, mirroring `MaxTrackClusterDistance` in `TrackClusterAssociationAlgorithm`).
* **trackClusterParallelDistance**: Parallel distance (projection of the position difference along the track direction) for the closest hit of the best-matching cluster that determined `trackClusterDistance`. Set to -1 if no cluster yields a valid distance or if the best distance exceeds `TcaMaxDistanceCut`.
* **trackClusterCosAngle**: Cos(angle) between this track's direction and the best-matching cluster's initial direction. When a valid distance is found, this is the cos angle of that cluster; when no distance is found for any cluster (including when the best distance exceeds `TcaMaxDistanceCut`), the largest cos angle across all clusters is recorded. Set to -2 if there are no clusters in the event.
* **matchedClusterEnergy**: Hadronic energy (GeV) of the best-matching cluster (the one that determined `trackClusterDistance`). Set to 0 if no cluster yields a valid distance (or if the best distance exceeds `TcaMaxDistanceCut`).
* **matchedClusterStartLayer**: Inner pseudo-layer of the best-matching cluster (the one that determined `trackClusterDistance`). Set to 0 if no cluster yields a valid distance.
* **matchedClusterNLayers**: Number of pseudo layers spanned by the best-matching cluster (`outer - inner + 1`). Set to 0 if no cluster yields a valid distance.
* **matchedClusterIsEm**: Flag (0 or 1) indicating if the best-matching cluster passes Pandora's electromagnetic shower identification (`pCluster->PassPhotonId()`, from `LCParticleIdPlugins::LCEmShowerId`). Set to 0 if no cluster yields a valid distance.
* **matchedClusterNHits**: Total number of calo hits in the best-matching cluster (`pCluster->GetNCaloHits()`). Set to 0 if no cluster yields a valid distance.
* **matchedClusterNMipLikeHits**: Number of MIP-like calo hits in the best-matching cluster (`pCluster->GetNPossibleMipHits()`). Set to 0 if no cluster yields a valid distance.

## Output ROOT Tree Structure

The output file `GaudiPfoMonitoring.root` contains a single TTree named `events` with the following branch naming convention:

| Prefix | Description | Multiplicity |
|--------|-------------|--------------|
| `evt_*` | Event-level scalars | 1 entry per event |
| `pfo_*` | PFO-level vectors | N entries per event (one per PFO) |
| `clus_*` | Cluster-level vectors | N entries per event (one per cluster) |
| `hit_*` | CaloHit-level vectors | N entries per event (one per hit) |
| `mc_*` | MC particle-level vectors | N entries per event (one per MC particle) |
| `trk_*` | Track-level vectors | N entries per event (one per track) |

## Usage

### Pandora XML Configuration

```xml
<algorithm type="PfoMonitoring">
  <!-- Optional: override any configurable parameters -->
  <CreatePfoMonData>true</CreatePfoMonData>
  <CreateClusterMonData>true</CreateClusterMonData>
  <CreateEventMonData>true</CreateEventMonData>
  <CreateCaloHitMonData>true</CreateCaloHitMonData>
  <CreateMCParticleMonData>true</CreateMCParticleMonData>
</algorithm>
```

### Gaudi Options File

```python
from Gaudi.Configuration import *
from Configurables import ApplicationMgr, EventDataSvc

# ... other algorithm configuration ...

from Configurables import SavePfoMonitoringTree
saver = SavePfoMonitoringTree("SavePfoMonitoringTree")

ApplicationMgr(
    TopAlg=[..., saver],
    EvtSel='NONE',
    EvtMax=100,
    ExtSvc=[EventDataSvc("EventDataSvc")],
)
```

## Building

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=<install_prefix> ..
make -j$(nproc)
make install
```

The build system uses YAML-based code generation: the six YAML definition files (`PfoMonData.yaml`, `ClusterMonData.yaml`, `EventMonData.yaml`, `CaloHitMonData.yaml`, `MCParticleMonData.yaml`, `TrackMonData.yaml`) are processed by `cmake/generate_pfo_monitoring.py` to produce C++ headers for the data classes and their Gaudi-compatible collections.