#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <glog/logging.h>
#include <glog/raw_logging.h>

#include <fstream>


class GLogger
{
public:
    GLogger(char* program, char* logdir)
    {
        //google::InitGoogleLogging(program);

        FLAGS_logtostderr = false;   //log messages go to stderr instead of logfiles
        FLAGS_alsologtostderr = false;    //log messages go to stderr in addition to logfiles
        FLAGS_colorlogtostderr=true;    //color messages logged to stderr (if supported by terminal)

        FLAGS_log_dir = logdir;   // logger output file
        FLAGS_stderrthreshold=google::INFO; //log messages at or above this level are copied to stderr in "
                                            //addition to logfiles.  This flag obsoletes --alsologtostderr
        FLAGS_stop_logging_if_full_disk = true;   //Stop attempting to log to disk if the disk is full

        //FLAGS_v = 5;		//自定义VLOG(m)时，m值小于此处设置值的语句才有输出
        //FLAGS_max_log_size;     //每个日志文件最大大小（MB级别）
        //FLAGS_minloglevel;       //输出日志的最小级别，即高于等于该级别的日志都将输出。

        google::InitGoogleLogging(program);
        google::InstallFailureSignalHandler();
        //默认捕捉 SIGSEGV 信号信息输出会输出到 stderr，可以通过下面的方法自定义输出方式：
        google::InstallFailureWriter(&SignalHandle);

    }
    ~GLogger()
    {
        google::ShutdownGoogleLogging();
    }

    //将信息输出到单独的文件和 LOG(ERROR)
    static void SignalHandle(const char* data, int size)
    {
        std::ofstream fs("./logs/glog_dump.log",std::ios::app);
        std::string str = std::string(data,size);
        fs<<str;
        fs.close();
        LOG(ERROR)<<str;
    }
};


#endif // LOGGER_HPP
