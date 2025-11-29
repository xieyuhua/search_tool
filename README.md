# search_tool
文本搜索工具，适合大量日志文件中找关键词，跨平台

编译和运行
编译命令：

gcc version 4.8.5 20150623 (Red Hat 4.8.5-44) (GCC) 

bash
gcc -o search_tool search_tool.c
使用示例
在当前目录的所有txt文件中搜索"hello"：

cp search_tool /usr/bin/


bash
./search_tool "hello" . log
在当前目录的搜索所有名文名有log字符文件中搜索"hello"：

查看帮助信息：

bash
./search_tool -h
功能特点
递归搜索：自动搜索指定目录及其所有子目录

详细输出：显示文件名、行号和匹配的行内容

错误处理：友好的错误提示信息

使用说明：提供详细的使用帮助



文件内容搜索工具 - 增强版文本文件搜索
使用说明:
  ./search_tool "搜索字符串" [目录] [文件模式] [显示上下文行]

文件匹配规则:
  - 只搜索文本文件和日志文件
  - 支持通配符匹配: * 匹配任意多个字符, ? 匹配单个字符
  - 如果不使用通配符，则匹配文件名中包含指定文本的文件（不区分大小写）
  - 如果文件模式为空，则匹配所有文本文件和日志文件
  - 自动识别以日期格式结尾的日志文件 (如: app-2025-05-05)

示例:
  ./search_tool "error" . ""               # 搜索所有文本文件和日志文件中的"error"
  ./search_tool "error" logs "*.log"       # 搜索所有.log文件中的"error"
  ./search_tool "DEBUG" . "app*"          # 搜索以app开头的文件中的"DEBUG"
  ./search_tool "user" . "*2025-05-05*"   # 搜索包含日期的文件中的"user"
  ./search_tool "TODO" src ""             # 搜索src目录所有文本文件中的"TODO"
  ./search_tool "hello"                     # 搜索当前目录所有文本文件和日志文件中的"hello"

支持的文本文件类型:
  源代码文件: .c, .h, .cpp, .java, .py, .js, .go, .rs, .php等
  标记语言: .html, .css, .xml, .json, .md, .tex等
  配置文件: .conf, .config, .ini, .yml, .properties等
  脚本文件: .sh, .bat, .ps1, .sql等
  纯文本: .txt, .csv, .text等
  日志文件: .log, .out, .err, 以及包含日期格式的文件如app-2025-05-05
