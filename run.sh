#!/bin/bash

# 定义数据集路径变量
SIFT_DATA_DIR="../UNG_data_sift1m" # 将此路径替换为你的sift数据集的实际位置

# 删除 build 文件夹及其所有内容
if [ -d "build" ]; then
    echo "删除 build 文件夹及其内容..."
    rm -rf build
fi

# # 下载并解压数据
# cd $SIFT_DATA_DIR
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
# ./build/tools/fvecs_to_bin --data_type float --input_file $SIFT_DATA_DIR/sift/sift_base.fvecs --output_file $SIFT_DATA_DIR/sift/sift_base.bin
# ./build/tools/fvecs_to_bin --data_type float --input_file $SIFT_DATA_DIR/sift/sift_query.fvecs --output_file $SIFT_DATA_DIR/sift/sift_query.bin

# # 生成基础标签
# ./build/tools/generate_base_labels \
#     --num_labels 12 --num_points 1000000 --distribution_type zipf \
#     --output_file $SIFT_DATA_DIR/sift/sift_base_12_labels_zipf.txt

# # 生成查询标签
# ./build/tools/generate_query_labels \
#     --num_points 10000 --distribution_type zipf --K 10 --scenario containment \
#     --input_file $SIFT_DATA_DIR/sift/sift_base_12_labels_zipf.txt --output_file $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment.txt \
#     --output_file_r $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment_r.txt \
#     --output_file_o $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment_o.txt

# # 生成gt
# ./build/tools/compute_groundtruth \
#     --data_type float --dist_fn L2 --scenario containment --K 10 --num_threads 32 \
#     --base_bin_file $SIFT_DATA_DIR/sift/sift_base.bin --base_label_file $SIFT_DATA_DIR/sift/sift_base_12_labels_zipf.txt \
#     --query_bin_file $SIFT_DATA_DIR/sift/sift_query.bin --query_label_file $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment_r.txt \
#     --gt_file $SIFT_DATA_DIR/sift/sift_gt_12_labels_zipf_containment.bin \
#     --nearest_nei_dist_path $SIFT_DATA_DIR/nearest_nei_dist

# echo "运行cal_coverage.py..."
# python3 cal_coverage.py $SIFT_DATA_DIR > output1.log

# echo "运行cal_cost.py..."
# python3 cal_cost.py $SIFT_DATA_DIR > output2.log

# 构建index
# ./build/apps/build_UNG_index \
#     --data_type float --dist_fn L2 --num_threads 32 --max_degree 32 --Lbuild 100 --alpha 1.2 \
#     --base_bin_file $SIFT_DATA_DIR/sift/sift_base.bin --base_label_file $SIFT_DATA_DIR/sift/sift_base_12_labels_zipf.txt \
#     --index_path_prefix $SIFT_DATA_DIR/index_files/UNG/sift_base_12_labels_zipf_general_cross6_R32_L100_A1.2/ \
#     --scenario general --num_cross_edges 6 

# 查询
## gt_mode: 0: ground truth is distance and in binary file, 1: ground truth is cost and in csv file, calculate costs, 2: ground truth is cost and in csv file, read costs
./build/apps/search_UNG_index \
    --data_type float --dist_fn L2 --num_threads 16 --K 10 --gt_mode 2\
    --base_bin_file $SIFT_DATA_DIR/sift/sift_base.bin --base_label_file $SIFT_DATA_DIR/sift/sift_base_12_labels_zipf.txt \
    --query_bin_file $SIFT_DATA_DIR/sift/sift_query.bin \
    --query_label_file_r $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment_r.txt \
    --query_label_file_o $SIFT_DATA_DIR/sift/sift_query_12_labels_zipf_containment_o.txt \
    --gt_file $SIFT_DATA_DIR/sift/sift_gt_12_labels_zipf_containment.bin \
    --cost_file $SIFT_DATA_DIR/cost/ \
    --sort_cost_file $SIFT_DATA_DIR/sort_cost/ \
    --index_path_prefix $SIFT_DATA_DIR/index_files/UNG/sift_base_12_labels_zipf_general_cross6_R32_L100_A1.2/ \
    --result_path_prefix ./results/UNG/sift_base_12_labels_zipf_containment_cross6_R32_L100_A1.2/ \
    --scenario containment --num_entry_points 16 --Lsearch 10 50 300 500 1000 1200 3000 4000 5000 