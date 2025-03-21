import os
import pandas as pd

# 定义results文件夹路径
results_dir = '/home/fengxiaoyao/UNG_data/words/words_0/results/results_dist'

# 初始化一个空的DataFrame来存储所有结果
all_results = pd.DataFrame()

# 遍历每个run文件夹
for run in range(1, 11):
    run_dir = os.path.join(results_dir, f'run_{run}')
    csv_path = os.path.join(run_dir, 'result_or_cost.csv')
    
    # 读取CSV文件
    df = pd.read_csv(csv_path)
    
    # 将当前run的结果添加到all_results中
    all_results = pd.concat([all_results, df], ignore_index=True)

# 按L列分组，并计算QPS和Recall的平均值
average_results = all_results.groupby('L').agg({'QPS': 'mean', 'Recall': 'mean'}).reset_index()

# 输出结果到新的CSV文件
output_csv_path = os.path.join(results_dir, 'average_results.csv')
average_results.to_csv(output_csv_path, index=False)

print(f"平均值已计算并保存到 {output_csv_path}")