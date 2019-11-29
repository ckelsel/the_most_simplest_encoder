// https://blog.csdn.net/leixiaohua1020/article/details/42078645
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x264.h>

// 安装x264开发库
// sudo apt-get install libx264-dev

// cuc_ieschool_640x360_yuv444p的yuv信息
// 色彩空间444
#define COLOR_SPACE X264_CSP_I444
// 宽度 640
#define WIDTH 640
// 高度 360
#define HEIGHT 360


int get_number_frames(char *file)
{
    int frames;

    FILE *src = fopen("../cuc_ieschool_640x360_yuv444p.yuv", "rb");
    if (!src) {
        printf("fopen ../cuc_ieschool_640x360_yuv444p.yuv failed\n");
        return -1;
    }

    fseek(src, 0, SEEK_END);

    // Y、U、V分量的大小都是 WIDTH*HEIGHT
    frames = ftell(src) / (WIDTH * HEIGHT * 3);

    fclose(src);

    return frames;
}

int main(int argc, char **argv)
{
    int frames = get_number_frames("../cuc_ieschool_640x360_yuv444p.yuv");

    FILE *src = fopen("../cuc_ieschool_640x360_yuv444p.yuv", "rb");
    if (!src) {
        printf("fopen ../cuc_ieschool_640x360_yuv444p.yuv failed\n");
        return -1;
    }

    FILE *dst = fopen("out.h264", "wb");
    if (!dst) {
        printf("fopen out.h264 failed\n");
        return -1;
    }

    int inal = 0;
    // x264_nal_t保存编码后的h264数据
    x264_nal_t *pnals = NULL;
    x264_t *x264 = NULL;
    // x264_picture_t保存编码前的yuv数据
    x264_picture_t *pic_in = malloc(sizeof(x264_picture_t));
    x264_picture_t *pic_out = malloc(sizeof(x264_picture_t));
    x264_param_t *param = malloc(sizeof(x264_param_t));

    // 用默认参数初始化param
    x264_param_default(param);

    // 设置视频的宽高
    param->i_width = WIDTH;
    param->i_height = HEIGHT;
    param->i_csp = COLOR_SPACE;

    x264_picture_init(pic_out);

    // 分配一帧的空间
    x264_picture_alloc(pic_in, param->i_csp, param->i_width, param->i_height);

    x264 = x264_encoder_open(param);

    int delay_frames = 0;
    int fflush_frames = 0;
    for(int i = 0; i < frames; i++) {
        // 读取Y的数据
        fread(pic_in->img.plane[0], WIDTH*HEIGHT, 1, src);
        // 读取U的数据
        fread(pic_in->img.plane[1], WIDTH*HEIGHT, 1, src);
        // 读取V的数据
        fread(pic_in->img.plane[2], WIDTH*HEIGHT, 1, src);

        // ?
        pic_in->i_pts = i;

        int ret = x264_encoder_encode(x264, &pnals, &inal, pic_in, pic_out);
        if (ret < 0) {
            break;
        }

        // 编码是异步的，有可能拿不到编码后的h264
        printf("ret %d, inal %d\n", ret, inal);
        if (ret == 0) {
            delay_frames++;
        }

        // 将转码后的h264保存到文件
        for (int j = 0; j < inal; j++) {
            fwrite(pnals[j].p_payload, 1, pnals[j].i_payload, dst);
        }
    }

    // “flush_encoder”模块，该模块使用的函数和编码模块是一样的。
    // 唯一的不同在于不再输入视频像素数据。
    // 它的作用在于输出编码器中剩余的码流数据。
    while (x264_encoder_delayed_frames(x264)) {
        int ret = x264_encoder_encode(x264, &pnals, &inal, NULL, pic_out);
        if (ret < 0) {
            break;
        } else if (ret) {
            fflush_frames++;

            printf("fflush one frame\n");
            for (int j = 0; j < inal; j++) {
                fwrite(pnals[j].p_payload, 1, pnals[j].i_payload, dst);
            }
        }
    }

    printf("delay_frames %d, fflush_frames %d\n", delay_frames, fflush_frames);

    x264_encoder_close(x264);

    x264_picture_clean(pic_in);
    free(pic_in);
    free(pic_out);
    free(param);

    fclose(src);
    fclose(dst);
    return 0;
}
