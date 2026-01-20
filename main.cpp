#define cimg_use_jpeg
#define cimg_use_png
#include "CImg.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "memStream.hpp"
using namespace cimg_library;
using namespace std;

struct Triple {
    unsigned short row, col;
    unsigned char value;
    Triple(unsigned short r, unsigned short c, unsigned char v) : row(r), col(c), value(v) {}
    Triple() : row(0), col(0), value(0) {}
};

int main(){
    CImg<unsigned char> image;
    CImg<unsigned char> compressed_image;
    CImg<unsigned char> gray_image;
    CImg<unsigned char> resized_image;
    CImg<unsigned char> rotated_image;
	while(true){
        cout<<"请输入选择的功能："<<endl;
        cout<<"1. 输入图像路径并加载图像(支持PNG、JPEG、PPM格式)"<<endl;
        cout<<"2. 图像稀疏压缩储存(三元组)"<<endl;
        cout<<"3. 图像灰度化"<<endl;
        cout<<"4. 图像尺寸缩放"<<endl;
        cout<<"5. 图像旋转"<<endl;
        cout<<"6. 压缩图像"<<endl;
        cout<<"7. 显示图像"<<endl;
        cout<<"8. 保存读入的图像为不同格式(支持PNG、JPEG、PPM格式)"<<endl;
        cout<<"0. 退出"<<endl;
        cout<<"选择：";
        string _choice;
        if(!getline(cin, _choice)) break;
        int choice = -1;
        try{
            choice = stoi(_choice);
        }
        catch(...){
            cout<<"无效的选择，请重新输入。"<<endl;
            continue;
        }
        switch(choice){
            case 1: {
                // 输入图像路径并加载图像
                string image_path;
                cout<<"请输入图像路径：";
                if(!getline(cin, image_path)) {
                    cout<<"输入错误，请重试。"<<endl;
                    break;
                }
                try {
                    image.load(image_path.c_str());
                    cout<<"图像加载成功，图像尺寸为："<<image.width()<<"x"<<image.height()<<endl;
                    CImgDisplay main_disp(image, "原始图像 (请点击窗口或按键继续)");
                    while (!main_disp.is_closed() && !main_disp.key()) {
                        main_disp.wait(); // 等待事件
                    }
                }
                catch (CImgIOException &e) {
                    cerr << "图像加载失败: " << e.what() << endl;
                }
                break;
            }
            case 2: {
                // 图像稀疏压缩(三元组)
                if(image.is_empty()){
                    cout<<"请先加载图像！"<<endl;
                    break;
                }
                // 转换为三元组并保存 (压缩)
                cout << "正在进行三元组压缩存储..." << endl;
                char *mem_buffer = nullptr;
                size_t mem_size = 0;
                FILE* fp = open_memstream(&mem_buffer, &mem_size);
                if(!fp) {
                    cerr << "无法打开内存流进行写入！" << endl;
                    break;
                }
                int w = image.width();
                int h = image.height();
                int d = image.depth();
                int s = image.spectrum();
                fwrite(&w, sizeof(int), 1, fp); // 宽度
                fwrite(&h, sizeof(int), 1, fp); // 高度
                fwrite(&d, sizeof(int), 1, fp); // 深度（2D图像一般为1）
                fwrite(&s, sizeof(int), 1, fp); // 颜色通道数
                int total_non_zero = 0;
                for(int c=0;c<s;c++) { // 对每个颜色通道遍历
                    vector<Triple> triplets; // 存储每个颜色通道的非零元素的三元组
                    for(int z=0;z<d;z++) { // 深度遍历，一般为1
                        for(int y=0;y<h;y++) { // 行遍历
                            for(int x=0;x<w;x++) { // 列遍历
                                unsigned char val = image(x, y, z, c);
                                if(val!=0) { // 非零元素存储
                                    triplets.push_back(Triple(y, x, val));
                                }
                            }
                        }
                    }
                    int count = (int)triplets.size();
                    fwrite(&count, sizeof(int), 1, fp); // 该通道非零元素个数
                    if(count>0) {
                        fwrite(triplets.data(), sizeof(Triple), count, fp); // 写入三元组数据
                    }
                    total_non_zero += count;
                }
                fclose_memstream(fp, &mem_buffer, &mem_size);
                cout << "压缩完成。非零元素总数: " << total_non_zero << endl;
                cout << "压缩数据大小: " << mem_size << " 字节" << endl;
                
                // 是否保存内存流数据到文件
                cout<<"是否将压缩数据保存到文件？(y/n)：";
                string save_choice;
                if(!getline(cin, save_choice)){
                    cout<<"输入错误，默认不保存。"<<endl;
                    save_choice = "n";
                }
                if(save_choice=="y" || save_choice=="Y"){
                    cout<<"请输入保存文件名（默认 compressed_data.bin）：";
                    string compressed_filename;
                    if(!getline(cin, compressed_filename)){
                        cout<<"输入错误，使用默认文件名。"<<endl;
                        compressed_filename = "compressed_data.bin";
                    }
                    if(compressed_filename.empty()){
                        cout<<"未输入文件名，使用默认文件名。"<<endl;
                        compressed_filename = "compressed_data.bin";
                    }
                    FILE* out_fp = fopen(compressed_filename.c_str(), "wb");
                    if(!out_fp){
                        cerr << "无法打开文件进行写入！" << endl;
                    }
                    else{
                        fwrite(mem_buffer, 1, mem_size, out_fp);
                        fclose(out_fp);
                        cout<<"压缩数据已保存到文件："<<compressed_filename<<endl;
                    }
                }
                else if(save_choice=="n" || save_choice=="N"){
                    cout<<"压缩数据未保存到文件。" << endl;
                }
                else{
                    cout<<"无效选择，压缩数据未保存到文件。" << endl;
                }

                // 读取并解压
                cout << "正在读取并解压数据..." << endl;
                CImg<unsigned char> restored_image;
                fp = fmemopen(mem_buffer, mem_size, "rb");
                if(!fp) {
                    cerr << "无法打开文件进行读取！" << endl;
                    if(mem_buffer){
                        free(mem_buffer);
                    }
                    break;
                }
                int rw, rh, rd, rs;
                if(fread(&rw, sizeof(int), 1, fp) != 1) { 
                    cerr<<"读取头信息失败"<<endl; 
                    fclose_memstream(fp, &mem_buffer, &mem_size); 
                    if(mem_buffer){
                        free(mem_buffer);
                    }
                    break; 
                }
                fread(&rh, sizeof(int), 1, fp); // 高度
                fread(&rd, sizeof(int), 1, fp); // 深度（2D图像一般为1）
                fread(&rs, sizeof(int), 1, fp); // 颜色通道数
                restored_image.assign(rw, rh, rd, rs, 0);
                for(int c=0;c<rs;c++) { // 对每个颜色通道遍历
                    int count = 0;
                    if(fread(&count, sizeof(int), 1, fp) != 1) break; // 读取该通道非零元素个数
                    if(count>0) {
                        vector<Triple> buffer(count);
                        fread(buffer.data(), sizeof(Triple), count, fp);
                        for(const auto& t : buffer) {
                            if(t.col>=0&&t.col<rw&&t.row>=0&&t.row<rh) {
                                restored_image(t.col,t.row,0,c)=t.value;
                            }
                        }
                    }
                }
                fclose_memstream(fp, &mem_buffer, &mem_size);
                if(mem_buffer){
                    free(mem_buffer);
                }
                cout << "解压完成。" << endl;
                CImgDisplay disp(restored_image, "解压后的图像 (三元组恢复)");
                while (!disp.is_closed() && !disp.key()) {
                    disp.wait();
                }

                // 是否保存解压后的图像
                cout<<"是否将解压后的图像保存到文件？(y/n)：";
                string save_restored_choice;
                if(!getline(cin, save_restored_choice)){
                    cout<<"输入错误，默认不保存。"<<endl;
                    save_restored_choice = "n";
                }
                if(save_restored_choice=="y" || save_restored_choice=="Y"){
                    cout<<"请输入保存文件名（默认 restored_image.png）：";
                    string restored_filename;
                    if(!getline(cin, restored_filename)){
                        cout<<"输入错误，使用默认文件名。"<<endl;
                        restored_filename="restored_image.png";
                    }
                    if(restored_filename.empty()){
                        cout<<"未输入文件名，使用默认文件名。"<<endl;
                        restored_filename="restored_image.png";
                    }
                    try {
                        restored_image.save(restored_filename.c_str());
                        cout<<"解压后的图像已保存到文件："<<restored_filename<<endl;
                    }
                    catch (CImgIOException &e) {
                        cerr<<"图像保存失败: "<<e.what()<<endl;
                    }
                }
                else if(save_restored_choice=="n" || save_restored_choice=="N"){
                    cout<<"解压后的图像未保存到文件。" << endl;
                }
                else{
                    cout<<"无效选择，解压后的图像未保存到文件。" << endl;
                }
                break;
            }
            case 3: {
                // 图像灰度化
                if(image.is_empty()){
                    cout<<"请先加载图像（选择0）！"<<endl;
                    break;
                }
                if(image.spectrum()!=3){
                    cout<<"当前图像不是彩色图像，无法灰度化！"<<endl;
                    break;
                }
                // (宽度, 高度, 深度=1, 颜色通道=1, 初始值=0)
                gray_image.assign(image.width(), image.height(), 1, 1, 0);
                cimg_forXY(gray_image,x,y) {
                    float r = image(x,y,0,0);
                    float g = image(x,y,0,1);
                    float b = image(x,y,0,2);
                    // 灰度化公式: Y = 0.299R + 0.587G + 0.114B
                    float gray_value = 0.299f * r + 0.587f * g + 0.114f * b;
                    gray_image(x,y,0,0) = static_cast<unsigned char>(gray_value);
                }
                cout<<"图像灰度化完成。"<<endl;
                CImgDisplay gray_disp(gray_image, "灰度化图像 (请点击窗口或按键继续)");
                while (!gray_disp.is_closed() && !gray_disp.key()) {
                    gray_disp.wait(); // 等待事件
                }
                break;
            }
            case 4: {
                // 图像尺寸缩放
                if(image.is_empty()){
                    cout<<"请先加载图像（选择0）！"<<endl;
                    break;
                }
                string width_str, height_str;
                cout<<"请输入新的宽度：";
                if(!getline(cin, width_str)){
                    cout<<"输入错误，使用原始宽度。"<<endl;
                    width_str = to_string(image.width());
                }
                cout<<"请输入新的高度：";
                if(!getline(cin, height_str)){
                    cout<<"输入错误，使用原始高度。"<<endl;
                    height_str = to_string(image.height());
                }
                int new_width = image.width();
                int new_height = image.height();
                try{
                    new_width = stoi(width_str);
                    new_height = stoi(height_str);
                }
                catch(...){
                    cout<<"无效的字符，使用原始高度。"<<endl;
                    new_height = image.height();
                }
                if(new_width<=0 || new_height<=0){
                    cout<<"尺寸值无效，使用原始宽度和高度。"<<endl;
                    new_width = image.width();
                    new_height = image.height();
                }
                resized_image = image.get_resize(new_width, new_height);
                cout<<"图像尺寸缩放完成，新尺寸为: "<<new_width<<"x"<<new_height<<endl;
                CImgDisplay resized_disp(resized_image, "缩放后图像 (请点击窗口或按键继续)");
                while(!resized_disp.is_closed() && !resized_disp.key()){
                    resized_disp.wait(); // 等待事件
                }
                break;
            }
            case 5: {
                // 图像旋转
                if(image.is_empty()){
                    cout<<"请先加载图像（选择0）！"<<endl;
                    break;
                }
                string angle_str;
                cout<<"请输入旋转角度（正值为逆时针旋转，负值为顺时针旋转）：";
                if(!getline(cin, angle_str)){
                    cout<<"输入错误，使用默认值0。"<<endl;
                    angle_str = "0";
                }
                float angle = 0.0f;
                try{
                    angle = stof(angle_str);
                }
                catch(...){
                    cout<<"无效的字符，使用默认值0。"<<endl;
                    angle = 0.0f;
                }
                rotated_image = image.get_rotate(angle);
                cout<<"图像旋转完成，旋转角度为: "<<angle<<" 度"<<endl;
                CImgDisplay rotated_disp(rotated_image, "旋转后图像 (请点击窗口或按键继续)");
                while(!rotated_disp.is_closed() && !rotated_disp.key()){
                    rotated_disp.wait(); // 等待事件
                }
                break;
            }
            case 6: {
                // 图像压缩
                if(image.is_empty()){
                    cout<<"请先加载图像（选择0）！"<<endl;
                    break;
                }
                string quality_str;
                cout<<"请输入压缩质量（1-100，数值越大质量越好）：";
                if(!getline(cin, quality_str)){
                    cout<<"输入错误，使用默认值75。"<<endl;
                    quality_str = "75";
                }
                int quality = 75; // 默认质量
                try{
                    quality = stoi(quality_str);
                }
                catch(...){
                    cout<<"无效的字符，使用默认值75。"<<endl;
                    quality = 75;
                }
                if(quality<1||quality>100){
                    cout<<"质量值超出范围，使用默认值75。"<<endl;
                    quality = 75;
                }
                char *jpeg_buffer = nullptr;
                size_t jpeg_size = 0;
                FILE *write_stream = open_memstream(&jpeg_buffer, &jpeg_size);
                if(!write_stream) {
                    cerr << "内存文件流打开失败！" << endl;
                    break;
                }
                try{
                    image.save_jpeg(write_stream, quality);
                }
                catch (CImgIOException &e) {
                    cerr << "图像压缩失败: " << e.what() << endl;
                    fclose_memstream(write_stream, &jpeg_buffer, &jpeg_size);
                    free(jpeg_buffer);
                    break;
                }
                fclose_memstream(write_stream, &jpeg_buffer, &jpeg_size);
                if(jpeg_buffer==nullptr&&jpeg_size==0) {
                    cerr << "压缩后图像数据为空！" << endl;
                    free(jpeg_buffer);
                    break;
                }
                FILE* read_stream=fmemopen(jpeg_buffer,jpeg_size,"rb");
                if(!read_stream) {
                    cerr << "内存读取流打开失败！" << endl;
                    free(jpeg_buffer);
                    break;
                }
                try{
                    compressed_image.load_jpeg(read_stream);
                }
                catch(CImgIOException &e){
                    cerr << "压缩后图像加载失败: " << e.what() << endl;
                    fclose_memstream(read_stream, &jpeg_buffer, &jpeg_size);
                    free(jpeg_buffer);
                    break;
                }
                fclose_memstream(read_stream, &jpeg_buffer, &jpeg_size);
                if(jpeg_buffer){
                    free(jpeg_buffer);
                }
                cout<<"图像压缩完成。"<<endl;
                CImgDisplay compressed_disp(compressed_image, "压缩后图像 (请点击窗口或按键继续)");
                while (!compressed_disp.is_closed() && !compressed_disp.key()) {
                    compressed_disp.wait(); // 等待事件
                }
                break;
            }
            case 7: {
                cout<<"请选择要显示的图像（image, compressed_image, gray_image, resized_image, rotated_image）：";
                string img_choice;
                if(!getline(cin, img_choice)){
                    cout<<"输入错误，请重试。"<<endl;
                    break;
                }
                CImg<unsigned char>* img_to_show = nullptr;
                if(img_choice == "image"){
                    img_to_show = &image;
                }
                else if(img_choice == "compressed_image"){
                    img_to_show = &compressed_image;
                }
                else if(img_choice == "gray_image"){
                    img_to_show = &gray_image;
                }
                else if(img_choice == "resized_image"){
                    img_to_show = &resized_image;
                }
                else if(img_choice == "rotated_image"){
                    img_to_show = &rotated_image;
                }
                else{
                    cout<<"无效的图像选择。"<<endl;
                    break;
                }
                if(img_to_show->is_empty()){
                    cout<<"所选图像为空，无法显示！"<<endl;
                    break;
                }
                CImgDisplay display(*img_to_show, "显示图像 (请点击窗口或按键继续)");
                while(!display.is_closed() && !display.key()){
                    display.wait(); // 等待事件
                }
                break;
            }
            case 8: {
                // 保存读入的图像为不同格式
                if(image.is_empty()){
                    cout<<"请先加载图像（选择0）！"<<endl;
                    break;
                }
                cout<<"请输入要保存的图片（image, compressed_image, gray_image, resized_image, rotated_image）：";
                string img_choice;
                if(!getline(cin, img_choice)){
                    cout<<"输入错误，请重试。"<<endl;
                    break;
                }
                CImg<unsigned char>* img_to_save = nullptr;
                if(img_choice == "image"){
                    img_to_save = &image;
                }
                else if(img_choice == "compressed_image"){
                    img_to_save = &compressed_image;
                }
                else if(img_choice == "gray_image"){
                    img_to_save = &gray_image;
                }
                else if(img_choice == "resized_image"){
                    img_to_save = &resized_image;
                }
                else if(img_choice == "rotated_image"){
                    img_to_save = &rotated_image;
                }
                else{
                    cout<<"无效的图像选择。"<<endl;
                    break;
                }
                if(img_to_save->is_empty()){
                    cout<<"所选图像为空，无法保存！"<<endl;
                    break;
                }
                string save_path;
                cout<<"请输入保存路径，支持PNG，JPEG，PPM（包含文件名和扩展名，如output.png）：";
                if(!getline(cin, save_path)){
                    cout<<"输入错误，请重试。"<<endl;
                    break;
                }
                try {
                    img_to_save->save(save_path.c_str());
                    cout<<"图像已保存为 " << save_path << endl;
                }
                catch (CImgIOException &e) {
                    cerr << "图像保存失败: " << e.what() << endl;
                }
                break;
            }
            case 0: {
                // 退出
                cout<<"退出程序。"<<endl;
                return 0;
            }
            default:
                cout<<"无效的选择，请重新输入。"<<endl;
        }
    }
    
}