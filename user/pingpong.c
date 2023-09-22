#include "user/user.h"

struct pipe {
  int input;
  int output;
};

struct pipe pipe_new() {
  int channel[2] = {0, 0};
  if (pipe(channel) != 0) {
    fprintf(2, "error: pipe failed\n");
    exit(1);
  }
  return (struct pipe){
      .input = channel[0],
      .output = channel[1],
  };
}

void process_child(int input, int output) {
  char response[5] = {0, 0, 0, 0, 0}; // NOLINT
  if (read(input, response, sizeof(response) - 1) != sizeof(response) - 1) {
    fprintf(2, "error: read failed\n");
    exit(3);
  }

  printf("%d: got %s\n", getpid(), response);

  char request[4] = "pong";
  if (write(output, request, sizeof(request)) != sizeof(request)) {
    fprintf(2, "error: write failed\n");
    exit(3);
  }
}

void process_parent(int input, int output) {
  char request[4] = "ping";
  if (write(output, request, sizeof(request)) != sizeof(request)) {
    fprintf(2, "error: write failed\n");
    exit(3);
  }

  char response[5] = {0, 0, 0, 0, 0}; // NOLINT
  if (read(input, response, sizeof(response) - 1) != sizeof(response) - 1) {
    fprintf(2, "error: read failed\n");
    exit(3);
  }

  printf("%d: got %s\n", getpid(), response);
}

int main() {
  const struct pipe parent_pipe = pipe_new();
  const struct pipe child_pipe = pipe_new();

  const int pid = fork();
  if (pid < 0) {
    fprintf(2, "error: fork failed\n");
    exit(2);
  } else if (pid == 0) {
    process_child(parent_pipe.input, child_pipe.output);
  } else {
    process_parent(child_pipe.input, parent_pipe.output);
  }

  exit(0);
}
