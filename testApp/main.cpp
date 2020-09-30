#include <iostream>
#include <vector>
#include <list>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

using namespace std;

using Data = vector<int>;         // raw data
using Task = function<int(int)> ; // task type F signature with bound 2nd argument
using TasksQueue = list<Task>;    // tasks queue type
static TasksQueue tq;

static std::mutex mtx_task;              // mutes for task queue access
static std::condition_variable cv_task;  // notifucation about vhanging task queue

static int sum{0}, cnt{0};// vatiables for sum & count of processed number of items

// use this function for count summ
int F(int a, int b) {
     return a+b;
}

// data producer thread
void Producer(Data&& src){

    auto addItem = [](int v){
        lock_guard<mutex> lk(mtx_task);
        //q.push_front(v);
        tq.push_back(bind(F,std::placeholders::_1, v));

        //cout << "push "<< v << endl <<flush;
        cv_task.notify_one();
    };

    for(int v: src)
    {
        addItem(v);
    }
}

// data customer thread
void Customer(int expected)
{
    while(true) //  cycle for process new items from producer
    {
        unique_lock<mutex> lk(mtx_task);
        cv_task.wait(lk, [expected](){return expected !=0 && cnt <= expected;}); // wait event to wake up thread

        {
            if(cnt == expected) // exit from thread of all items are handled
            {
                lk.unlock();
                return;
            }

            if(!tq.empty()) // check existing items in queue
            {
                auto fv = tq.back();
                tq.pop_back();
                cnt++;
                //cout << "pop "<< ", cnt "<<cnt << endl <<flush;
                sum = fv(sum); // call function from task list
            }
        }

        lk.unlock();
    }
}




int main()
{
    const uint CNT_THREADS{3};    // configue number of threads
    Data data{1,2,3,4,5,6,7,8,9,9}; //sum 54 avg 5.4

    if(data.empty()) // just additional check for data
    {
        cout << "can't calculate average value for empty data set" << endl << flush;
        return 0;
    }

    // init thread pool
    vector<thread> work_threads(CNT_THREADS);
    for(uint i=0; i<CNT_THREADS;i++)
        work_threads[i] = thread(Customer, data.size());

    // init producer thread
    thread src_thread(Producer, move(data));
    src_thread.join();

    // waiting until all customer threads completes their job
    for(uint i=0; i<CNT_THREADS;i++)
        work_threads[i].join();

    //  calculate and show result;
    cout << "avg="<< double(sum)/cnt << endl << flush;
    return 0;
}
