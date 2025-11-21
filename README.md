# NS-3 NR Network Slicing Simulation

This repository contains an NS-3 simulation framework for 5G NR network slicing with support for multiple traffic types (URLLC, eMBB, mMTC) and dataset generation for machine learning applications.

## Overview

The simulation generates realistic 5G NR network traces across multiple slices with configurable parameters. The parser extracts and processes these traces into a unified time-series dataset suitable for Deep Reinforcement Learning (DRL) training.

## Repository Structure

```
.
├── config.txt              # Simulation configuration parameters
├── nr-multi-slice-sim.cc              # NS-3 simulation scenario (place in ns-3-dev/scratch/)
├── parser.py               # Trace parser and dataset generator
└── README.md
```

## Files Description

### `config.txt`

Configuration file for the NS-3 simulation. Parameters include:

- **Network Topology**
  - `gNbNum`: Number of gNodeBs (base stations)
  - `ueNum`: Number of User Equipment (UEs)
- **Traffic Slicing**
  - `trafficTypes`: Comma-separated list of traffic types (urllc, embb, mmtc)
  - `prbUrllc`, `prbEmbb`, `prbMmtc`: Physical Resource Blocks allocated per slice
- **Radio Configuration**
  - `centralFrequencyBand1`, `centralFrequencyBand2`: Carrier frequencies (Hz)
  - `referenceNumerology`: Numerology index (0=15kHz, 1=30kHz, 2=60kHz, etc.)
- **Timing**
  - `simTimeMs`: Total simulation time in milliseconds
  - `udpAppStartTimeMs`: Application start time in milliseconds

### `nr-multi-slice-sim.cc`

Main NS-3 simulation file implementing the 5G NR network slicing scenario.

**Features:**

- Multi-slice configuration with dedicated PRB allocation
- Support for URLLC, eMBB, and mMTC traffic profiles
- Generates comprehensive trace files for MAC, RLC, PDCP, and PHY layers
- Configurable through `config.txt`

### `parser.py`

Python-based trace parser that processes NS-3 output files into a unified dataset.

**Capabilities:**

- Parses 15+ trace files (SINR, pathloss, MAC/RLC/PDCP stats, etc.)
- Creates time-aligned dataset with 1ms resolution
- Computes instantaneous metrics:
  - Throughput (Mbps)
  - Delay (ms)
  - Jitter (ms)
- Outputs CSV file ready for ML training

## Prerequisites

- **NS-3** (version 3.x with NR module)
- **Python 3.7+**
- **Python packages:**
  ```bash
  pip install pandas numpy
  ```

## Usage

### 1. Setup Simulation

Copy the simulation file to NS-3 scratch directory:

```bash
cp nr-multi-slice-sim.cc ~/ns-3-dev/scratch/
cp config.txt ~/ns-3-dev/
```

### 2. Configure Parameters

Edit `config.txt` to adjust simulation parameters:

```bash
nano config.txt
```

### 3. Run Simulation

```bash
cd ~/ns-3-dev
./ns3 run scratch/slicing
```

The simulation will generate trace files in `~/ns-3-dev/`:

- `DlCtrlSinr.txt`, `DlDataSinr.txt`
- `DlPathlossTrace.txt`, `UlPathlossTrace.txt`
- `NrDlMacStats.txt`, `NrUlMacStats.txt`
- `NrDlPdcpRxStats.txt`, `NrDlPdcpTxStats.txt`
- `NrDlRxRlcStats.txt`, `NrDlTxRlcStats.txt`
- `NrUlPdcpRxStats.txt`, `NrUlPdcpTxStats.txt`
- `NrUlRlcRxStats.txt`, `NrUlRlcTxStats.txt`
- `RxPacketTrace.txt`

### 4. Generate Dataset

Run the parser to create the unified dataset:

```bash
python parser.py
```

**Output:** `ns3_drl_training_data.csv`

## Dataset Structure

The generated dataset contains time-series data with 1ms resolution:

| Column                                                      | Description                  |
| ----------------------------------------------------------- | ---------------------------- |
| `time`                                                      | Timestamp (seconds)          |
| `dl_sinr_mean`, `dl_sinr_std`, `dl_sinr_min`, `dl_sinr_max` | Downlink SINR statistics     |
| `dl_pathloss`, `ul_pathloss`                                | Path loss (dB)               |
| `dl_throughput_mbps`, `ul_throughput_mbps`                  | Instantaneous throughput     |
| `dl_delay_ms`, `ul_delay_ms`                                | Packet delay                 |
| `dl_jitter_ms`, `ul_jitter_ms`                              | Delay variation              |
| `dl_mcs_mean`, `ul_mcs_mean`                                | Modulation and Coding Scheme |
| `dl_pdcp_packets`, `ul_pdcp_packets`                        | Packet counts                |
| `rx_sinr_mean`, `rx_cqi_mean`, `rx_bler_mean`               | Reception statistics         |

**Total columns:** ~40-50 depending on trace availability

## Customization

### Adding New Traffic Types

1. Update `config.txt` with new traffic type
2. Modify `nr-multi-slice-sim.cc` to implement traffic pattern
3. Re-run simulation and parser

### Changing Time Resolution

Edit `parser.py`:

```python
dataset = parser.create_unified_dataset(time_resolution=0.001)  # 1ms (default)
```

For 10ms resolution, use `time_resolution=0.010`

### Custom Metrics

Add computation in `parser.py` under `calculate_instantaneous_metrics()`:

```python
def calculate_instantaneous_metrics(self, df):
    # Your custom metric
    df['custom_metric'] = df['dl_throughput_mbps'] / df['dl_delay_ms']
    return df
```

## Example Configuration

**Low-latency URLLC scenario:**

```
gNbNum=2
ueNum=10
trafficTypes=urllc
prbUrllc=100
centralFrequencyBand1=28e9
referenceNumerology=2
simTimeMs=2000
udpAppStartTimeMs=500
```

**Mixed traffic scenario:**

```
gNbNum=4
ueNum=20
trafficTypes=urllc,embb,mmtc
prbUrllc=30
prbEmbb=100
prbMmtc=20
centralFrequencyBand1=28e9
centralFrequencyBand2=28.2e9
referenceNumerology=1
simTimeMs=5000
udpAppStartTimeMs=1000
```
