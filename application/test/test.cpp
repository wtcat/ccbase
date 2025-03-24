
#include <assert.h>
#include <iostream>
#include <type_traits>

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/callback.h"
#include "base/bind.h"
#include "base/timer.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/file_util_proxy.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/worker_pool.h"

//#include "spdlog/spdlog.h"
//#include "spdlog/sinks/basic_file_sink.h"
//
//#include "leveldb/db.h"

#include <opencv2/opencv.hpp>


#pragma warning(disable:4251)

//using file_proxy = base::FileUtilProxy;

namespace {
void Hello() {
    std::cout << "hello,demo!\n";
}

void TestWorker() {
    std::cout << "hello,everyone!\n";
}

int GetResult() {
    return 0x123456;
}

void ProcessResult(int result) {
    printf("Result: 0x%x\n", result);
}

void ReadCB(base::PlatformFileError err, const char* buf, int len) {
    printf("ReadCB: %s len(%d)\n", buf, len);
}

void CreateCB(base::PlatformFileError err, base::PassPlatformFile file, bool created) {
    //if (err == 0) {
    //    base::PlatformFile handle = file.ReleaseValue();
    //    base::FileUtilProxy::Read(base::ThreadTaskRunnerHandle::Get().get(),
    //        handle, 0, 11, base::Bind(ReadCB));
    //}
}

void Timout() {
    //file_util::WriteFile(FilePath(FILE_PATH_LITERAL("test.txt")), "Hello World", 11);
    //base::FileUtilProxy::CreateOrOpen(base::ThreadTaskRunnerHandle::Get().get(),
    //    FilePath(FILE_PATH_LITERAL("test.txt")), base::PLATFORM_FILE_OPEN,
    //    base::Bind(CreateCB));

    LOG(INFO) << "Time out";
}
} // namespace

//std::is_integral<N>::value && 

template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
auto add_general(T a, T b) {
    return a + b;
}

constexpr int Return5(int v) { return v; }


template<typename... Args>
constexpr auto AddSum(Args&&... args) {
    return (args + ...);
}

template<typename Type, size_t Size>
class TestContainer {
public:

    template<typename... Args>
    constexpr TestContainer(Args... args) : buf_(args...) {}

    class Iterator {
    public:
        Iterator(Type* ptr) : ptr_(ptr) {}
        Type& operator*() const {
            return *ptr_;
        }
        Iterator& operator++() {
            ++ptr_;
            return *this;
        }
        Iterator& operator++(int) {
            Iterator temp = *this;
            ++ptr_;
            return temp;
        }
        bool operator==(const Iterator& iter) {
            return ptr_ == iter.ptr_;
        }
        bool operator!=(const Iterator& iter) {
            return ptr_ != iter.ptr_;
        }
    private:
        Type* ptr_;
    };

    Iterator begin() {
        return Iterator(buf_);
    }
    Iterator end() {
        return Iterator(buf_ + Size);
    }

    Type& operator[](size_t index) {
        return buf_[index];
    }

private:
    Type buf_[Size];
};


int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    MessageLoop message_loop;
    base::WorkerPool worker_pool;


    TestContainer<int, 3> array = { 1, 2, 3 };
    for (auto iter : array) {
    }

    AddSum(1, 2, 3, 4, 5, 6, 7);

    //OpenCV
    cv::Mat img = cv::imread("HeartRatepx.png", cv::IMREAD_UNCHANGED);
    printf("w(%d) h(%d) channels(%d) elesize(%d)\n",
        img.cols,
        img.rows,
        img.channels(),
        (int)img.elemSize()
    );

    //cv::Mat oimg(img.size(), img.type());
    cv::Mat oimg = img.clone();

    if (oimg.channels() == 4) {
        for (int y = 0; y < oimg.rows; y++) {
            uchar* pixel_ptr = oimg.ptr<uchar>(y);
            for (int x = 0; x < oimg.cols * oimg.channels(); x+=4) {
                //pixel_ptr[x + 0] =  255 - pixel_ptr[0]; //Alpha
                //pixel_ptr[x + 1] =  255 - pixel_ptr[1]; //B
                //pixel_ptr[x + 2] =  255 - pixel_ptr[2]; //G
                pixel_ptr[x + 3] = 255;// 255 - pixel_ptr[3]; //R
            }
        }

        //cv::imwrite("HR.png", img);
    }

    cv::imshow("HR", oimg);
    cv::waitKey();

    //Log


    base::Callback func_cb = base::Bind(&Return5);
    func_cb.Run(0);

    add_general(1, 2);
    // add_general("str", 2);
#if 0
    auto logger = spdlog::basic_logger_mt("F", "logs/basic-log.txt");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Hello world\n");

    //LevelDB
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "testdb.db", &db);
    if (!status.ok())
        return -1;
    db->Put(leveldb::WriteOptions(), leveldb::Slice("hello"), leveldb::Slice("Everyone"));
    delete db;
#endif

    base::Thread runner("runner-1");
    runner.Start();
    
    MessageLoop* runner_loop = runner.message_loop();

    base::Timer timer(FROM_HERE, base::TimeDelta::FromSeconds(1), 
        base::Bind(&Timout), false);
    timer.Reset();
    //timer.Start(FROM_HERE, base::TimeDelta::FromSeconds(1), base::Bind(&Timout));

    worker_pool.PostTask(FROM_HERE, base::Bind(&TestWorker), true);
    //worker_pool.PostTaskAndReply(FROM_HERE, base::Bind(&TestWorker), base::Bind(&GetResult), false);

    runner_loop->PostTask(FROM_HERE, base::Bind(&Hello));
    message_loop.PostTask(FROM_HERE, base::Bind(&Hello));
    message_loop.Run();
    runner.StopSoon();

    return 0;
}
