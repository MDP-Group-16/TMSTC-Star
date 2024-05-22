#include <ros/ros.h>

#include <nav_msgs/OccupancyGrid.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <iostream>
#include <fstream>
#include <PathCut.h>
#include <MaximumSubRectDivision.h>
#include <boost/bind.hpp>
#include <DARPPlanner.h>
#include <Dinic.h>
#include <m_TSP.h>
#include <HeuristicPartition.h>

using std::cout;
using std::endl;
using std::ios;

#define PI 3.141592654
#define HORIZONTAL 0
#define VERTICAL 1
#define FIRST 0
#define LAST 1

class GetCoveragePathServer
{
private:
    // ros param
    string MST_shape;
    string allocate_method;
    int GA_max_iter, unchanged_iter; // GA config
    MTSP *mtsp; // GA config
    int hp_max_iter; // heuristic

public:
    GetCoveragePathServer(int argc, char **argv)
    {
        ros::init(argc, argv, "map_to_coverage_path_server");
        ros::NodeHandle n;

        ROS_INFO("Starting Coverage Path Service Server.");
        ros::ServiceServer ss = n.advertiseService("get_coverage_path", &MapToCoveragePath::create_plan, &server);

        ros::Subscriber = n.subscribe("map", 10, &MapTransformer::map_callback, this);
        coverage_map_pub = n.advertise<nav_msgs::OccupancyGrid>("coverage_map", 1);

        ros::Subscriber map_sub = n.subscribe("map", 10, &MapTransformer::map_callback, this);

        if (!n.getParam("/allocate_method", allocate_method))
        {
            ROS_ERROR("Please specify the algorithm.");
            return;
        }

        if (!n.getParam("/MST_shape", MST_shape))
        {
            ROS_ERROR("Please set your desired MST shape.");
            return;
        }

        if (!n.getParam("/GA_max_iter", GA_max_iter))
        {
            ROS_ERROR("PLease specify GA times");
            return;
        }

        if (!n.getParam("/unchanged_iter", unchanged_iter))
        {
            ROS_ERROR("GA break iter");
            return;
        }

        if (!n.getParam("/hp_max_iter", hp_max_iter))
        {
            ROS_ERROR("specify heuristic solver iteration.");
            return;
        }

        robot_pos.resize(robot_num);

        // visualize paths for testing
        // only for small maps, big maps cause bad alloc, because of long paths
        path_publishers.resize(robot_num);
        for (int i = 0; i < robot_num; ++i)
        {
            std::string topic_string = "robot" + std::to_string(i + 1) + "/path";
            path_publishers[i] = n.advertise<nav_msgs::Path>(topic_string, 1);
        }

        // initialize Map and Region(resize and fill in)
        Map.resize(cmh / 2, vector<int>(cmw / 2, 0));
        Region.resize(cmh, vector<int>(cmw, 0));
        // 去掉地图上的孤岛 一次BFS即可
        eliminateIslands();
        showMapAndRegionInf();

        // 写入实验相关信息
        test_data_file.open(data_file_path, ios::in | ios::out | ios::app);
        if (coverAndReturn)
            test_data_file << "\nRETURN ";
        else
            test_data_file << "\nNOT-RET ";

        test_data_file << allocate_method << " " << MST_shape;
        test_data_file << " robot num: " << robot_num;
        test_data_file.close();

        if (!createPaths(&n))
            return;

        ROS_INFO("Begin full coverage...");
        tic = ros::Time::now().toSec();

        sendGoals();
    }

    bool get_coverage_path(TMSTC-Star::CoveragePath::Request  &req, TMSTC-Star::CoveragePath::Response &res)
        {
            nav_msgs::OccupancyGrid coverage_map;
            ROS_INFO("Received coverage path request.");

            // convert map to grid
            nav_msgs::OccupancyGrid coverage_map = map_to_grid(req.map);
            
            //generate paths
            createPaths(coverage_map);

            nav_msgs::OccupancyGrid map;
            nav_msgs::OccupancyGrid coverage_map;

            ROS_INFO("Finished creating coverage path.");
            return true;
        }

    // Generate the coverage pths for all robots as a nav_msgs::Path.
    bool createPaths(const nav_msgs::OccupancyGrid& coverage_map, const TMSTC-Star::CoveragePath::Request &req)
    {
        double origin_x = coverage_map.info.origin.position.x;
        double origin_y = coverage_map.info.origin.position.y;
        double cmres = coverage_map.info.resolution;

        Map
        Region
        robot_pos
        int robot_num = req.num_robots

        robot_init_pos = req.initial_poses;

        for (int i = 0; i < robot_num; ++i)
        {
            cout << robot_pos[i].first << " " << robot_pos[i].second << endl;
            int robot_index = (int)((robot_pos[i].first - origin_x) / cmres) + ((int)((robot_pos[i].second - origin_y) / cmres) * cmw); // 如果地图原点不是(0, 0)，则需要转换时减掉原点xy值
            robot_init_pos.push_back(robot_index);
            cout << "No." << i + 1 << "'s robot initial position index: " << robot_index << endl;
        }

        switch(allocate_method) {
            case "DARP": 
                for (int i = 0; i < robot_num; ++i)
                {+
                    int robot_index = (int)((robot_pos[i].first - origin_x) / cmres) + ((int)((robot_pos[i].second - origin_y) / cmres) * cmw); // 如果地图原点不是(0, 0)，则需要转换时减掉原点xy值
                    robot_init_pos.push_back(robot_index);
                    cout << "No." << i + 1 << "'s robot initial position index: " << robot_index << endl;
                }


                // TODO
                // Map, robot_pos, robot_num, &coverage_map
                DARPPlanner *planner = new DARPPlanner(Map, Region, robot_pos, robot_num, &coverage_map);

                if (MST_shape == "RECT_DIV")
                    paths_idx = planner->plan("RECT_DIV");
                else if (MST_shape == "DFS_VERTICAL")
                    paths_idx = planner->plan("DFS_VERTICAL");
                else if (MST_shape == "DFS_HORIZONTAL")
                    paths_idx = planner->plan("DFS_HORIZONTAL");
                else if (MST_shape == "KRUSKAL")
                    paths_idx = planner->plan("KRUSKAL");
                else if (MST_shape == "ACO_OPT")
                    paths_idx = planner->plan("ACO_OPT");
                else if (MST_shape == "DINIC")
                    paths_idx = planner->plan("DINIC");
                else
                {
                    ROS_ERROR("Please check shape's name in launch file!");
                    return false;
                }
                break;
            case "MSTC": 
                if (MST_shape == "DINIC")
                    MST = dinic.dinic_solver(Map, true);
                else
                {
                    ONE_TURN_VAL = 0.0;
                    Division mstc_div(Map);
                    if (MST_shape == "RECT_DIV")
                        MST = mstc_div.rectDivisionSolver();
                    else if (MST_shape == "DFS_VERTICAL")
                        MST = mstc_div.dfsWithStackSolver(VERTICAL);
                    else if (MST_shape == "DFS_HORIZONTAL")
                        MST = mstc_div.dfsWithStackSolver(HORIZONTAL);
                    else if (MST_shape == "BFS_VERTICAL")
                        MST = mstc_div.bfsSolver(VERTICAL);
                    else if (MST_shape == "BFS_HORIZONTAL")
                        MST = mstc_div.bfsSolver(HORIZONTAL);
                    else if (MST_shape == "KRUSKAL")
                        MST = mstc_div.kruskalSolver();
                    else if (MST_shape == "ACO_OPT")
                    {
                        ACO_STC aco(1, 1, 1, 0.15, 60, 300, Map, MST);
                        MST = aco.aco_stc_solver();
                    }
                    else if (MST_shape == "HEURISTIC")
                    {
                        HeuristicSolver::HeuristicPartition hp(Map, hp_max_iter);
                        MST = hp.hpSolver(true);
                    }
                    else
                    {
                        ROS_ERROR("Please check shape's name in launch file!");
                        return false;
                    }
                }

                PathCut cut(Map, Region, MST, robot_init_pos, nh, boost::make_shared<nav_msgs::OccupancyGrid>(map), useROSPlanner, coverAndReturn);
                paths_idx = cut.cutSolver();
                break;
        
            case "MSTP": 
                ONE_TURN_VAL = 0.0;

                // crossover_pb, mutation_pb, elite_rate, sales_men, max_iter, ppl_sz, unchanged_iter;
                GA_CONF conf{0.9, 0.01, 0.3, robot_num, GA_max_iter, 60, unchanged_iter};
                if (MST_shape == "HEURISTIC")
                {
                }
                else if (MST_shape == "OARP")
                {
                    cout << Map.size() << ", " << Map[0].size() << "\n";
                    dinic.dinic_solver(Map, false);
                    dinic.formBricksForMTSP(Map);
                    mtsp = new MTSP(conf, dinic, robot_init_pos, cmw, coverAndReturn);

                    for (int i = 0; i < GA_max_iter; ++i)
                    {
                        mtsp->GANextGeneration();
                        if (mtsp->unchanged_gens >= unchanged_iter)
                            break;
                        if (i % 1000 == 0)
                        {
                            // display cost for debugging
                            cout << "GA-cost: " << mtsp->best_val << "\n";
                        }
                    }

                    paths_idx = mtsp->MTSP2Path();

                    for (int i = 0; i < paths_idx.size(); ++i)
                    {
                        cout << "path " << i << ": ";
                        for (int j = 0; j < paths_idx[i].size(); ++j)
                        {
                            cout << paths_idx[i][j] << " ";
                        }
                        cout << "\n";
                    }
                    // return false;
                }
                else
                {
                    ROS_ERROR("Please check shape's name in launch file!");
                    return false;
                }
                break;
            default:
                ROS_ERROR("Please check allocated method's name in launch file!");
                return false;
        }

        // try to reduce some useless points in path in order to acclerate
        paths_cpt_idx.resize(robot_num);
        paths_idx = shortenPathsAndGetCheckpoints(paths_cpt_idx);

        // 将index paths转化为一个个位姿
        paths.resize(paths_idx.size());
        for (int i = 0; i < paths_idx.size(); ++i)
        {
            paths[i].header.frame_id = "map";
            for (int j = 0; j < paths_idx[i].size(); ++j)
            {
                int dy = paths_idx[i][j] / cmw;
                int dx = paths_idx[i][j] % cmw;

                // 判断每个goal的yaw
                double yaw = 0;
                if (j < paths_idx[i].size() - 1)
                {
                    int dx1 = paths_idx[i][j + 1] % cmw;
                    int dy1 = paths_idx[i][j + 1] / cmw;

                    yaw = atan2(1.0 * (dy1 - dy), 1.0 * (dx1 - dx));
                }
                // yaw转四元数
                paths[i].poses.push_back({});
                paths[i].poses.back().pose.position.x = dx * cmres + 0.25;
                paths[i].poses.back().pose.position.y = dy * cmres + 0.25;
                paths[i].poses.back().pose.orientation = tf::createQuaternionMsgFromYaw(yaw);
            }
        }


        return true;
    }

    //Convert a Robot position in meters to an index in the occupancy grid
    int posToIndex(){
        pixel_x * res - origin_x = real_x
        pixel_y * res - origin_y = real_y
    }

    bool isSameLine(int &a, int &b, int &c)
    {
        return a + c == 2 * b;
    }

    Mat shortenPathsAndGetCheckpoints(Mat &paths_cpt_idx)
    {
        Mat paths_idx_tmp(robot_num, vector<int>{});
        for (int i = 0; i < robot_num; ++i)
        {
            paths_idx_tmp[i].push_back(paths_idx[i][0]);
            paths_cpt_idx[i].push_back(0);

            int interval = 4;
            bool cpt_flag = false;
            for (int step = 1; step < paths_idx[i].size() - 1; step++)
            {
                cpt_flag = !isSameLine(paths_idx[i][step - 1], paths_idx[i][step], paths_idx[i][step + 1]);
                if (allocate_method == "MTSP" && mtsp->pt_brickid.count(paths_idx[i][step]))
                {
                    cpt_flag = true;
                }

                if (!cpt_flag && interval)
                {
                    interval--;
                }
                else
                {
                    if (cpt_flag)
                        paths_cpt_idx[i].push_back(paths_idx_tmp[i].size());

                    paths_idx_tmp[i].push_back(paths_idx[i][step]);
                    interval = 4;
                }
            }
            paths_cpt_idx[i].push_back(paths_idx_tmp[i].size());
            paths_idx_tmp[i].push_back(paths_idx[i].back());
        }

        return paths_idx_tmp;
    }

    void eliminateIslands()
    {
        int dir[4][2] = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};
        vector<vector<bool>> vis(coverage_map.info.width, vector<bool>(coverage_map.info.height, false));
        queue<pair<int, int>> que;
        int sx = (robot_pos[0].first - coverage_map.info.origin.position.x) / coverage_map.info.resolution;
        int sy = (robot_pos[0].second - coverage_map.info.origin.position.y) / coverage_map.info.resolution;
        que.push({sx, sy});
        vis[sx][sy] = true;

        while (!que.empty())
        {
            pair<int, int> p = que.front();
            que.pop();
            coverage_map.data[p.second * coverage_map.info.width + p.first] = 2;
            for (int i = 0; i < 4; ++i)
            {
                int dx = p.first + dir[i][0], dy = p.second + dir[i][1];
                if (dx >= 0 && dy >= 0 && dx < coverage_map.info.width && dy < coverage_map.info.height && coverage_map.data[dy * coverage_map.info.width + dx] && !vis[dx][dy])
                {
                    que.push({dx, dy});
                    vis[dx][dy] = true;
                }
            }
        }

        for (int i = 0; i < coverage_map.data.size(); ++i)
        {
            coverage_map.data[i] = coverage_map.data[i] == 2 ? 1 : 0;
        }
    }

    //Convert the recevied map to a grid with a cell width equal to the tool width, each free cell in the grid will then be covered by the robots.
    nav_msgs::OccupancyGrid map_to_grid(const nav_msgs::OccupancyGrid& map, float tool_width)
    {
        ROS_INFO("Converting received map to a grid for coverage planning.");

        // create coverage map
        nav_msgs::OccupancyGrid coverage_map;
        coverage_map.header = map.header;
        coverage_map.info.origin = map.info.origin;
        coverage_map.info.resolution = tool_width;
        coverage_map.info.width = (map.info.resolution * map.info.width) / coverage_map.info.resolution;
        coverage_map.info.height = (map.info.resolution * map.info.height) / coverage_map.info.resolution;
        coverage_map.data.resize(coverage_map.info.width * coverage_map.info.height);

        // convert map to coverage map
        int scale = coverage_map.info.resolution / map.info.resolution;
        for (int row = 0; row < coverage_map.info.height; ++row)
        {
            for (int col = 0; col < coverage_map.info.width; ++col)
            {
                int sx = row * scale, sy = col * scale;
                bool valid = true;
                for (int i = sx; i < sx + scale; ++i)
                {
                    if (!valid)
                        break;
                    for (int j = sy; j < sy + scale; ++j)
                    {
                        if (map.data[i * map.info.width + j] != 0)
                        {
                            valid = false;
                            break;
                        }
                    }
                }
                coverage_map.data[row * coverage_map.info.width + col] = valid ? 1 : 0;
            }
        }

        return coverage_map;
    }
};

int main(int argc, char **argv)
{

    GetCoveragePathServer server = GetCoveragePathServer();

    ros::spin();

    return 0;
}