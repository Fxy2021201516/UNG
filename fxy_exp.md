# UNG实验

请参考run.sh，跑不同实验时需要修改生成文件地址

## 1 生成基础标签
generate_base_labels.cpp:未改动，sift1m固定使用zipf分布数据。

TODO：如果使用其他分布生成labels，仍需修改

> 生成了sift/sift_base_12_labels_zipf.txt

## 2 生成query标签
generate_query_labels.cpp:需要生成required和optional两种标签。修改了zipf类，让其可以生成两种labels，并写入到txt文件中。

> 生成了sift/sift_query_12_labels_zipf_containment_r.txt 和 sift/sift_query_12_labels_zipf_containment_o.txt

## 3 生成根据距离算的gt
- compute_groundtruth.cpp：调用了run函数，需要修改run函数
- filtered_scan.cpp：定义了run函数。run函数中有answer_one_query，将查到的K个结果存下来。添加函数answer_one_query_and_all_dis_to_csv,将入口超集和及子集中所有向量和查询向量的距离都记录下来，存到UNG_data/nearest_nei_dist文件夹下。search过程中不会查到这些向量之外的向量了。

> 生成了 nearest_nei_dist文件夹

## 4 计算覆盖率c
运行cal_coverage.py

> 生成了coverage文件夹

## 5 计算cost
运行cal_cost.py

> 生成了cost文件夹

## 6 build index
运行build_UNG_index.cpp文件

> 生成了index_file文件夹

## 7 search
运行search_UNG_index.cpp文件。
其中，gt_mode决定了是否生成sort_cost文件夹。1：生成sort_cost文件   2：读取sort_cost文件

> 生成了sort_cost文件夹