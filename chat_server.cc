#include <deque>
#include <iostream>
#include <list>
#include <memory>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "chat.grpc.pb.h"

class ChatImpl final : public chat::Chat::Service {
public:
  grpc::Status PostMessage(grpc::ServerContext *context,
                           const chat::PostMessageRequest *request,
                           chat::PostMessageResponse *response) override {
    std::cout << "PM: " << request->user() << ">" << request->text() << std::endl;
    auto message = std::make_shared<chat::Message>();
    message->set_user(request->user());
    message->set_text(request->text());

    response->mutable_message()->CopyFrom(*message);

    PushMessage(std::move(message));

    return grpc::Status::OK;
  }

  grpc::Status StartRTM(grpc::ServerContext *context,
                        const chat::StartRTMRequest *,
                        grpc::ServerWriter<chat::Message> *stream) override {
    std::unique_lock<std::mutex> lock(mutex_);
    auto message_queue = rtm_queues_.emplace(rtm_queues_.end());
    for (;;) {
      cond_.wait(lock, [=] { return !message_queue->empty(); });
      while (!message_queue->empty()) {
        auto message = std::move(message_queue->front());
        message_queue->pop_front();
        lock.unlock();
        const bool ok = stream->Write(*message);
        lock.lock();
        if (!ok) {
          rtm_queues_.erase(message_queue);
          return grpc::Status::OK;
        }
      }
    }
  }

private:
  void PushMessage(std::shared_ptr<const chat::Message> message) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto &message_queue : rtm_queues_)
        message_queue.push_back(message);
    }
    cond_.notify_all();
  }

  std::condition_variable cond_;
  std::mutex mutex_;
  std::list<std::deque<std::shared_ptr<const chat::Message>>> rtm_queues_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ChatImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}
