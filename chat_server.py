#!/usr/bin/env python

"""The Python implementation of the gRPC chat server."""

from threading import Lock
import Queue
import time

import chat_pb2

_ONE_DAY_IN_SECONDS = 60 * 60 * 24

class ChatServicer(chat_pb2.BetaChatServicer):
  """Provides methods that implement functionality of chat server."""

  def __init__(self):
    self.rtm_queues = []
    self.rtm_queues_lock = Lock()

  def stop_rtm(self):
    with self.rtm_queues_lock:
      for message_queue in self.rtm_queues:
        message_queue.put(None)

  def PostMessage(self, request, context):
    print "PM: [" + request.user + "]: " + request.text
    message = chat_pb2.Message(user=request.user, text=request.text)
    with self.rtm_queues_lock:
      for message_queue in self.rtm_queues:
        message_queue.put(message)
    return chat_pb2.PostMessageResponse(message=message)

  def StartRTM(self, request, context):
    message_queue = Queue.Queue()
    with self.rtm_queues_lock:
      self.rtm_queues.append(message_queue)
    while True:
      message = message_queue.get()
      if message is None:
        break
      yield message
    with self.rtm_queues_lock:
      self.rtm_queues.remove(message_queue)

def serve():
  servicer = ChatServicer()

  server_address = '[::]:50051'
  server = chat_pb2.beta_create_Chat_server(servicer)
  server.add_insecure_port(server_address)
  server.start()
  print "Server listening on " + server_address
  try:
    while True:
      time.sleep(_ONE_DAY_IN_SECONDS)
  except KeyboardInterrupt:
    servicer.stop_rtm()
    server.stop(0)

if __name__ == '__main__':
  serve()
