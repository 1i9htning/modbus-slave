#pragma once
#include <string>
#include <fstream>
#include <mutex>

#define Info(format, ...) Record_LevelOne(format, true, ##__VA_ARGS__)
#define Warn(format, ...) Record_LevelTwo(format, true, ##__VA_ARGS__)
#define Error(format, ...) Record_LevelThree(format, true, ##__VA_ARGS__)
#define Debug(format, ...) Record_LevelZero(__FILE__, __LINE__, format, true, ##__VA_ARGS__)

#define Info_Silent(format, ...) Record_LevelOne(format, false, ##__VA_ARGS__)
#define Warn_Silent(format, ...) Record_LevelTwo(format, false, ##__VA_ARGS__)
#define Error_Silent(format, ...) Record_LevelThree(format, false, ##__VA_ARGS__)
#define Debug_Silent(format, ...) Record_LevelZero(__FILE__, __LINE__, format, false, ##__VA_ARGS__)

class XLogger
{
public:
    XLogger();
    XLogger(std::string fileName, std::string filePath);
    XLogger(std::string filePathAndName);
    ~XLogger();
    void Initialize(std::string fileName, std::string filePath);
    void Initialize(std::string filePathAndName);
    void Initialize();
    void SetSizeLimit(unsigned long long size);
    void Record_LevelOne(const char *format, bool printSwitch, ...);
    void Record_LevelTwo(const char *format, bool printSwitch, ...);
    void Record_LevelThree(const char *format, bool printSwitch, ...);
    void Record_LevelZero(const char *file, int line, const char *format, bool printSwitch, ...);

private:
    void Write(std::string text);
    void GetFileSize();

    std::string m_filePathAndName;
    unsigned long long m_sizeLimit;
    unsigned long long m_fileSize;

    std::mutex m_writeLock;

    std::fstream m_fileStream;
};

namespace XLog
{
    void Initialize(std::string fileName, std::string filePath);
    void Initialize(std::string filePathAndName);
    void Initialize();
    void SetSizeLimit(unsigned long long size);
    void Record_LevelOne(const char *format, bool printSwitch, ...);
    void Record_LevelTwo(const char *format, bool printSwitch, ...);
    void Record_LevelThree(const char *format, bool printSwitch, ...);
    void Record_LevelZero(const char *file, int line, const char *format, bool printSwitch, ...);
} // namespace XLog

enum FormatType
{
    BINARY = 2,
    OCTAL = 8,
    DECIMAL = 10,
    HEXADECIMAL = 16
};

extern std::string Processing(const char *format, va_list args);
extern void ConvertFormat(std::string &originData, FormatType type);
extern std::string GetTime();
extern std::string GetCodePosition(const char *file, int line);