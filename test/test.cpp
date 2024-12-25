
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

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;
    MessageLoop message_loop;
    base::WorkerPool worker_pool;
    

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
