#include <chrono>
#include <fstream>
#include <numeric>
#include <iostream>
#include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/program_options.hpp>
#include "uni_nav_graph.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <filesystem>
#include <unordered_map>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace fssy = std::filesystem;

// 定义 lambda 表达式
auto comp = [](const CostEntry &a, const CostEntry &b)
{
    return a.cost < b.cost; // 最大堆
};
// 读取 cost/cost_result_x.csv 文件
void read_cost_file(const std::string &file_path, std::vector<CostEntry> &cost_entries)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // 跳过 CSV 头部

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string value;
        CostEntry entry;

        // 读取 Vector ID
        std::getline(ss, value, ',');
        entry.vector_id = static_cast<ANNS::IdxType>(std::stoi(value));

        // 读取 Cost
        std::getline(ss, value, ',');
        entry.cost = std::stof(value);

        cost_entries.push_back(entry);
    }
    file.close();
}

// 将排序后的 gt 结果存入文件
void save_sorted_cost(const std::vector<CostEntry> &sorted_entries, int query_id, std::string sort_cost_file)
{
    std::string output_dir = sort_cost_file;
    fssy::create_directories(output_dir); // 确保目录存在

    std::string output_file = output_dir + "sorted_cost_result_" + std::to_string(query_id) + ".csv";
    std::ofstream file(output_file);

    if (!file.is_open())
    {
        std::cerr << "Failed to create file: " << output_file << std::endl;
        return;
    }

    file << "Vector ID,Cost\n"; // 写入 CSV 头部

    for (const auto &entry : sorted_entries)
    {
        file << entry.vector_id << "," << entry.cost << "\n";
    }

    file.close();
}

// 处理单个查询
void process_query(int query_id, int K, std::pair<ANNS::IdxType, float> *gt, std::mutex &mtx, std::string cost_file, std::string sort_cost_file)
{
    std::string cost_dir = cost_file;
    std::string file_path = cost_dir + "cost_result_" + std::to_string(query_id) + ".csv";
    std::vector<CostEntry> cost_entries;

    if (fssy::exists(file_path))
    {
        read_cost_file(file_path, cost_entries);
    }
    else
    {
        std::cerr << "File not found: " << file_path << std::endl;
        return;
    }

    // 使用优先队列找到最小的 K 个
    std::priority_queue<CostEntry, std::vector<CostEntry>, decltype(comp)> pq(comp);

    for (const auto &entry : cost_entries)
    {
        pq.push(entry);
        if (pq.size() > K)
        {
            pq.pop();
        }
    }

    // 将结果存入 gt
    std::vector<CostEntry> top_k_entries;
    while (!pq.empty())
    {
        top_k_entries.push_back(pq.top());
        pq.pop();
    }
    std::reverse(top_k_entries.begin(), top_k_entries.end()); // 从小到大排序

    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < top_k_entries.size(); ++i)
    {
        gt[query_id * K + i] = std::make_pair(top_k_entries[i].vector_id, top_k_entries[i].cost);
    }

    // 将排序后的结果存入 sort_cost 目录
    save_sorted_cost(top_k_entries, query_id, sort_cost_file);
}

// 加载所有 cost 文件并找到最小的 K 个(并行)
void load_cost_files(std::pair<ANNS::IdxType, float> *gt, int num_queries, int K, std::string cost_file, std::string sort_cost_file)
{
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (int query_id = 0; query_id < num_queries; ++query_id)
    {
        threads.emplace_back(process_query, query_id, K, gt, std::ref(mtx), cost_file, sort_cost_file);
    }

    for (auto &t : threads)
    {
        t.join();
    }
}

// 从已排序的 cost 文件中读取并将结果存入 gt
void load_sorted_cost_to_gt(std::pair<ANNS::IdxType, float> *gt, int num_queries, int K, std::string sort_cost_file)
{
    std::string sorted_cost_dir = sort_cost_file;

    for (int query_id = 0; query_id < num_queries; ++query_id)
    {
        // std::cout << "Processing query_id: " << query_id << std::endl;
        std::string file_path = sorted_cost_dir + "sorted_cost_result_" + std::to_string(query_id) + ".csv";
        std::ifstream file(file_path);

        if (!file.is_open())
        {
            std::cerr << "File not found: " << file_path << std::endl;
            continue;
        }

        std::string line;
        std::getline(file, line); // 跳过 CSV 头部

        std::vector<CostEntry> cost_entries;
        int i = 0;
        while (std::getline(file, line) && i < K)
        {
            std::stringstream ss(line);
            std::string value;
            CostEntry entry;

            // 读取 Vector ID
            std::getline(ss, value, ',');
            entry.vector_id = static_cast<ANNS::IdxType>(std::stoi(value));

            // 读取 Cost
            std::getline(ss, value, ',');
            entry.cost = std::stof(value);

            // 将 entry 添加到 cost_entries
            cost_entries.push_back(entry);

            // 将结果存入 gt
            gt[query_id * K + i] = std::make_pair(entry.vector_id, entry.cost);

            ++i;
        }

        file.close();
    }
}

int main(int argc, char **argv)
{
    std::string data_type, dist_fn, scenario;
    std::string base_bin_file, query_bin_file, base_label_file, query_label_file, gt_file, index_path_prefix, result_path_prefix;
    std::string query_label_file_r, query_label_file_o;
    std::string cost_file, sort_cost_file;
    ANNS::IdxType K, num_entry_points;
    std::vector<ANNS::IdxType> Lsearch_list;
    uint32_t num_threads;
    int gt_mode; // 0: ground truth is distance and in binary file, 1: ground truth is cost and in csv file, calculate costs, 2: ground truth is cost and in csv file, read costs

    try
    {
        po::options_description desc{"Arguments"};
        desc.add_options()("help,h", "Print information on arguments");
        desc.add_options()("gt_mode", po::value<int>(&gt_mode)->required(),
                           "gt_mode <0/1/2>");
        desc.add_options()("data_type", po::value<std::string>(&data_type)->required(),
                           "data type <int8/uint8/float>");
        desc.add_options()("dist_fn", po::value<std::string>(&dist_fn)->required(),
                           "distance function <L2/IP/cosine>");
        desc.add_options()("base_bin_file", po::value<std::string>(&base_bin_file)->required(),
                           "File containing the base vectors in binary format");
        desc.add_options()("query_bin_file", po::value<std::string>(&query_bin_file)->required(),
                           "File containing the query vectors in binary format");
        desc.add_options()("base_label_file", po::value<std::string>(&base_label_file)->default_value(""),
                           "Base label file in txt format");
        desc.add_options()("query_label_file", po::value<std::string>(&query_label_file)->default_value(""),
                           "Query label file in txt format");
        desc.add_options()("query_label_file_r", po::value<std::string>(&query_label_file_r)->default_value(""),
                           "Query label file in txt format");
        desc.add_options()("query_label_file_o", po::value<std::string>(&query_label_file_o)->default_value(""),
                           "Query label file in txt format");
        desc.add_options()("gt_file", po::value<std::string>(&gt_file)->required(),
                           "Filename for the computed ground truth in binary format");
        desc.add_options()("cost_file", po::value<std::string>(&cost_file)->required(),
                           "Filename for the cost_file");
        desc.add_options()("sort_cost_file", po::value<std::string>(&sort_cost_file)->required(),
                           "Filename for the sort_cost_file");
        desc.add_options()("K", po::value<ANNS::IdxType>(&K)->required(),
                           "Number of ground truth nearest neighbors to compute");
        desc.add_options()("num_threads", po::value<uint32_t>(&num_threads)->default_value(ANNS::default_paras::NUM_THREADS),
                           "Number of threads to use");
        desc.add_options()("result_path_prefix", po::value<std::string>(&result_path_prefix)->required(),
                           "Path to save the querying result file");

        // graph search parameters
        desc.add_options()("scenario", po::value<std::string>(&scenario)->default_value("containment"),
                           "Scenario for building UniNavGraph, <equality/containment/overlap/nofilter>");
        desc.add_options()("index_path_prefix", po::value<std::string>(&index_path_prefix)->required(),
                           "Prefix of the path to load the index");
        desc.add_options()("num_entry_points", po::value<ANNS::IdxType>(&num_entry_points)->default_value(ANNS::default_paras::NUM_ENTRY_POINTS),
                           "Number of entry points in each entry group");
        desc.add_options()("Lsearch", po::value<std::vector<ANNS::IdxType>>(&Lsearch_list)->multitoken()->required(),
                           "Number of candidates to search in the graph");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cout << desc;
            return 0;
        }
        po::notify(vm);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        return -1;
    }

    // check scenario
    if (scenario != "containment" && scenario != "equality" && scenario != "overlap")
    {
        std::cerr << "Invalid scenario: " << scenario << std::endl;
        return -1;
    }

    // load query data
    std::shared_ptr<ANNS::IStorage> query_storage = ANNS::create_storage(data_type);
    query_storage->load_from_file(query_bin_file, query_label_file_r);

    std::shared_ptr<ANNS::IStorage> query_storage_o = ANNS::create_storage(data_type);
    query_storage_o->load_from_file(query_bin_file, query_label_file_o);

    // load index
    ANNS::UniNavGraph index;
    index.load(index_path_prefix, data_type);

    // preparation
    auto num_queries = query_storage->get_num_points();
    std::shared_ptr<ANNS::DistanceHandler> distance_handler = ANNS::get_distance_handler(data_type, dist_fn);
    auto gt = new std::pair<ANNS::IdxType, float>[num_queries * K];
    std::vector<std::unordered_map<ANNS::IdxType, float>> all_cost_entries;
    if (gt_mode == 0)
        ANNS::load_gt_file(gt_file, gt, num_queries, K);
    else if (gt_mode == 1)
    {
        load_cost_files(gt, num_queries, K, cost_file, sort_cost_file);
    }
    else if (gt_mode == 2)
    {
        load_sorted_cost_to_gt(gt, num_queries, K, sort_cost_file);
    }
    auto results = new std::pair<ANNS::IdxType, float>[num_queries * K];

    // search
    std::vector<float> all_cmps, all_qpss, all_recalls;
    std::cout << "Start querying ..." << std::endl;
    for (auto Lsearch : Lsearch_list)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<float> num_cmps(num_queries);
        index.search(query_storage, query_storage_o, distance_handler, num_threads, Lsearch, num_entry_points, scenario, K, results, num_cmps, 0.5);
        auto time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();

        // statistics
        std::cout << "- Lsearch=" << Lsearch << ", time=" << time_cost << "ms" << std::endl;
        all_qpss.push_back(num_queries * 1000.0 / time_cost);
        all_cmps.push_back(std::accumulate(num_cmps.begin(), num_cmps.end(), 0) / num_queries);
        // all_recalls.push_back(ANNS::calculate_recall(gt, results, num_queries, K));
        float overall_recall = ANNS::calculate_recall_to_csv(gt, results, num_queries, K, result_path_prefix + "recall.csv");
        all_recalls.push_back(overall_recall);

        // write to result file
        std::ofstream out(result_path_prefix + "result_L" + std::to_string(Lsearch) + ".csv");
        out << "GT,Result" << std::endl;
        for (auto i = 0; i < num_queries; i++)
        {
            for (auto j = 0; j < K; j++)
            {
                out << gt[i * K + j].first << " ";
            }
            out << ",";
            for (auto j = 0; j < K; j++)
            {
                out << gt[i * K + j].second << " ";
            }
            out << ",";
            for (auto j = 0; j < K; j++)
            {
                out << results[i * K + j].first << " ";
            }
            out << ",";
            for (auto j = 0; j < K; j++)
            {
                out << results[i * K + j].second << " ";
            }
            out << std::endl;
        }
    }

    // write to result file
    fs::create_directories(result_path_prefix);
    std::ofstream out(result_path_prefix + "result_or_cost.csv");
    out << "L,Cmps,QPS,Recall" << std::endl;
    for (auto i = 0; i < Lsearch_list.size(); i++)
        out << Lsearch_list[i] << "," << all_cmps[i] << "," << all_qpss[i] << "," << all_recalls[i] << std::endl;
    out.close();
    delete[] gt;
    std::cout << "- all done" << std::endl;
    return 0;
}