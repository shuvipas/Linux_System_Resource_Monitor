#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <unistd.h>

#include <unordered_map>
#include <algorithm>
#include <set>
#define IS_DIGIT(a) ((a)>='0' && (a)<='9')


using cpu_ticks = unsigned long long;
namespace fs = std::filesystem;

struct Process {
    pid_t pid = 0;
    uid_t uid;
    std::string cmd;
    size_t mem_kb = 0;
    
    double cpu_percent = 0.0;
    size_t utime = 0;
    size_t stime = 0;
    size_t last_time = 0;
  
};

struct ProcessGroup {
    std::string group_name;
    int count = 0;
    double cpu_percent = 0.0;
    size_t total_mem_kb = 0;
    cpu_ticks total_utime = 0;
    cpu_ticks total_stime = 0;
    cpu_ticks last_total_time = 0;
};

// double debug_used_mem_gb=0;
// double debug_total_cpu_percent = 0.0;
struct SysInfo{
    size_t total_mem = 0;
    double tot_mem_gb = 0;
    size_t free_mem = 0;
    size_t available_mem = 0;
    double used_mem_percent = 0.0;
    double used_mem_gb = 0.0;
    double total_cpu_percent = 0.0;
    int page_size = 0;

    cpu_ticks cpu_prev_tot_time = 0;
    cpu_ticks cpu_prev_idle_time = 0;
    std::unordered_map<std::string, cpu_ticks> cpu_prev_process_times;
    int core_number = 0;

    size_t uid_min =0;
    size_t uid_max =0;


};
SysInfo my_system;

// Read system-wide total CPU time from /proc/stat
void readSystemCpuTime(cpu_ticks& curr_tot_time, cpu_ticks& curr_idle_time) {
    std::ifstream stat_file("/proc/stat");
    std::string line;
    std::getline(stat_file, line);
    
    cpu_ticks user, nice, system, idle, iowait;
    sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu", 
           &user, &nice, &system, &idle, &iowait);
   
    curr_tot_time = user + nice + system + idle + iowait;
    curr_idle_time = idle + iowait;
}

void readProcCpuTimes(std::string pid_str, Process& proc) {
    std::string stat_path = "/proc/" + pid_str + "/stat";
    std::ifstream stat_file(stat_path);
    std::string line;
    
    if (std::getline(stat_file, line)) {

        std::istringstream iss(line);
        std::string token;
        // Skip first 13 fields
        for (int i = 0; i < 13; i++) iss >> token;        
        // Read utime (14) and stime (15)
        iss >> proc.utime >> proc.stime;
    }
}

// Calculate CPU percentage for all processes
void calculateCpuPercentages(std::unordered_map<std::string, ProcessGroup>& groups) {
    cpu_ticks curr_tot_time, curr_idle_time;
    readSystemCpuTime(curr_tot_time, curr_idle_time);

    if (my_system.cpu_prev_tot_time > 0) {
        cpu_ticks total_delta = curr_tot_time - my_system.cpu_prev_tot_time;
        cpu_ticks idle_delta = curr_idle_time - my_system.cpu_prev_idle_time;
        
        if (total_delta > 0) {
            my_system.total_cpu_percent = 100.0 * (total_delta - idle_delta) / total_delta;
        }
        
        // Calculate group CPU percentages
        for (auto& [group_name, group] : groups) {
            cpu_ticks current_group_time = group.total_utime + group.total_stime;
            
            if (my_system.cpu_prev_process_times.count(group_name)) {
                cpu_ticks prev_time = my_system.cpu_prev_process_times[group_name];
                if (current_group_time > prev_time && total_delta > 0) {
                    cpu_ticks group_delta = current_group_time - prev_time;
                    group.cpu_percent = (100.0 * group_delta / total_delta);
                }
            }
            my_system.cpu_prev_process_times[group_name] = current_group_time;
        }
    } else {
        // First run - store baseline
        for (auto& [group_name, group] : groups) {
            my_system.cpu_prev_process_times[group_name] = group.total_utime + group.total_stime;
        }
    }
    
    my_system.cpu_prev_tot_time = curr_tot_time;
    my_system.cpu_prev_idle_time = curr_idle_time;
}



bool readProcPid(std::string pid_str,pid_t & pid){
    // Check if directory name is a number (PID)
    // proc.pid = 0;
    for (char c : pid_str) {
        if (!IS_DIGIT(c)) return false;
        pid = pid*10 + (c -'0');
    }
    return true;
}
void readProcCmd(std::string& pid_str,std::string& proc_cmd){
    std::string comm_path = "/proc/" + pid_str + "/comm";
    std::ifstream comm_file(comm_path);
    if (! std::getline(comm_file, proc_cmd)) {
        proc_cmd = "unknown";
    }
}

uid_t readProcUid(std::string pid_str){
        uid_t uid;
        std::string status_path = "/proc/" + pid_str + "/status";
        std::ifstream status_file(status_path);
        std::string line;
        
        while (std::getline(status_file, line)) {
            if (line.find("Uid:") == 0) {
                // std::istringstream iss(line.substr(4));
                // iss >> proc.uid;
                sscanf(line.c_str(), "Uid: %d", &uid);
                break;
            }
        }
        return uid;
}


void readProcMem(std::string pid_str, Process& proc){
    std::string status_path = "/proc/" + pid_str + "/status";
    std::ifstream status_file(status_path);
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.find("VmRSS:") == 0) {
            sscanf(line.c_str(), "VmRSS: %lu kB", &proc.mem_kb);
            break;
        }
    }
}

void readTotMem(){
    std::ifstream meminfo("/proc/meminfo"); // mem info in kb
    std::string line;
    
    while (std::getline(meminfo, line)) {
        if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lu kB", &my_system.available_mem);
            my_system.used_mem_gb = (double) (my_system.total_mem - my_system.available_mem) / (1024.0*1024.0); 
            if(my_system.total_mem) my_system.used_mem_percent = 100.0 * (1.0  -(double) my_system.available_mem / (my_system.total_mem));
        }
    }
    
}
void readSystemInfo() {
    my_system.page_size = (double) sysconf(_SC_PAGESIZE);
    // Read total memory
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu kB", &my_system.total_mem);
            my_system.tot_mem_gb = (double)my_system.total_mem / (1024.0 * 1024.0);
            break;
        }
    }
    
    std::ifstream cpuinfo("/proc/cpuinfo"); 
    while (std::getline(cpuinfo, line)) {
        if (line.find("processor") == 0) { 
            my_system.core_number++;
        }
    }  
    
    std::ifstream login_defs("/etc/login.defs");
    while (std::getline(login_defs, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Remove leading whitespace for easier parsing
        line.erase(0, line.find_first_not_of(" \t"));
        
        // Parse UID and GID ranges [citation:3][citation:8]
        if (line.find("UID_MIN") == 0) {
            sscanf(line.c_str(), "UID_MIN %lu", &my_system.uid_min);
        } else if (line.find("UID_MAX") == 0) {
            sscanf(line.c_str(), "UID_MAX %lu", &my_system.uid_max);
        }
    }
   
}


void assignGroupName(uid_t uid, std::string& pid_str,std::string& group_name){
    if(uid < my_system.uid_min || uid>my_system.uid_max){
        group_name = "system_processes";
        }
    else{
        readProcCmd(pid_str, group_name);
    }
}

void scanProcesses(std::unordered_map<std::string, ProcessGroup>& proc_groups) {
    
    std::set<std::string> current_groups;
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;
        
        std::string pid_str = entry.path().filename();
        pid_t curr_pid = 0;
        if(!readProcPid(pid_str ,curr_pid)) continue;
        // current_pids.insert(curr_pid);
        
        Process temp_proc;
        temp_proc.pid = curr_pid;
        uid_t uid =  readProcUid(pid_str);
        std::string group_name;
        assignGroupName(uid, pid_str, group_name);
             
        readProcMem(pid_str, temp_proc);
        readProcCpuTimes(pid_str, temp_proc);

        current_groups.insert(group_name);

        ProcessGroup& group = proc_groups[group_name];
        group.group_name = group_name;
        group.count++;
        group.total_mem_kb += temp_proc.mem_kb;
        group.total_utime += temp_proc.utime;
        group.total_stime += temp_proc.stime;
    }
    // Remove dead processes
    for (auto it = proc_groups.begin(); it != proc_groups.end(); ) {
        if (current_groups.find(it->first) == current_groups.end()) {
            it = proc_groups.erase(it);
        } else {
            ++it;
        }
    }
}


void displaySystemInfo(){
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "System Resources: "<<"RAM: " << my_system.tot_mem_gb << " GB | "
          << my_system.core_number<<" CPU cores\nRAM used:" 
          << my_system.used_mem_percent << "% ("<< my_system.used_mem_gb<<" GB) used \n"
          << "CPU: " << my_system.total_cpu_percent << "%  used\n";
}
/*
void displayProcesses(const std::unordered_map<pid_t, Process>& processes_map) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Found " << processes_map.size() << " processes\n\n";
    
    // Convert map to vector for sorting
    std::vector<const Process*> sorted_processes;
    for (const auto& [pid, proc] : processes_map) {
        sorted_processes.push_back(&proc);
    }
    
    std::sort(sorted_processes.begin(), sorted_processes.end(), //highest Ram usage first
              [](const Process* a, const Process* b) { 
                  return a->mem_kb > b->mem_kb; 
              });
    
    std::cout << std::left 
              << std::setw(8) << "PID" 
              << std::setw(8) << "UID" 
              << std::setw(10) << "RAM(GB)" 
              << std::setw(8) << "CPU%" 
              << "COMMAND\n";
    
    std::cout << std::string(45, '-') << "\n";
    
    int limit = (int)sorted_processes.size();// std::min(15, (int)sorted_processes.size());
    for (int i = 0; i < limit; i++) {
        const Process* p = sorted_processes[i];
        std::cout << std::left 
                  << std::setw(8) << p->pid 
                  << std::setw(8) << p->uid 
                  << std::setw(10) << (((double) p->mem_kb) / (1024.0 * 1024.0)) 
                  << std::setw(8) << p->cpu_percent 
                  << p->cmd << "\n";
        // std::cout << "DEBUG: PID " << p->pid << " mem_kb=" << p->mem_kb 
        //   << " calculated_GB=" << (((double) p->mem_kb) / (1024.0*1024.0)) << "\n";
        //debug
        // debug_used_mem_gb += (((double) p->mem_kb) / (1024.0 * 1024.0));
        // debug_total_cpu_percent += p->cpu_percent;
    }
    // std::cout << " debug_used_mem_gb = "<<debug_used_mem_gb << "    from proc = "<<my_system.used_mem_gb<<std::endl;
    // std::cout << " debug_total_cpu_percent = "<<debug_total_cpu_percent << "    from proc = "<<my_system.total_cpu_percent<<std::endl;

    
    if (processes_map.size() > limit) {
        std::cout << "... and " << (processes_map.size() - limit) << " more processes\n";
    }
}
*/
void displayProcesses(const std::unordered_map<std::string, ProcessGroup>& groups) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Found " << groups.size() << " process groups\n\n";
    
    // Convert to vector and sort by memory usage
    std::vector<const ProcessGroup*> sorted_groups;
    for (const auto& [name, group] : groups) {
        sorted_groups.push_back(&group);
    }
    
    std::sort(sorted_groups.begin(), sorted_groups.end(),
              [](const ProcessGroup* a, const ProcessGroup* b) { 
                  return a->total_mem_kb > b->total_mem_kb; 
              });
    
    std::cout << std::left 
              << std::setw(20) << "GROUP" 
              << std::setw(8) << "COUNT" 
              << std::setw(12) << "RAM(GB)" 
              << std::setw(8) << "CPU%" 
              << "\n";
    
    std::cout << std::string(50, '-') << "\n";
    
    for (const auto* group : sorted_groups) {
        std::cout << std::left 
                  << std::setw(20) << group->group_name.substr(0, 19)
                  << std::setw(8) << group->count 
                  << std::setw(12) << (((double)group->total_mem_kb) / (1024.0 * 1024.0))
                  << std::setw(8) << group->cpu_percent 
                  << "\n";
    }
}
void displayMain(const std::unordered_map<std::string, ProcessGroup>& proc_groups) {
    
    std::cout << "\033[2J\033[1;1H";  // clear screen
    
    displaySystemInfo();
    std::cout << "\n";  // Add spacing
    displayProcesses(proc_groups);
}

int main(){
    std::unordered_map<std::string, ProcessGroup> proc_groups;  
    readSystemInfo();
    
    readTotMem();
    scanProcesses(proc_groups);  
    calculateCpuPercentages(proc_groups);
    sleep(1);
    readTotMem();
    scanProcesses(proc_groups);
    calculateCpuPercentages(proc_groups);
    displayMain(proc_groups);  
    return 0;
}
