/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <opencv2/opencv.hpp>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <thread>
#include "NvEglRenderer.h"
#include "NvUtils.h"
#include "NvCudaProc.h"
#include "nvbuf_utils.h"


#include "camera_v4l2_cuda.h"

#define MJPEG_EOS_SEARCH_SIZE 4096

static bool quit = false;

using namespace std;

static void
print_usage(void)
{
    printf("\n\tUsage: camera_v4l2_cuda [OPTIONS]\n\n"
           "\tExample: \n"
           "\t./camera_v4l2_cuda -d /dev/video0 -s 640x480 -f YUYV -n 30 -c\n\n"
           "\tSupported options:\n"
           "\t-d\t\tSet V4l2 video device node\n"
           "\t-s\t\tSet output resolution of video device\n"
           "\t-f\t\tSet output pixel format of video device (supports only YUYV/YVYU/UYVY/VYUY/GREY/MJPEG)\n"
           "\t-r\t\tSet renderer frame rate (30 fps by default)\n"
           "\t-n\t\tSave the n-th frame before VIC processing\n"
           "\t-c\t\tEnable CUDA aglorithm (draw a black box in the upper left corner)\n"
           "\t-v\t\tEnable verbose message\n"
           "\t-h\t\tPrint this usage\n\n"
           "\tNOTE: It runs infinitely until you terminate it with <ctrl+c>\n");
}

static unsigned long long GetMilliSecond()
{
	timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (time.tv_sec*1000 + (time.tv_nsec/(1000*1000)));
}

static bool
parse_cmdline(context_t * ctx, int argc, char **argv)
{
    int c;

    if (argc < 2)
    {
        print_usage();
        exit(EXIT_SUCCESS);
    }
    
    return true;
}

static void
set_defaults(context_t * ctx)
{
    memset(ctx, 0, sizeof(context_t));

    ctx->cam_devname = "/dev/video0";
    ctx->cam_fd = -1;
    ctx->cam_pixfmt = V4L2_PIX_FMT_YUYV;
    ctx->cam_w = 1920;
    ctx->cam_h = 1080;
    ctx->resize_w = 1920;
    ctx->resize_h = 1080;
    ctx->frame = 0;
    ctx->save_n_frame = 30;

    ctx->g_buff = NULL;
    ctx->capture_dmabuf = true;
    ctx->renderer = NULL;
    ctx->fps = 30;

    ctx->enable_cuda = false;
    ctx->egl_image = NULL;
    ctx->egl_display = EGL_NO_DISPLAY;

    ctx->enable_verbose = false;
}

static nv_color_fmt nvcolor_fmt[] =
{
    {V4L2_PIX_FMT_YUYV, NvBufferColorFormat_YUYV},

};

static NvBufferColorFormat
get_nvbuff_color_fmt(unsigned int v4l2_pixfmt)
{
    unsigned i;

    for (i = 0; i < sizeof(nvcolor_fmt) / sizeof(nvcolor_fmt[0]); i++)
    {
        if (v4l2_pixfmt == nvcolor_fmt[i].v4l2_pixfmt)
            return nvcolor_fmt[i].nvbuff_color;
    }

    return NvBufferColorFormat_Invalid;
}

static bool
save_frame_to_file(context_t * ctx, struct v4l2_buffer * buf)
{
    int file;

    file = open(ctx->cam_file, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (-1 == file)
        ERROR_RETURN("Failed to open file for frame saving");

    if (-1 == write(file, ctx->g_buff[buf->index].start,
                ctx->g_buff[buf->index].size))
    {
        close(file);
        ERROR_RETURN("Failed to write frame into file");
    }

    close(file);

    return true;
}

static bool
nvbuff_do_clearchroma (int dmabuf_fd)
{
  NvBufferParams params = {0};
  void *sBaseAddr[3] = {NULL};
  int ret = 0;
  int size;
  unsigned i;

  ret = NvBufferGetParams (dmabuf_fd, &params);
  if (ret != 0)
    ERROR_RETURN("%s: NvBufferGetParams Failed \n", __func__);

  for (i = 1; i < params.num_planes; i++) {
    ret = NvBufferMemMap (dmabuf_fd, i, NvBufferMem_Read_Write, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemMap Failed \n", __func__);

    /* Sync device cache for CPU access since data is from VIC */
    ret = NvBufferMemSyncForCpu (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemSyncForCpu Failed \n", __func__);

    size = params.height[i] * params.pitch[i];
    memset (sBaseAddr[i], 0x80, size);

    /* Sync CPU cache for VIC access since data is from CPU */
    ret = NvBufferMemSyncForDevice (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemSyncForDevice Failed \n", __func__);

    ret = NvBufferMemUnMap (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemUnMap Failed \n", __func__);
  }

  return true;
}

static bool
camera_initialize(context_t * ctx)
{
    struct v4l2_format fmt;
    struct v4l2_capability     cap;

    /* Open camera device */
    ctx->cam_fd = open(ctx->cam_devname, O_RDWR,0);
    if (ctx->cam_fd == -1)
        ERROR_RETURN("Failed to open camera device %s: %s (%d)",
                ctx->cam_devname, strerror(errno), errno);

     memset(&cap, 0x0, sizeof(v4l2_capability));
    if (ioctl(ctx->cam_fd, VIDIOC_QUERYCAP, &cap)) {
        ERROR_RETURN("Failed to VIDIOC_QUERYCAP: %s (%d)",
                strerror(errno), errno);
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("m_cameraId:%s Capture not supported\n", ctx->cam_devname);
        return false;
    }
    
    /* Set camera output format */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = ctx->cam_w;
    fmt.fmt.pix.height = ctx->cam_h;
    fmt.fmt.pix.pixelformat = ctx->cam_pixfmt;
    //fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(ctx->cam_fd, VIDIOC_S_FMT, &fmt) < 0)
        ERROR_RETURN("Failed to set camera output format: %s (%d)",
                strerror(errno), errno);

    /* Get the real format in case the desired is not supported */
    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_G_FMT, &fmt) < 0)
        ERROR_RETURN("Failed to get camera output format: %s (%d)",
                strerror(errno), errno);
    if (fmt.fmt.pix.width != ctx->cam_w ||
            fmt.fmt.pix.height != ctx->cam_h ||
            fmt.fmt.pix.pixelformat != ctx->cam_pixfmt)
    {
        WARN("The desired format is not supported");
        ctx->cam_w = fmt.fmt.pix.width;
        ctx->cam_h = fmt.fmt.pix.height;
        ctx->cam_pixfmt =fmt.fmt.pix.pixelformat;
    }

    struct v4l2_streamparm streamparm;
    memset (&streamparm, 0x00, sizeof (struct v4l2_streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (ctx->cam_fd, VIDIOC_G_PARM, &streamparm);

    printf("Camera ouput format: (%d x %d)  stride: %d, imagesize: %d, frate: %u / %u\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fmt.fmt.pix.bytesperline,
            fmt.fmt.pix.sizeimage,
            streamparm.parm.capture.timeperframe.denominator,
            streamparm.parm.capture.timeperframe.numerator);

    return true;
}

static bool
display_initialize(context_t * ctx)
{
    /* Create EGL renderer */
    ctx->renderer = NvEglRenderer::createEglRenderer("renderer0",
            ctx->resize_w, ctx->resize_h, 0, 0);
    if (!ctx->renderer)
        ERROR_RETURN("Failed to create EGL renderer");
    ctx->renderer->setFPS(ctx->fps);

    if (ctx->enable_cuda)
    {
        /* Get defalut EGL display */
        ctx->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (ctx->egl_display == EGL_NO_DISPLAY)
            ERROR_RETURN("Failed to get EGL display connection");

        /* Init EGL display connection */
        if (!eglInitialize(ctx->egl_display, NULL, NULL))
            ERROR_RETURN("Failed to initialize EGL display connection");
    }

    return true;
}

static bool
init_components(context_t * ctx)
{
    if (!camera_initialize(ctx))
        ERROR_RETURN("Failed to initialize camera device");

    //if (!display_initialize(ctx))
       // ERROR_RETURN("Failed to initialize display");

    INFO("Initialize v4l2 components successfully");
    return true;
}

static bool
request_camera_buff(context_t *ctx)
{
    /* Request camera v4l2 buffer */
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
	printf("buf cnt:%d\n",V4L2_BUFFERS_NUM);
    rb.count = V4L2_BUFFERS_NUM;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_DMABUF;
    if (ioctl(ctx->cam_fd, VIDIOC_REQBUFS, &rb) < 0)
        ERROR_RETURN("Failed to request v4l2 buffers: %s (%d)",
                strerror(errno), errno);
    if (rb.count != V4L2_BUFFERS_NUM)
        ERROR_RETURN("V4l2 buffer number is not as desired");

    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        struct v4l2_buffer buf;

        /* Query camera v4l2 buf length */
        memset(&buf, 0, sizeof buf);
        buf.index = index;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_DMABUF;

        if (ioctl(ctx->cam_fd, VIDIOC_QUERYBUF, &buf) < 0)
            ERROR_RETURN("Failed to query buff: %s (%d)",
                    strerror(errno), errno);

        /* TODO: add support for multi-planer
           Enqueue empty v4l2 buff into camera capture plane */
        buf.m.fd = (unsigned long)ctx->g_buff[index].dmabuff_fd;
        if (buf.length != ctx->g_buff[index].size)
        {
            WARN("Camera v4l2 buf length is not expected:%d %d\n",buf.length,ctx->g_buff[index].size);
            ctx->g_buff[index].size = buf.length;
        }

        if (ioctl(ctx->cam_fd, VIDIOC_QBUF, &buf) < 0)
            ERROR_RETURN("Failed to enqueue buffers: %s (%d)",
                    strerror(errno), errno);
    }

    return true;
}

static bool
request_camera_buff_mmap(context_t *ctx)
{
    /* Request camera v4l2 buffer */
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
    rb.count = V4L2_BUFFERS_NUM;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    if (ioctl(ctx->cam_fd, VIDIOC_REQBUFS, &rb) < 0)
        ERROR_RETURN("Failed to request v4l2 buffers: %s (%d)",
                strerror(errno), errno);
    if (rb.count != V4L2_BUFFERS_NUM)
        ERROR_RETURN("V4l2 buffer number is not as desired");

    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        struct v4l2_buffer buf;

        /* Query camera v4l2 buf length */
        memset(&buf, 0, sizeof buf);
        buf.index = index;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(ctx->cam_fd, VIDIOC_QUERYBUF, &buf) < 0)
            ERROR_RETURN("Failed to query buff: %s (%d)",
                    strerror(errno), errno);

        ctx->g_buff[index].size = buf.length;
        ctx->g_buff[index].start = (unsigned char *)
            mmap (NULL /* start anywhere */,
                    buf.length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    ctx->cam_fd, buf.m.offset);
        if (MAP_FAILED == ctx->g_buff[index].start)
            ERROR_RETURN("Failed to map buffers");

        if (ioctl(ctx->cam_fd, VIDIOC_QBUF, &buf) < 0)
            ERROR_RETURN("Failed to enqueue buffers: %s (%d)",
                    strerror(errno), errno);
    }

    return true;
}

static bool
prepare_buffers_mjpeg(context_t * ctx)
{
    NvBufferCreateParams input_params = {0};

    /* Allocate global buffer context */
    ctx->g_buff = (nv_buffer *)malloc(V4L2_BUFFERS_NUM * sizeof(nv_buffer));
    if (ctx->g_buff == NULL)
        ERROR_RETURN("Failed to allocate global buffer context");
    memset(ctx->g_buff, 0, V4L2_BUFFERS_NUM * sizeof(nv_buffer));

    input_params.payloadType = NvBufferPayload_SurfArray;
    input_params.width = ctx->cam_w;
    input_params.height = ctx->cam_h;
    input_params.layout = NvBufferLayout_Pitch;
    input_params.colorFormat = get_nvbuff_color_fmt(V4L2_PIX_FMT_YUV420M);
    input_params.nvbuf_tag = NvBufferTag_NONE;

    /* Create Render buffer */
    if (-1 == NvBufferCreateEx(&ctx->render_dmabuf_fd, &input_params))
        ERROR_RETURN("Failed to create NvBuffer");

    ctx->capture_dmabuf = false;
    if (!request_camera_buff_mmap(ctx))
        ERROR_RETURN("Failed to set up camera buff");

    INFO("Succeed in preparing mjpeg buffers");
    return true;
}

static bool
prepare_buffers(context_t * ctx)
{
    NvBufferCreateParams input_params = {0};

    /* Allocate global buffer context */
    ctx->g_buff = (nv_buffer *)malloc(V4L2_BUFFERS_NUM * sizeof(nv_buffer));
    if (ctx->g_buff == NULL)
        ERROR_RETURN("Failed to allocate global buffer context");

    input_params.payloadType = NvBufferPayload_SurfArray;
    input_params.width = ctx->cam_w;
    input_params.height = ctx->cam_h;
    input_params.layout = NvBufferLayout_Pitch;

    /* Create buffer and provide it with camera */
    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        int fd;
        NvBufferParams params = {0};

        input_params.colorFormat = get_nvbuff_color_fmt(ctx->cam_pixfmt);
        input_params.nvbuf_tag = NvBufferTag_CAMERA;
		input_params.payloadType = NvBufferPayload_SurfArray;
        input_params.width = ctx->cam_w;
        input_params.height = ctx->cam_h;
        input_params.layout = NvBufferLayout_Pitch;
        if (-1 == NvBufferCreateEx(&fd, &input_params))
            ERROR_RETURN("Failed to create NvBuffer");

        ctx->g_buff[index].dmabuff_fd = fd;
		ctx->g_buff[index].size = ctx->cam_w*ctx->cam_h*2;
		
        if (-1 == NvBufferGetParams(fd, &params))
            ERROR_RETURN("Failed to get NvBuffer parameters");

        printf("yuyv %d params[fd:%d size:%d nv_buffer_size:%d num_planes:%d]\n",fd,params.dmabuf_fd,params.memsize,
            params.nv_buffer_size,params.num_planes);
        
        /* TODO: add multi-planar support
           Currently only supports YUV422 interlaced single-planar */
        if (ctx->capture_dmabuf) {
            if (-1 == NvBufferMemMap(ctx->g_buff[index].dmabuff_fd, 0, NvBufferMem_Read_Write,
                        (void**)&ctx->g_buff[index].start))
                ERROR_RETURN("Failed to map buffer");
        }
		printf("[%s %d] alloc dmabuf fd:%d viraddr:%p\n",__FUNCTION__,__LINE__,ctx->g_buff[index].dmabuff_fd,ctx->g_buff[index].start);
    }

    input_params.colorFormat = NvBufferColorFormat_ARGB32;
    input_params.nvbuf_tag = NvBufferTag_VIDEO_CONVERT;
    input_params.width = ctx->resize_w;
    input_params.height = ctx->resize_h;
    /* Create Render buffer */
    if (-1 == NvBufferCreateEx(&ctx->render_dmabuf_fd, &input_params))
        ERROR_RETURN("Failed to create NvBuffer");
        
    NvBufferParams params = {0};
    if (-1 == NvBufferGetParams(ctx->render_dmabuf_fd, &params))
            ERROR_RETURN("Failed to get NvBuffer parameters");
    printf("ABGR %d params[fd:%d size:%d nv_buffer_size:%d num_planes:%d]\n",ctx->render_dmabuf_fd,params.dmabuf_fd,params.memsize,
            params.nv_buffer_size,params.num_planes);

    if (-1 == NvBufferMemMap(ctx->render_dmabuf_fd, 0, NvBufferMem_Read_Write,
                        (void**)&ctx->render_dmabuf_start))
                ERROR_RETURN("Failed to map buffer");


    printf("render start:%p\n",ctx->render_dmabuf_start);
    
    if (ctx->capture_dmabuf) {
        if (!request_camera_buff(ctx))
            ERROR_RETURN("Failed to set up camera buff");
    } else {
        if (!request_camera_buff_mmap(ctx))
            ERROR_RETURN("Failed to set up camera buff");
    }

    INFO("Succeed in preparing stream buffers");
    return true;
}

static bool
start_stream(context_t * ctx)
{
    enum v4l2_buf_type type;

    /* Start v4l2 streaming */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_STREAMON, &type) < 0)
        ERROR_RETURN("Failed to start streaming: %s (%d)",
                strerror(errno), errno);

    usleep(200);

    INFO("Camera video streaming on ...");
   
    return true;
}

static void
signal_handle(int signum)
{
    printf("Quit due to exit command from user!\n");
    quit = true;
}

static bool
cuda_postprocess(context_t *ctx, int fd)
{
    if (ctx->enable_cuda)
    {
        /* Create EGLImage from dmabuf fd */
        ctx->egl_image = NvEGLImageFromFd(ctx->egl_display, fd);
        if (ctx->egl_image == NULL)
            ERROR_RETURN("Failed to map dmabuf fd (0x%X) to EGLImage",
                    ctx->render_dmabuf_fd);

        /* Pass this buffer hooked on this egl_image to CUDA for
           CUDA processing - draw a rectangle on the frame */
        HandleEGLImage(&ctx->egl_image);

        /* Destroy EGLImage */
        NvDestroyEGLImage(ctx->egl_display, ctx->egl_image);
        ctx->egl_image = NULL;
    }

    return true;
}


int WriteToFile(const char* path, uint8_t* buffer, uint64_t length)
{
    FILE *fp = NULL;
    int write_length = 0;
    fp = fopen(path, "wb+");
    if(fp == NULL)
    {
        printf("open (wb+)%s failed\n",path);
        return -1;
    }
    write_length = fwrite(buffer, 1, length, fp);
    fclose(fp);
    fp = NULL;
    return write_length;
}

extern int InitFdReader(int chn);
extern int InitFdWriter(int chn);
extern int FdRead(int handle,int* fd,void* readBuf,int bufLen,int timeOut);
extern int FdWrite(int handle,int fd, void* writeBuf, int writeLen);

static bool
start_capture(context_t * ctx)
{
    struct sigaction sig_action;
    //struct pollfd fds[1];
    NvBufferTransformParams transParams;

    /* Register a shuwdown handler to ensure
       a clean shutdown if user types <ctrl+c> */
    sig_action.sa_handler = signal_handle;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;
    sigaction(SIGINT, &sig_action, NULL);

    if (ctx->cam_pixfmt == V4L2_PIX_FMT_MJPEG)
        ctx->jpegdec = NvJPEGDecoder::createJPEGDecoder("jpegdec");

    /* Init the NvBufferTransformParams */
    memset(&transParams, 0, sizeof(transParams));
    transParams.transform_flip = NvBufferTransform_None;
    transParams.transform_flag = NVBUFFER_TRANSFORM_CROP_SRC | NVBUFFER_TRANSFORM_CROP_DST;
    transParams.transform_filter = NvBufferTransform_Filter_Smart;
    transParams.src_rect.top = 0;
    transParams.src_rect.left = 0;
    transParams.src_rect.width = ctx->cam_w;
    transParams.src_rect.height = ctx->cam_h;
    transParams.dst_rect.left = 0;
    transParams.dst_rect.top = 0;
    transParams.dst_rect.width = ctx->resize_w;
    transParams.dst_rect.height = ctx->resize_h;

	if(nullptr != ctx->renderer)
	{
		/* Enable render profiling information */
		ctx->renderer->enableProfiling();
	}
	unsigned long long last = GetMilliSecond();
	unsigned int lastFrame =0;
	bool videoLoss = false;
    //fds[0].fd = ctx->cam_fd;
    //fds[0].events = POLLIN;
    /* Wait for camera event with timeout = 5000 ms */
    std::thread ti = thread([&videoLoss](){
       while(!quit)
       {
            static const char* cmd_list[3][3] = {
                {
                    "echo 0 > /sys/class/tz_gpio/camera_0_1-power/value",
                    "echo 1 > /sys/class/tz_gpio/camera_0_1-power/value",
                    "/usr/bin/configure-camera/camera_cfg SELECT_A ADDR_A max9295_OS02H10_1920_1080_1x2_list_ruiming v1_v2"
                },
                {
                    "echo 0 > /sys/class/tz_gpio/camera_2_3-power/value",
                    "echo 1 > /sys/class/tz_gpio/camera_2_3-power/value",
                    "/usr/bin/configure-camera/camera_cfg SELECT_B ADDR_B max9295_OS02H10_1920_1080_1x2_list_ruiming v3_v4"
                },
                {
                    "echo 0 > /sys/class/tz_gpio/camera_4_5-power/value",
                    "echo 1 > /sys/class/tz_gpio/camera_4_5-power/value",
                    "/usr/bin/configure-camera/camera_cfg SELECT_C ADDR_C max9295_OS02H10_1920_1080_1x2_list_ruiming v5_v6"
                },
            };
            
            if(videoLoss)
            {
                printf("video loss\n");
                for(int i = 0;i < 3; i++)
                {
                    system(cmd_list[0][i]);
                    sleep(1);
                }
                //videoLoss = false;
            }
            sleep(1);
       }
    });

    int frameCnt = 0;
    while (!quit)
    {
		/*int ret = poll(fds, 1, 40);
		if(ret < 0)
		{
			break;
		}
		
		if(ret == 0)
		{
			usleep(10*1000);
		}
		
        if (fds[0].revents & POLLIN) */
		fd_set fds;
		struct timeval tv;
		int r;
		FD_ZERO (&fds);
		FD_SET (ctx->cam_fd, &fds);
		/* Timeout. */
		tv.tv_sec = 0;
		tv.tv_usec = 800*1000;
		r = select (ctx->cam_fd + 1, &fds, NULL, NULL, &tv);

		if (0 == r) {
			printf("cameraId %d select timeout lost\n", ctx->cam_fd);
			videoLoss = true;
			frameCnt = 0;
			//continue;
		}
		//videoLoss = false;
		{
            struct v4l2_buffer v4l2_buf;
			
            /* Dequeue a camera buff */
            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (ctx->capture_dmabuf)
                v4l2_buf.memory = V4L2_MEMORY_DMABUF;
            else
                v4l2_buf.memory = V4L2_MEMORY_MMAP;
			
            if (ioctl(ctx->cam_fd, VIDIOC_DQBUF, &v4l2_buf) < 0)
			{
                ERROR_RETURN("Failed to dequeue camera buff: %s (%d)",
                        strerror(errno), errno);
				
				printf("no frame\n");
				usleep(10*1000);
				continue;
			}
			frameCnt++;	
			printf("videoloss:%d\n",videoLoss);
			//printf("index:%d fd:%d fd:%d\n",v4l2_buf.index,ctx->g_buff[v4l2_buf.index].dmabuff_fd,v4l2_buf.m.fd);
            ctx->frame++;
            lastFrame++;
            if(GetMilliSecond() - last >= 1000)
            {
                printf("cam fps:%d\n",lastFrame);
                lastFrame = 0;
                last = GetMilliSecond();
            }
             //NvBufferTransformParams transParams;
            //memset(&transParams, 0, sizeof(transParams));
            transParams.transform_flag = NVBUFFER_TRANSFORM_FILTER;
            transParams.transform_filter = NvBufferTransform_Filter_Smart;
			 if (-1 == NvBufferTransform(ctx->g_buff[v4l2_buf.index].dmabuff_fd, ctx->render_dmabuf_fd, &transParams))
            {
               printf("Failed to convert the buffer\n");
            }  
            
            NvBufferMemSyncForCpu (ctx->render_dmabuf_fd, 0, (void**)&ctx->render_dmabuf_start);
			extern int sendFd(int fd, void* sndBuf, int sndLen);
            int chn = 0;
			//sendFd(ctx->render_dmabuf_fd, &chn,sizeof(chn));
			FdWrite(3,ctx->render_dmabuf_fd, &chn, sizeof(chn));
			
			//sendFd(ctx->g_buff[v4l2_buf.index].dmabuff_fd, &chn,sizeof(chn));
            /* Save the n-th frame to file */
            if (ctx->frame == ctx->save_n_frame)
            {
                //cv::Mat brga = cv::Mat(1080,1920,CV_8UC4,ctx->render_dmabuf_start,1920*4);
                //cv::Mat rgb;
                //cv::cvtColor(brga, rgb, cv::COLOR_BGRA2BGR);
               // WriteToFile("yuyv", ctx->g_buff[v4l2_buf.index].start, ctx->g_buff[v4l2_buf.index].size);
                WriteToFile("argb", ctx->render_dmabuf_start, ctx->cam_w*ctx->cam_h*4);
                //ctx->save_n_frame = 0;
                //printf("write frame!!!!\n");
                //save_frame_to_file(ctx, &v4l2_buf);
            }
			#if 0
            if (ctx->cam_pixfmt == V4L2_PIX_FMT_MJPEG) {
                int fd = 0;
                uint32_t width, height, pixfmt;
                unsigned int i = 0;
                unsigned int eos_search_size = MJPEG_EOS_SEARCH_SIZE;
                unsigned int bytesused = v4l2_buf.bytesused;
                uint8_t *p;

                /* v4l2_buf.bytesused may have padding bytes for alignment
                   Search for EOF to get exact size */
                if (eos_search_size > bytesused)
                    eos_search_size = bytesused;
                for (i = 0; i < eos_search_size; i++) {
                    p =(uint8_t *)(ctx->g_buff[v4l2_buf.index].start + bytesused);
                    if ((*(p-2) == 0xff) && (*(p-1) == 0xd9)) {
                        break;
                    }
                    bytesused--;
                }

                /* Decoding MJPEG frame */
                if (ctx->jpegdec->decodeToFd(fd, ctx->g_buff[v4l2_buf.index].start,
                    bytesused, pixfmt, width, height) < 0)
                    ERROR_RETURN("Cannot decode MJPEG");

                /* Convert the decoded buffer to YUV420P */
                if (-1 == NvBufferTransform(fd, ctx->render_dmabuf_fd,
                        &transParams))
                    ERROR_RETURN("Failed to convert the buffer");
            } else {
                if (ctx->capture_dmabuf) {
                    /* Cache sync for VIC operation since the data is from CPU */
                    NvBufferMemSyncForDevice(ctx->g_buff[v4l2_buf.index].dmabuff_fd, 0,
                            (void**)&ctx->g_buff[v4l2_buf.index].start);
                } else {
                    /* Copies raw buffer plane contents to an NvBuffer plane */
                    Raw2NvBuffer(ctx->g_buff[v4l2_buf.index].start, 0,
                             ctx->cam_w, ctx->cam_h, ctx->g_buff[v4l2_buf.index].dmabuff_fd);
                }

                /*  Convert the camera buffer from YUV422 to YUV420P */
                if (-1 == NvBufferTransform(ctx->g_buff[v4l2_buf.index].dmabuff_fd, ctx->render_dmabuf_fd,
                            &transParams))
                    ERROR_RETURN("Failed to convert the buffer");

                if (ctx->cam_pixfmt == V4L2_PIX_FMT_GREY) {
                    if(!nvbuff_do_clearchroma(ctx->render_dmabuf_fd))
                        ERROR_RETURN("Failed to clear chroma");
                }
            }
            cuda_postprocess(ctx, ctx->render_dmabuf_fd);
			if(nullptr != ctx->renderer)
			{
				/* Preview */
				ctx->renderer->render(ctx->render_dmabuf_fd);
			}
			#endif
            /* Enqueue camera buffer back to driver */
            if (ioctl(ctx->cam_fd, VIDIOC_QBUF, &v4l2_buf))
                ERROR_RETURN("Failed to queue camera buffers: %s (%d)",
                        strerror(errno), errno);

            if(frameCnt > V4L2_BUFFERS_NUM)
			{
			    printf("video ok\n");
                videoLoss = false;
			}
        }
    }
	if(nullptr != ctx->renderer)
	{
		/* Print profiling information when streaming stops */
		ctx->renderer->printProfilingStats();
	}
	
    if (ctx->cam_pixfmt == V4L2_PIX_FMT_MJPEG)
        delete ctx->jpegdec;

    return true;
}

static bool
stop_stream(context_t * ctx)
{
    enum v4l2_buf_type type;

    /* Stop v4l2 streaming */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_STREAMOFF, &type))
        ERROR_RETURN("Failed to stop streaming: %s (%d)",
                strerror(errno), errno);

    INFO("Camera video streaming off ...");
    return true;
}

int Recive()
{
    extern int ReciveFd(int* fd,void* recvBuf,int bufLen,int timeOut);
        
    int filcnt = 0;
    char fileName[12];
    sprintf(fileName,"./out.rgb");
    FILE *fp = fopen(fileName, "wb+");
    if(fp == NULL)
    {
        printf("open (wb+)%s failed\n",fileName);
        return -1;
    }
    int cnt = 0;
    uint64_t lastTime = GetMilliSecond();
    while(1)
    {
        int fd = 0;
        int chn = 0;
        char buffer[128];
        //int len = ReciveFd(&fd,&chn,sizeof(chn),1000);
        if(GetMilliSecond() - lastTime >= 1000)
        {
            printf("fps:%d\n",cnt);
            cnt = 0;
            lastTime = GetMilliSecond();
        }
        int len = FdRead(3,&fd,buffer,128,1000);
        //printf("get fd:%d\n",fd);
        if(fd <= 0)
        {
            continue;
        }
        cnt++;

        NvBufferParams params = {0};
        //if (-1 == NvBufferGetParams(fd, &params))
        //    ERROR_RETURN("Failed to get NvBuffer parameters");

        char* start = nullptr;
        start = (char*)mmap(NULL,1920*1080*4,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
        //if (-1 == NvBufferMemMap(fd, 0, NvBufferMem_Read_Write,(void**)&start))
        //    ERROR_RETURN("Failed to map buffer");
        if(start == nullptr)
        {
            perror("mmap error\n");
            return -1;
        }
        //abrg to brg
        vector<uint8_t> out(1920*1080*3);
        unsigned char* pRGBA = (uint8_t*)start;
        uint8_t* pRGB = &out[0];
        int j = 0;
        for(int i = 0;i < 1920*1080*3;)
        {
            uint8x16x4_t rgba = vld4q_u8(pRGBA);
            uint8x16x3_t rgb = vld3q_u8(pRGB);
            
            i+=3;
            j+=4;
        }
        
        //fwrite(&out[0], 1, 1920*1080*3, fp);
        close(fd);
        
        //break;
    }
    
    fclose(fp);
    fp = NULL;
}


int
main(int argc, char *argv[])
{
    if(argc == 1)
    {
        return Recive();
    }
    
    context_t ctx;
    int error = 0;

    set_defaults(&ctx);

    CHECK_ERROR(parse_cmdline(&ctx, argc, argv), cleanup,
            "Invalid options specified");

    /* Initialize camera and EGL display, EGL Display will be used to map
       the buffer to CUDA buffer for CUDA processing */
    CHECK_ERROR(init_components(&ctx), cleanup,
            "Failed to initialize v4l2 components");

    if (ctx.cam_pixfmt == V4L2_PIX_FMT_MJPEG) {
        CHECK_ERROR(prepare_buffers_mjpeg(&ctx), cleanup,
                "Failed to prepare v4l2 buffs");
    } else {
        CHECK_ERROR(prepare_buffers(&ctx), cleanup,
                "Failed to prepare v4l2 buffs");
    }

    CHECK_ERROR(start_stream(&ctx), cleanup,
            "Failed to start streaming");

    CHECK_ERROR(start_capture(&ctx), cleanup,
            "Failed to start capturing")

    CHECK_ERROR(stop_stream(&ctx), cleanup,
            "Failed to stop streaming");

cleanup:
    if (ctx.cam_fd > 0)
        close(ctx.cam_fd);

    if (ctx.renderer != NULL)
        delete ctx.renderer;

    if (ctx.egl_display && !eglTerminate(ctx.egl_display))
        printf("Failed to terminate EGL display connection\n");

    if (ctx.g_buff != NULL)
    {
        for (unsigned i = 0; i < V4L2_BUFFERS_NUM; i++) {
            if (ctx.g_buff[i].dmabuff_fd)
                NvBufferDestroy(ctx.g_buff[i].dmabuff_fd);
            if (ctx.cam_pixfmt == V4L2_PIX_FMT_MJPEG)
                munmap(ctx.g_buff[i].start, ctx.g_buff[i].size);
        }
        free(ctx.g_buff);
    }

    NvBufferDestroy(ctx.render_dmabuf_fd);

    if (error)
        printf("App run failed\n");
    else
        printf("App run was successful\n");

    return -error;
}
