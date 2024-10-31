import multiprocessing as mp
import csv
import ROOT
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
plt.style.use('classic')

CHUNKSIZE = 10000
nCh = 4
nADC = 240


def get_event_info(ADC):
    Pedestal = np.mean(ADC[:, 1:21], axis=1)  # 1부터 20까지의 평균
    max_values = np.max(ADC, axis=1)
    max_indices = np.argmax(ADC, axis=1) 
    Peak = max_values - Pedestal  

    return Peak, Pedestal, max_indices 

# 청크 단위로 데이터를 처리하는 함수
def process_chunk(dff, ts, tot):
    n_evt_used = 0
    pair = []
    
    for ievt in dff['readout'].unique():
        df_event = dff.loc[(dff['readout'] == ievt)]
        if df_event.empty:
            continue
        
        n_evt_used += 1
        df_col = df_event[df_event['isCol'] == True]
        df_row = df_event[df_event['isCol'] == False]

        timestamp_diff = ts
        tot_time_limit = tot

        for indc in df_col.index:
            for indr in df_row.index:
                if df_col['tot_us'][indc] == 0 or df_row['tot_us'][indr] == 0:
                    continue
                if (abs(df_col['timestamp'][indc] - df_row['timestamp'][indr]) < timestamp_diff) & \
                   (abs(df_col['tot_us'][indc] - df_row['tot_us'][indr]) / df_col['tot_us'][indc] * 100 < tot_time_limit):
                    
                    average_tot = (df_col['tot_us'][indc] + df_row['tot_us'][indr]) / 2
                    pair.append([df_col['location'][indc], df_row['location'][indr], 
                                 df_col['timestamp'][indc], df_row['timestamp'][indr], 
                                 df_col['tot_us'][indc], df_row['tot_us'][indr], average_tot])
    
    return pair

# 병렬 처리 메인 함수
def parallel_process(f, ts, tot):
    cpu_count = mp.cpu_count()
    pool = mp.Pool(cpu_count)
    results = []
    
    # 헤더를 먼저 읽어 저장
    df_header = pd.read_csv(f, sep='\t', nrows=1)
    #print( df_header.columns)
    
    # 첫 번째 청크는 헤더 포함, 이후 청크는 헤더 없이 처리
    first_chunk = True
    for chunk in pd.read_csv(f, sep='\t', chunksize=CHUNKSIZE):
        if first_chunk:
            result = pool.apply_async(process_chunk, args=(chunk, ts, tot))
            first_chunk = False
        else:
            # 헤더를 없애고 청크를 읽기
            chunk.columns = df_header.columns
            result = pool.apply_async(process_chunk, args=(chunk, ts, tot))
        
        results.append(result)

    pool.close()
    pool.join()

    final_pairs = []
    for result in results:
        final_pairs.extend(result.get())

    return final_pairs

