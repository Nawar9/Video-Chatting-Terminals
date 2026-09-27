#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <jpeglib.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>


enum Commands {
    TEST = 1,
    SET_OUTPUT,
    SET_MODE,
    SET_WIDTH,
    SET_HEIGHT,
    SET_NUM_COLS,
    SET_NUM_ROWS,
    CAPTURE_FRAME,
    ACK = 0xFE,
    RESET = 0xFF,
};

enum Output {
    PC,
    TERMINAL,
};

enum Mode {
    PIC,
    CAM,
};


const char* VIDEO_DEVICE = "/dev/video0";
const char* SERIAL_DEVICE = "/dev/ttyUSB1";
const char* SERIAL_DEVICE_1 = "/dev/ttyUSB0";
const speed_t BAUD_RATE = B1000000;

uint8_t output = PC;
uint8_t mode = PIC;

int fd_cam = -1;
int fd_serial= -1;

uint16_t pic_width = 0;
uint16_t pic_height = 0;

uint16_t frame_width = 0;
uint16_t frame_height = 0;

uint16_t num_cols = 0;
uint16_t num_rows = 0;

uint16_t pic_num_accumulative_cols = 0;
uint16_t pic_num_accumulative_rows = 0;

uint16_t frame_num_accumulative_cols = 0;
uint16_t frame_num_accumulative_rows = 0;

uint8_t* pic_buffer = NULL;

struct v4l2_buffer buf;
uint8_t* frame_buffer = NULL;


#define BUFFER_SIZE 1024

typedef struct {
    uint8_t data[BUFFER_SIZE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} CircularBuffer;

CircularBuffer receive_buffer;


void enqueue(uint8_t data) {
    pthread_mutex_lock(&receive_buffer.mutex);
    if (receive_buffer.size >= BUFFER_SIZE) {
        pthread_cond_wait(&receive_buffer.cond, &receive_buffer.mutex);
    }

    receive_buffer.rear = (receive_buffer.rear + 1) % BUFFER_SIZE;
    receive_buffer.data[receive_buffer.rear] = data;
    receive_buffer.size++;

    pthread_cond_signal(&receive_buffer.cond);
    pthread_mutex_unlock(&receive_buffer.mutex);
}

uint8_t dequeue() {
    pthread_mutex_lock(&receive_buffer.mutex);
    if (receive_buffer.size <= 0) {
        pthread_cond_wait(&receive_buffer.cond, &receive_buffer.mutex);
    }

    uint8_t data = receive_buffer.data[receive_buffer.front];
    receive_buffer.front = (receive_buffer.front + 1) % BUFFER_SIZE;
    receive_buffer.size--;

    pthread_cond_signal(&receive_buffer.cond);
    pthread_mutex_unlock(&receive_buffer.mutex);
    return data;
}

void* receive_thread(void* arg) {
    while(1) {
        uint8_t byte;
        if (read(fd_serial, &byte, 1) == 1) {
            enqueue(byte);
        }
    }
    return NULL;
}

void init_receiving_thread() {
    receive_buffer.front = 0;
    receive_buffer.rear = -1;
    receive_buffer.size = 0;
    pthread_mutex_init(&receive_buffer.mutex, NULL);
    pthread_cond_init(&receive_buffer.cond, NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, receive_thread, NULL);
}

void destroy_buffers() {
    pthread_mutex_destroy(&receive_buffer.mutex);
    pthread_cond_destroy(&receive_buffer.cond);
    
    free(pic_buffer);
}

void open_camera_device() {
    fd_cam = open(VIDEO_DEVICE, O_RDWR);
    if (fd_cam == -1) {
        perror("Error opening camera device");
        exit(EXIT_FAILURE);
    }
}

void request_buffer() {
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_cam, VIDIOC_REQBUFS, &req) == -1) {
        perror("Error requesting buffer");
        exit(EXIT_FAILURE);
    }
}

void set_camera_format() {
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = frame_width;
    fmt.fmt.pix.height = frame_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if (ioctl(fd_cam, VIDIOC_S_FMT, &fmt) == -1) {
        perror("Error setting camera format");
        exit(EXIT_FAILURE);
    }
}

void query_buffer() {
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd_cam, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("Error querying buffer");
        exit(EXIT_FAILURE);
    }
}

void map_buffer_into_memory() {
    frame_buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_cam, buf.m.offset);
    if (frame_buffer == MAP_FAILED) {
        perror("Error mapping buffer");
        exit(EXIT_FAILURE);
    }
}

void queue_buffer() {
    if (ioctl(fd_cam, VIDIOC_QBUF, &buf) == -1) {
        perror("Error queueing buffer");
        exit(EXIT_FAILURE);
    }
}

void dequeue_buffer() {
    if (ioctl(fd_cam, VIDIOC_DQBUF, &buf) == -1) {
        perror("Error dequeueing buffer");
        exit(EXIT_FAILURE);
    }
}

void start_streaming() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_cam, VIDIOC_STREAMON, &type) == -1) {
        perror("Error starting stream");
        exit(EXIT_FAILURE);
    }
}

void stop_streaming() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_cam, VIDIOC_STREAMOFF, &type) == -1) {
        perror("Error stopping stream");
        exit(EXIT_FAILURE);
    }
}

void init_camera() {
    open_camera_device();
    set_camera_format();
    request_buffer();
    query_buffer();
    map_buffer_into_memory();
    queue_buffer();
    start_streaming();
}

void close_camera() {
    stop_streaming();
    munmap(frame_buffer, buf.length);
    close(fd_cam);
}

void open_serial_port() {
    fd_serial = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY);
    if (fd_serial == -1) {
        fd_serial = open(SERIAL_DEVICE_1, O_RDWR | O_NOCTTY);
        if (fd_serial == -1) {
            perror("Error opening serial port");
            exit(EXIT_FAILURE);
        }
    }
}

void configure_serial_port(speed_t baud_rate) {
    struct termios options;
    tcgetattr(fd_serial, &options);
    cfsetospeed(&options, baud_rate);
    cfsetispeed(&options, baud_rate);
    options.c_cflag |= (CLOCAL | CREAD);                    // Enable receiver and set local mode
    options.c_cflag &= ~PARENB;                             // Disable parity
    options.c_cflag &= ~CSTOPB;                             // Set one stop bit
    options.c_cflag &= ~CSIZE;                              // Mask the character size bits
    options.c_cflag |= CS8;                                 // Set 8 data bits
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);     // Set raw mode
    options.c_iflag &= ~(INPCK | ISTRIP);                   // Disable parity checking
    options.c_oflag &= ~OPOST;                              // Disable output processing

    if (tcsetattr(fd_serial, TCSANOW, &options) != 0) {
        perror("Error configuring serial port");
        exit(EXIT_FAILURE);
    }
}

void init_serial(speed_t baud_rate) {
    open_serial_port();
    configure_serial_port(baud_rate);
}

int send_byte(uint8_t byte) {
    if (write(fd_serial, &byte, 1) != 1) {
        perror("Error writing byte");
        return -1;
    }
    return 0;
}

uint8_t receive_byte() {
    return dequeue();
}

void close_serial() {
    close(fd_serial);
}

uint8_t *read_jpeg(const char* filename, uint16_t* width, uint16_t* height) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* infile;
    uint8_t* image;
    JSAMPARRAY buffer;
    int row_stride;

    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Can't open input file\n");
        exit(1);
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;

    row_stride = cinfo.output_width * cinfo.output_components;
    image = (unsigned char *)malloc(row_stride * cinfo.output_height);
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(image + cinfo.output_scanline * row_stride, buffer[0], row_stride);
    }

    fclose(infile);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return image;
}

void send_pixel_jpeg(uint16_t x, uint16_t y) {
    size_t index = (y * pic_num_accumulative_rows * pic_width + x * pic_num_accumulative_cols + (pic_num_accumulative_rows / 2) * pic_width + pic_num_accumulative_cols / 2) * 3;
    send_byte((pic_buffer[index] + pic_buffer[index + 1] + pic_buffer[index + 2]) / 3);
}

void send_pixel_yuv(uint16_t x, uint16_t y) {
    size_t index = (y * frame_num_accumulative_rows * frame_width + x * frame_num_accumulative_cols + (frame_num_accumulative_rows / 2) * frame_width + frame_num_accumulative_cols / 2) * 2;
    send_byte(frame_buffer[index]);
}

void send_frame() {
    if (output == PC) {
        printf("\033[H");
    }

    if (mode == CAM) {
        dequeue_buffer();
        queue_buffer();
    }

    for (uint16_t y = 0; y < num_rows; y++) {
        for (uint16_t x = 0; x < num_cols; x++) {
            if (mode == PIC) {
                send_pixel_jpeg(x, y);
            } else if (mode == CAM) {
                send_pixel_yuv(num_cols - x, y);
            }

            uint8_t response = receive_byte();
            if (response == RESET) {
                printf("\033c");
                return;
            }

            if (output == PC) {
                printf("%c", response);
                fflush(stdout);
            } else if (output == TERMINAL) {
                if (response != ACK) {
                    fprintf(stderr, "No ACK byte\n");
                    return;
                }
            }
        }

        if (output == PC && y < num_rows - 1) {
            printf("\n");
        }
    }
}

void handle_command(uint8_t cmd) {
    switch (cmd) {
        case TEST:
            printf("Test!\n");
            uint8_t test_byte = receive_byte();
            printf("Test data: %d\n", test_byte);
            send_byte(test_byte);
            break;

        case SET_OUTPUT:
            printf("Set output!\n");
            output = receive_byte();
            printf("Output: %d\n", output);
            break;

        case SET_MODE:
            printf("Set mode!\n");
            mode = receive_byte();
            printf("Mode: %d\n", mode);
            break;

        case SET_WIDTH:
            printf("Set frame width!\n");
            frame_width = (uint16_t)receive_byte() << 8 | receive_byte();
            if (num_cols != 0) {
                frame_num_accumulative_cols = frame_width / num_cols;
            }
            printf("Frame width: %d\n", frame_width);
            break;

        case SET_HEIGHT:
            printf("Set height!\n");
            frame_height= (uint16_t)receive_byte() << 8 | receive_byte();
            if (num_rows != 0) {
                frame_num_accumulative_rows = frame_height / num_rows;
            }
            printf("Frame height: %d\n", frame_height);
            break;

        case SET_NUM_COLS:
            printf("Set number of columns!\n");
            num_cols = (uint16_t)receive_byte() << 8 | receive_byte();
            if (num_cols != 0) {
                frame_num_accumulative_cols = frame_width / num_cols;
                pic_num_accumulative_cols = pic_width / num_cols;
            }
            printf("Number of columns: %d\n", num_cols);
            break;

        case SET_NUM_ROWS:
            printf("Set number of rows!\n");
            num_rows = (uint16_t)receive_byte() << 8 | receive_byte();
            if (num_rows != 0) {
                frame_num_accumulative_rows = frame_height / num_rows;
                pic_num_accumulative_rows = pic_height / num_rows;
            }
            printf("Number of rows: %d\n", num_rows);
            break;

        case CAPTURE_FRAME:
            send_frame();
            break;
    }
}

int main() {
    init_camera();
    init_serial(BAUD_RATE);
    init_receiving_thread();
    pic_buffer = read_jpeg("./pic.jpg", &pic_width, &pic_height);

    printf("Start!\n");

    while(1) {
        handle_command(receive_byte());
    }

    close_camera();
    close_serial();
    destroy_buffers();

    return 0;
}