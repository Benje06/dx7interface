#include "logger.h"

/*** LOG MANAGER ***/
void Logger::set_log_level(unsigned int lvl){
    std::lock_guard<std::mutex> lock(log_mutex);
    log_level = lvl;
};

void Logger::log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if( log_level == 0){
        return;
    }else if( log_level >= 1){
        if( log_file.is_open() ){
            log_file << msg << std::endl;
        };
        if( log_level >= 2){
            std::cout << msg << std::endl;
        };
    }
};
void Logger::log_error(const std::string& err_msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if( log_level == 0){
        return;
    }else if( log_level >= 1){
        if( log_file.is_open() ){
            log_file << "            " << std::endl;
            log_file << "*************************** ERROR ***************************** " << '\r';
            log_file << "\t" << err_msg << '\r';
            log_file << "*************************************************************** " << '\r';
            log_file << "            " << '\r';
        };
        if( log_level >= 2){
            std::cout <<  "*** ERROR *** " << err_msg << std::endl;
        };
    };
};
void Logger::flush() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()){
        log_file.flush();
    };
    std::cout.flush();
};

/*** LOG MANAGER ***/
LogManager& LogManager::instance() {
    static LogManager instance;
    return instance;
};

LogManager::LogManager() = default;
LogManager::~LogManager() = default;

void LogManager::add_handler(std::shared_ptr<Logger> handler) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    handlers.push_back(handler);
};

void LogManager::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    for (auto& handler : handlers) {
        handler->log(message);
        handler->flush();
    };
};
void LogManager::log_error(const std::string& message) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    for (auto& handler : handlers) {
        handler->log_error(message);
        handler->flush();
    };
};