#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>


const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 360;

const char* VIDEO_DEVICE = "/dev/video0";

const char ascii_chars[] = "@#W$9876543210?!abc;:+=-,,...                  ";


void process_frame(unsigned char *frame_data) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    const int num_cols = w.ws_col;
    const int num_rows = w.ws_row;
    const int num_accumulative_cols = FRAME_WIDTH / num_cols;
    const int num_accumulative_rows = FRAME_HEIGHT / num_rows;

    for (int y = 0; y < num_rows; y++) {
        for (int x = num_cols - 1; x >= 0; x--) {
            unsigned int brightness = 0;
            for (int z = 0; z < num_accumulative_rows; z++) {
                for (int w = num_accumulative_cols - 1; w >= 0; w--) {
                    int index = (y * num_accumulative_rows * FRAME_WIDTH + x * num_accumulative_cols + z * FRAME_WIDTH + w) * 2;
                    brightness += frame_data[index];
                }
            }
            brightness = brightness * (sizeof(ascii_chars) / sizeof(ascii_chars[0]) - 1) / (255 * num_accumulative_cols * num_accumulative_rows);
            printf("%c", ascii_chars[brightness]);
        }
        if (y < num_rows - 1) {
            printf("\n");
        }
    }
    
    printf("\033[H");
}

int main() {
    int fd = open(VIDEO_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Error opening camera device");
        exit(EXIT_FAILURE);
    }

    // Set camera format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = FRAME_WIDTH;
    fmt.fmt.pix.height = FRAME_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("Error setting camera format");
        exit(EXIT_FAILURE);
    }

    // Request a single buffer for capturing
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("Error requesting buffer");
        exit(EXIT_FAILURE);
    }

    // Query buffer information
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("Error querying buffer");
        exit(EXIT_FAILURE);
    }

    // Map the buffer into memory
    void *buffer_start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer_start == MAP_FAILED) {
        perror("Error mapping buffer");
        exit(EXIT_FAILURE);
    }

    // Queue the buffer for capturing
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        perror("Error queueing buffer");
        exit(EXIT_FAILURE);
    }

    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Error starting stream");
        exit(EXIT_FAILURE);
    }

    while(1) {
        // Dequeue the buffer for capturing
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("Error dequeueing buffer");
            exit(EXIT_FAILURE);
        }

        // Process the captured frame
        process_frame((unsigned char *)buffer_start);

        // Queue the buffer again for capturing
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Error queueing buffer");
            exit(EXIT_FAILURE);
        }
    }

    // Stop streaming
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("Error stopping stream");
        exit(EXIT_FAILURE);
    }

    // Unmap buffer and close device
    munmap(buffer_start, buf.length);
    close(fd);

    return 0;
}