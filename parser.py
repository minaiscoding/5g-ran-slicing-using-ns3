import pandas as pd
import numpy as np
from pathlib import Path
import re

class NS3TraceParser:
    def __init__(self, trace_dir):
        """Initialize parser with directory containing trace files"""
        self.trace_dir = Path(trace_dir).expanduser()
        self.data = {}
    
    def parse_file(self, filename, skiprows=0, has_comment_header=False):
        """Parse trace file and return dataframe"""
        filepath = self.trace_dir / filename
        
        try:
            if has_comment_header:
                with open(filepath, 'r') as f:
                    lines = f.readlines()
                
                header_line = None
                data_start = 0
                
                for i, line in enumerate(lines):
                    if line.startswith('%'):
                        header_line = line[1:].strip()
                        data_start = i + 1
                        break
                
                if header_line:
                    delimiter = '\t' if '\t' in header_line else r'\s+'
                    from io import StringIO
                    data_content = ''.join(lines[data_start:])
                    df = pd.read_csv(StringIO(header_line + '\n' + data_content), 
                                   sep=delimiter, engine='python')
                else:
                    df = pd.read_csv(filepath, sep=r'\s+', skiprows=skiprows)
            else:
                df = pd.read_csv(filepath, sep=r'\s+', skiprows=skiprows, comment='%')
            
            print(f"Loaded {filename}: {len(df)} rows")
            return df
            
        except Exception as e:
            print(f"Error: {filename}: {e}")
            return None
    
    def parse_all_traces(self):
        """Parse all trace files"""
        print("Parsing trace files...")
        
        # DL Control SINR
        self.data['dl_ctrl_sinr'] = self.parse_file('DlCtrlSinr.txt')
        
        # DL Data SINR
        self.data['dl_data_sinr'] = self.parse_file('DlDataSinr.txt')
        
        # DL Pathloss
        self.data['dl_pathloss'] = self.parse_file('DlPathlossTrace.txt')
        
        # DL MAC Stats
        self.data['dl_mac'] = self.parse_file('NrDlMacStats.txt', has_comment_header=True)
        
        # DL PDCP Stats
        self.data['dl_pdcp_rx'] = self.parse_file('NrDlPdcpRxStats.txt')
        self.data['dl_pdcp_tx'] = self.parse_file('NrDlPdcpTxStats.txt')
        
        # DL RLC Stats
        self.data['dl_rlc_rx'] = self.parse_file('NrDlRxRlcStats.txt')
        self.data['dl_rlc_tx'] = self.parse_file('NrDlTxRlcStats.txt')
        
        # UL MAC Stats
        self.data['ul_mac'] = self.parse_file('NrUlMacStats.txt', has_comment_header=True)
        
        # UL PDCP Stats
        self.data['ul_pdcp_rx'] = self.parse_file('NrUlPdcpRxStats.txt')
        self.data['ul_pdcp_tx'] = self.parse_file('NrUlPdcpTxStats.txt')
        
        # UL RLC Stats
        self.data['ul_rlc_rx'] = self.parse_file('NrUlRlcRxStats.txt')
        self.data['ul_rlc_tx'] = self.parse_file('NrUlRlcTxStats.txt')
        
        # UL Pathloss
        self.data['ul_pathloss'] = self.parse_file('UlPathlossTrace.txt')
        
        # RX Packet Trace
        self.data['rx_packet'] = self.parse_file('RxPacketTrace.txt')
        
        print(f"Parsed {len([k for k, v in self.data.items() if v is not None])} files")
    
    def create_unified_dataset(self, time_resolution=0.001):
        """
        Create unified time-series dataset with forward-fill for non-instantaneous values
        
        Args:
            time_resolution: Time step in seconds (default 1ms)
        """
        print("\nCreating unified dataset...")
        
        # Collect all unique timestamps and round them to the grid
        all_times = set()
        
        for name, df in self.data.items():
            if df is not None and not df.empty:
                time_col = self._get_time_column(df)
                if time_col:
                    print(f"  {name}: time column = '{time_col}'")
                    rounded_times = (np.round(df[time_col].values / time_resolution) * time_resolution)
                    all_times.update(rounded_times)
                else:
                    print(f"  {name}: NO TIME COLUMN FOUND (columns: {list(df.columns)})")
        
        # Create time index from rounded values
        if not all_times:
            print("No time data found!")
            return None
        
        min_time = min(all_times)
        max_time = max(all_times)
        
        # Create regular time grid
        time_index = np.arange(min_time, max_time + time_resolution, time_resolution)
        
        print(f"Time range: {min_time:.6f}s to {max_time:.6f}s")
        print(f"Time steps: {len(time_index)}")
        
        # Initialize result dataframe
        result = pd.DataFrame({'time': time_index})
        
        # Process each trace file (now with rounded timestamps)
        result = self._merge_dl_sinr(result, time_resolution)
        result = self._merge_pathloss(result, time_resolution)
        result = self._merge_mac_stats(result, time_resolution)
        result = self._merge_pdcp_stats(result, time_resolution)
        result = self._merge_rlc_stats(result, time_resolution)
        result = self._merge_rx_packet(result, time_resolution)
        
        # Forward fill for non-instantaneous values
        print("\nForward-filling non-instantaneous values...")
        result = result.ffill()
        
        # Fill remaining NaN with 0 (for beginning of trace)
        result = result.fillna(0)
        
        return result
    
    def _get_time_column(self, df):
        """Identify time column in dataframe"""
        possible_names = ['Time', 'time', 'time(s)', 'Time(sec)']
        for col in df.columns:
            if col in possible_names or 'time' in col.lower():
                return col
        return None
    
    def _merge_dl_sinr(self, result, time_resolution):
        """Merge DL SINR data"""
        if self.data['dl_data_sinr'] is not None:
            df = self.data['dl_data_sinr'].copy()
            df['time'] = np.round(df['Time'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'SINR(dB)': ['mean', 'std', 'min', 'max'],
                'CellId': 'first',
                'RNTI': 'first'
            }).reset_index()
            
            agg_df.columns = ['time', 'dl_sinr_mean', 'dl_sinr_std', 
                            'dl_sinr_min', 'dl_sinr_max', 'cellid', 'rnti']
            
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def _merge_pathloss(self, result, time_resolution):
        """Merge pathloss data"""
        # DL Pathloss
        if self.data['dl_pathloss'] is not None:
            df = self.data['dl_pathloss'].copy()
            df['time'] = np.round(df['Time(sec)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'pathLoss(dB)': 'mean',
                'CellId': 'first'
            }).reset_index()
            
            agg_df = agg_df.rename(columns={'pathLoss(dB)': 'dl_pathloss'})
            result = result.merge(agg_df[['time', 'dl_pathloss']], on='time', how='left')
        
        # UL Pathloss
        if self.data['ul_pathloss'] is not None:
            df = self.data['ul_pathloss'].copy()
            df['time'] = np.round(df['Time(sec)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'pathLoss(dB)': 'mean'
            }).reset_index()
            
            agg_df = agg_df.rename(columns={'pathLoss(dB)': 'ul_pathloss'})
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def _merge_mac_stats(self, result, time_resolution):
        """Merge MAC layer statistics"""
        # DL MAC
        if self.data['dl_mac'] is not None:
            df = self.data['dl_mac'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'tbSize': ['sum', 'mean'],
                'mcs': 'mean',
                'harqId': 'count'
            }).reset_index()
            
            agg_df.columns = ['time', 'dl_tb_size_total', 'dl_tb_size_mean', 
                            'dl_mcs_mean', 'dl_mac_transmissions']
            
            result = result.merge(agg_df, on='time', how='left')
        
        # UL MAC
        if self.data['ul_mac'] is not None:
            df = self.data['ul_mac'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'tbSize': ['sum', 'mean'],
                'mcs': 'mean',
                'harqId': 'count'
            }).reset_index()
            
            agg_df.columns = ['time', 'ul_tb_size_total', 'ul_tb_size_mean', 
                            'ul_mcs_mean', 'ul_mac_transmissions']
            
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def _merge_pdcp_stats(self, result, time_resolution):
        """Merge PDCP layer statistics"""
        # DL PDCP
        if self.data['dl_pdcp_rx'] is not None:
            df = self.data['dl_pdcp_rx'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'packetSize': ['sum', 'mean', 'count'],
                'delay(s)': 'mean'
            }).reset_index()
            
            agg_df.columns = ['time', 'dl_pdcp_bytes', 'dl_pdcp_pkt_size_mean', 
                            'dl_pdcp_packets', 'dl_pdcp_delay_mean']
            
            result = result.merge(agg_df, on='time', how='left')
        
        # UL PDCP
        if self.data['ul_pdcp_rx'] is not None:
            df = self.data['ul_pdcp_rx'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'packetSize': ['sum', 'mean', 'count'],
                'delay(s)': 'mean'
            }).reset_index()
            
            agg_df.columns = ['time', 'ul_pdcp_bytes', 'ul_pdcp_pkt_size_mean', 
                            'ul_pdcp_packets', 'ul_pdcp_delay_mean']
            
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def _merge_rlc_stats(self, result, time_resolution):
        """Merge RLC layer statistics"""
        # DL RLC
        if self.data['dl_rlc_rx'] is not None:
            df = self.data['dl_rlc_rx'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'packetSize': ['sum', 'mean', 'count'],
                'delay(s)': 'mean'
            }).reset_index()
            
            agg_df.columns = ['time', 'dl_rlc_bytes', 'dl_rlc_pkt_size_mean', 
                            'dl_rlc_packets', 'dl_rlc_delay_mean']
            
            result = result.merge(agg_df, on='time', how='left')
        
        # UL RLC
        if self.data['ul_rlc_rx'] is not None:
            df = self.data['ul_rlc_rx'].copy()
            df['time'] = np.round(df['time(s)'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'packetSize': ['sum', 'mean', 'count'],
                'delay(s)': 'mean'
            }).reset_index()
            
            agg_df.columns = ['time', 'ul_rlc_bytes', 'ul_rlc_pkt_size_mean', 
                            'ul_rlc_packets', 'ul_rlc_delay_mean']
            
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def _merge_rx_packet(self, result, time_resolution):
        """Merge RX packet trace"""
        if self.data['rx_packet'] is not None:
            df = self.data['rx_packet'].copy()
            df['time'] = np.round(df['Time'].values / time_resolution) * time_resolution
            
            agg_df = df.groupby('time').agg({
                'SINR(dB)': 'mean',
                'CQI': 'mean',
                'corrupt': 'sum',
                'TBler': 'mean',
                'tbSize': 'sum'
            }).reset_index()
            
            agg_df = agg_df.rename(columns={
                'SINR(dB)': 'rx_sinr_mean',
                'CQI': 'rx_cqi_mean',
                'corrupt': 'rx_corrupt_count',
                'TBler': 'rx_bler_mean',
                'tbSize': 'rx_tb_size_total'
            })
            
            result = result.merge(agg_df, on='time', how='left')
        
        return result
    
    def calculate_instantaneous_metrics(self, df):
        """
        Calculate instantaneous throughput, delay, and jitter at each timestep (1ms)
        
        Args:
            df: Dataset dataframe
        """
        print("\nCalculating instantaneous metrics...")
        
        # DL instantaneous throughput (Mbps at each 1ms timestep)
        if 'dl_pdcp_bytes' in df.columns:
            df['dl_throughput_mbps'] = (df['dl_pdcp_bytes'] * 8) / (0.001 * 1e6)  # Convert bytes to Mbps
        
        # UL instantaneous throughput
        if 'ul_pdcp_bytes' in df.columns:
            df['ul_throughput_mbps'] = (df['ul_pdcp_bytes'] * 8) / (0.001 * 1e6)
        
        # DL delay (already calculated as mean delay per timestep in PDCP stats)
        if 'dl_pdcp_delay_mean' in df.columns:
            df['dl_delay_ms'] = df['dl_pdcp_delay_mean'] * 1000  # Convert to ms
        
        # UL delay
        if 'ul_pdcp_delay_mean' in df.columns:
            df['ul_delay_ms'] = df['ul_pdcp_delay_mean'] * 1000
        
        # Calculate jitter as the absolute difference in delay between consecutive timesteps
        if 'dl_delay_ms' in df.columns:
            df['dl_jitter_ms'] = df['dl_delay_ms'].diff().abs()
            df['dl_jitter_ms'] = df['dl_jitter_ms'].fillna(0)  # First value has no previous reference
        
        if 'ul_delay_ms' in df.columns:
            df['ul_jitter_ms'] = df['ul_delay_ms'].diff().abs()
            df['ul_jitter_ms'] = df['ul_jitter_ms'].fillna(0)
        
        
        return df
    
    def save_dataset(self, df, output_path='ns3_training_data.csv'):
        """Save dataset to CSV"""
        df.to_csv(output_path, index=False)
        print(f"\n{'='*70}")
        print(f"Dataset saved to: {output_path}")
        print(f"{'='*70}")
        print(f"Shape: {df.shape[0]} rows × {df.shape[1]} columns")
        
        print(f"\nTime coverage:")
        print(f"  Start: {df['time'].min():.6f}s")
        print(f"  End: {df['time'].max():.6f}s")
        print(f"  Duration: {df['time'].max() - df['time'].min():.6f}s")
        
        print(f"\nAll columns ({len(df.columns)}):")
        for i, col in enumerate(df.columns, 1):
            non_zero = (df[col] != 0).sum()
            print(f"  {i:2d}. {col:40s} (non-zero: {non_zero:5d}/{len(df)})")
        
        print(f"\nFirst few rows:")
        print(df.head(10))
        
        print(f"\nSample statistics:")
        print(df.describe())


if __name__ == "__main__":
    # Initialize parser
    parser = NS3TraceParser("~/ns-3-dev")
    
    # Parse all trace files
    parser.parse_all_traces()
    
    # Create unified dataset (1ms resolution)
    dataset = parser.create_unified_dataset(time_resolution=0.001)
    
    if dataset is not None:
        # Calculate instantaneous metrics (throughput, delay, jitter)
        dataset = parser.calculate_instantaneous_metrics(dataset)
        
        # Save to CSV
        parser.save_dataset(dataset, 'ns3_drl_training_data.csv')
        