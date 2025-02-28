import os
from concurrent.futures import ProcessPoolExecutor, as_completed
import argparse

def read_queries(file_path):
    """从给定路径读取查询文件，并返回包含int列表的列表"""
    with open(file_path, 'r') as file:
        queries = [list(map(int, line.strip().split(','))) for line in file.readlines()]
    return queries

def read_vectors(file_path):
    """从给定路径读取向量文件，并返回包含int列表的列表"""
    with open(file_path, 'r') as file:
        vectors = [list(map(int, line.strip().split(','))) for line in file.readlines()]
    return vectors

def calculate_coverage(query, vector):
    """计算单个查询和向量之间的属性覆盖率"""
    query_set = set(query)
    vector_set = set(vector)
    intersection = query_set.intersection(vector_set)
    if len(query_set) == 0:
        return 0.0
    return len(intersection) / len(query_set)

def process_query(query, vectors, output_dir, query_index):
    """处理单个查询并保存结果"""
    output_file_path = os.path.join(output_dir, f"coverage_result_{query_index}.txt")
    with open(output_file_path, 'w') as output_file:
        for vector in vectors:
            coverage = calculate_coverage(query, vector)
            output_file.write(f"{coverage}\n")
    return query_index


def main(sift_data_dir):
    # 文件路径，基于 sift_data_dir 构建
    query_file_path = os.path.join(sift_data_dir, "sift/sift_query_12_labels_zipf_containment_o.txt")
    vector_file_path = os.path.join(sift_data_dir, "sift/sift_base_12_labels_zipf.txt")
    output_dir = os.path.join(sift_data_dir, "coverage")
    
    # 如果输出目录不存在，则创建它
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 读取数据
    queries = read_queries(query_file_path)
    vectors = read_vectors(vector_file_path)
    
    # 使用 ProcessPoolExecutor 并行处理查询
    with ProcessPoolExecutor() as executor:
        futures = [executor.submit(process_query, query, vectors, output_dir, i) for i, query in enumerate(queries)]
        
        for future in as_completed(futures):
            query_index = future.result()
            print(f"已完成查询 {query_index} 的处理")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process queries based on provided dataset directory.")
    parser.add_argument("sift_data_dir", type=str, help="Directory containing the sift dataset folders")
    args = parser.parse_args()

    main(args.sift_data_dir)