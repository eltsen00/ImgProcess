#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)

// Windows 兼容：使用临时文件模拟，并提供关闭时提取缓冲的函数
static inline FILE* open_memstream_win(char** /*buffer*/, size_t* /*size*/) {
    // 用 tmpfile 模拟，真正的 buffer/size 由 fclose_memstream_win 来填充。
    return tmpfile();
}

// 关闭并把文件内容读入内存缓冲区
static inline int fclose_memstream_win(FILE* stream, char** buffer, size_t* size) {
    if (!stream || !buffer || !size) return -1; // 参数检查
    long cur = ftell(stream); // 获取当前文件位置（即文件大小）
    if (cur < 0) return -1; // 错误处理
    size_t len = static_cast<size_t>(cur);
    rewind(stream); // 回到文件开头
    char* out = static_cast<char*>(malloc(len + 1)); // 分配实际要储存到的内存区域
    if (!out) return -1; // 分配失败

    size_t n = fread(out, 1, len, stream); // 读取数据
    if (n != len) { // 读取失败
        free(out);
        return -1;
    }
    out[n] = '\0'; // 与 glibc 行为一致，结尾补 0（对二进制可有可无）
    fclose(stream);
    // 设置输出
    *buffer = out;
    *size = n;
    return 0;
}

// 仅用于从内存读取：把 buf 写入临时文件并回到开头
static inline FILE* fmemopen_win(void* buf, size_t size, const char* /*mode*/) {
    FILE* f = tmpfile(); // 创建临时文件
    if (!f) return nullptr;
    if (size) { // 写入数据
        if (fwrite(buf, 1, size, f) != size) { // 写入失败
            fclose(f);
            return nullptr;
        }
    }
    rewind(f); // 回到文件开头
    return f;
}

// 提供跨平台别名/宏
#define open_memstream(BUF,SZ) open_memstream_win(BUF,SZ)
#define fmemopen(BUF,SZ,MODE)  fmemopen_win(BUF,SZ,MODE)
#define fclose_memstream(STREAM,BUF,SZ) fclose_memstream_win(STREAM,BUF,SZ)

#else
// 非 Windows 系统：直接使用 glibc 提供的函数，系统原生实现
#define fclose_memstream(STREAM,BUF,SZ) fclose(STREAM)

#endif