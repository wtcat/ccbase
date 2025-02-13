
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

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "leveldb/db.h"


#pragma warning(disable:4251)

//using file_proxy = base::FileUtilProxy;

namespace storage {

template<typename T, int N, typename std::enable_if<N <= 65535, void>::type* = nullptr>
class StorageList {
public:
    using NodeType  = unsigned short;
    using BitVector = unsigned long;
    enum {
        kInvalidNode = (1 << sizeof(NodeType)*8) - 1,
        kBitmapUnitSize  = sizeof(BitVector) * 8,
        kBitmapVectorSize = N / kBitmapUnitSize + 1
    };

    template<typename ValueType>
    struct Node {
        ValueType data;
        NodeType next;
        NodeType prev;

        Node(): next(kInvalidNode), prev(kInvalidNode) {}
        const ValueType* value() const {
            return &data;
        }
        ValueType *value() {
            return &data;
        }
    };

    int Allocate() {
        int node;

        // Allocate node ID from bitmap
        for (int i = 0; i < kBitmapVectorSize; i++) {
            BitVector bitmap_mask = bitmap_[i];
            if (bitmap_mask) {
                node = ffs(bitmap_mask) - 1;

                /* If the node id large than the max limit then break loop */
                if (node >= N)
                    break;

                bitmap_[i] |= 0x1u << node;
                return node;
            }
        }

        // If has no free node, we should use the oldest node
        node = first_;
        RemoveInternal(node);
        assert(node < N);
        return node;
    }

    void Append(int node) {
        Node<T>* pnode = nodes_ + node;

        //If the list is empty
        if (last_ == kInvalidNode) {
            first_ = last_ = node;
            return;
        }

        //Append to the list tail
        nodes_[last_].next = node;
        pnode->prev = last_;
        pnode->next = kInvalidNode;
        last_ = node;
    }

    void Remove(int node) {
        bitmap_[node / kBitmapUnitSize] &= ~(0x1 << node);
        RemoveInternal(node);
    }
    
private:
    void RemoveInternal(int node) {
        Node<T>* pnode = nodes_ + node;

        // Remove node
        if (pnode->next != kInvalidNode)
            nodes_[pnode->next].prev = pnode->prev;
        if (pnode->prev != kInvalidNode)
            nodes_[pnode->prev].next = pnode->next;

        // Fix the first and last pointer
        if (first_ == node)
            first_ = pnode->next;
        if (last_ == node)
            last_ = pnode->prev;

        // Clear node
        pnode->next = kInvalidNode;
        pnode->prev = kInvalidNode;
    }

private:
    BitVector bitmap_[kBitmapVectorSize] = {0};
    Node<T>   nodes_[N];
    NodeType  first_{kInvalidNode};
    NodeType  last_{kInvalidNode};
};



} //namespace storage


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

template<typename T, typename std::enable_if<std::is_integral<T>::value, T>::type* = nullptr>
auto add_general(T a, T b) {
    return a + b;
}

constexpr int Return5(int v) { return v; }

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    MessageLoop message_loop;
    base::WorkerPool worker_pool;

    storage::StorageList<char, 10> list;

    base::Callback func_cb = base::Bind(&Return5);
    func_cb.Run(0);

    add_general(1, 2);
    // add_general("str", 2);

    auto logger = spdlog::basic_logger_mt("F", "logs/basic-log.txt");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Hello world\n");

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
