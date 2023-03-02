#include "RateMonotonic.h"

/**
 * @brief Construct a new Rate Monotonic:: Rate Monotonic object
 * 
 */
RateMonotonic::RateMonotonic() {}

/**
 * @brief Compute priority according to the RM policy
 *  周期小的task优先级高，按照周期从小到大排序
 * @param tasks 
 * @return std::map<const char*, Task> 
 */
std::map<const char*, Task> RateMonotonic::prioritize(std::map<const char*, Task> tasks) {
    int max_priority = tasks.size();
    m_task_vector.clear();
    for (auto pair : tasks) {
        m_task_vector.push_back(pair.second);
    }
    // 按照周期从小到大排序
    std::sort(m_task_vector.begin(), m_task_vector.end(), rateMonotonicSorter);
    for (auto elem : m_task_vector) {
        (tasks.at(elem.name)).set_priority(max_priority);
        --max_priority;
    }

    return tasks;
}

/**
 * @brief Prioritized tasks structure getter
 * 
 * @return std::vector<Task> 
 */
std::vector<Task> RateMonotonic::get_prioritized_tasks() const {
    return m_task_vector;
}

/**
 * @brief Reset priorities by clearing the structure
 * 
 */
void RateMonotonic::reset_priorities()  {
    m_task_vector.clear();
}

// 是否可以通过该策略进行调度？
bool RateMonotonic::compute_sufficient_condition(int ntasks, double charge) {
    double condition = ntasks * (pow(2, 1.0/ntasks) - 1);
    if (charge != 0 && charge <= condition) {
        return true;
    } 
    return false;
}