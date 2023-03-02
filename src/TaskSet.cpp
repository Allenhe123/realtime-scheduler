#include "TaskSet.h"

TaskSet::TaskSet() {}

/**
 * @brief register a task in the set
 * 
 * @param tsk 
 */
void TaskSet::register_task(Task tsk) {
    if (m_tasks.find(tsk.name) == m_tasks.end()) {
        // Task not registered, free to go
        m_tasks.insert(std::pair<const char*, Task>(tsk.name, tsk));
        ++m_number_of_tasks;
        // 每增加一个task就重新计算LCM
        this->compute_hyper_period();
        printf("%sTask <%s> has been registered.\n%s", BOLDGREEN, tsk.name, RESET);
    } else {
        std::cout << BOLDRED << "Failed to register task: id already registered." << RESET << std::endl;
    }
}

/**
 * @brief remove a task from the set
 * 
 * @param task_id 
 */
void TaskSet::remove_task(const char* task_id) {
    if (m_tasks.find(task_id) == m_tasks.end()) {
        printf("%sTask <%s> has not been registered, failed to remove.\n%s", BOLDRED, task_id, RESET);
    } else {
        m_tasks.erase(task_id);
        --m_number_of_tasks;
        // 每删除一个task就重新计算LCM
        this->compute_hyper_period();
        printf("%sTask <%s> has been removed\n%s", BOLDGREEN, task_id, RESET);
    }   
}

/**
 * @brief compute hyper period of the set
 * 
 */
void TaskSet::compute_hyper_period() {
    // int hyper_periods[m_number_of_tasks];
    int* hyper_periods = new int[m_number_of_tasks];
    int i = 0;
    for (auto const& [key, val] : m_tasks) {
        hyper_periods[i] = val.get_period();
        ++i;
    }
    // int n = sizeof(hyper_periods) / sizeof(hyper_periods[0]);
    int n = i;
    m_hyper_period = findlcm(hyper_periods, n);
    // 重新生成time_table，大小为hyper_period
    std::vector<const char*> tempVec(m_hyper_period, "");
    m_time_table = tempVec;

    delete []hyper_periods;
}

/**
 * @brief get the task set
 * 
 * @return std::map<const char*, Task> 
 */
std::map<const char*, Task> TaskSet::get_task_set() const {
    return m_tasks;
}

/**
 * @brief get number of tasks registered
 * 
 * @return int 
 */
int TaskSet::get_number_of_tasks() const {
    return m_number_of_tasks;
}

/**
 * @brief tasks set printer
 * 
 */
void TaskSet::print_task_set() {
    printf("%s=======================\n", BOLDWHITE);
    printf("TASK SET {\n");
    printf("%s\thyperperiod = %d\n", BOLDBLUE, m_hyper_period);
    for (auto it = m_tasks.cbegin(); it != m_tasks.cend(); ++it) {
        printf("\t");
        (it->second).print_task();
    }
    printf("%s}\n=======================\n%s", RESET, RESET);
}

/**
 * @brief schedule task set according to a chosen policy
 * 
 * @param scheduler 
 */
void TaskSet::schedule(int scheduler) {
    std::cout << BOLDYELLOW << "Scheduling the tasks..." << RESET << std::endl;
    bool ok;
    double processor_charge = 0;
    double ch = 0;
    for (auto it = m_tasks.cbegin(); it != m_tasks.cend(); ++it) {
        processor_charge += (it->second).get_utilization();     // wcet / period
        ch += (it->second).get_ch();                            // wcet / deadline
    }
    // 必须小于1
    if (processor_charge > 1) {
        std::cout << BOLDRED << "The Task Set is not schedulable." << RESET; 
        exit(EXIT_FAILURE);
    }
    switch(scheduler) {
        // 按照周期从小到大排序, 周期小的task优先级高
        case RATE_MONOTONIC: {
            auto rm = RateMonotonic();
            // 计算充分条件：
            ok = rm.compute_sufficient_condition(this->m_number_of_tasks, processor_charge);
            if (ok) {
                // 每个task设置了优先级以后的原任务数组
                this->m_tasks = rm.prioritize(this->m_tasks);
                // 根据周期从小到大排序后的任务数组
                this->m_priority_vector = rm.get_prioritized_tasks();
                std::cout << BOLDGREEN << "Priorities of the task set have been computed successfully." << RESET << std::endl;
            } else {
                int yn;
                std::cout << BOLDRED << "The current Task Set might not be schedulable by RMS. Do you still want to try scheduling it with RMS ? 1/0 --- " << RESET; 
                std::cin >> yn;
                if (yn == 1) {
                    this->m_tasks = rm.prioritize(this->m_tasks);
                    this->m_priority_vector = rm.get_prioritized_tasks();
                } else exit(1);
            }
            // 计算time_table
            this->compute_time_table();
            std::cout << BOLDGREEN << "Schedule successfully computed." << RESET << std::endl;
        } break;
        // 按照deadline进行排序，deadline小的优先级高
        case DEADLINE_MONOTONIC: {
            auto dm = DeadlineMonotonic();
            
            ok = dm.compute_sufficient_condition(this->m_number_of_tasks, ch);
            if (ok) {
                this->m_tasks = dm.prioritize(this->m_tasks);
                this->m_priority_vector = dm.get_prioritized_tasks();
                std::cout << BOLDGREEN << "Priorities of the task set have been computed successfully." << RESET << std::endl;
            } else {
                int yn;
                std::cout << BOLDRED << "The current Task Set might not be schedulable by DMS. Do you still want to try scheduling it with RMS ? 1/0 --- " << RESET; 
                std::cin >> yn;
                if (yn == 1) {
                    this->m_tasks = dm.prioritize(this->m_tasks);
                    this->m_priority_vector = dm.get_prioritized_tasks();
                } else exit(1);
            }
            this->compute_time_table();
            std::cout << BOLDGREEN << "Schedule successfully computed." << RESET << std::endl;
        } break;
        //Earliest Deadline First
        case EARLIEST_DEADLINE_FIRST: {
            std::cout << BOLDRED << "Scheduler not supported yet" << RESET << std::endl;
        } break;
        default: {
            std::cout << BOLDRED << "Scheduler not supported yet" << RESET << std::endl;
        } break;
    }
}

/**
 * @brief get availability time table
 * 
 * @return std::vector<const char*> 
 */
std::vector<const char*> TaskSet::get_time_table() const {
    return m_time_table;
}

/**
 * @brief compute availability time table (schedule)
 * 
 */
void TaskSet::compute_time_table() {
    // 根据周期从小到大排序后的任务数组
    for (int tsk=0; tsk<m_priority_vector.size(); ++tsk) {
        std::vector<int> response_time;
        std::vector<int> waiting_time;
        std::vector<int> activations_rank;
        std::vector<int> deactivation_rank;
        
        for (int p=0; p<m_hyper_period; ++p) {
            int period = p * m_priority_vector[tsk].get_period() + m_priority_vector[tsk].get_offset();
            int deadline = (p + 1) * m_priority_vector[tsk].get_deadline() + m_priority_vector[tsk].get_offset();
            
            if (period >= m_hyper_period) {
                break;
            }
            // 一个hyper_period内该task的开始运行时刻
            activations_rank.push_back(period);
            // 一个hyper_period内该task的deadline时刻
            deactivation_rank.push_back(deadline);
        }
        int i = 0;
        // 遍历一个hyper_period内该task的各个开始运行时刻
        for (auto elem: activations_rank) {
            response_time.push_back(0);
            waiting_time.push_back(0);
            int init_activation = elem;
            bool mutual_excl = false;
            // 对该任务的每个开始时刻，[开始时刻，开始时刻+WCET] 这个区间是否与其他任务有重叠 
            for (int j=0; j<m_priority_vector[tsk].get_computation(); ++j) {
                while (m_time_table[elem+j] != "") {
                    if (elem+1+j >= m_hyper_period) {
                        std::cout << BOLDRED << "Failed to schedule task set" << RESET << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    elem++;
                }
                if (elem + j > m_hyper_period) {
                    response_time.pop_back();
                    waiting_time.pop_back();
                    break;
                }
                // 计算等待时间。若开始时刻与其他任务有重叠，则在开始时刻基础上+1，直到找到一个不重叠的时刻
                if (j == 0) {
                    waiting_time[i] = elem - init_activation;
                    // 等待时间超过deadline则miss
                    if (waiting_time[i] > m_priority_vector[tsk].get_deadline() && !mutual_excl) {
                        m_tasks.at(m_priority_vector[tsk].name).set_deadlines_missed(m_tasks.at(m_priority_vector[tsk].name).get_statistics().deadlines_missed + 1);
                        if (m_tasks.at(m_priority_vector[tsk].name).get_statistics().deadlines_missed == 1) {
                            m_tasks.at(m_priority_vector[tsk].name).set_first_deadline_missed_t(deactivation_rank[i]);
                        }
                        mutual_excl = true;
                    }
                }
                // 计算响应时间。若开始时刻=WCET-1
                if (j == (m_priority_vector[tsk].get_computation() - 1)) {
                    response_time[i] = elem + j - init_activation + 1;
                    if (response_time[i] > m_priority_vector[tsk].get_deadline() && !mutual_excl) {
                        m_tasks.at(m_priority_vector[tsk].name).set_deadlines_missed(m_tasks.at(m_priority_vector[tsk].name).get_statistics().deadlines_missed + 1);
                        if (m_tasks.at(m_priority_vector[tsk].name).get_statistics().deadlines_missed == 1) {
                            m_tasks.at(m_priority_vector[tsk].name).set_first_deadline_missed_t(deactivation_rank[i]);
                        }
                        mutual_excl = true;
                    }
                }
                // 对时间表进行标记，任务开始时刻--任务名
                m_time_table[elem + j] = m_priority_vector[tsk].name;
            }
            ++i;
        }
        if (response_time.size() != 0) {
            double art = std::accumulate(response_time.begin(), response_time.end(), 0) / response_time.size();
            m_tasks.at(m_priority_vector[tsk].name).set_average_response_time(art);
        }
        if (waiting_time.size() != 0) {
            double awt = std::accumulate(waiting_time.begin(), waiting_time.end(), 0) / waiting_time.size();
            m_tasks.at(m_priority_vector[tsk].name).set_average_waiting_time(awt);
        }
    }
}

/**
 * @brief print statistics of each task in the set
 * 
 */
void TaskSet::print_statistics() const {
    printf("%s=======================%s\n", BOLDBLUE, RESET);
    for (auto &pair : m_tasks) {
        pair.second.pretty_print_statistics();
    }
    printf("%s=======================%s\n", BOLDBLUE, RESET);
}

/**
 * @brief ugly schedule printer
 * 
 */
void TaskSet::print_schedule() const {
    printf("\n==SCHEDULE==\n");
    for ( auto &pair : m_tasks ) {
        printf("%s |", pair.first);
        for ( auto &elem : m_time_table ) {
            if (pair.first == elem) {
                printf("█");
            } else {
                printf("_");
            }
        }
        printf("\n");
    }
}

