#!/bin/bash

# 定义数据集路径变量
DATA_DIR="../UNG_data/sift/sift_0" # 将此路径替换为数据集的实际位置

# 删除 build 文件夹及其所有内容
if [ -d "build" ]; then
    echo "删除 build 文件夹及其内容..."
    rm -rf build
fi

# 下载并解压数据
# cd $DATA_DIR
# tar -zxvf sift.tar.gz
# cd ..
# cd UNG

# 创建 build 目录并编译代码
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../codes/ # Build with Release mode
make -j
cd ..


# # 转换数据格式
# ./build/tools/fvecs_to_bin --data_type float --input_file $DATA_DIR/sift/sift_base.fvecs --output_file $DATA_DIR/sift/sift_base.bin
# ./build/tools/fvecs_to_bin --data_type float --input_file $DATA_DIR/sift/sift_query.fvecs --output_file $DATA_DIR/sift/sift_query.bin

# # 生成基础标签
# ./build/tools/generate_base_labels \
#     --num_labels 12 --num_points 1000000 --distribution_type zipf \
#     --output_file $DATA_DIR/sift/sift_base_labels.txt

# # 生成查询标签
# ./build/tools/generate_query_labels \
#     --num_points 10000 --distribution_type zipf --K 10 --scenario containment \
#     --input_file $DATA_DIR/sift/sift_base_labels.txt --output_file $DATA_DIR/sift/sift_query_labels_containment.txt \
#     --output_file_r $DATA_DIR/sift/sift_query_labels_containment_r.txt \
#     --output_file_o $DATA_DIR/sift/sift_query_labels_containment_o.txt

# # 生成gt
# ./build/tools/compute_groundtruth \
#     --data_type float --dist_fn L2 --scenario containment --K 10 --num_threads 32 \
#     --base_bin_file $DATA_DIR/sift/sift_base.bin --base_label_file $DATA_DIR/sift/sift_base_labels.txt \
#     --query_bin_file $DATA_DIR/sift/sift_query.bin --query_label_file $DATA_DIR/sift/sift_query_labels_containment_r.txt \
#     --gt_file $DATA_DIR/sift/sift_gt_labels_containment.bin \
#     --nearest_nei_dist_path $DATA_DIR/nearest_nei_dist

# echo "运行cal_coverage.py..."
# python3 cal_coverage.py $DATA_DIR > output1.log

# echo "运行cal_cost.py..."
# python3 cal_cost.py $DATA_DIR > output2.log

# 构建index
./build/apps/build_UNG_index \
    --data_type float --dist_fn L2 --num_threads 32 --max_degree 32 --Lbuild 100 --alpha 1.2 \
    --base_bin_file $DATA_DIR/sift/sift_base.bin --base_label_file $DATA_DIR/sift/sift_base_labels.txt \
    --index_path_prefix $DATA_DIR/index_files/UNG/sift_base_labels_general_cross6_R32_L100_A1.2/ \
    --scenario general --num_cross_edges 6 

# 查询
## gt_mode: 0: ground truth is distance and in binary file, 1: ground truth is cost and in csv file, calculate costs, 2: ground truth is cost and in csv file, read costs

# NUM_RUNS=100

# 定义存储结果的目录
RESULT_DIR="$DATA_DIR/results"
# mkdir -p $RESULT_DIR

# 初始化recall和QPS的总和
TOTAL_RECALL=0
TOTAL_QPS=0

for ((i=1; i<=1; i++))
do
    echo "Running iteration $i..."

    # 运行搜索命令
    ./build/apps/search_UNG_index \
        --data_type float --dist_fn L2 --num_threads 16 --K 5 --gt_mode 4\
        --base_bin_file $DATA_DIR/sift/sift_base.bin \
        --base_label_file $DATA_DIR/sift/sift_base_labels.txt \
        --query_bin_file $DATA_DIR/sift/sift_query.bin \
        --query_label_file_r $DATA_DIR/sift/sift_query_labels_containment_r.txt \
        --query_label_file_o $DATA_DIR/sift/sift_query_labels_containment_o.txt \
        --gt_file $DATA_DIR/sift/sift_gt_labels_containment.bin \
        --cost_file $DATA_DIR/cost/ \
        --sort_cost_file $DATA_DIR/sort_cost/ \
        --dist_file $DATA_DIR/nearest_nei_dist/ \
        --sort_dist_file $DATA_DIR/sort_dist/ \
        --index_path_prefix $DATA_DIR/index_files/UNG/sift_base_labels_general_cross6_R32_L100_A1.2/ \
        --result_path_prefix $RESULT_DIR/results_dist/run_$i/ \
        --scenario containment --num_entry_points 16 --Lsearch 5 6 7 8 9 10 20 30 50 300 500 1000

    echo "Iteration $i: Recall = $RECALL, QPS = $QPS"
done

# 计算平均值
# python3 cal_recall_qps_average.py
