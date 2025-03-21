import matplotlib
matplotlib.use('Agg')  # 在导入pyplot之前设置后端
import matplotlib.pyplot as plt
import pandas as pd
# 读取csv文件
df = pd.read_csv('/home/fengxiaoyao/UNG_data/words/words_0/results/average_results.csv')  

# 确认列名与CSV文件中的列名相匹配
qps = df['QPS'].to_numpy()  # 或者使用 df['QPS'].values 如果使用的是较旧版本的pandas
recall = df['Recall'].to_numpy()  # 或者使用 df['Recall'].values 如果使用的是较旧版本的pandas

# 创建图表
plt.figure(figsize=(10, 6))
plt.plot(qps, recall, marker='o')
plt.title('Recall vs QPS')
plt.xlabel('QPS')
plt.ylabel('Recall')
plt.grid(True)
# 保存图表到文件
plt.savefig('output.png')