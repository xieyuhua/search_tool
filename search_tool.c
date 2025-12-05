#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LENGTH 4096

// 函数声明
void search_in_file(const char *filename, const char *search_str, int context_lines);
int search_in_directory(const char *dir_path, const char *file_pattern, const char *search_str, int recursive, int context_lines);
int matches_pattern(const char *filename, const char *pattern);
int is_text_file(const char *filename);
int is_log_file(const char *filename);
int ends_with_date(const char *filename);
void print_usage(const char *program_name);

// 检查路径是否为目录
int is_directory(const char *path) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) != 0) {
        return 0;
    }
    return S_ISDIR(stat_buf.st_mode);
}

// 获取文件名（不含路径）
const char* get_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = strrchr(path, '\\');
    }
    return filename ? filename + 1 : path;
}

// 检查字符串是否以日期格式结尾 (YYYY-MM-DD)
int ends_with_date(const char *filename) {
    const char* pure_name = get_filename(filename);
    int len = strlen(pure_name);
    
    // 需要至少10个字符来容纳 YYYY-MM-DD
    if (len < 10) {
        return 0;
    }
    
    // 检查最后10个字符是否为日期格式
    const char* end = pure_name + len - 10;
    
    // 检查格式: YYYY-MM-DD
    // 位置: 0123456789
    // 示例: 2025-05-05
    int i; 
    for ( i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            // 第4和第7个字符应该是 '-'
            if (end[i] != '-' && end[i] != '.' && end[i] != '_') {
                return 0;
            }
        } else {
            // 其他位置应该是数字
            if (!isdigit(end[i])) {
                return 0;
            }
        }
    }
    
    // 进一步验证: 月份应该在01-12之间，日期应该在01-31之间
    int month = (end[5] - '0') * 10 + (end[6] - '0');
    int day = (end[8] - '0') * 10 + (end[9] - '0');
    
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }
    
    return 1;
}

// 检查文件是否为文本文件（通过扩展名判断）
int is_text_file(const char *filename) {
    const char* pure_name = get_filename(filename);
    const char* dot = strrchr(pure_name, '.');
    
    if (dot == NULL) {
        // 没有扩展名，检查是否为日志文件
        return is_log_file(filename);
    }
    
    // 常见的文本文件扩展名
    const char* text_extensions[] = {
        ".txt", ".c", ".h", ".cpp", ".hpp", ".java", ".py", ".js", 
        ".html", ".css", ".xml", ".json", ".md", ".csv",
        ".conf", ".config", ".ini", ".yml", ".yaml", ".properties",
        ".sh", ".bat", ".ps1", ".sql", ".php", ".rb", ".pl", ".lua",
        ".go", ".rs", ".swift", ".kt", ".dart", ".ts", ".jsx", ".tsx",
        ".vue", ".svelte", ".rst", ".tex", ".bib", ".asc", ".text",
        ".ini", ".cfg", ".toml", NULL
    };
    int i ;
    for ( i = 0; text_extensions[i] != NULL; i++) {
        if (strcasecmp(dot, text_extensions[i]) == 0) {
            return 1;
        }
    }
    
    // 检查是否为日志文件
    return is_log_file(filename);
}

// 检查文件是否为日志文件
int is_log_file(const char *filename) {
    const char* pure_name = get_filename(filename);
    
    // 常见的日志文件扩展名
    const char* log_extensions[] = {
        ".log", ".logs", ".out", ".err", ".debug", ".trace",
        ".audit", ".event", ".history", ".journal", NULL
    };
    
    const char* dot = strrchr(pure_name, '.');
    if (dot != NULL) {
        int i ;
        for ( i = 0; log_extensions[i] != NULL; i++) {
            if (strcasecmp(dot, log_extensions[i]) == 0) {
                return 1;
            }
        }
    }
    
    // 检查文件名是否包含常见的日志文件关键词
    const char* log_keywords[] = {
        "log", "logs", "debug", "error", "trace", "audit",
        "event", "history", "journal", "record", "report", NULL
    };
    
    char lower_name[256];
    strncpy(lower_name, pure_name, sizeof(lower_name)-1);
    lower_name[sizeof(lower_name)-1] = '\0';
    char *p ;
    for ( p = lower_name; *p; ++p) *p = tolower(*p);
    int i;
    for ( i = 0; log_keywords[i] != NULL; i++) {
        if (strstr(lower_name, log_keywords[i]) != NULL) {
            return 1;
        }
    }
    
    // 检查是否为日期格式结尾的文件（如 app-2025-05-05）
    return ends_with_date(filename);
}

// 检查文件名是否匹配模式（支持通配符和子字符串匹配）
int matches_pattern(const char *filename, const char *pattern) {
    // 如果模式为空，匹配所有文本文件
    if (pattern == NULL || pattern[0] == '\0') {
        return is_text_file(filename);
    }
    
    const char* pure_name = get_filename(filename);
    
    // 检查是否是文本文件
    if (!is_text_file(filename)) {
        return 0;
    }
    
    // 检查模式是否包含通配符
    int has_wildcard = (strchr(pattern, '*') != NULL) || (strchr(pattern, '?') != NULL);
    
    if (has_wildcard) {
        // 使用通配符匹配
#ifdef _WIN32
        // Windows 使用简单的通配符匹配
        return wildcard_match(pattern, pure_name);
#else
        // Linux/Mac 使用 fnmatch (如果可用)
        #ifdef HAVE_FNMATCH_H
        return fnmatch(pattern, pure_name, 0) == 0;
        #else
        // 如果没有 fnmatch，使用自定义通配符匹配
        return wildcard_match(pattern, pure_name);
        #endif
#endif
    } else {
        // 使用简单的子字符串匹配（不区分大小写）
        char lower_name[256];
        char lower_pattern[256];
        strncpy(lower_name, pure_name, sizeof(lower_name)-1);
        strncpy(lower_pattern, pattern, sizeof(lower_pattern)-1);
        lower_name[sizeof(lower_name)-1] = '\0';
        lower_pattern[sizeof(lower_pattern)-1] = '\0';
        
        

        // 确保在函数开头声明变量
        char *p;
        
        // 修正循环语句
        for (p = lower_name; *p; ++p) *p = tolower(*p);
        for (p = lower_pattern; *p; ++p) *p = tolower(*p);
        
        
        return strstr(lower_name, lower_pattern) != NULL;
    }
}

// 通配符匹配函数 (用于没有 fnmatch 的环境)
int wildcard_match(const char *pattern, const char *text) {
    if (*pattern == '\0' && *text == '\0') return 1;
    if (*pattern == '*' && *(pattern+1) != '\0' && *text == '\0') return 0;
    
    if (*pattern == '?' || *pattern == *text) {
        return wildcard_match(pattern+1, text+1);
    }
    
    if (*pattern == '*') {
        return wildcard_match(pattern+1, text) || 
               wildcard_match(pattern, text+1);
    }
    
    return 0;
}

// void search_in_file(const char *filename, const char *search_str) {
//     FILE *file = fopen(filename, "r");
//     if (!file) {
//         return; // 静默处理无法打开的文件
//     }
//     char line[MAX_LINE_LENGTH];
//     int line_number = 0;
//     int found_count = 0;
    
//     while (fgets(line, sizeof(line), file)) {
//         line_number++;
//         line[strcspn(line, "\n")] = 0;
//         if (strstr(line, search_str) != NULL) {
//             printf("文件: %s, 第 %d 行: %s\n", filename, line_number, line);
//             found_count++;
//         }
//     }
//     fclose(file);
//     if (found_count > 0) {
//         printf("--> 在 %s 中找到 %d 个匹配项\n\n", filename, found_count);
//     }
// }


void search_in_file(const char *filename, const char *search_str, int context_lines) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    
    // 参数安全检查
    if (context_lines < 0) context_lines = 0;
    if (context_lines > 5) context_lines = 5; // 限制上下文行数
    
    char line[20480];
    int line_number = 0;
    int found_count = 0;
    
    // 第一遍：统计匹配行
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (check_match_optimized(line, search_str) == 1) {
        // if (strstr(line, search_str) != NULL) {
            found_count++;
        }
    }
    
    if (found_count == 0) {
        fclose(file);
        return;
    }
    
    // 第二遍：显示结果
    rewind(file);
    line_number = 0;
    int current_match = 0;
    
    // printf("在文件 %s 中找到 %d 个匹配项:\n", filename, found_count);
    // printf("========================================\n");
    
    // 简单的行缓存（最多缓存context_lines行）
    char **prev_lines = NULL;
    int *prev_line_nums = NULL;
    int prev_count = 0;
    
    if (context_lines > 0) {
        prev_lines = malloc(context_lines * sizeof(char*));
        prev_line_nums = malloc(context_lines * sizeof(int));
        
        if (prev_lines && prev_line_nums) {
            int i;
            for ( i = 0; i < context_lines; i++) {
                prev_lines[i] = malloc(20480);
                if (prev_lines[i]) {
                    prev_lines[i][0] = '\0';
                    prev_line_nums[i] = 0;
                }
            }
        }
    }
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        line[strcspn(line, "\n")] = 0;
        
        // 更新前几行缓存
        if (context_lines > 0 && prev_lines && prev_line_nums) {
            if (prev_count < context_lines) {
                strncpy(prev_lines[prev_count], line, 20479);
                prev_lines[prev_count][20479] = '\0';
                prev_line_nums[prev_count] = line_number;
                prev_count++;
            } else {
                // 移动缓存（FIFO）
                int i;
                for ( i = 0; i < context_lines - 1; i++) {
                    strncpy(prev_lines[i], prev_lines[i + 1], 20479);
                    prev_lines[i][20479] = '\0';
                    prev_line_nums[i] = prev_line_nums[i + 1];
                }
                strncpy(prev_lines[context_lines - 1], line, 20479);
                prev_lines[context_lines - 1][20479] = '\0';
                prev_line_nums[context_lines - 1] = line_number;
            }
        }
        
        // if (strstr(line, search_str) != NULL) {
        if (check_match_optimized(line, search_str) == 1) {
            current_match++;
            printf("在文件 %s 匹配项 %d (第 %d 行):\n", filename, current_match, line_number);
            printf("----------------------------------------\n");
            
            // 显示前面的上下文
            if (context_lines > 0 && prev_lines) {
                int i ;
                for ( i = 0; i < prev_count; i++) {
                    if (prev_line_nums[i] < line_number) {
                        printf("  第 %4d 行: %s\n", prev_line_nums[i], prev_lines[i]);
                    }
                }
            }
            
            // 显示匹配行
            printf("> 第 %4d 行: %s\n", line_number, line);
            
            // 显示后面的上下文
            int i ;
            for ( i = 1; i <= context_lines; i++) {
                char next_line[20480];
                if (fgets(next_line, sizeof(next_line), file)) {
                    next_line[strcspn(next_line, "\n")] = 0;
                    printf("  第 %4d 行: %s\n", line_number + i, next_line);
                } else {
                    break;
                }
            }
            printf("\n");
            
            // 如果读取了后面的行，需要调整文件指针
            if (context_lines > 0) {
                // 这里简化处理：后面的匹配项可能显示不完整
                // 对于精确显示，需要更复杂的逻辑
            }
        }
    }
    
    printf("========================================\n\n");
    
    // 安全释放内存
    if (prev_lines) {
        int i ;
        for ( i = 0; i < context_lines; i++) {
            if (prev_lines[i]) free(prev_lines[i]);
        }
        free(prev_lines);
    }
    if (prev_line_nums) free(prev_line_nums);
    
    fclose(file);
}

int check_match_optimized(const char* line, const char* search_str) {
    char temp[512];
    char* token;
    int i;
    
    // 复制字符串到临时缓冲区
    strncpy(temp, search_str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    // 检查是否包含 |（或关系）
    if (strchr(search_str, '|') != NULL) {
        token = strtok(temp, "|");
        while (token != NULL) {
            if (strstr(line, token) != NULL) {
                return 1; // 匹配成功
            }
            token = strtok(NULL, "|");
        }
        return 0; // 没有匹配
    }
    // 检查是否包含 &（与关系）
    else if (strchr(search_str, '&') != NULL) {
        token = strtok(temp, "&");
        while (token != NULL) {
            if (strstr(line, token) == NULL) {
                return 0; // 有一个不匹配就失败
            }
            token = strtok(NULL, "&");
        }
        return 1; // 全部匹配
    }
    // 单个字符串
    else {
        return strstr(line, search_str) != NULL;
    }
}

// // 高性能版本 - 避免字符串拷贝
// int check_match_optimized(const char* line, const char* search_str) {
//     const char *start = search_str;
//     const char *current = search_str;
//     int is_or_mode = 0;
//     int is_and_mode = 0;
    
//     // 快速检测模式
//     while (*current) {
//         if (*current == '|') {
//             is_or_mode = 1;
//             break;
//         } else if (*current == '&') {
//             is_and_mode = 1;
//             break;
//         }
//         current++;
//     }
    
//     // 重置指针
//     current = search_str;
    
//     if (is_or_mode) {
//         // 或关系 - 任一匹配即返回
//         const char *token_start = search_str;
//         const char *token_end = search_str;
        
//         while (*current) {
//             if (*current == '|' || *(current + 1) == '\0') {
//                 if (*current == '|') {
//                     token_end = current;
//                 } else {
//                     token_end = current + 1; // 包含最后一个字符
//                 }
                
//                 // 提取token长度
//                 size_t token_len = token_end - token_start;
//                 if (token_len > 0) {
//                     // 使用strncmp进行精确比较，避免strstr的全局搜索
//                     if (strstr(line, token_start) != NULL) {
//                         return 1;
//                     }
//                 }
                
//                 token_start = current + 1;
//             }
//             current++;
//         }
//         return 0;
        
//     } else if (is_and_mode) {
//         // 与关系 - 所有必须匹配
//         const char *token_start = search_str;
//         const char *token_end = search_str;
        
//         while (*current) {
//             if (*current == '&' || *(current + 1) == '\0') {
//                 if (*current == '&') {
//                     token_end = current;
//                 } else {
//                     token_end = current + 1;
//                 }
                
//                 size_t token_len = token_end - token_start;
//                 if (token_len > 0) {
//                     if (strstr(line, token_start) == NULL) {
//                         return 0; // 任一不匹配立即返回
//                     }
//                 }
                
//                 token_start = current + 1;
//             }
//             current++;
//         }
//         return 1;
        
//     } else {
//         // 单个字符串
//         if(strstr(line, search_str) != NULL){
//              return 1;
//         }else{
//              return 0;
//         }
//         // 单个字符串
//         // return strstr(line, search_str) != NULL;
//     }
// }

// 递归搜索目录
int search_in_directory(const char *dir_path, const char *file_pattern, const char *search_str, int recursive, int context_lines) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return 0;
    }

    struct dirent *entry;
    struct stat stat_buf;
    char full_path[MAX_PATH_LENGTH];
    int file_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
#ifdef _WIN32
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, entry->d_name);
#else
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
#endif
        
        if (stat(full_path, &stat_buf) != 0) {
            continue;
        }
        
        if (S_ISDIR(stat_buf.st_mode)) {
            if (recursive) {
                file_count += search_in_directory(full_path, file_pattern, search_str, recursive, context_lines);
            }
        } else if (S_ISREG(stat_buf.st_mode)) {
            if (matches_pattern(entry->d_name, file_pattern)) {
                search_in_file(full_path, search_str, context_lines);
                file_count++;
            }
        }
    }
    
    closedir(dir);
    return file_count;
}

// 显示使用说明
void print_usage(const char *program_name) {
    printf("文件内容搜索工具 - 增强版文本文件搜索\n");
    printf("使用说明:\n");
    printf("  %s \"搜索字符串\" [目录] [文件模式] [显示上下文行]\n", program_name);
    printf("\n文件匹配规则:\n");
    printf("  - 只搜索文本文件和日志文件\n");
    printf("  - 支持通配符匹配: * 匹配任意多个字符, ? 匹配单个字符\n");
    printf("  - 如果不使用通配符，则匹配文件名中包含指定文本的文件（不区分大小写）\n");
    printf("  - 如果文件模式为空，则匹配所有文本文件和日志文件\n");
    printf("  - 自动识别以日期格式结尾的日志文件 (如: app-2025-05-05)\n");
    printf("\n示例:\n");
    printf("  %s \"error\" . \"\"               # 搜索所有文本文件和日志文件中的\"error\"\n", program_name);
    printf("  %s \"error\" logs \"*.log\"       # 搜索所有.log文件中的\"error\"\n", program_name);
    printf("  %s \"DEBUG\" . \"app*\"          # 搜索以app开头的文件中的\"DEBUG\"\n", program_name);
    printf("  %s \"user\" . \"*2025-05-05*\"   # 搜索包含日期的文件中的\"user\"\n", program_name);
    printf("  %s \"TODO\" src \"\"             # 搜索src目录所有文本文件中的\"TODO\"\n", program_name);
    printf("  %s \"hello\"                     # 搜索当前目录所有文本文件和日志文件中的\"hello\"\n", program_name);
    printf("\n支持的文本文件类型:\n");
    printf("  源代码文件: .c, .h, .cpp, .java, .py, .js, .go, .rs, .php等\n");
    printf("  标记语言: .html, .css, .xml, .json, .md, .tex等\n");
    printf("  配置文件: .conf, .config, .ini, .yml, .properties等\n");
    printf("  脚本文件: .sh, .bat, .ps1, .sql等\n");
    printf("  纯文本: .txt, .csv, .text等\n");
    printf("  日志文件: .log, .out, .err, 以及包含日期格式的文件如app-2025-05-05\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 帮助选项
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    const char *search_str = argv[1];
    const char *dir_path = ".";
    const char *file_pattern = ""; // 默认匹配所有文本文件和日志文件
    int context_line = 0; 
    // 解析参数
    if (argc >= 3) {
        // 第二个参数可能是目录或文件模式
        if (is_directory(argv[2])) {
            dir_path = argv[2];
            if (argc >= 4) {
                file_pattern = argv[3];
            }
        } else {
            // 第二个参数是文件模式
            file_pattern = argv[2];
            if (argc >= 4 && is_directory(argv[3])) {
                dir_path = argv[3];
            }
        }
    }
    context_line = 0;
    if (argc >= 4) {
        context_line = atoi(argv[4]);
    }
    
    printf("开始搜索...\n");
    printf("搜索字符串: \"%s\"\n", search_str);
    printf("搜索目录: %s\n", dir_path);
    
    if (file_pattern[0] == '\0') {
        printf("文件模式: (所有文本文件和日志文件)\n");
    } else {
        printf("文件模式: %s\n", file_pattern);
        if (strchr(file_pattern, '*') != NULL || strchr(file_pattern, '?') != NULL) {
            printf("匹配方式: 通配符匹配\n");
        } else {
            printf("匹配方式: 文件名包含匹配（不区分大小写）\n");
        }
    }
    printf("上下行数: %d 行\n", context_line);
    
    printf("========================================\n\n");
    
    int file_count = search_in_directory(dir_path, file_pattern, search_str, 1, context_line);
    
    printf("========================================\n");
    printf("搜索完成，共检查了 %d 个匹配文件\n", file_count);
    
    return 0;
}