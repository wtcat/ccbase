
#include <assert.h>
#include <iostream>

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

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "leveldb/db.h"

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

template<
    typename K, 
    typename V, 
    int N,
    typename std::enable_if<std::is_integral<K>::value, void>::type* = nullptr>
class Hash {
public:
    enum { HASK_DEPTH = 4 };
    template<typename Key, typename Value>
    struct HashNode {
        Key    key;
        Value  value;
        bool   valid;
    };

    Hash() : hash_mask_(N / HASK_DEPTH - 1) {}
    int key(K k) const {
        return (k & hash_mask_) * HASK_DEPTH;
    }
    bool Insert(K k, V v) {
        int index = key(k);
        for (int i = 0; i < HASK_DEPTH; i++, index++) {
            if (!hash_nodes_[index].valid) {
                hash_nodes_[index].key   = k;
                hash_nodes_[index].value = v;
                hash_nodes_[index].valid = true;
                return true;
            }
        }
        return false;
    }
    HashNode<K, V> *Find(K k) {
        int index = key(k);
        for (int i = 0; i < HASK_DEPTH; i++, index++) {
            if (hash_nodes_[index].valid && hash_nodes_[index].key == k)
                return &hash_nodes_[index];
        }
        return nullptr;
    }

private:
    HashNode<K, V> hash_nodes_[N] = {};
    const int hash_mask_;
};




int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    MessageLoop message_loop;
    base::WorkerPool worker_pool;
    int key_array[] = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,  61, 62, 63, 64, 65, 66 };
    int val_array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  11, 11, 12, 13, 14, 15 };
    Hash<int, int, 16> hash;

    for (int i = 0; i < 16; i++) {
        hash.Insert(key_array[i], val_array[i]);
    }
    for (int i = 0; i < 16; i++) {
        auto node = hash.Find(key_array[i]);
        assert(node != nullptr);
        assert(val_array[i] == node->value);
    }

    //OpenCV
    cv::Mat img = cv::imread("HeartRatepx.png", cv::IMREAD_UNCHANGED);
    printf("w(%d) h(%d) channels(%d) elesize(%d)\n",
        img.cols,
        img.rows,
        img.channels(),
        img.elemSize()
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
