# search_tool
文本搜索工具，适合大量日志文件中找关键词，跨平台

编译和运行
编译命令：

gcc version 4.8.5 20150623 (Red Hat 4.8.5-44) (GCC) 

bash
gcc -o search_tool search_tool.c
使用示例
在当前目录的所有txt文件中搜索"hello"：

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
