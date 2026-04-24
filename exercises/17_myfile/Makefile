CC = gcc
# 编译参数：
# -Wall：打开常见警告
# -g：保留调试信息
# -O2：常规优化
# -fPIE：生成位置无关的可执行代码，便于后续链接成 PIE 程序
CFLAGS = -Wall -g -O2 -fPIE

# 最终生成的可执行文件名
TARGET := 17_myfile
# 源文件列表
SRCS := 17_myfile.c
# 由源文件自动推导出的目标文件列表
OBJS := $(SRCS:.c=.o)

# 默认目标：执行 make 时先构建最终程序
all: $(TARGET)

# 链接规则：
# 1) 依赖 $(OBJS)，只要目标文件变了就重新链接
# 2) -pie：把最终产物链接成 PIE 可执行文件
#    PIE 的全称是 Position Independent Executable。
#    这种可执行文件在内存中的装载地址可以变化，ELF 头里通常会表现成 ET_DYN，
#    这正是本题想让你识别到的 "Shared Object/PIE" 类型。
# 3) -o $@：输出到当前目标名，也就是 $(TARGET)
# 4) $^：GNU make 的自动变量，表示“这个目标的全部依赖文件”
#    在这里就是 $(OBJS)，也就是所有 .o 文件。
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -pie -o $@ $^

# 编译规则：
# 任何 %.c 都可以编译成同名 %.o
# $< 表示“第一个依赖文件”（这里就是 .c 文件）
# $@ 表示“当前目标文件”（这里就是 .o 文件）
# 例如：17_myfile.o: 17_myfile.c
# 这条规则就会变成：gcc ... -c -o 17_myfile.o 17_myfile.c
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理构建产物
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
