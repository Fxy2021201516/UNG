#!/bin/bash

# 删除 build 文件夹及其所有内容
if [ -d "build" ]; then
    echo "删除 build 文件夹及其内容..."
    rm -rf build
fi

# 清理 data 文件夹，保留 sift.tar.gz
# if [ -d "data" ]; then
#     echo "清理 data 文件夹，保留 sift.tar.gz..."
#     cd data
#     # 删除除 sift.tar.gz 之外的所有文件和文件夹
#     find . -mindepth 1 ! -name 'sift.tar.gz' -exec rm -rf {} +
#     cd ..
# fi

# 创建 build 目录并编译代码
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../codes/ # Build with Release mode
make -j
cd ..

# 下载并解压数据
# cd ./data
# tar -zxvf sift.tar.gz
# cd ..

# 转换数据格式
# ./build/tools/fvecs_to_bin --data_type float --input_file ./data/sift/sift_base.fvecs --output_file ./data/sift/sift_base.bin
# ./build/tools/fvecs_to_bin --data_type float --input_file ./data/sift/sift_query.fvecs --output_file ./data/sift/sift_query.bin

# 生成基础标签
# ./build/tools/generate_base_labels \
#     --num_labels 12 --num_points 1000000 --distribution_type zipf \
#     --output_file ./data/sift/sift_base_12_labels_zipf.txt

# 生成查询标签
# ./build/tools/generate_query_labels \
#     --num_points 10000 --distribution_type zipf --K 10 --scenario containment \
#     --input_file ./data/sift/sift_base_12_labels_zipf.txt --output_file ./data/sift/sift_query_12_labels_zipf_containment.txt \
#     --output_file_r ./data/sift/sift_query_12_labels_zipf_containment_r.txt \
#     --output_file_o ./data/sift/sift_query_12_labels_zipf_containment_o.txt

# 生成gt
# ./build/tools/compute_groundtruth \
#     --data_type float --dist_fn L2 --scenario containment --K 10 --num_threads 32 \
#     --base_bin_file ./data/sift/sift_base.bin --base_label_file ./data/sift/sift_base_12_labels_zipf.txt \
#     --query_bin_file ./data/sift/sift_query.bin --query_label_file ./data/sift/sift_query_12_labels_zipf_containment_r.txt \
#     --gt_file ./data/sift/sift_gt_12_labels_zipf_containment.bin


# 构建index
# ./build/apps/build_UNG_index \
#     --data_type float --dist_fn L2 --num_threads 32 --max_degree 32 --Lbuild 100 --alpha 1.2 \
#     --base_bin_file ./data/sift/sift_base.bin --base_label_file ./data/sift/sift_base_12_labels_zipf.txt \
#     --index_path_prefix ./data/index_files/UNG/sift_base_12_labels_zipf_general_cross6_R32_L100_A1.2/ \
#     --scenario general --num_cross_edges 6

# 查询
./build/apps/search_UNG_index \
    --data_type float --dist_fn L2 --num_threads 16 --K 10 \
    --base_bin_file ./data/sift/sift_base.bin --base_label_file ./data/sift/sift_base_12_labels_zipf.txt \
    --query_bin_file ./data/sift/sift_query.bin \
    --query_label_file_r ./data/sift/sift_query_12_labels_zipf_containment_r.txt \
    --query_label_file_o ./data/sift/sift_query_12_labels_zipf_containment_o.txt \
    --gt_file ./data/sift/sift_gt_12_labels_zipf_containment.bin \
    --index_path_prefix ./data/index_files/UNG/sift_base_12_labels_zipf_general_cross6_R32_L100_A1.2/ \
    --result_path_prefix ./results/UNG/sift_base_12_labels_zipf_containment_cross6_R32_L100_A1.2/ \
    --scenario containment --num_entry_points 16 --Lsearch 10 50 300 500 1000 1200 3000