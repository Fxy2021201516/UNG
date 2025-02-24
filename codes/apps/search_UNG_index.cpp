#include <chrono>
#include <fstream>
#include <numeric>
#include <iostream>
#include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/program_options.hpp>
#include "uni_nav_graph.h"
#include "utils.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace fssy = std::filesystem;

struct CostEntry
{
    ANNS::IdxType vector_id;
    float cost;
};

// 读取 cost_result_x.csv 文件
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
void save_sorted_cost(const std::vector<CostEntry> &sorted_entries, int query_id)
{
    std::string output_dir = "./data/sort_cost/";
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

// 读取 cost 目录下所有 CSV 文件，并找到最小的 K 个
void load_cost_files(std::pair<ANNS::IdxType, float> *gt, int num_queries, int K)
{
    std::string cost_dir = "./data/cost/";

    for (int query_id = 0; query_id < num_queries; ++query_id)
    {
        std::cout << "Processing query_id: " << query_id << std::endl;
        std::vector<CostEntry> cost_entries;
        std::string file_path = cost_dir + "cost_result_" + std::to_string(query_id) + ".csv";

        if (fssy::exists(file_path))
            read_cost_file(file_path, cost_entries);
        else
        {
            std::cerr << "File not found: " << file_path << std::endl;
            continue;
        }

        // 按 cost 进行排序
        std::sort(cost_entries.begin(), cost_entries.end(), [](const CostEntry &a, const CostEntry &b)
                  { return a.cost < b.cost; });

        // 选取最小的 K 个并存入 gt
        for (int i = 0; i < std::min(K, static_cast<int>(cost_entries.size())); ++i)
        {
            gt[query_id * K + i] = std::make_pair(cost_entries[i].vector_id, cost_entries[i].cost);
        }

        // 将排序后的结果存入 sort_cost 目录
        save_sorted_cost(cost_entries, query_id);
    }
}

int main(int argc, char **argv)
{
    std::string data_type, dist_fn, scenario;
    std::string base_bin_file, query_bin_file, base_label_file, query_label_file, gt_file, index_path_prefix, result_path_prefix;
    std::string query_label_file_r, query_label_file_o;
    ANNS::IdxType K, num_entry_points;
    std::vector<ANNS::IdxType> Lsearch_list;
    uint32_t num_threads;
    int gt_mode = 1; // 0: ground truth is distance and in binary file, 1: ground truth is cost and in csv file

    try
    {
        po::options_description desc{"Arguments"};
        desc.add_options()("help,h", "Print information on arguments");
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

    // load index
    ANNS::UniNavGraph index;
    index.load(index_path_prefix, data_type);

    // preparation
    auto num_queries = query_storage->get_num_points();
    std::shared_ptr<ANNS::DistanceHandler> distance_handler = ANNS::get_distance_handler(data_type, dist_fn);
    auto gt = new std::pair<ANNS::IdxType, float>[num_queries * K];
    if (gt_mode == 0)
        ANNS::load_gt_file(gt_file, gt, num_queries, K);
    else
    {
        load_cost_files(gt, num_queries, K);
    }
    auto results = new std::pair<ANNS::IdxType, float>[num_queries * K];

    // search
    std::vector<float> all_cmps, all_qpss, all_recalls;
    std::cout << "Start querying ..." << std::endl;
    for (auto Lsearch : Lsearch_list)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<float> num_cmps(num_queries);
        index.search(query_storage, distance_handler, num_threads, Lsearch, num_entry_points, scenario, K, results, num_cmps);
        auto time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();

        // statistics
        std::cout << "- Lsearch=" << Lsearch << ", time=" << time_cost << "ms" << std::endl;
        all_qpss.push_back(num_queries * 1000.0 / time_cost);
        all_cmps.push_back(std::accumulate(num_cmps.begin(), num_cmps.end(), 0) / num_queries);
        all_recalls.push_back(ANNS::calculate_recall(gt, results, num_queries, K));

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
                out << results[i * K + j].first << " ";
            }
            out << std::endl;
        }
    }

    // write to result file
    fs::create_directories(result_path_prefix);
    std::ofstream out(result_path_prefix + "result_or.csv");
    out << "L,Cmps,QPS,Recall" << std::endl;
    for (auto i = 0; i < Lsearch_list.size(); i++)
        out << Lsearch_list[i] << "," << all_cmps[i] << "," << all_qpss[i] << "," << all_recalls[i] << std::endl;
    out.close();
    delete[] gt;
    std::cout << "- all done" << std::endl;
    return 0;
}