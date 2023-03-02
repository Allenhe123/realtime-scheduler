#include "TaskSet.h"
#include <Python.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <csv.hpp>

volatile sig_atomic_t stop;

void inthand(int signum) {
    stop = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "usage: rt_sched filename\n";
        return -1;
    }

    const std::string path{"test.csv"};
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        std::cout << "file not exist\n";
        return -1;
    }

    auto header = csv::get_header(path);
    for (const auto& v: header) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    auto tups = csv::to_tuples<std::string, int, int, int, int, int>(path);
    if (tups.empty()) {
        std::cout << "file empty\n";
        return -1;
    }
    tups.erase(tups.begin());

    TaskSet ts = TaskSet();
    for (const auto& t : tups) {
        std::cout << std::get<0>(t) << " " << std::get<1>(t) << " " << 
            std::get<2>(t) << " " << std::get<3>(t) << " " << std::get<4>(t) 
            << " " << std::get<5>(t) <<"\n";

        ts.register_task(Task(std::get<0>(t).c_str(), std::get<1>(t), 
            std::get<2>(t), std::get<3>(t), std::get<4>(t), std::get<5>(t)));
    }
    ts.print_task_set();

    std::string q;
    std::cout << "Simulate on a core, Enter the schedule policy you want to use (rm, dm, edf) --> ";
    std::cin >> q;
    if (q == "rm") {
        ts.schedule(RATE_MONOTONIC);
    } else if (q == "dm") {
        ts.schedule(DEADLINE_MONOTONIC);
    } else if (q == "edf") {
        ts.schedule(EARLIEST_DEADLINE_FIRST);
    }

    ts.print_statistics();
    std::cout << "Print statistics (deadline misses, average response time, average waiting time...) done\n";

    stream_schedule_to_file(ts.get_time_table(), argv[1]);
    std::cout << "write time table done\n";

    plot_gantt_from_python(argc, argv);
    std::cout << "Plot schedule done\n";

    return 0;
}
