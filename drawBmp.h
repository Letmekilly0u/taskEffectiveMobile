#pragma once
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>

class BMP {
public:

    BMP(const std::string& fileName) {
        if (openBmp(fileName)) displayBMP(data, bmp_info_header.bit_count, bmp_info_header.width, bmp_info_header.height);
    }

    ~BMP()
    {
        closeBmp(file_header, bmp_info_header, bmp_color_header, data);
    }

private:
#pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t file_type{ 0x4D42 };          // Тип файла всегда BM,который равен 0x4D42 (хранится как hex uint16_t в little endian)
        uint32_t file_size{ 0 };               // Размер файла (в байтах)
        uint16_t reserved1{ 0 };               // Зарезервирован, всегда 0
        uint16_t reserved2{ 0 };               // Зарезервирован, всегда 0
        uint32_t offset_data{ 0 };             // Начальная позиция пиксельных данных (байты от начала файла)
    };

    struct BMPInfoHeader {
        uint32_t size{ 0 };                      // Размер этого заголовка (в байтах)
        int32_t width{ 0 };                      // Ширина растрового изображения в пикселях
        int32_t height{ 0 };                     // Высота растрового изображения в пикселях
                                                 // если положительное, снизу вверх, с началом в нижнем левом углу)
                                                 // (если отрицательное, сверху вниз, с началом в верхнем левом углу)
        uint16_t planes{ 1 };                    // Количество плоскостей для целевого устройства, это всегда 1
        uint16_t bit_count{ 0 };                 // Количество бит на пиксель
        uint32_t compression{ 0 };               // 0 или 3 — несжатые. ЭТА ПРОГРАММА РАССМАТРИВАЕТ ТОЛЬКО НЕСЖАТЫЕ BMP-изображения
        uint32_t size_image{ 0 };                // 0 — для несжатых изображений
        int32_t x_pixels_per_meter{ 0 };
        int32_t y_pixels_per_meter{ 0 };
        uint32_t colors_used{ 0 };               // Количество индексов цветов в таблице цветов. Используйте 0 для максимального количества цветов, разрешенного bit_count
        uint32_t colors_important{ 0 };          // Количество цветов, используемых для отображения битовой карты. Если 0, требуются все цвета
    };

    struct BMPColorHeader {
        uint32_t red_mask{ 0x00ff0000 };         // Битовая маска для красного канала
        uint32_t green_mask{ 0x0000ff00 };       // Битовая маска для зеленого канала
        uint32_t blue_mask{ 0x000000ff };        // Битовая маска для синего канала
        uint32_t alpha_mask{ 0xff000000 };       // Битовая маска для альфа-канала
        uint32_t color_space_type{ 0x73524742 }; // По умолчанию "sRGB" (0x73524742)
        uint32_t unused[16]{ 0 };                // Неиспользуемые данные для цветового пространства sRGB
    };
#pragma pack(pop)

    BMPFileHeader file_header;
    BMPInfoHeader bmp_info_header;
    BMPColorHeader bmp_color_header;
    std::vector<uint8_t> data;

    bool openBmp(const std::string& fileName) {
        std::ifstream inp{ fileName, std::ios_base::binary };
        if (inp)
        {
            inp.read((char*)&file_header, sizeof(file_header));

            if (file_header.file_type != 0x4D42)
            {
                std::cerr << "Error! Unrecognized file format.\n";
                return false;
            }

            inp.read((char*)&bmp_info_header, sizeof(bmp_info_header));

            // Переход к местоположению данных пикселей
            inp.seekg(file_header.offset_data, inp.beg);

            // Настройка поля заголовка для вывода.
            // Некоторые редакторы помещают дополнительную информацию в файл изображения, мы сохраняем только заголовки и данные.
            if(bmp_info_header.bit_count == 32 || bmp_info_header.bit_count == 24)
            {
            if (bmp_info_header.bit_count == 32) {
                bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            }
            else {
                bmp_info_header.size = sizeof(BMPInfoHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            file_header.file_size = file_header.offset_data;

            if (bmp_info_header.height < 0) {
                std::cout << "The program can treat only BMP images with the origin in the bottom left corner!\n";
                return false;
            }

            data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

            // Здесь мы проверяем, нужно ли учитывать заполнение строк.
            if (bmp_info_header.width % 4 == 0) {
                inp.read((char*)data.data(), data.size());
                file_header.file_size += static_cast<uint32_t>(data.size());
            }
            else {
                row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
                uint32_t new_stride = make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - row_stride);

                for (int y = 0; y < bmp_info_header.height; ++y) {
                    inp.read((char*)(data.data() + row_stride * y), row_stride);
                    inp.read((char*)padding_row.data(), padding_row.size());
                }
                file_header.file_size += static_cast<uint32_t>(data.size()) + bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
            }
            inp.close();
            return true;
            }
            else
            {
                inp.close();
                std::cout << "Only for 24 or 32 bits bmp.\n";
                return false;
            }
        }
        else 
        {
            inp.close();
            std::cout << "Unable to open the input image file.\n";
            return false;
        }
    }

    void displayBMP(const std::vector<uint8_t>& data, uint32_t bit_count, uint32_t bmpWidth, uint32_t bmpHeight)
    {
        uint32_t count = 0;   // Счетчик для подсчёта количества символов на текущей строке
        uint32_t counter = 0; 
        uint32_t index = 0;
        bool errorFlag = false;

        std::string* outArray = new std::string[bmpHeight];
        for (size_t i = 0; i < bmpHeight; i++)
        {
            outArray[i] = "";
        }

        if (bit_count == 24)
        {
            for (int i = data.size() - 1; i >= 0 && !errorFlag; i--)
            {
                if (data[i] == 255)
                {
                    count++;
                    if (count >= bit_count / 8)
                    {
                        outArray[index] += "#";
                        count = 0;
                    }
                    counter++;
                }
                else if (data[i] == 0)
                {
                    count++;
                    if (count >= bit_count / 8)
                    {
                        outArray[index] += ".";
                        count = 0;
                    }
                    counter++;
                }
                else
                {
                    std::cerr << "This image contains invalid colors.\n";
                    errorFlag = true;
                }

                if (counter == bmpWidth * (bit_count / 8) && !errorFlag)
                {
                    counter = 0;
                    index++;
                }
            }
        }
        else if (bit_count == 32)
        {
            for (size_t i = data.size() - 4; i < data.size() && !errorFlag; i -= 4) {
                if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0) {
                    outArray[index] += ".";
                    counter += 4;
                }
                else if (data[i] == 255 && data[i + 1] == 255 && data[i + 2] == 255) {
                    outArray[index] += "#";
                    counter += 4;
                }
                else {
                    std::cerr << "This image contains invalid colors.\n";
                    errorFlag = true;
                }

                if (counter == bmpWidth * (bit_count / 8) && !errorFlag) {
                    counter = 0;
                    index++;
                }
            }
        }
        else
        {
            std::cerr << "Data transfer error.\n";
            errorFlag = true;
        }


        if (!errorFlag)
        {
            for (size_t i = 0; i < bmpHeight; i++)
            {
                std::reverse(outArray[i].begin(), outArray[i].end());
                std::cout << outArray[i] + "\n";
            }
        }
    }

    void closeBmp(BMPFileHeader file_header, BMPInfoHeader bmp_info_header, BMPColorHeader bmp_color_header, std::vector<uint8_t> data)
    {
        file_header = { NULL };
        bmp_info_header = { NULL };
        bmp_color_header = { NULL };
        data.clear();
    }

    uint32_t row_stride{ 0 };

    //Добавляет 1 к rov_stride, если только он не делится на align_stride.
    uint32_t make_stride_aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }
};