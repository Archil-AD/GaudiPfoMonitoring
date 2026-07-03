# GaudiPfoMonitoring
A software framework to identify PandoraPFA algorithms that require tuning.

This framework combines PandoraPFA algorithms with Gaudi (key4hep's event processing framework) to produce a simple ROOT ntuple containing the key CaloHit and Cluster variables on which PandoraPFA applies cuts during Particle Flow reconstruction.

In addition, reconstructed PFO variables are included for performance evaluation.

## Architecture

The framework consists of two Gaudi algorithms:

1. **`PfoMonitoringAlgorithm`** (Pandora algorithm, registered as `"PfoMonitoring"`): Runs within the Pandora event processing chain. It extracts CaloHit, Cluster, PFO, and event-level monitoring data from the current Pandora lists and stores them in the Gaudi Event Store as custom collections.

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

## Configurable Parameters

The `PfoMonitoringAlgorithm` exposes the following configurable parameters via Pandora XML settings:

### Monitoring toggles
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `CreatePfoMonData` | bool | `true` | Enable/disable PFO-level monitoring |
| `CreateClusterMonData` | bool | `true` | Enable/disable cluster-level monitoring |
| `CreateEventMonData` | bool | `true` | Enable/disable event-level monitoring |
| `CreateCaloHitMonData` | bool | `true` | Enable/disable calorimeter hit-level monitoring |

### Neutral hadron classification
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `NeutralHadronEnergyFractionCut` | float | `0.7` | Minimum neutral energy fraction for a neutral PFO to be classified as a correct neutral hadron |

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

## Monitoring Variables

The framework monitors four main categories of data, each provided as a TTree for analysis.

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

## Output ROOT Tree Structure

The output file `GaudiPfoMonitoring.root` contains a single TTree named `events` with the following branch naming convention:

| Prefix | Description | Multiplicity |
|--------|-------------|--------------|
| `evt_*` | Event-level scalars | 1 entry per event |
| `pfo_*` | PFO-level vectors | N entries per event (one per PFO) |
| `clus_*` | Cluster-level vectors | N entries per event (one per cluster) |
| `hit_*` | CaloHit-level vectors | N entries per event (one per hit) |

## Usage

### Pandora XML Configuration

```xml
<algorithm type="PfoMonitoring">
  <!-- Optional: override any configurable parameters -->
  <CreatePfoMonData>true</CreatePfoMonData>
  <CreateClusterMonData>true</CreateClusterMonData>
  <CreateEventMonData>true</CreateEventMonData>
  <CreateCaloHitMonData>true</CreateCaloHitMonData>
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

The build system uses YAML-based code generation: the four YAML definition files (`PfoMonData.yaml`, `ClusterMonData.yaml`, `EventMonData.yaml`, `CaloHitMonData.yaml`) are processed by `cmake/generate_pfo_monitoring.py` to produce C++ headers for the data classes and their Gaudi-compatible collections.




# Tuning Guide: Optimizing PandoraPFA Parameters for a Detector (work in progress)

This guide explains how to use the monitoring output from `GaudiPfoMonitoring` to systematically optimise the PandoraPFA reconstruction parameters for a specific detector configuration.

## 1. Overview of the PandoraPFA Reconstruction Chain

PandoraPFA's reconstruction proceeds through several stages, each with configurable algorithms. The monitored variables in `GaudiPfoMonitoring` map to specific decision points in this chain:

```
Digitised CaloHits
        ↓
[1] CaloHitPreparation
    - Hit isolation tagging
    - MIP-like hit tagging
        ↓
[2] ConeClustering
    - Seed clusters with tracks
    - Grow clusters by associating hits (ConeClustering)
        ↓
[3] TopologicalAssociation (Cluster Merging)
    ├── LoopingTracks            (handle looping particles)
    ├── BrokenTracks             (handle broken track segments)
    ├── ShowerMipMerging         (merge MIP-like segments with shower segments)
    ├── BackscatteredTracks      (handle back-scattered particles)
    ├── ProximityBasedMerging    (merge close clusters)
    ├── ConeBasedMerging         (merge clusters inside shower cones)
    ├── MipPhotonSeparation      (...)
    ├── HighEnergyPhotonRecovery (...)
    ├── SoftClusterMerging       (merge tiny, low-energy clusters)
    ├── IsolatedHitMerging       (attach isolated hits to clusters)
        ↓
[4] FragmentRemoval
    ├── RecoPhotonFragmentMerging
    ├── MainFragmentRemoval
    ├── NeutralFragmentRemoval
    ├── ...
        ↓
[5] ParticleId
    ├── LCEmShowerId             (EM shower identification)
    ├── PhotonId                 (photon identification)
    ├── FinalParticleId
        ↓
[6] PfoCreation
    - Build Particle Flow Objects from clusters+tracks
```

## 2. Strategy for Parameter Optimisation

### Step 1: Identify Which Stage Needs Tuning

Use the event-level monitoring variables (`evt_*` branches) to identify reconstruction deficiencies.
Start with single particle simulation.

|Particle | Observable | What It Indicates | Suspect Stage(s) |
|---------|------------|-------------------|------------------|
| K-long | `evt_unclusteredIsolatedEnergy` / `evt_unclusteredNonIsolatedEnergy` | High unclustered energy = missed hits | CaloHitPreparation (isolation cuts too loose) |
| Muon | `Sum$(clus_nMipEcalHits)/Sum$(clus_nEcalHits)` and `Sum$(clus_nMipHcalHits)/Sum$(clus_nHcalHits)` | Small fraction of muon hits flagged as MIP-like | CaloHitPreparation (MIP-like cuts too tight) |
| K-long | `evt_nClusters` | More than 1 cluster per particle, Cluster fragmentation  | ProximityBasedMerging, ConeBasedMerging, SoftClusterMerging, IsolatedHitMerging |
| Charged pion | `evt_fChargedEnergyRecoNeutral` | Charged particle energy mis-reconstructed as neutral | TopologicalAssociation (merging failures), FragmentRemoval |



### Step 2: Analyse Hit-Level Decisions with CaloHitMonData and ClusterMonData

The `hit_*` branches reveal how individual hits are classified:

- **Isolated hits**: Plot `hit_isIsolated` vs. `hit_isolationNearbyHits` and `hit_shortestIsolationDist`. If too many genuine hits are flagged as isolated, the isolation cuts are too tight. If too few background hits are isolated, cuts are too loose.

  *Parameters to adjust* (in `CaloHitPreparationAlgorithm`):
  - `IsolationCutDistanceFine` / `IsolationCutDistanceCoarse` — increase to make isolation more restrictive
  - `IsolationNLayers` — increase to examine more layers (fewer isolated hits)
  - `IsolationSearchSafetyFactor` — increase to examine more hits
  - `IsolationMaxNearbyHits` — increase to require more nearby hits before tagging a hit as non-isolated

- **MIP-like hits**: Plot `Sum$(clus_nMipEcalHits)/Sum$(clus_nEcalHits)` and `Sum$(clus_nMipHcalHits)/Sum$(clus_nHcalHits)` for single muons. If MIP-like tagging is wrong, check `hit_isolationNearbyHits`.
Hits are tagged as MIP-like if MIP scale energy is above 5 MIPs and there is at most 1 nearby hit in the same layer.

   *Parameters to adjust* (in `CaloHitPreparationAlgorithm`):
   - `MipLikeMipCut` — MIP equivalent energy threshold (default: `5`)
   - `MipNCellsForNearbyHit` — cell-level distance for "nearby" hit definition (default: `2`)
   - `MipMaxNearbyHits` — max nearby hits for MIP tagging (default: `1`)

### Step 3: Analyse Cluster Merging with ClusterMonData

The `clus_*` branches are the most powerful for tuning **ProximityBasedMergingAlgorithm** and **ConeBasedMergingAlgorithm**. The key strategy is to compare clusters that *should* have been merged (true fragments) with those that *should not* (genuinely distinct).

#### 3a. ProximityBasedMerging

This algorithm merges a daughter cluster into a parent cluster if the daughter is identified as a "fragment". A cluster is a fragment if **any** of three criteria is met:

1. **Close hit fraction** (easiest to tune):
   ```
   IsFragment = (closeHitFraction > MinCloseHitFraction)
   ```
   - **`MinCloseHitFraction`** (default: `0.2`): Fraction of daughter hits close to parent.
   - **`CloseHitThreshold`** (default: `50.0` mm): Distance threshold for a hit to be "close".
   
   **Tuning**: Use `clus_closeHitFraction`. For true fragment pairs, this should be high; for distinct clusters, low. Adjust `MinCloseHitFraction` at the crossing point. Adjust `CloseHitThreshold` to change the distance scale.

2. **Contact fraction**:
   ```
   IsFragment = (contactFraction > MinContactFraction)
   ```
   - **`MinContactFraction`** (default: `0.3`): Fraction of overlapping layers in contact.
   - **`ClusterContactThreshold`** (default: `2.0`): Maximum distance between layer centroids for "contact".
   
   **Tuning**: Use `clus_contactFraction` and `clus_layerSpan`. True fragments typically have high contact fraction and positive layer span. Adjust `MinContactFraction` and `ClusterContactThreshold` to separate fragments from distinct clusters.

3. **Helix projection** (for track-associated parent clusters):
   - **`MaxClusterHelixDistance`** (default: `50.0` mm): Max mean distance between helix projection and daughter hits.
   - **`MaxHelixPathlengthToDaughter`** (default: `300.0` mm): Max pathlength from calorimeter to daughter.
   
   **Tuning**: This criterion is specific to events with tracks. If you see track-associated clusters missing their fragments, increase `MaxClusterHelixDistance`.

The **preselection** of mergeable clusters is controlled by:
- **`CanMergeMinMipFraction`** (default: `0.7`): Minimum MIP fraction.
- **`CanMergeMaxRms`** (default: `5.0` mm): Maximum hit RMS.

Use `clus_canBeMerged` and the distributions of `clus_rms` and MIP-related variables to verify these cuts.

The **pair-level cuts** before considering fragments:
- **`MinClusterInnerLayer`** (default: `6`): Skip pairs where both start below this layer.
- **`MinLayerSpan`** (default: `-2`): Minimum `min(outer(parent)-inner(daughter), outer(daughter)-inner(parent))`. Use `clus_layerSpan`.
- **`MinShowerLayerSpan`** (default: `-4`): Minimum `inner(daughter) - showerStart(parent)`. Use `clus_showerLayerSpan`.
- **`MaxGenericDistance`** (default: `50.0` mm): Maximum perpendicular distance. Use `clus_minGenericDistance`.
- **`MaxParallelDistance`** (default: `1000.0` mm): Maximum parallel distance. Use `clus_minParallelDistance`.
- **`MaxInnerLayerSeparation`** (default: `500.0` mm): Maximum separation between inner layer centroids. Use `clus_minInnerLayerSeparation`.
- **`NGenericDistanceLayers`** (default: `5`): Number of layers to check.
- **`NAdjacentLayersToExamine`** (default: `2`): Adjacent layers in daughter.

**Tuning workflow for ProximityBasedMerging**:
1. Plot `clus_closeHitFraction` for true fragments (`isInPfo=1`) vs. non-fragments (`isInPfo=0`, `isEm=0`). Set `MinCloseHitFraction` at the separation point.
2. Plot `clus_contactFraction` vs. `clus_layerSpan`. Identify the region where true fragments cluster.
3. Check `clus_minGenericDistance` vs. `clus_layerSpan`. Tune `MaxGenericDistance` and `MinLayerSpan` to include true fragments while excluding false positives.
4. Verify that `clus_canBeMerged` correctly classifies clusters using `clus_rms` and MIP information.

#### 3b. ConeBasedMerging

This algorithm merges a daughter cluster into a parent if enough daughter hits fall within a cone projected from the parent's MIP fit.

- **`MinConeFraction`** (default: `0.5`): Minimum fraction of daughter hits in cone.

**Tuning**: Use `clus_fractionInCone`. For true fragments, this should be high; for distinct clusters, low. Set `MinConeFraction` accordingly.

The cone is constructed from the parent's MIP segment fit. Parameters controlling the fit:
- **`MinLayersToShowerStart`** (default: `4`): Minimum layers between inner layer and shower start for a valid fit.

The cone opening angle:
- **`ConeCosineHalfAngle`** (default: `0.9` → ~26° half-angle): Increase to make the cone wider (merge more), decrease to make it narrower (merge less).

The radial direction sanity checks prevent false associations at small angles:
- **`MinCosConeAngleWrtRadial`** (default: `0.25`)
- **`CosConeAngleWrtRadialCut1`** (default: `0.5`) and **`MinHitSeparationCut1`** (default: `sqrt(1000) ≈ 31.6` mm)
- **`CosConeAngleWrtRadialCut2`** (default: `0.75`) and **`MinHitSeparationCut2`** (default: `sqrt(1500) ≈ 38.7` mm)

These cuts prevent the algorithm from merging hits at large distances in low-angle cones (which would be unphysical). Tighten these for detectors with larger radial extent.

**Tuning workflow for ConeBasedMerging**:
1. Plot `clus_fractionInCone` for true fragments vs. non-fragments. Separate by `clus_showerLayerSpan` to understand the dependence.
2. Verify the MIP fit validity: check `clus_dCosR` and `clus_rms` for parent-like clusters.
3. If merging is too aggressive, tighten `ConeCosineHalfAngle` or increase `MinConeFraction`.

#### 3c. SoftClusterMerging

This algorithm targets small, low-energy clusters ("soft clusters") that may be fragments of larger clusters. It is designed to catch fragments missed by `ProximityBasedMerging` and `ConeBasedMerging`. A soft cluster is merged into the nearest suitable parent cluster if it passes several criteria.

**Soft cluster identification** (`IsSoftCluster`):

A cluster is classified as "soft" if **any** of the following is true:
1. **`MaxHitsInSoftCluster`** (default: `5`): Cluster has ≤ this many hits.
2. **`MaxLayersSpannedBySoftCluster`** (default: `3`): Cluster spans ≤ this many pseudo-layers.
3. **`MaxHadEnergyForSoftClusterNoTrack`** (default: `2.0` GeV): Cluster hadronic energy < this threshold.
4. **Hadronic energy < `MinClusterHadEnergy`** (default: `0.25` GeV): Very low-energy clusters are always soft.

A cluster is explicitly **excluded** from being soft if:
- It has track associations (hasAssociatedTrack = 1)
- It passes photon ID AND has electromagnetic energy > **`MinClusterEMEnergy`** (default: `0.025` GeV)

**Parent cluster selection** (`FindBestParentCluster`):

For each hit in the soft daughter cluster, the algorithm finds nearby hits from other clusters within a search radius determined by granularity:
- **`MaxClusterDistanceFine`** (default: `100.0` mm): Search radius for fine-granularity hits.
- **`MaxClusterDistanceCoarse`** (default: `250.0` mm): Search radius for coarse-granularity hits.

Parent candidates must have:
- Hadronic energy ≥ **`MinClusterHadEnergy`** (default: `0.25` GeV)
- Number of hits > **`MaxHitsInSoftCluster`** (default: `5`)

The best parent is the one whose hit is closest to a daughter hit (ties broken by higher parent energy).

**Merge decision** (`CanMergeSoftCluster`):

Once the closest hit-pair distance is found, the daughter is merged if any of the following is true:
1. **`ClosestDistanceCut0`** (default: `50.0` mm): Distance < this → auto-merge.
2. **`ClosestDistanceCut1`** (default: `100.0` mm) AND **`InnerLayerCut1`** (default: `20`): Distance < Cut1 AND daughter inner layer > Cut1 → merge.
3. **`ClosestDistanceCut2`** (default: `250.0` mm) AND **`InnerLayerCut2`** (default: `40`): Distance < Cut2 AND daughter inner layer > Cut2 → merge.
4. Fallback: Distance ≤ corresponding granularity distance cut (Fine/Coarse) AND (daughter hadronic energy < `MinClusterHadEnergy` OR daughter hits < `MinHitsInCluster` (default: `5`)).

**Tuning**:

The `clus_*` monitoring variables that are most relevant for SoftClusterMerging:
- `clus_energy` (hadronic): Check against `MinClusterHadEnergy` and `MaxHadEnergyForSoftClusterNoTrack`
- `clus_nHits`: Check against `MaxHitsInSoftCluster` and `MinHitsInCluster`
- `clus_nLayers`: Check against `MaxLayersSpannedBySoftCluster`
- `clus_startLayer`: Check against `InnerLayerCut1` (20) and `InnerLayerCut2` (40)
- `clus_minClusterDistance`: Compare with the ClosestDistanceCut thresholds
- `clus_isInPfo`: After reconstruction, check if soft clusters were merged
- `clus_passPhotonId` and `clus_isEm`: Used to exclude photon-like clusters from soft classification

**Tuning workflow for SoftClusterMerging**:
1. Identify soft clusters that should have been merged: Select `isInPfo=0` clusters with low `nHits` (≤5) or low `nLayers` (≤3) or low `energy` (<2 GeV), and check their `minClusterDistance` to the nearest cluster. If the distance is small (<50 mm) but the cluster wasn't merged, `ClosestDistanceCut0` may need increasing.
2. If many clusters with `nHits` between 6-10 are fragments, increase `MaxHitsInSoftCluster` (but be careful not to merge genuine small clusters).
3. If soft clusters at deep layers aren't being merged, check their `startLayer` and adjust `InnerLayerCut1`/`InnerLayerCut2` or the corresponding distance cuts.
4. If photon-like soft clusters are incorrectly being kept separate, check `clus_passPhotonId` and `clus_isEm`. Increase `MinClusterEMEnergy` to exclude fewer clusters.
5. If the algorithm is over-merging (creating large merged clusters from distinct particles), reduce `MaxClusterDistanceFine`/`MaxClusterDistanceCoarse` or reduce the distance cuts.

#### 3d. IsolatedHitMerging

This algorithm operates in two parts:

**Part 1 — Redistribute small cluster hits**:
Small clusters (with ≤ **`MinHitsInCluster`** hits, default: `4`) that belong to the current input list are deleted and their constituent hits are redistributed to the nearest suitable host cluster within **`MaxRecombinationDistance`** (default: `250.0` mm). Host candidates must have more hits than the deleted cluster.

**Part 2 — Attach isolated hits**:
Remaining available isolated hits (flagged by `hit_isIsolated=1`) are attached to the nearest cluster within **`MaxRecombinationDistance`** (default: `250.0` mm).

In both parts, hits are only assigned to clusters whose direction (via `GetInitialDirection()`) satisfies **`MinCosOpeningAngle`** (default: `0.0` → no directional cut). The distance is measured from the hit position to each cluster layer centroid, and the minimum is used.

**Tuning**:

The `hit_*` and `clus_*` monitoring variables most relevant for IsolatedHitMerging:
- `hit_isIsolated`: Determines which hits are eligible in Part 2
- `hit_shortestIsolationDist` and `hit_isolationNearbyHits`: Helps understand why hits are isolated
- `hit_distToMostEnergeticClusterCentroid`: Spatial relationship to the lead cluster
- `clus_nHits`: Clusters with ≤ `MinHitsInCluster` hits are candidates for deletion in Part 1
- `clus_isInPfo`: After reconstruction, check if hits remain unclustered

**Tuning workflow for IsolatedHitMerging**:
1. Check `evt_unclusteredIsolatedEnergy` and `evt_unclusteredNonIsolatedEnergy`. High values indicate isolated hits not being merged.
2. Plot `hit_shortestIsolationDist` for unclustered hits. If many are within 250 mm of existing clusters but still unclustered, increase `MaxRecombinationDistance`.
3. If clusters with 4-5 hits are being wrongly broken up and redistributed, increase `MinHitsInCluster`.
4. If the angular preselection is rejecting good associations, plot the cosine of the opening angle between the hit and cluster directions. If values are clustered near 0, the `MinCosOpeningAngle` cut of 0.0 is fine; if negative, it may be rejecting hits. Increase `MinCosOpeningAngle` to be more permissive (e.g., -0.5).
5. Since `MinHitsInCluster` also controls Part 2 (host clusters must have more hits than the deleted cluster), increasing it helps more small clusters find hosts but risks merging distinct clusters.

### Step 4: Analyse Shower Identification with ClusterMonData

The `LCEmShowerId` plugin uses shower shape variables to classify clusters as electromagnetic or hadronic. This feeds into `isEm` and `passPhotonId`.

- **`HighRadLengths`** (default: `30.0`): Used to compute `fractionOfEnergyAboveHighRadLengths`.

**Tuning**: Plot `nRadiationLengths`, `layer90RadLengths`, `showerMaxRadLengths`, `radial90`, and `fractionOfEnergyAboveHighRadLengths` vs. `isEm` (truth from MC). For ECAL-only clusters (high fraction of hits in ECAL), the `isEm` flag should generally be `1`. Adjust `HighRadLengths` to change the high-radiation-length energy fraction threshold.

### Step 5: Validate with Event-Level Confusion Metrics

After adjusting parameters, re-run the reconstruction and examine:
- `evt_fChargedEnergyRecoCharged` should be close to 1.0
- `evt_fNeutralEnergyRecoNeutral` should be close to 1.0
- `evt_fChargedEnergyRecoNeutral` and `evt_fNeutralEnergyRecoCharged` (confusion) should be minimised
- `evt_unclusteredIsolatedEnergy` and `evt_unclusteredNonIsolatedEnergy` should be minimised

The algorithm summary at process termination also provides PFO-level statistics:
- Correct/Wrong Charged Hadron PFOs
- Correct/Wrong Neutral Hadron PFOs

## 3. Detector-Specific Considerations

### Fine vs. Coarse Granularity

Detectors with different granularity in ECAL and HCAL require different `IsolationCutDistanceFine` and `IsolationCutDistanceCoarse`:
- **Fine granularity** : Use small isolation distances (default `25` mm).
- **Coarse granularity** : Use larger isolation distances (default `200` mm).
- Adjust these parameters based on cell size.

### Number of Pseudo-Layers

The `pseudoLayer` assignments depend on the detector geometry. All layer-dependent cuts (`MinClusterInnerLayer`, `MinLayerSpan`, `MinShowerLayerSpan`, `NGenericDistanceLayers`, `IsolationNLayers`, etc.) should be checked against a detector's pseudo-layer structure. For detectors with fewer layers, reduce these numbers proportionally.

### Material Budget (Radiation Lengths)

The `LCEmShowerId` plugin relies on radiation length calculations. If a detector has a substantially different material budget:
- Check `nRadiationLengths`, `nRadiationLengthsBeforeClusterStart`, `layer90RadLengths`, and `showerMaxRadLengths` distributions.
- Adjust `HighRadLengths` to match the approximate number of radiation lengths in the ECAL.

### Magnetic Field

The helix-based fragment identification depends on the B-field value. For detectors with B-fields different from 3.5-4 T (CLIC/ILD), the `MaxClusterHelixDistance` and `MaxHelixPathlengthToDaughter` may need adjustment.


