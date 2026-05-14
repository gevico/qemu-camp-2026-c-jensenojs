#include <stddef.h>
#include <stdio.h>

/*
 * 11 简单事件处理器（回调机制）
 * 要求：
 *  - 事件类型用枚举定义（enum { EVENT_A, EVENT_B, EVENT_MAX }）。
 *  - 注册函数：void register_event(enum EVENT_TYPE type, void (*callback)(void*), void* arg)。
 *  - 触发函数：void trigger_event(enum EVENT_TYPE type)。
 *  - 测试：注册 EVENT_A 的回调打印 "Event A triggered"，触发后应输出该信息
 */

/* 定义事件类型枚举 */
enum EVENT_TYPE { EVENT_A = 0, EVENT_B, EVENT_MAX };

/* 定义回调函数类型
 *
 * typedef void (*event_callback_t)(void* arg);
 *
 * 这条声明怎么读——从标识符开始逆时针拆解：
 *
 *   1. event_callback_t           → event_callback_t 是……
 *   2. (*event_callback_t)        → ……一个指针（括号强制先绑 *）
 *   3. (*event_callback_t)(void*) → ……指向一个接受 void* 参数的函数
 *   4. void (*event_callback_t)(void*) → ……该函数返回 void
 *
 * 合起来：event_callback_t 是一个"指向返回 void、接受 void* 参数的函数"的指针类型。
 *
 * 对比普通指针声明，结构相同：
 *   int  *p;             // p 是一个 int*（先写类型，再写 *变量名）
 *   void (*fp)(void*);   // fp 是一个函数指针（先写返回类型，再写 (*变量名)(参数)）
 *
 * 关键在于括号 —— 括号改变了 * 和 () 的绑定顺序。
 * C 的运算符优先级：() 高于 *。
 *
 * ───────────────────────────────────────────────────────
 * 写法 A：void *fp(void*)             不加括号
 *
 * 解析过程：() 优先级更高，先绑住 fp，使 fp 成为"函数"。
 *          * 被挤到返回类型一侧，变成"返回 void*"。
 *
 * 结果：fp 是"一个接受 void*、返回 void* 的函数"——fp 本身就是函数定义。
 *
 * 使用场景：
 *   // 场景 1：你需要定义一个符合这种签名的函数
 *   void *thread_func(void *arg) {
 *       int *val = (int*)arg;
 *       printf("got %d\n", *val);
 *       return NULL;
 *   }
 *   // 这个函数就是 void *thread_func(void*) 的形式
 *   // 它会被传给 pthread_create(&tid, NULL, thread_func, &val);
 *
 *   // 场景 2：你要声明一个"返回指针的函数"——不是指针，是函数
 *   void *allocate_buffer(size_t size);  // 函数原型，allocate_buffer 是函数名
 *
 * ───────────────────────────────────────────────────────
 * 写法 B：void (*fp)(void*)           加括号
 *
 * 解析过程：括号强行让 * 先绑住 fp，使 fp 成为"指针"。
 *          () 绑到指针后面，变成"指向……函数的指针"。
 *
 * 结果：fp 是"一个指向接受 void*、返回 void 的函数的指针"——fp 是一个变量，存函数地址。
 *
 * 使用场景：
 *   // 场景 1：保存一个函数地址，以后通过它调用
 *   void on_click(void *user_data) {
 *       printf("clicked\n");
 *   }
 *   void (*handler)(void*) = &on_click;   // 存函数地址
 *   handler(NULL);                          // 通过指针调用
 *
 *   // 场景 2：把函数作为参数传到别处（回调模式，就是这个文件的核心）
 *   void register_event(enum EVENT_TYPE type,
 *                       void (*callback)(void*),   // ← 这里是写法 B
 *                       void *arg);
 *   register_event(EVENT_A, &on_click, msg);  // 传函数地址，不调用
 *   // 注册后，触发时由 trigger_event 内部调用 callback(arg)
 *
 *   // 场景 3：函数指针数组（这个文件用的是数组存回调）
 *   static void (*g_callbacks[EVENT_MAX])(void*);  // 存多个函数地址
 *   g_callbacks[EVENT_A] = &on_click;              // 按类型存
 *
 * ───────────────────────────────────────────────────────
 * 快速区分：看到标识符先被绑住了什么？
 *
 *   void *fp(void*)   → "fp 是一个函数"     → 用 fp(args)  来调用
 *   void (*fp)(void*) → "fp 是一个指针"     → 用 fp = &fn 赋值；用 fp(args) 或 (*fp)(args) 调用
 *
 * typedef 只是把"变量名"的位置改成"类型名"：
 *   声明变量：     void (*fp)(void*);
 *   定义类型：typedef void (*event_callback_t)(void*);
 */
typedef void (*event_callback_t)(void* arg);

/*
 * 使用两个数组分别保存每个事件类型的回调函数和其参数
 * 索引与 enum EVENT_TYPE 对应
 */
static event_callback_t g_callbacks[EVENT_MAX] = {0};
static void* g_callback_args[EVENT_MAX] = {0};

/*
 * 注册事件函数：为指定事件类型设置回调与参数
 */
void register_event(enum EVENT_TYPE type, void (*callback)(void*), void* arg) {
    if (type >= EVENT_MAX) return;
    g_callbacks[type] = callback;
    g_callback_args[type] = arg;
}

/*
 * 触发事件函数：若已注册回调则调用
 */
void trigger_event(enum EVENT_TYPE type) {
    if (type >= EVENT_MAX) return;
    if (g_callbacks[type]) {
        g_callbacks[type](g_callback_args[type]);
    }
}

/*
 * 测试函数：注册并触发 EVENT_A
 */
static void on_event_a(void* arg) {
    const char* msg = (const char*)arg;
    if (msg) {
        printf("%s\n", msg);
    }
}

int main(void) {
    /* 期待输出：Event A triggered */
    const char* msg = "Event A triggered";
    register_event(EVENT_A, on_event_a, (void*)msg);
    trigger_event(EVENT_A);
    return 0;
}
