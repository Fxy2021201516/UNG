import os
import pandas as pd
import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed

def normalize(value, min_val, max_val):
    dis = max(min_val, min(value, max_val))
    return (dis - min_val) / (max_val - min_val)

def process_query(query_id, coverage_folder, nearest_nei_folder, output_folder):
    try:
        # 读取对应的coverage_result_x.txt文件
        coverage_file = os.path.join(coverage_folder, f'coverage_result_{query_id}.txt')
        
        with open(coverage_file, 'r') as f:
            coverage = [float(line.strip()) for line in f.readlines()]
        
        # 读取对应的nearest_nei_dis_x.csv文件
        nearest_nei_file = os.path.join(nearest_nei_folder, f'{query_id}_nearest_nei_dis.csv')
        nearest_nei_df = pd.read_csv(nearest_nei_file)
        
        distance_min = 0.0
        distance_max = 1e6
        coverage_min = 0.0
        coverage_max = 1.0
        
        cost_results = []
        vector_ids = [] 
        
        for _, row in nearest_nei_df.iterrows():
            nearest_neighbor = int(row['Nearest Neighbor'])
            distance = row['Distance']
            
            coverage_value = coverage[nearest_neighbor]
    
            normalized_distance = normalize(distance, distance_min, distance_max)
            normalized_coverage = normalize(coverage_value, coverage_min, coverage_max)
            
            cost = alpha * normalized_distance - (1 - alpha) * normalized_coverage
            
            vector_ids.append(nearest_neighbor)
            cost_results.append(cost)
        
        os.makedirs(output_folder, exist_ok=True)
        output_file = os.path.join(output_folder, f'cost_result_{query_id}.csv')
        
        df = pd.DataFrame({
            'Vector ID': vector_ids,
            'Cost': cost_results
        })
        
        df.to_csv(output_file, index=False)
        
        return query_id
    except Exception as e:
        print(f"Query {query_id} failed due to error: {e}")
        return None

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calculate costs based on coverage and nearest neighbor distances.")
    parser.add_argument("words_data_dir", type=str, help="Directory containing the dataset folders")
    args = parser.parse_args()
    
    SIFT_DATA_DIR = args.words_data_dir
    
    coverage_folder = os.path.join(SIFT_DATA_DIR, 'coverage')
    nearest_nei_folder = os.path.join(SIFT_DATA_DIR, 'nearest_nei_dist')
    output_folder = os.path.join(SIFT_DATA_DIR, 'cost')

    alpha = 0.5  # 设置alpha值
    
    # 确保输出目录存在
    os.makedirs(output_folder, exist_ok=True)
    
    with ProcessPoolExecutor() as executor:
        futures = {executor.submit(process_query, query_id, coverage_folder, nearest_nei_folder, output_folder): query_id for query_id in range(1000)}
        for future in as_completed(futures):
            query_id = futures[future]
            try:
                result = future.result()
                if result is not None:
                    print(f"Query {result} cost calculation completed.")
            except Exception as exc:
                print(f'Query {query_id} generated an exception: {exc}')