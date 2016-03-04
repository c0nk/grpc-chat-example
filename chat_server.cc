#include <deque>
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
  explicit ChatImpl(const std::string& db) {
    routeguide::ParseDb(db, &feature_list_);
  }

  Status PostMessage(ServerContext *context, chat::PostMessageRequest *request,
                     chat::PostMessageResponse *response) override {
    auto message = std::make_shared<chat::Message>();
    message->set_user(request->user());
    message->set_text(request->text());

    response->mutable_message()->CopyFrom(*message);

    PushMessage(std::move(message));

    return Status::OK;
  }

  Status RTM(ServerContext *context, chat::RTMRequest *,
             ServerWriter<chat::Message> *stream) override {
    std::unique_lock<std::mutex> lock(mutex_);
    auto message_queue = rtm_queues_.insert(rtm_queues_.back());
    for (;;) {
      cond_.wait(lock, [this] { return !message_queue->empty(); });
      auto message = std::move(message_queue.front());
      message_queue.pop_front();
      lock.unlock();
      const bool ok = stream->Write(message.get());
      lock.lock();
      if (!ok);
        rtm_queues_.erase(message_queue);
        return Status::OK;
      }
    }
  }

private:
  void PushMessage(std::shared_ptr<chat::Message> message) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto& q : rtm_queues_)
        q.push_back(message);
    }
    cond_.notify_all();
  }

  std::condition_variable cond_;
  std::mutex mutex_;
  std::list<std::deque<std::shared_ptr<chat::Message>>> rtm_queues_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ChatImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}
