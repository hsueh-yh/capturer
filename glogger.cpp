#include "glogger.h"

//将信息输出到单独的文件和 LOG(ERROR)
static void SignalHandle(const char *data, int size)
{
    std::string dumpPath = "./logs/glog_dump.log";
    std::ofstream fs(dumpPath.c_str(),std::ios::app);
    std::string str = std::string(data,size);
    fs<<str;
    fs.close();
    LOG(ERROR)<<str;
}

GLogger::GLogger( const char *program, const char *logdir):
        logdir_(logdir)
{
    //google::InitGoogleLogging(program);

    FLAGS_logtostderr = false;      //log messages go to stderr instead of logfiles
    FLAGS_alsologtostderr = true;    //log messages go to stderr in addition to logfiles
    FLAGS_colorlogtostderr = true;    //color messages logged to stderr (if supported by terminal)
    FLAGS_v = 255;
    logdir_ = logdir;
    FLAGS_log_dir = logdir;   // logger output file
    //google::SetLogDestination(google::INFO,logdir);
    //google::SetLogDestination(google::WARNING,logdir);
    //google::SetLogDestination(google::GLOG_ERROR,logdir);
    FLAGS_minloglevel = google::INFO;   //Messages logged at a lower level than this don't
                                        //actually get logged anywhere
    FLAGS_stop_logging_if_full_disk = true;   //Stop attempting to log to disk if the disk is full
    FLAGS_logbufsecs = 60;

    //FLAGS_v = 5;		//自定义VLOG(m)时，m值小于此处设置值的语句才有输出
    //FLAGS_max_log_size;     //每个日志文件最大大小（MB级别）
    //FLAGS_minloglevel;       //输出日志的最小级别，即高于等于该级别的日志都将输出。

    google::InitGoogleLogging(program);
    google::InstallFailureSignalHandler();
    //默认捕捉 SIGSEGV 信号信息输出会输出到 stderr，可以通过下面的方法自定义输出方式：
    google::InstallFailureWriter(&SignalHandle);

}
GLogger::~GLogger()
{
    google::ShutdownGoogleLogging();
}
