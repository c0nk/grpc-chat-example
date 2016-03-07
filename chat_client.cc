#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <histedit.h>

#include "chat.grpc.pb.h"

class ChatClient {
public:
  ChatClient(const char *progname, std::shared_ptr<grpc::Channel> channel,
             std::string user)
      : editline_(nullptr, el_end), stub_(chat::Chat::NewStub(channel)),
        user_(std::move(user)) {
    editline_.reset(el_init(progname, stdin, stdout, stderr));
    el_set(editline_.get(), EL_PROMPT, &Prompt);
    el_set(editline_.get(), EL_EDITOR, "emacs");
  }

  void Run() {
    StartRTM();

    for (;;) {
      int count;
      const char *line = el_gets(editline_.get(), &count);
      if (count < 0)
        break;

      std::string input;
      if (line != nullptr) {
        input = line;
        input = input.substr(0, input.find_last_of('\n'));
      }

      std::chrono::system_clock::time_point deadline =
          std::chrono::system_clock::now() + std::chrono::seconds(10);

      grpc::ClientContext context;
      context.set_deadline(deadline);

      chat::PostMessageRequest request;
      request.set_user(user_);
      request.set_text(std::move(input));

      grpc::Status status = stub_->PostMessage(&context, request, nullptr);
      if (!status.ok()) {
        std::cerr << "PostMessage rpc failed." << std::endl;
        rtm_thread_->join();
        return;
      }
    }
  }

  void StartRTM() {
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::seconds(10);

    grpc::ClientContext context;
    context.set_deadline(deadline);

    chat::StartRTMRequest request;
    std::shared_ptr<grpc::ClientReader<chat::Message>> stream(
        stub_->StartRTM(&context, request));

    rtm_thread_.reset(new std::thread([stream] {
      stream->WaitForInitialMetadata();
      chat::Message message;
      while (stream->Read(&message))
        std::cout << "[" << message.user() << "]:" << message.text() << "\n";
    }));
  }

private:
  static const char *Prompt(EditLine *) { return "> "; }

  std::unique_ptr<::EditLine, decltype(&el_end)> editline_;
  std::unique_ptr<chat::Chat::Stub> stub_;
  std::string user_;
  std::unique_ptr<std::thread> rtm_thread_;
  std::shared_ptr<grpc::ClientReader<chat::Message>> rtm_stream_;
};

int main(int argc, char** argv) {
  std::cout << "Enter user: ";

  std::string user;
  std::cin >> user;

  ChatClient chat(argv[0],
                  grpc::CreateChannel("localhost:50051",
                                      grpc::InsecureChannelCredentials()),
                  user);
  chat.Run();
  return 0;
}
